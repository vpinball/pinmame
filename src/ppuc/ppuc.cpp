#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include <shared_mutex>
#include <mutex>

#include <AL/al.h>
#include <AL/alc.h>

#include "yaml-cpp/yaml.h"

#include "Event.h"
#include "../libpinmame/libpinmame.h"
#include "dmd/dmd.h"
#include "pin2dmd/pin2dmd.h"
#include "zedmd/zedmd.h"
#include "serialib/serialib.h"
#include "cargs/cargs.h"
#include "dmdcommon/dmdcommon.h"
#if defined(SERUM_SUPPORT)
    #include "../../../libserum/src/serum-decode.h"
#endif

typedef unsigned char UINT8;
typedef unsigned short UINT16;

#define MAX_AUDIO_BUFFERS 4
#define MAX_AUDIO_QUEUE_SIZE 10
#define MAX_EVENT_SEND_QUEUE_SIZE 10
#define MAX_EVENT_RECV_QUEUE_SIZE 10
#define MAX_IO_BOARDS 8

ALuint _audioSource;
ALuint _audioBuffers[MAX_AUDIO_BUFFERS];
std::queue<void*> _audioQueue;
int _audioChannels;
int _audioSampleRate;

// Event message buffer
UINT8 msg[6] = {0};
// Config Event message buffer
UINT8 cmsg[11] = {0};
// Serial object
serialib serial;

int pin2dmd = 0;
int zedmd = 0;

UINT8 dmd_planes_buffer[12288] = {0};
UINT8 dmd_serum_planes_buffer[12288] = {0};
UINT8 dmd_frame_buffer[16384] = {0};
UINT16 dmd_width;
UINT16 dmd_height;
int dmd_depth;
bool dmd_sam_spa;
bool dmd_ready = false;
std::shared_mutex dmd_shared_mutex;
std::condition_variable_any dmd_cv;

Event* event_send_queue[MAX_EVENT_SEND_QUEUE_SIZE];
Event* event_recv_queue[MAX_EVENT_RECV_QUEUE_SIZE];
int event_send_index = 0;
int event_recv_index = 0;
std::mutex event_send_mutex;
std::mutex event_recv_mutex;
std::condition_variable event_send_cv;

YAML::Node ppuc_config;

bool opt_debug = false;
bool opt_no_serial = false;
bool opt_serum = false;
bool opt_console_display = false;
int game_state = 0;

static struct cag_option options[] = {
    {
        .identifier = 'c',
        .access_letters = "c",
        .access_name = "config",
        .value_name = "VALUE",
        .description = "Path to config file (required)"
    },
    {
        .identifier = 'r',
        .access_letters = "r",
        .access_name = "rom",
        .value_name = "VALUE",
        .description = "Path to ROM file (optional, overwrites setting in config file)"
    },
    {
        .identifier = 's',
        .access_letters = "s",
        .access_name = "serial",
        .value_name = "VALUE",
        .description = "Serial device (optional, overwrites setting in config file)"
    },
    {
        .identifier = 'n',
        .access_letters = "n",
        .access_name = "no-serial",
        .value_name = NULL,
        .description = "No serial communication to controllers (optional)"
    },
#if defined(SERUM_SUPPORT)
    {
        .identifier = 'u',
        .access_letters = "u",
        .access_name = "serum",
        .value_name = NULL,
        .description = "Enable Serum colorization (optional)"
    },
    {
        .identifier = 't',
        .access_letters = "t",
        .access_name = "serum-timeout",
        .value_name = "VALUE",
        .description = "Serum timeout in milliseconds to ignore unknown frames (optional)"
    },
    {
        .identifier = 'p',
        .access_letters = "p",
        .access_name = "serum-skip-frames",
        .value_name = "VALUE",
        .description = "Serum ignore number of unknown frames (optional)"
    },
#endif
    {
        .identifier = 'i',
        .access_letters = "i",
        .access_name = "console-display",
        .value_name = NULL,
        .description = "Enable console display (optional)"
    },
    {
        .identifier = 'd',
        .access_letters = "d",
        .access_name = "debug",
        .value_name = NULL,
        .description = "Enable debug output (optional)"
    },
    {
        .identifier = 'h',
        .access_letters = "h",
        .access_name = "help",
        .description = "Show help"
    }
};


