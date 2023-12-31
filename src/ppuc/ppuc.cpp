#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include <shared_mutex>
#include <mutex>

#include <AL/al.h>
#include <AL/alc.h>

#include "../libpinmame/libpinmame.h"
#include "../../../libppuc/src/PPUC.h"
#include "../../../libppuc/src/Event.h"
#include "dmd/dmd.h"
#include "pin2dmd/pin2dmd.h"
#include "cargs/cargs.h"

#if defined(ZEDMD_SUPPORT)
#include "../../../libzedmd/src/ZeDMD.h"
ZeDMD zedmd;
#endif
#if defined(SERUM_SUPPORT)
#include "../../../libserum/src/serum-decode.h"
#endif

#define MAX_AUDIO_BUFFERS 4
#define MAX_AUDIO_QUEUE_SIZE 10
#define MAX_EVENT_SEND_QUEUE_SIZE 10
#define MAX_EVENT_RECV_QUEUE_SIZE 10
#define DMD_FRAME_BUFFERS 10

ALuint _audioSource;
ALuint _audioBuffers[MAX_AUDIO_BUFFERS];
std::queue<void *> _audioQueue;
int _audioChannels;
int _audioSampleRate;

PPUC *ppuc;
int pin2dmd_connected = 0;
int zedmd_connected = 0;

uint8_t dmd_planes_buffer[12288] = {0};
uint8_t dmd_serum_planes_buffer[12288] = {0};
uint8_t dmd_frame_buffer[DMD_FRAME_BUFFERS][16384] = {0};
uint8_t dmd_frame_buffer_width[DMD_FRAME_BUFFERS] = {0};
uint8_t dmd_frame_buffer_height[DMD_FRAME_BUFFERS] = {0};
uint8_t dmd_frame_buffer_depth[DMD_FRAME_BUFFERS] = {0};
int dmd_frame_buffer_position = 0;
std::atomic<bool> dmd_ready = false;
std::shared_mutex dmd_shared_mutex;
std::condition_variable_any dmd_cv;
std::atomic<bool> stop_flag = false;

bool opt_debug = false;
bool opt_no_serial = false;
bool opt_serum = false;
bool opt_console_display = false;
int game_state = 0;

static struct cag_option options[] = {
    {.identifier = 'c',
     .access_letters = "c",
     .access_name = "config",
     .value_name = "VALUE",
     .description = "Path to config file (required)"},
    {.identifier = 'r',
     .access_letters = "r",
     .access_name = "rom",
     .value_name = "VALUE",
     .description = "Path to ROM file (optional, overwrites setting in config file)"},
    {.identifier = 's',
     .access_letters = "s",
     .access_name = "serial",
     .value_name = "VALUE",
     .description = "Serial device (optional, overwrites setting in config file)"},
    {.identifier = 'n',
     .access_letters = "n",
     .access_name = "no-serial",
     .value_name = NULL,
     .description = "No serial communication to controllers (optional)"},
#if defined(SERUM_SUPPORT)
    {.identifier = 'u',
     .access_letters = "u",
     .access_name = "serum",
     .value_name = NULL,
     .description = "Enable Serum colorization (optional)"},
    {.identifier = 't',
     .access_letters = "t",
     .access_name = "serum-timeout",
     .value_name = "VALUE",
     .description = "Serum timeout in milliseconds to ignore unknown frames (optional)"},
    {.identifier = 'p',
     .access_letters = "p",
     .access_name = "serum-skip-frames",
     .value_name = "VALUE",
     .description = "Serum ignore number of unknown frames (optional)"},
#endif
    {.identifier = 'i',
     .access_letters = "i",
     .access_name = "console-display",
     .value_name = NULL,
     .description = "Enable console display (optional)"},
    {.identifier = 'd',
     .access_letters = "d",
     .access_name = "debug",
     .value_name = NULL,
     .description = "Enable debug output (optional)"},
    {.identifier = 'h',
     .access_letters = "h",
     .access_name = "help",
     .description = "Show help"}};

void CALLBACK Game(PinmameGame *game)
{
    printf("Game(): name=%s, description=%s, manufacturer=%s, year=%s, flags=%lu, found=%d\n",
           game->name, game->description, game->manufacturer, game->year, (unsigned long)game->flags, game->found);
}

