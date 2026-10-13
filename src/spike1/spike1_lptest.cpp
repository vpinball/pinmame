// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - runs a set through libpinmame (pinmame64.dll) the way a VPX table
// does, without VPX: the library is loaded at run time, the game started by its set name, coins
// dropped and START pressed on a timetable (seconds from when the machine runs), switches pressed
// for a time, and the video displays (the LCD insert, WWE's LCD, Spider-Man's RGB matrix) saved as
// PNG pictures once a second, with the coils, lamps and sound the library hands out reported along
// the way (and the frame rate in the library's own log line at the end).
//
//    spike1_lptest <pinmame64.dll> <vpm path> <set> [--seconds N] [--frames <dir>] [--report <s>]
//                  [--coin <switch>] [--coins N] [--coin-at <s>] [--start <switch>] [--start-at <s>]
//                  [--press <s>:<switch>[:<ms>]]... [--lamp <n>]...
//
// The vpm path holds roms/<set>.zip (or roms/<set>/) and nvram/<set>.nv as PinMAME has them; the
// defaults are Ghostbusters LE's switches (LEFT COIN 81, START BUTTON 78). A press shorter than
// 20 ms is timed exactly, 0 ms closes and opens the switch back to back; a negative switch is one
// closed at rest, opened for the time. --lamp prints that lamp's level as it changes. SPIKE1_LOG
// passes the subsystem's log on. Windows only: it loads the library with LoadLibrary.

#include "../libpinmame/libpinmame.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {

using clock_type = std::chrono::steady_clock;
clock_type::time_point g_t0 = clock_type::now();
double now_s() { return std::chrono::duration<double>(clock_type::now() - g_t0).count(); }

std::string g_frames;
std::atomic<int> g_state{0};
std::atomic<unsigned> g_display_updates{0}, g_display_changes{0}, g_saved{0}, g_solenoid_changes{0};
std::atomic<long long> g_audio_samples{0};
std::mutex g_peak_lock;
float g_peak = 0.0f;
int g_next_save = 0;

bool write_png(const std::string &path, const std::vector<uint8_t> &rgb, uint32_t w, uint32_t h)
{
	static std::array<uint32_t, 256> crc_table = [] {
		std::array<uint32_t, 256> t{};
		for (uint32_t n = 0; n < 256; n++) {
			uint32_t c = n;
			for (int k = 0; k < 8; k++) c = (c & 1) ? 0xedb88320u ^ (c >> 1) : c >> 1;
			t[n] = c;
		}
		return t;
	}();
	auto crc = [](const uint8_t *p, size_t n, uint32_t c = 0xffffffffu) {
		for (size_t i = 0; i < n; i++) c = crc_table[(c ^ p[i]) & 0xff] ^ (c >> 8);
		return c;
	};
	std::vector<uint8_t> raw;
	raw.reserve(size_t(h) * (size_t(w) * 3 + 1));
	for (uint32_t y = 0; y < h; y++) {
		raw.push_back(0); // no filter
		raw.insert(raw.end(), rgb.begin() + ptrdiff_t(size_t(y) * w * 3), rgb.begin() + ptrdiff_t(size_t(y + 1) * w * 3));
	}
	std::vector<uint8_t> z = { 0x78, 0x01 };
	uint32_t a = 1, b2 = 0;
	for (uint8_t v : raw) { a = (a + v) % 65521; b2 = (b2 + a) % 65521; }
	for (size_t at = 0; at < raw.size(); ) {
		const size_t n = std::min<size_t>(65535, raw.size() - at);
		z.push_back(at + n == raw.size() ? 1 : 0);
		z.push_back(uint8_t(n)); z.push_back(uint8_t(n >> 8)); z.push_back(uint8_t(~n)); z.push_back(uint8_t(~n >> 8));
		z.insert(z.end(), raw.begin() + ptrdiff_t(at), raw.begin() + ptrdiff_t(at + n));
		at += n;
	}
	const uint32_t adler = (b2 << 16) | a;
	z.push_back(uint8_t(adler >> 24)); z.push_back(uint8_t(adler >> 16)); z.push_back(uint8_t(adler >> 8)); z.push_back(uint8_t(adler));
	std::FILE *fp = std::fopen(path.c_str(), "wb");
	if (!fp) return false;
	auto be32 = [](uint32_t v, uint8_t *o) { o[0] = uint8_t(v >> 24); o[1] = uint8_t(v >> 16); o[2] = uint8_t(v >> 8); o[3] = uint8_t(v); };
	auto chunk = [&](const char *type, const uint8_t *data, size_t n) {
		uint8_t head[8];
		be32(uint32_t(n), head);
		std::memcpy(head + 4, type, 4);
		std::fwrite(head, 1, 8, fp);
		if (n) std::fwrite(data, 1, n, fp);
		uint32_t c = crc(head + 4, 4);
		c = crc(data, n, c) ^ 0xffffffffu;
		uint8_t tail[4];
		be32(c, tail);
		std::fwrite(tail, 1, 4, fp);
	};
	static const uint8_t sig[8] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a };
	std::fwrite(sig, 1, 8, fp);
	uint8_t ihdr[13];
	be32(w, ihdr); be32(h, ihdr + 4);
	ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
	chunk("IHDR", ihdr, 13);
	chunk("IDAT", z.data(), z.size());
	chunk("IEND", nullptr, 0);
	std::fclose(fp);
	return true;
}