void queueEvent(Event* event) {
    std::unique_lock<std::mutex> ul(event_send_mutex);

    event_send_queue[event_send_index++] = event;

    if (event_send_index >= MAX_EVENT_SEND_QUEUE_SIZE) {
        event_send_index = 0;
    }

    ul.unlock();
    event_send_cv.notify_one();
}

Event* receiveEvent() {
    if (!opt_no_serial) {
        std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

        // Set a timeout of 1ms when waiting for an I/O board event.
        while ((std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start)).count() < 1000) {
            if (serial.available() >= 6) {
                UINT8 poll[6] = {0};
                if (serial.readBytes(poll, 6)) {
                    if (poll[0] == 255 && poll[5] == 255) {
                        return new Event(poll[1], (((UINT16) poll[2]) << 8) + poll[3], poll[4]);
                    }
                }
                return NULL;
            }
        }
    }

    return NULL;
}

void sendEvent(Event* event) {
    if (!opt_no_serial) {
        //     = (UINT8) 255;
        msg[1] = event->sourceId;
        msg[2] = event->eventId >> 8;
        msg[3] = event->eventId & 0xff;
        msg[4] = event->value;
        //     = (UINT8) 255;

        if (serial.writeBytes(msg, 6)) {
            if (opt_debug) printf("Sent event %d %d %d.\n", event->sourceId, event->eventId, event->value);
        } else {
            printf("Error: Could not send event %d %d %d.\n", event->sourceId, event->eventId, event->value);
        }

        if (event->sourceId == EVENT_POLL_EVENTS) {
            bool null_event = false;
            Event *event_recv;
            while (!null_event && (event_recv = receiveEvent())) {
                switch (event_recv->sourceId) {
                    case EVENT_NULL:
                        null_event = true;
                        break;

                    default:
                        if (opt_debug)
                            printf("Received event %d %d %d.\n", event_recv->sourceId, event_recv->eventId, event_recv->value);

                        event_recv_mutex.lock();
                        event_recv_queue[event_recv_index++] = event_recv;
                        event_recv_mutex.unlock();

                        if (event_recv_index >= MAX_EVENT_RECV_QUEUE_SIZE) {
                            event_recv_index = 0;
                        }
                        break;
                }

                // delete the event and free the memory
                delete event_recv;
            }
        }
    }

    // delete the event and free the memory
    delete event;
}

void sendEvent(ConfigEvent* event) {
    if (!opt_no_serial) {
        //      = (UINT8) 255;
        cmsg[1] = event->sourceId;
        cmsg[2] = event->boardId;
        cmsg[3] = event->topic;
        cmsg[4] = event->key;
        cmsg[5] = event->index;
        cmsg[6] = event->value >> 24;
        cmsg[7] = (event->value >> 16) & 0xff;
        cmsg[8] = (event->value >> 8) & 0xff;
        cmsg[9] = event->value & 0xff;
        //      = (UINT8) 255;

        if (serial.writeBytes(cmsg, 11)) {
            if (opt_debug) printf("Sent config event %d %d %d.\n", event->boardId, event->topic, event->key);
        } else {
            printf("Error: Could not send event %d %d %d.\n", event->boardId, event->topic, event->key);
        }
    }
    // delete the event and free the memory
    delete event;
}

void handleEvent(Event* event) {
    switch (event->sourceId) {
        case EVENT_SOURCE_SWITCH:
            if (opt_debug)
                printf("Switch update received: switchNo=%d, switchState=%d\n",
                        event->eventId,
                        event->value);
            PinmameSetSwitch(event->eventId, event->value);
            break;
    }

    // delete the event and free the memory
    delete event;
}