void CALLBACK OnStateUpdated(int state, const void *p_userData)
{
    if (opt_debug)
        printf("OnStateUpdated(): state=%d\n", state);

    if (!state)
    {
        exit(1);
    }
    else
    {
        PinmameMechConfig mechConfig;
        memset(&mechConfig, 0, sizeof(mechConfig));

        mechConfig.sol1 = 11;
        mechConfig.length = 240;
        mechConfig.steps = 240;
        mechConfig.type = PINMAME_MECH_FLAGS_NONLINEAR | PINMAME_MECH_FLAGS_REVERSE | PINMAME_MECH_FLAGS_ONESOL;
        mechConfig.sw[0].swNo = 32;
        mechConfig.sw[0].startPos = 0;
        mechConfig.sw[0].endPos = 5;

        PinmameSetMech(0, &mechConfig);

        game_state = state;
    }
}

void CALLBACK OnLogMessage(PINMAME_LOG_LEVEL logLevel, const char *format, va_list args, const void *p_userData)
{
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);

    if (logLevel == PINMAME_LOG_LEVEL_INFO)
    {
        printf("INFO: %s", buffer);
    }
    else if (logLevel == PINMAME_LOG_LEVEL_ERROR)
    {
        printf("ERROR: %s", buffer);
    }
}

void CALLBACK OnDisplayAvailable(int index, int displayCount, PinmameDisplayLayout *p_displayLayout, const void *p_userData)
{
    if (opt_debug)
        printf("OnDisplayAvailable(): index=%d, displayCount=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
               index,
               displayCount,
               p_displayLayout->type,
               p_displayLayout->top,
               p_displayLayout->left,
               p_displayLayout->width,
               p_displayLayout->height,
               p_displayLayout->depth,
               p_displayLayout->length);
}

void CALLBACK OnDisplayUpdated(int index, void *p_displayData, PinmameDisplayLayout *p_displayLayout, const void *p_userData)
{
    if (p_displayData == nullptr)
    {
        return;
    }

    if (opt_debug)
        printf("OnDisplayUpdated(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
               index,
               p_displayLayout->type,
               p_displayLayout->top,
               p_displayLayout->left,
               p_displayLayout->width,
               p_displayLayout->height,
               p_displayLayout->depth,
               p_displayLayout->length);

    std::unique_lock<std::shared_mutex> ul(dmd_shared_mutex);
    if ((p_displayLayout->type & PINMAME_DISPLAY_TYPE_DMD) == PINMAME_DISPLAY_TYPE_DMD)
    {
        dmd_frame_buffer_width[dmd_frame_buffer_position] = p_displayLayout->width;
        dmd_frame_buffer_height[dmd_frame_buffer_position] = p_displayLayout->height;
        dmd_frame_buffer_depth[dmd_frame_buffer_position] = p_displayLayout->depth;
        memcpy(dmd_frame_buffer[dmd_frame_buffer_position++], p_displayData, p_displayLayout->width * p_displayLayout->height);
        if (dmd_frame_buffer_position >= DMD_FRAME_BUFFERS)
        {
            dmd_frame_buffer_position = 0;
        }
        dmd_ready = true;
    }
    else
    {
        // todo
        // DumpAlphanumeric(index, (uint16_t*)p_displayData, p_displayLayout);
    }
    ul.unlock();
    dmd_cv.notify_all();
}

void Pin2DmdThread()
{
    int pin2dmd_frame_buffer_position = 0;

    while (true)
    {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []()
                    { return dmd_ready || stop_flag; });
        sl.unlock();
        if (stop_flag)
        {
            return;
        }

        while (pin2dmd_frame_buffer_position != dmd_frame_buffer_position)
        {
            dmdConvertToFrame(dmd_frame_buffer_width[pin2dmd_frame_buffer_position], dmd_frame_buffer_height[pin2dmd_frame_buffer_position], dmd_frame_buffer[pin2dmd_frame_buffer_position], dmd_planes_buffer, dmd_frame_buffer_depth[pin2dmd_frame_buffer_position]);
            Pin2dmdRender(dmd_frame_buffer_width[pin2dmd_frame_buffer_position], dmd_frame_buffer_height[pin2dmd_frame_buffer_position], dmd_planes_buffer, dmd_frame_buffer_depth[pin2dmd_frame_buffer_position]);

            pin2dmd_frame_buffer_position++;
            if (pin2dmd_frame_buffer_position >= DMD_FRAME_BUFFERS)
            {
                pin2dmd_frame_buffer_position = 0;
            }
        }
    }
}