void PINMAMECALLBACK on_state(int state, void *)
{
	g_state = state;
	std::printf("[%7.3f] state %d\n", now_s(), state);
}

void PINMAMECALLBACK on_display_available(int index, int count, PinmameDisplayLayout *l, void *)
{
	std::printf("[%7.3f] display %d of %d: type %d, %dx%d at %d,%d, depth %d\n", now_s(), index, count, l->type, l->width, l->height, l->left, l->top, l->depth);
}

void PINMAMECALLBACK on_display_updated(int index, void *data, PinmameDisplayLayout *l, void *)
{
	// the library passes the frame only when it changed: the last one stands for the screen until then
	static std::vector<uint8_t> last;
	g_display_updates++;
	if (data) g_display_changes++;
	if ((l->type & PINMAME_DISPLAY_TYPE_VIDEO) != PINMAME_DISPLAY_TYPE_VIDEO) return;
	if (data) {
		const uint8_t *p = static_cast<const uint8_t *>(data);
		last.assign(p, p + size_t(l->width) * l->height * (l->depth == 16 ? 2 : 3));
	}
	if (last.empty() || g_frames.empty()) return;
	const int second = int(now_s());
	if (second < g_next_save) return;
	g_next_save = second + 1;
	const uint32_t w = uint32_t(l->width), h = uint32_t(l->height);
	std::vector<uint8_t> rgb(size_t(w) * h * 3);
	if (l->depth == 16) { // 5.6.5
		const uint16_t *p = reinterpret_cast<const uint16_t *>(last.data());
		for (size_t i = 0; i < size_t(w) * h; i++) {
			const uint16_t c = p[i];
			rgb[3 * i] = uint8_t(((c >> 11) & 0x1f) * 255 / 31);
			rgb[3 * i + 1] = uint8_t(((c >> 5) & 0x3f) * 255 / 63);
			rgb[3 * i + 2] = uint8_t((c & 0x1f) * 255 / 31);
		}
	}
	else std::memcpy(rgb.data(), last.data(), rgb.size());
	char name[64];
	std::snprintf(name, sizeof(name), "/display%d_%03d.png", index, second);
	if (write_png(g_frames + name, rgb, w, h)) g_saved++;
}

int PINMAMECALLBACK on_audio_available(PinmameAudioInfo *a, void *)
{
	std::printf("[%7.3f] audio: format %d, %d channels, %.0f Hz, %.2f fps, %d samples a frame, buffer %d\n", now_s(),
		a->format, a->channels, a->sampleRate, a->framesPerSecond, a->samplesPerFrame, a->bufferSize);
	return a->samplesPerFrame;
}

int PINMAMECALLBACK on_audio_updated(void *buffer, int samples, void *)
{
	const float *f = static_cast<const float *>(buffer);
	float peak = 0.0f;
	for (int i = 0; i < samples; i++) peak = std::max(peak, std::fabs(f[i]));
	{
		std::lock_guard<std::mutex> lock(g_peak_lock);
		g_peak = std::max(g_peak, peak);
	}
	g_audio_samples += samples;
	return samples;
}

void PINMAMECALLBACK on_mech_available(int, PinmameMechInfo *, void *) {}
void PINMAMECALLBACK on_mech_updated(int, PinmameMechInfo *, void *) {}

void PINMAMECALLBACK on_solenoid(PinmameSolenoidState *s, void *)
{
	if (g_solenoid_changes++ < 200) std::printf("[%7.3f] solenoid %d %s\n", now_s(), s->solNo, s->state ? "on" : "off");
}