void EventThread() {
    int index = 0;

    while (true) {
        std::unique_lock<std::mutex> ul(event_send_mutex);
        event_send_cv.wait(ul, [index]() { return index != event_send_index; });

        if (index < event_send_index) {
            for (int i = index; i <= event_send_index; i++) {
                sendEvent(event_send_queue[i]);
            }
        }
        else {
            for (int i = index; i < MAX_EVENT_SEND_QUEUE_SIZE; i++) {
                sendEvent(event_send_queue[i]);
            }
            for (int i = 0; i <= event_send_index; i++) {
                sendEvent(event_send_queue[i]);
            }
        }
        index = event_send_index;
    }
}

void CALLBACK Game(PinmameGame* game) {
    printf("Game(): name=%s, description=%s, manufacturer=%s, year=%s, flags=%lu, found=%d\n",
           game->name, game->description, game->manufacturer, game->year, (unsigned long)game->flags, game->found);
}

void CALLBACK OnStateUpdated(int state) {
    if (opt_debug) printf("OnStateUpdated(): state=%d\n", state);

    if (!state) {
        exit(1);
    }
    else {
        PinmameMechConfig mechConfig;
        memset(&mechConfig, 0, sizeof(mechConfig));

        mechConfig.sol1 = 11;
        mechConfig.length = 240;
        mechConfig.steps = 240;
        mechConfig.type = NONLINEAR | REVERSE | ONESOL;
        mechConfig.sw[0].swNo = 32;
        mechConfig.sw[0].startPos = 0;
        mechConfig.sw[0].endPos = 5;

        PinmameSetMech(0, &mechConfig);

        game_state = state;
    }
}

void CALLBACK OnLogMessage(const char* format, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s\n", buffer);
}

void CALLBACK OnDisplayAvailable(int index, int displayCount, PinmameDisplayLayout* p_displayLayout) {
    if (opt_debug) printf("OnDisplayAvailable(): index=%d, displayCount=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
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

void CALLBACK OnDisplayUpdated(int index, void* p_displayData, PinmameDisplayLayout* p_displayLayout) {
    if (opt_debug) printf("OnDisplayUpdated(): index=%d, type=%d, top=%d, left=%d, width=%d, height=%d, depth=%d, length=%d\n",
                          index,
                          p_displayLayout->type,
                          p_displayLayout->top,
                          p_displayLayout->left,
                          p_displayLayout->width,
                          p_displayLayout->height,
                          p_displayLayout->depth,
                          p_displayLayout->length);

    if ((p_displayLayout->type & DMD) == DMD) {
        std::unique_lock<std::shared_mutex> ul(dmd_shared_mutex);

        dmd_width = p_displayLayout->width;
        dmd_height = p_displayLayout->height;
        dmd_depth = p_displayLayout->depth;
        memcpy(dmd_frame_buffer, p_displayData, dmd_width*dmd_height);
        //dmdConvertToFrame(dmd_width, dmd_height, (UINT8 *) p_displayData, dmd_frame_buffer, dmd_depth, dmd_sam_spa);

        dmd_ready = true;

        ul.unlock();
        dmd_cv.notify_all();
    }
    else {
        // todo
        //DumpAlphanumeric(index, (UINT16*)p_displayData, p_displayLayout);
    }
}

void Pin2DmdThread() {
    while (true) {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []() { return dmd_ready; });
        sl.unlock();

        DmdCommon_ConvertFrameToPlanes(dmd_width, dmd_height, dmd_frame_buffer, dmd_planes_buffer, dmd_depth);
        Pin2dmdRender(dmd_width, dmd_height, dmd_planes_buffer, dmd_depth, dmd_sam_spa);
    }
}

void ZeDmdThread() {
    while (true) {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []() { return dmd_ready; });
        sl.unlock();

        if (opt_serum) {
#if defined(SERUM_SUPPORT)
            UINT8 palette[192] = {0};
            UINT8 rotations[24] = {0};
            UINT32 triggerID;
            UINT32 hashcode;
            int frameID;

            Serum_SetStandardPalette(dmdGetDefaultPalette(dmd_depth), dmd_depth);

            if (Serum_ColorizeWithMetadata(dmd_frame_buffer, dmd_width, dmd_height,
                           &palette[0], &rotations[0], &triggerID, &hashcode, &frameID)) {

                DmdCommon_ConvertFrameToPlanes(dmd_width, dmd_height, dmd_frame_buffer, dmd_serum_planes_buffer, 6);
                ZeDmdRenderSerum(dmd_width, dmd_height, dmd_serum_planes_buffer, &palette[0], &rotations[0]);

                // todo: send DMD Event with triggerID
            }
#endif
        }
        else {
            DmdCommon_ConvertFrameToPlanes(dmd_width, dmd_height, dmd_frame_buffer, dmd_planes_buffer, dmd_depth);
            ZeDmdRender(dmd_width, dmd_height, dmd_planes_buffer, dmd_depth);
        }
    }
}