void ZeDmdThread()
{
#if defined(ZEDMD_SUPPORT)
    int zedmd_frame_buffer_position = 0;
    uint16_t prevWidth = 0;
    uint16_t prevHeight = 0;
    uint8_t prevBitDepth = 0;

    while (true)
    {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []()
                    { return dmd_ready || stop_flag; });
        sl.unlock();
        if (stop_flag)
        {
            return;
        }

        while (zedmd_frame_buffer_position != dmd_frame_buffer_position)
        {
            if (dmd_frame_buffer_width[zedmd_frame_buffer_position] != prevWidth || dmd_frame_buffer_height[zedmd_frame_buffer_position] != prevHeight)
            {
                prevWidth = dmd_frame_buffer_width[zedmd_frame_buffer_position];
                prevHeight = dmd_frame_buffer_height[zedmd_frame_buffer_position];
                zedmd.SetFrameSize(prevWidth, prevHeight);
            }
            uint8_t renderBuffer[prevWidth * prevHeight];
            memcpy(renderBuffer, dmd_frame_buffer[zedmd_frame_buffer_position], prevWidth * prevHeight);

            if (opt_serum)
            {
#if defined(SERUM_SUPPORT)
                uint8_t palette[192] = {0};
                uint8_t rotations[24] = {0};
                UINT32 triggerID;
                UINT32 hashcode;
                int frameID;

                Serum_SetStandardPalette(zedmd.GetDefaultPalette(dmd_frame_buffer_depth[zedmd_frame_buffer_position]), dmd_frame_buffer_depth[zedmd_frame_buffer_position]);

                if (Serum_ColorizeWithMetadata(renderBuffer, prevWidth, prevHeight, &palette[0], &rotations[0], &triggerID, &hashcode, &frameID))
                {
                    // Force other render modes to reload the default palette.
                    prevBitDepth = 0;

                    zedmd.RenderColoredGray6(renderBuffer, palette, rotations);

                    // todo: send DMD Event with triggerID
                }
#endif
            }
            else
            {
                if (dmd_frame_buffer_depth[zedmd_frame_buffer_position] != prevBitDepth)
                {
                    prevBitDepth = dmd_frame_buffer_depth[zedmd_frame_buffer_position];
                    zedmd.SetDefaultPalette(prevBitDepth);
                }

                switch (prevBitDepth)
                {
                case 2:
                    zedmd.RenderGray2(renderBuffer);
                    break;

                case 4:
                    zedmd.RenderGray4(renderBuffer);
                    break;

                default:
                    printf("Unsupported ZeDMD bit depth: %d\n", prevBitDepth);
                }
            }

            zedmd_frame_buffer_position++;
            if (zedmd_frame_buffer_position >= DMD_FRAME_BUFFERS)
            {
                zedmd_frame_buffer_position = 0;
            }
        }
    }
#endif
}

void ConsoleDmdThread()
{
    int console_frame_buffer_position = 0;

    while (true)
    {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []()
                    { return dmd_ready || stop_flag; });
        sl.unlock();
        if (stop_flag)
        {
            return;
        }

        while (console_frame_buffer_position != dmd_frame_buffer_position)
        {
            dmdConsoleRender(dmd_frame_buffer_width[console_frame_buffer_position], dmd_frame_buffer_height[console_frame_buffer_position], dmd_frame_buffer[console_frame_buffer_position], dmd_frame_buffer_depth[console_frame_buffer_position]);

            if (!opt_debug)
                printf("\033[%dA", dmd_frame_buffer_height[console_frame_buffer_position]);

            console_frame_buffer_position++;
            if (console_frame_buffer_position >= DMD_FRAME_BUFFERS)
            {
                console_frame_buffer_position = 0;
            }
        }
    }
}

void ResetDmdThread()
{
    while (true)
    {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []()
                    { return dmd_ready || stop_flag; });
        sl.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        dmd_ready = false;

        if (stop_flag)
        {
            return;
        }
    }
}