void PINMAMECALLBACK on_console(void *, int, void *) {}
int PINMAMECALLBACK is_key_pressed(PINMAME_KEYCODE, void *) { return 0; }

void PINMAMECALLBACK on_log(PINMAME_LOG_LEVEL level, const char *format, va_list args, void *)
{
	char buffer[1024];
	std::vsnprintf(buffer, sizeof(buffer), format, args);
	std::printf("[%7.3f] %s: %s\n", now_s(), level == PINMAME_LOG_LEVEL_ERROR ? "ERROR" : level == PINMAME_LOG_LEVEL_INFO ? "INFO" : "DEBUG", buffer);
}

void PINMAMECALLBACK on_sound_command(int, int, void *) {}

template <typename T> bool bind(HMODULE dll, T &fn, const char *name)
{
	fn = reinterpret_cast<T>(reinterpret_cast<void *>(GetProcAddress(dll, name)));
	if (!fn) std::fprintf(stderr, "%s is not in the library\n", name);
	return fn != nullptr;
}

} // namespace

int main(int argc, char **argv)
{
	if (argc < 4) {
		std::fprintf(stderr, "usage: spike1_lptest <pinmame64.dll> <vpm path> <set> [--seconds N] [--frames <dir>] [--report <s>]\n"
		                     "       [--coin <switch>] [--coins N] [--coin-at <s>] [--start <switch>] [--start-at <s>]\n"
		                     "       [--press <s>:<switch>[:<ms>]]... [--lamp <n>]...\n");
		return 2;
	}
	const std::string dll_path = argv[1], vpm = argv[2], set = argv[3];
	double seconds = 30.0, coin_at = 8.0, start_at = 12.0; // seconds of a running machine
	struct press { double at = 0.0; int sw = 0, ms = 200; bool down = false, done = false; };
	double report_every = 5.0;
	std::vector<press> presses;
	std::vector<int> watch_lamps;
	int coin = 81, coins = 4, start = 78;
	for (int i = 4; i < argc; i++) {
		const std::string a = argv[i];
		auto next = [&]() -> const char * { return i + 1 < argc ? argv[++i] : ""; };
		if (a == "--seconds") seconds = std::atof(next());
		else if (a == "--frames") g_frames = next();
		else if (a == "--coin") coin = std::atoi(next());
		else if (a == "--coins") coins = std::atoi(next());
		else if (a == "--coin-at") coin_at = std::atof(next());
		else if (a == "--start") start = std::atoi(next());
		else if (a == "--start-at") start_at = std::atof(next());
		else if (a == "--report") report_every = std::atof(next());
		else if (a == "--lamp") watch_lamps.push_back(std::atoi(next()));
		else if (a == "--press") { // <s>:<switch>[:<ms>]; a negative switch is opened (one closed at rest)
			press p;
			if (std::sscanf(next(), "%lf:%d:%d", &p.at, &p.sw, &p.ms) >= 2) presses.push_back(p);
		}
		else { std::fprintf(stderr, "unknown argument %s\n", a.c_str()); return 2; }
	}

	HMODULE dll = LoadLibraryExA(dll_path.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (!dll) { std::fprintf(stderr, "cannot load %s (%lu)\n", dll_path.c_str(), GetLastError()); return 1; }
	decltype(&PinmameSetConfig) set_config;
	decltype(&PinmameSetHandleKeyboard) set_keyboard;
	decltype(&PinmameSetHandleMechanics) set_mechanics;
	decltype(&PinmameSetSolenoidMask) set_solenoid_mask;
	decltype(&PinmameRun) run;
	decltype(&PinmameIsRunning) is_running;
	decltype(&PinmameStop) stop;
	decltype(&PinmameSetSwitch) set_switch;
	decltype(&PinmameGetHardwareGen) hardware_gen;
	decltype(&PinmameGetMaxLamps) max_lamps;
	decltype(&PinmameGetChangedLamps) changed_lamps;
	if (!bind(dll, set_config, "PinmameSetConfig") || !bind(dll, set_keyboard, "PinmameSetHandleKeyboard")
		|| !bind(dll, set_mechanics, "PinmameSetHandleMechanics") || !bind(dll, set_solenoid_mask, "PinmameSetSolenoidMask")
		|| !bind(dll, run, "PinmameRun") || !bind(dll, is_running, "PinmameIsRunning") || !bind(dll, stop, "PinmameStop")
		|| !bind(dll, set_switch, "PinmameSetSwitch") || !bind(dll, hardware_gen, "PinmameGetHardwareGen")
		|| !bind(dll, max_lamps, "PinmameGetMaxLamps") || !bind(dll, changed_lamps, "PinmameGetChangedLamps"))
		return 1;

	PinmameConfig config = {
		PINMAME_AUDIO_FORMAT_FLOAT, 44100, "",
		&on_state, &on_display_available, &on_display_updated, &on_audio_available, &on_audio_updated,
		&on_mech_available, &on_mech_updated, &on_solenoid, &on_console, &is_key_pressed, &on_log, &on_sound_command,
	};
	std::snprintf(const_cast<char *>(config.vpmPath), PINMAME_MAX_PATH, "%s", vpm.c_str());
	set_config(&config);
	set_keyboard(0);
	set_mechanics(0);
	set_solenoid_mask(2, 2); // the lamps and coils as levels, as VPX tables for Spike 1 ask

	g_t0 = clock_type::now();
	const PINMAME_STATUS status = run(set.c_str());
	std::printf("[%7.3f] PinmameRun(%s) = %d\n", now_s(), set.c_str(), status);
	if (status != PINMAME_STATUS_OK) return 1;

	std::vector<PinmameLampState> lamps(size_t(std::max(1, max_lamps())));
	unsigned lamp_changes = 0;
	int coins_done = 0;
	bool started = false;
	double next_report = 1.0, run_at = -1.0;
	for (;;) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		if (run_at < 0.0) {
			if (g_state != 1) { if (now_s() > 600.0) break; continue; }
			run_at = now_s();
			std::printf("[%7.3f] running: hardware gen 0x%llx\n", run_at, (unsigned long long)hardware_gen());
		}
		const double t = now_s() - run_at;
		if (t >= seconds) break;
		const int changed = changed_lamps(lamps.data());
		for (int k = 0; k < changed; k++) {
			lamp_changes++;
			if (std::find(watch_lamps.begin(), watch_lamps.end(), lamps[k].lampNo) != watch_lamps.end())
				std::printf("[%7.3f] lamp %d %d\n", now_s(), lamps[k].lampNo, lamps[k].state);
		}
		// a coin each 0.6 s from coin_at, closed for 0.15 s
		if (coins_done < coins && t >= coin_at + 0.6 * coins_done) {
			set_switch(coin, 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(150));
			set_switch(coin, 0);
			std::printf("[%7.3f] coin %d (switch %d)\n", now_s(), ++coins_done, coin);
		}
		if (!started && t >= start_at) {
			started = true;
			set_switch(start, 1);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
			set_switch(start, 0);
			std::printf("[%7.3f] START (switch %d)\n", now_s(), start);
		}
		for (auto &p : presses) {
			if (p.done || t < p.at) continue;
			const int sw = p.sw < 0 ? -p.sw : p.sw, pressed = p.sw < 0 ? 0 : 1;
			if (!p.down && p.ms < 20) { // timed here exactly: the loop's own step is 20 ms
				p.down = p.done = true;
				set_switch(sw, pressed);
				const auto until = clock_type::now() + std::chrono::microseconds(p.ms * 1000);
				while (clock_type::now() < until) {}
				set_switch(sw, !pressed);
				std::printf("[%7.3f] switch %d %s for %d ms\n", now_s(), sw, pressed ? "pressed" : "opened", p.ms);
			} else if (!p.down) {
				p.down = true;
				set_switch(sw, pressed);
			} else if (t >= p.at + p.ms / 1000.0) {
				p.done = true;
				set_switch(sw, !pressed);
				std::printf("[%7.3f] switch %d %s for %d ms\n", now_s(), sw, pressed ? "pressed" : "opened", p.ms);
			}
		}
		if (t >= next_report) {
			next_report += report_every;
			float peak;
			{
				std::lock_guard<std::mutex> lock(g_peak_lock);
				peak = g_peak;
				g_peak = 0.0f;
			}
			std::printf("[%7.3f] running %d: %u display updates (%u changed), %u pictures, %u solenoid changes, %u lamp changes, %lld audio samples, peak %.3f\n",
				t, is_running(), g_display_updates.load(), g_display_changes.load(), g_saved.load(), g_solenoid_changes.load(), lamp_changes, g_audio_samples.load(), peak);
		}
	}
	std::printf("[%7.3f] stopping\n", now_s());
	stop();
	for (int i = 0; i < 500 && is_running(); i++) std::this_thread::sleep_for(std::chrono::milliseconds(20));
	std::printf("[%7.3f] stopped (running %d)\n", now_s(), is_running());
	return 0;
}