void ConsoleDmdThread() {
    while (true) {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []() { return dmd_ready; });

        if (opt_serum) {
#if defined(SERUM_SUPPORT)
            UINT8 palette[192] = {0};
            UINT8 rotations[24] = {0};
            UINT32 triggerID;
            UINT32 hashcode;
            int frameID;

            Serum_SetStandardPalette(dmdGetDefaultPalette(dmd_depth), dmd_depth);

            if (Serum_ColorizeWithMetadata(dmd_frame_buffer, dmd_width, dmd_height,
                           &palette[0], &rotations[0], &triggerID, &hashcode, &frameID)) {

                dmdConsoleRender(dmd_width, dmd_height, dmd_frame_buffer, 6);

                // todo: send DMD Event with triggerID
            }
#endif
        }
        else {
            dmdConsoleRender(dmd_width, dmd_height, dmd_frame_buffer, dmd_depth);
        }

        if (!opt_debug) printf("\033[%dA", dmd_height);
        sl.unlock();
    }
}

void ResetDmdThread() {
    while (true) {
        std::shared_lock<std::shared_mutex> sl(dmd_shared_mutex);
        dmd_cv.wait(sl, []() { return dmd_ready; });
        sl.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        dmd_ready = false;
    }
}

int CALLBACK OnAudioAvailable(PinmameAudioInfo* p_audioInfo) {
    if (opt_debug) printf("OnAudioAvailable(): format=%d, channels=%d, sampleRate=%.2f, framesPerSecond=%.2f, samplesPerFrame=%d, bufferSize=%d\n",
                          p_audioInfo->format,
                          p_audioInfo->channels,
                          p_audioInfo->sampleRate,
                          p_audioInfo->framesPerSecond,
                          p_audioInfo->samplesPerFrame,
                          p_audioInfo->bufferSize);

    _audioChannels = p_audioInfo->channels;
    _audioSampleRate = (int) p_audioInfo->sampleRate;

    for (int index = 0; index < MAX_AUDIO_BUFFERS; index++) {
        int bufferSize = p_audioInfo->samplesPerFrame * _audioChannels * sizeof(int16_t);
        void* p_buffer = malloc(bufferSize);
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

int CALLBACK OnAudioUpdated(void* p_buffer, int samples) {
    if (_audioQueue.size() >= MAX_AUDIO_QUEUE_SIZE) {
        while (!_audioQueue.empty()) {
            void* p_destBuffer = _audioQueue.front();

            free(p_destBuffer);
            _audioQueue.pop();
        }
    }

    int bufferSize = samples * _audioChannels * sizeof(int16_t);
    void* p_destBuffer = malloc(bufferSize);
    memcpy(p_destBuffer, p_buffer, bufferSize);

    _audioQueue.push(p_destBuffer);

    ALint buffersProcessed;
    alGetSourcei(_audioSource, AL_BUFFERS_PROCESSED, &buffersProcessed);

    if (buffersProcessed <= 0) {
        return samples;
    }

    while (buffersProcessed > 0) {
        ALuint buffer = 0;
        alSourceUnqueueBuffers(_audioSource, 1, &buffer);

        if (_audioQueue.size() > 0) {
            void* p_destBuffer = _audioQueue.front();

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

    if (state != AL_PLAYING) {
        alSourcePlay(_audioSource);
    }

    return samples;
}

void CALLBACK OnSolenoidUpdated(PinmameSolenoidState* p_solenoidState) {
    if (opt_debug) printf("OnSolenoidUpdated: solenoid=%d, state=%d\n", p_solenoidState->solNo,  p_solenoidState->state);
    queueEvent(new Event(EVENT_SOURCE_SOLENOID, (UINT16) p_solenoidState->solNo, (UINT8) p_solenoidState->state));
}

void CALLBACK OnMechAvailable(int mechNo, PinmameMechInfo* p_mechInfo) {
    if (opt_debug) printf("OnMechAvailable: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
                          mechNo,
                          p_mechInfo->type,
                          p_mechInfo->length,
                          p_mechInfo->steps,
                          p_mechInfo->pos,
                          p_mechInfo->speed);
}

void CALLBACK OnMechUpdated(int mechNo, PinmameMechInfo* p_mechInfo) {
    if (opt_debug) printf("OnMechUpdated: mechNo=%d, type=%d, length=%d, steps=%d, pos=%d, speed=%d\n",
                          mechNo,
                          p_mechInfo->type,
                          p_mechInfo->length,
                          p_mechInfo->steps,
                          p_mechInfo->pos,
                          p_mechInfo->speed);
}

void CALLBACK OnConsoleDataUpdated(void* p_data, int size) {
    if (opt_debug) printf("OnConsoleDataUpdated: size=%d\n", size);
}

int CALLBACK IsKeyPressed(PINMAME_KEYCODE keycode) {
    return 0;
}

int main (int argc, char *argv[]) {
    char identifier;
    cag_option_context cag_context;
    const char *config_file = NULL;
    const char *opt_rom = NULL;
    const char *opt_serial = NULL;
#if defined(SERUM_SUPPORT)
    const char *opt_serum_timeout = NULL;
    const char *opt_serum_skip_frames = NULL;
#endif
    UINT8 boardsToPoll[MAX_IO_BOARDS];
    UINT8 numBoardsToPoll = 0;

    cag_option_prepare(&cag_context, options, CAG_ARRAY_SIZE(options), argc, argv);
    while (cag_option_fetch(&cag_context)) {
        identifier = cag_option_get(&cag_context);
        switch (identifier) {
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

    if (!config_file) {
        printf("No config file provided. Use option -c /path/to/config/file.\n");
        return -1;
    }

    // Load config file. But options set via command line are preferred.
    ppuc_config = YAML::LoadFile(config_file);

    if (!opt_debug) opt_debug = ppuc_config["debug"].as<bool>();

    std::string c_rom = ppuc_config["rom"].as<std::string>();
    if (!opt_rom) opt_rom = c_rom.c_str();

    std::string c_serial = ppuc_config["serialPort"].as<std::string>();
    if (!opt_serial) opt_serial = c_serial.c_str();

    PinmameConfig config = {
            AUDIO_FORMAT_INT16,
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
    };

#if defined(_WIN32) || defined(_WIN64)
    snprintf((char*)config.vpmPath, PINMAME_MAX_VPM_PATH, "%s%s\\pinmame\\", getenv("HOMEDRIVE"), getenv("HOMEPATH"));
#else
    snprintf((char*)config.vpmPath, PINMAME_MAX_VPM_PATH, "%s/.pinmame/", getenv("HOME"));
#endif

#if defined(SERUM_SUPPORT)
    if (opt_serum) {
        int pwidth;
        int pheight;
        unsigned int pnocolors;
        unsigned int pntriggers;

        char tbuf[1024];
        strcpy(tbuf, config.vpmPath);
        if ((tbuf[strlen(tbuf) - 1] != '\\') && (tbuf[strlen(tbuf) - 1] != '/')) strcat(tbuf, "/");
        strcat(tbuf, "altcolor");

        bool serum_loaded = Serum_Load(tbuf, opt_rom, &pwidth, &pheight, &pnocolors, &pntriggers);
        if (serum_loaded) {
            if (opt_debug) printf("Serum: loaded %s.cRZ.\n", opt_rom);

            if (opt_serum_timeout) {
                UINT16 serum_timeout;
                std::stringstream st(opt_serum_timeout);
                st >> serum_timeout;
                Serum_SetIgnoreUnknownFramesTimeout(serum_timeout);
            }

            if (opt_serum_skip_frames) {
                UINT8 serum_skip_frames;
                std::stringstream ssf(opt_serum_skip_frames);
                ssf >> serum_skip_frames;
                Serum_SetMaximumUnknownFramesToSkip(serum_skip_frames);
            }
        }
        else {
            if (opt_debug) printf("Serum: %s.cRZ not found.\n", opt_rom);
            opt_serum = false;
        }
    }
#endif

    // Initialize displays.
    pin2dmd = Pin2dmdInit();
    if (opt_debug) printf("PIN2DMD: %d\n", pin2dmd);
    std::thread t_pin2dmd;
    if (pin2dmd) {
        t_pin2dmd = std::thread(Pin2DmdThread);
    }

    zedmd = ZeDmdInit(opt_serial);
    std::thread t_zedmd;
    if (zedmd) {
        t_zedmd = std::thread(ZeDmdThread);
    }

    std::thread t_consoledmd;
    if (opt_console_display) {
      t_consoledmd = std::thread(ConsoleDmdThread);
    }

    std::thread t_resetdmd(ResetDmdThread);

    std::thread t_event;
    if (!opt_no_serial) {
        // Connection to serial port.
        char errorOpening = serial.openDevice(opt_serial, 115200);
        // If connection fails, return the error code otherwise, display a success message
        if (errorOpening != 1) {
            printf("Unable to open serial device: %s\n", opt_serial);
            return errorOpening;
        }

        // Disable DTR, otherwise Arduino will reset permanently.
        serial.clearDTR();
        msg[0] = (UINT8) 255;
        msg[5] = (UINT8) 255;
        cmsg[0] = (UINT8) 255;
        cmsg[10] = (UINT8) 255;

        // Wait for the serial communication to be established before continuing.
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        t_event = std::thread(EventThread);
    }

    const YAML::Node& boards = ppuc_config["boards"];
    for (YAML::Node n_board : boards) {
        if (n_board["pollEvents"].as<bool>()) {
            boardsToPoll[numBoardsToPoll++] = n_board["number"].as<UINT8>();
        }
    }

    // Send switch configuration to I/O boards
    const YAML::Node& switches = ppuc_config["switches"];
    for (YAML::Node n_switch : switches) {
        UINT8 index = 0;
        sendEvent(new ConfigEvent(
                n_switch["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_SWITCHES,
                index++,
                (UINT8) CONFIG_TOPIC_PORT,
                n_switch["port"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_switch["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_SWITCHES,
                index++,
                (UINT8) CONFIG_TOPIC_NUMBER,
                n_switch["number"].as<UINT32>()
                ));
    }

    // Send PWM configuration to I/O boards
    const YAML::Node& pwmOutput = ppuc_config["pwmOutput"];
    for (YAML::Node n_pwmOutput : pwmOutput) {
        UINT8 index = 0;
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_PORT,
                n_pwmOutput["port"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_NUMBER,
                n_pwmOutput["number"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_POWER,
                n_pwmOutput["power"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_MIN_PULSE_TIME,
                n_pwmOutput["minPulseTime"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_MAX_PULSE_TIME,
                n_pwmOutput["maxPulseTime"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_HOLD_POWER,
                n_pwmOutput["holdPower"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_HOLD_POWER_ACTIVATION_TIME,
                n_pwmOutput["holdPowerActivationTime"].as<UINT32>()
        ));
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_FAST_SWITCH,
                n_pwmOutput["fastFlipSwitch"].as<UINT32>()
        ));
        std::string c_type = n_pwmOutput["type"].as<std::string>();
        UINT32 type = 1; // "coil"
        if (strcmp(c_type.c_str(), "flasher")) {
            type = 2;
        }
        else if (strcmp(c_type.c_str(), "lamp")) {
            type = 3;
        }
        sendEvent(new ConfigEvent(
                n_pwmOutput["board"].as<UINT8>(),
                (UINT8) CONFIG_TOPIC_PWM,
                index++,
                (UINT8) CONFIG_TOPIC_TYPE,
                type
        ));
    }

    // Wait before continuing.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Tell I/O boards to read initial switch states, for example coin door closed.
    sendEvent(new Event(EVENT_READ_SWITCHES, 1));

    // Initialize the sound device
    const ALCchar *defaultDeviceName = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
    ALCdevice *device = alcOpenDevice(defaultDeviceName);
    ALCcontext *context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);
    alGenSources((ALuint) 1, &_audioSource);
    alGenBuffers(MAX_AUDIO_BUFFERS, _audioBuffers);

    PinmameSetConfig(&config);

    PinmameSetDmdMode(PINMAME_DMD_MODE::RAW);
    PinmameSetHandleKeyboard(0);
    PinmameSetHandleMechanics(0);

#if defined(_WIN32) || defined(_WIN64)
    // Avoid compile error C2131. Use a larger constant value instead.
    PinmameLampState changedLampStates[256];
#else
    PinmameLampState changedLampStates[PinmameGetMaxLamps()];
#endif

	if (PinmameRun(opt_rom) == OK) {
        dmd_sam_spa = PinmameGetHardwareGen() & (SAM | SPA);

        // Pinball machines were slower than modern CPUs. There's no need to update states too frequently at full speed.
        int sleep_us = 1000;
        // Poll I/O boards for events (mainly switches) every 50us.
        int poll_interval_ms = 50;
        int poll_trigger = poll_interval_ms * 1000 / sleep_us;
        int index_recv = 0;

        while (1) {
			std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));

            if (!game_state) continue;

            if (--poll_trigger <= 0) {
                poll_trigger = poll_interval_ms * 1000 / sleep_us;

                for (int i = 0; i < numBoardsToPoll; i++) {
                    queueEvent(new Event(EVENT_POLL_EVENTS, 1, boardsToPoll[i]));
                }
            }

            event_recv_mutex.lock();
            bool handle_events = (index_recv != event_recv_index);
            event_recv_mutex.unlock();

            if (handle_events) {
                if (index_recv < event_recv_index) {
                    for (int i = index_recv; i <= event_recv_index; i++) {
                        handleEvent(event_recv_queue[i]);
                    }
                }
                else {
                    for (int i = index_recv; i < MAX_EVENT_RECV_QUEUE_SIZE; i++) {
                        handleEvent(event_recv_queue[i]);
                    }
                    for (int i = 0; i <= event_recv_index; i++) {
                        handleEvent(event_recv_queue[i]);
                    }
                }
                index_recv = event_recv_index;
            }

            int count = PinmameGetChangedLamps(changedLampStates);
            for (int c = 0; c < count;) {
                UINT16 lampNo = changedLampStates[c].lampNo;
                UINT8 lampState = changedLampStates[c].state == 0 ? 0 : 1;

                if (opt_debug) printf("Lamp updated: lampNo=%d, lampState=%d\n",
                       lampNo,
                       lampState);

                queueEvent(new Event(EVENT_SOURCE_LIGHT, lampNo, lampState));
            }
		}
	}

    // Close the serial device
    serial.closeDevice();

	return 0;
}