int CALLBACK OnAudioAvailable(PinmameAudioInfo *p_audioInfo, const void *p_userData)
{
    if (opt_debug)
        printf("OnAudioAvailable(): format=%d, channels=%d, sampleRate=%.2f, framesPerSecond=%.2f, samplesPerFrame=%d, bufferSize=%d\n",
               p_audioInfo->format,
               p_audioInfo->channels,
               p_audioInfo->sampleRate,
               p_audioInfo->framesPerSecond,
               p_audioInfo->samplesPerFrame,
               p_audioInfo->bufferSize);

    _audioChannels = p_audioInfo->channels;
    _audioSampleRate = (int)p_audioInfo->sampleRate;

    for (int index = 0; index < MAX_AUDIO_BUFFERS; index++)
    {
        int bufferSize = p_audioInfo->samplesPerFrame * _audioChannels * sizeof(int16_t);
        void *p_buffer = malloc(bufferSize);
        memset(p_buffer, 0, bufferSize);

        alBufferData(_audioBuffers[index], _audioChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
                     p_buffer,
                     bufferSize,
                     _audioSampleRate);
    }

    alSourceQueueBuffers(_audioSource, MAX_AUDIO_BUFFERS, _audioBuffers);
    alSourcePlay(_audioSource);

    return p_audioInfo->samplesPerFrame;
}

int CALLBACK OnAudioUpdated(void *p_buffer, int samples, const void *p_userData)
{
    if (_audioQueue.size() >= MAX_AUDIO_QUEUE_SIZE)
    {
        while (!_audioQueue.empty())
        {
            void *p_destBuffer = _audioQueue.front();

            free(p_destBuffer);
            _audioQueue.pop();
        }
    }

    int bufferSize = samples * _audioChannels * sizeof(int16_t);
    void *p_destBuffer = malloc(bufferSize);
    memcpy(p_destBuffer, p_buffer, bufferSize);

    _audioQueue.push(p_destBuffer);

    ALint buffersProcessed;
    alGetSourcei(_audioSource, AL_BUFFERS_PROCESSED, &buffersProcessed);

    if (buffersProcessed <= 0)
    {
        return samples;
    }

    while (buffersProcessed > 0)
    {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(_audioSource, 1, &buffer);

        if (_audioQueue.size() > 0)
        {
            void *p_destBuffer = _audioQueue.front();

            alBufferData(buffer,
                         _audioChannels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
                         p_destBuffer,
                         bufferSize,
                         _audioSampleRate);

            free(p_destBuffer);
            _audioQueue.pop();
        }

        alSourceQueueBuffers(_audioSource, 1, &buffer);
        buffersProcessed--;
    }

    ALint state;
    alGetSourcei(_audioSource, AL_SOURCE_STATE, &state);

    if (state != AL_PLAYING)
    {
        alSourcePlay(_audioSource);
    }

    return samples;
}

void CALLBACK OnSolenoidUpdated(PinmameSolenoidState *p_solenoidState, const void *p_userData)
{
    if (opt_debug)
        printf("OnSolenoidUpdated: solenoid=%d, state=%d\n", p_solenoidState->solNo, p_solenoidState->state);

    ppuc->SetSolenoidState(p_solenoidState->solNo, p_solenoidState->state);
}

void CALLBACK OnMechAvailable(int mechNo, PinmameMechInfo *p_mechInfo, const void *p_userData)
{
    if (opt_debug)
        printf("OnMechAvailable: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
               mechNo,
               p_mechInfo->type,
               p_mechInfo->length,
               p_mechInfo->steps,
               p_mechInfo->pos,
               p_mechInfo->speed);
}

void CALLBACK OnMechUpdated(int mechNo, PinmameMechInfo *p_mechInfo, const void *p_userData)
{
    if (opt_debug)
        printf("OnMechUpdated: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
               mechNo,
               p_mechInfo->type,
               p_mechInfo->length,
               p_mechInfo->steps,
               p_mechInfo->pos,
               p_mechInfo->speed);
}

void CALLBACK OnConsoleDataUpdated(void *p_data, int size, const void *p_userData)
{
    if (opt_debug)
        printf("OnConsoleDataUpdated: size=%d\n", size);
}

int CALLBACK IsKeyPressed(PINMAME_KEYCODE keycode, const void *p_userData)
{
    return 0;
}

int main(int argc, char *argv[])
{
    char identifier;
    cag_option_context cag_context;
    const char *config_file = NULL;
    const char *opt_rom = NULL;
    const char *opt_serial = NULL;
#if defined(SERUM_SUPPORT)
    const char *opt_serum_timeout = NULL;
    const char *opt_serum_skip_frames = NULL;
#endif

    cag_option_prepare(&cag_context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&cag_context))
    {
        identifier = cag_option_get(&cag_context);
        switch (identifier)
        {
        case 'c':
            config_file = cag_option_get_value(&cag_context);
            break;
        case 'r':
            opt_rom = cag_option_get_value(&cag_context);
            break;
        case 's':
            opt_serial = cag_option_get_value(&cag_context);
            break;
        case 'n':
            opt_no_serial = true;
            break;
#if defined(SERUM_SUPPORT)
        case 'u':
            opt_serum = true;
            break;
        case 't':
            opt_serum_timeout = cag_option_get_value(&cag_context);
            break;
        case 'p':
            opt_serum_skip_frames = cag_option_get_value(&cag_context);
            break;
#endif
        case 'i':
            opt_console_display = true;
            break;
        case 'd':
            opt_debug = true;
            break;
        case 'h':
            printf("Usage: ppuc [OPTION]...\n");
            cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
            return 0;
        }
    }

    if (!config_file)
    {
        printf("No config file provided. Use option -c /path/to/config/file.\n");
        return -1;
    }

    ppuc = new PPUC();

    // Load config file. But options set via command line are preferred.
    ppuc->LoadConfiguration(config_file);

    if (!opt_debug)
    {
        opt_debug = ppuc->GetDebug();
    }
    ppuc->SetDebug(opt_debug);

    if (opt_rom)
    {
        ppuc->SetRom(opt_rom);
    }
    else
    {
        opt_rom = ppuc->GetRom();
    }

    if (opt_serial)
    {
        ppuc->SetSerial(opt_serial);
    }
    else
    {
        // opt_serial will be ignored by ZeDMD later.
        opt_serial = ppuc->GetSerial();
    }

    // Initialize displays.
    // ZeDMD messes with USB ports. when searching for the DMD.
    // So it is important to start that search before PIN2DMD
    // or the RS485 BUS get initialized.
    std::thread t_zedmd;
#if defined(ZEDMD_SUPPORT)
    zedmd.IgnoreDevice(opt_serial);
    // zedmd_connected = (int)zedmd.OpenWiFi("192.168.178.125", 3333);
    zedmd_connected = (int)zedmd.Open();
    if (opt_debug)
        printf("ZeDMD: %d\n", zedmd_connected);
    if (zedmd_connected)
    {
        if (opt_debug)
            zedmd.EnableDebug();

        zedmd.DisablePreUpscaling();
        //zedmd.EnablePreUpscaling();

        t_zedmd = std::thread(ZeDmdThread);
    }
#endif

    pin2dmd_connected = Pin2dmdInit();
    if (opt_debug)
        printf("PIN2DMD: %d\n", pin2dmd_connected);
    std::thread t_pin2dmd;
    if (pin2dmd_connected)
    {
        t_pin2dmd = std::thread(Pin2DmdThread);
    }

    std::thread t_consoledmd;
    if (opt_console_display)
    {
        t_consoledmd = std::thread(ConsoleDmdThread);
    }

    std::thread t_resetdmd(ResetDmdThread);

    if (!opt_no_serial && !ppuc->Connect())
    {
        printf("Unable to open serial communication to PPUC boards.\n");
        return 1;
    }

    PinmameConfig config = {
        PINMAME_AUDIO_FORMAT_INT16,
        44100,
        "",
        &OnStateUpdated,
        &OnDisplayAvailable,
        &OnDisplayUpdated,
        &OnAudioAvailable,
        &OnAudioUpdated,
        &OnMechAvailable,
        &OnMechUpdated,
        &OnSolenoidUpdated,
        &OnConsoleDataUpdated,
        &IsKeyPressed,
        &OnLogMessage,
        NULL,
    };

#if defined(_WIN32) || defined(_WIN64)
    snprintf((char *)config.vpmPath, PINMAME_MAX_PATH, "%s%s\\pinmame\\", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
#else
    snprintf((char *)config.vpmPath, PINMAME_MAX_PATH, "%s/.pinmame/", getenv("HOME"));
#endif

#if defined(SERUM_SUPPORT)
    if (opt_serum)
    {
        int pwidth;
        int pheight;
        unsigned int pnocolors;
        unsigned int pntriggers;

        char tbuf[1024];
        strcpy(tbuf, config.vpmPath);
        if ((tbuf[strlen(tbuf) - 1] != '\\') && (tbuf[strlen(tbuf) - 1] != '/'))
            strcat(tbuf, "/");
        strcat(tbuf, "altcolor");

        bool serum_loaded = Serum_Load(tbuf, opt_rom, &pwidth, &pheight, &pnocolors, &pntriggers);
        if (serum_loaded)
        {
            if (opt_debug)
                printf("Serum: loaded %s.cRZ.\n", opt_rom);

            if (opt_serum_timeout)
            {
                uint16_t serum_timeout;
                std::stringstream st(opt_serum_timeout);
                st >> serum_timeout;
                Serum_SetIgnoreUnknownFramesTimeout(serum_timeout);
            }

            if (opt_serum_skip_frames)
            {
                uint8_t serum_skip_frames;
                std::stringstream ssf(opt_serum_skip_frames);
                ssf >> serum_skip_frames;
                Serum_SetMaximumUnknownFramesToSkip(serum_skip_frames);
            }
        }
        else
        {
            if (opt_debug)
                printf("Serum: %s.cRZ not found.\n", opt_rom);
            opt_serum = false;
        }
    }
#endif

    // Initialize the sound device
    const ALCchar *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *device = alcOpenDevice(defaultDeviceName);
    if (!device)
    {
        printf("failed to alcOpenDevice for %s\n", defaultDeviceName);
        return 1;
    }

    ALCcontext *context = alcCreateContext(device, NULL);
    if (!context)
    {
        printf("failed call to alcCreateContext\n");
        return 1;
    }

    alcMakeContextCurrent(context);
    alGenSources((ALuint)1, &_audioSource);
    alGenBuffers(MAX_AUDIO_BUFFERS, _audioBuffers);

    PinmameSetConfig(&config);

    PinmameSetDmdMode(PINMAME_DMD_MODE_RAW);
    // PinmameSetSoundMode(PINMAME_SOUND_MODE_ALTSOUND);
    PinmameSetHandleKeyboard(0);
    PinmameSetHandleMechanics(0);

#if defined(_WIN32) || defined(_WIN64)
    // Avoid compile error C2131. Use a larger constant value instead.
    PinmameLampState changedLampStates[256];
#else
    PinmameLampState changedLampStates[PinmameGetMaxLamps()];
#endif

    if (PinmameRun(opt_rom) == PINMAME_STATUS_OK)
    {
        // Pinball machines were slower than modern CPUs. There's no need to update states too frequently at full speed.
        int sleep_us = 1000;
        // Poll I/O boards for events (mainly switches) every 50us.
        int poll_interval_ms = 50;
        int poll_trigger = poll_interval_ms * 1000 / sleep_us;
        int index_recv = 0;

        ppuc->StartUpdates();

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));

            if (!game_state)
                continue;

            if (--poll_trigger <= 0)
            {
                poll_trigger = poll_interval_ms * 1000 / sleep_us;

                PPUCSwitchState *switchState;
                while ((switchState = ppuc->GetNextSwitchState()) != nullptr)
                {
                    if (opt_debug)
                    {
                        printf("Switch updated: #%d, %d\n", switchState->number, switchState->state);
                    }
                    PinmameSetSwitch(switchState->number, switchState->state);
                };
            }

            int count = PinmameGetChangedLamps(changedLampStates);
            for (int c = 0; c < count; c++)
            {
                uint16_t lampNo = changedLampStates[c].lampNo;
                uint8_t lampState = changedLampStates[c].state == 0 ? 0 : 1;

                if (opt_debug)
                {
                    printf("Lamp updated: #%d, %d\n", lampNo, lampState);
                }

                ppuc->SetLampState(lampNo, lampState);
            }
        }
    }

    stop_flag = true;
    if (t_pin2dmd.joinable())
    {
        t_pin2dmd.join();
    }
    if (t_zedmd.joinable())
    {
        t_zedmd.join();
    }
    if (t_consoledmd.joinable())
    {
        t_consoledmd.join();
    }
    if (t_resetdmd.joinable())
    {
        t_resetdmd.join();
    }

    if (!opt_no_serial)
    {
        // Close the serial device
        ppuc->Disconnect();
    }

    if (device)
        alcCloseDevice(device);

    return 0;
}
