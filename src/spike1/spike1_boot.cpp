// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - standalone bring-up harness
//
// Runs a Spike 1 game program on the subsystem alone, without PinMAME around it, and prints what
// the emulated Linux sees: console output, device traffic and the first system call or event that
// is not supported yet.
//
//   spike1boot --root <dir> --game <guest path> [--state <dir>] [--seconds <n>] [--clock <hz>] [--trace]
//
// <dir> is the machine's extracted root filesystem; <guest path> is the program inside it, for
// example /games/<name>/game

#include "spike1_cpu.h"
#include "spike1_linux.h"
#include "spike1_memory.h"

#include <chrono>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

extern machine_config *p2k_active_config;

static spike1_linux *s_linux;

// The DMD as text: one character per dot, darkest to brightest
static void print_dmd(const spike1_linux &os)
{
	static const char shades[] = " .,:;-=+*%#&$@@@";
	const uint8_t *dots = os.devices().dmd_frame();
	std::printf("DMD at %.3f s, frame %llu\n", double(os.now_ns()) / 1e9, (unsigned long long)os.devices().dmd_frame_count());
	for (uint32_t y = 0; y < spike1_devices::DMD_HEIGHT; y++) {
		char line[spike1_devices::DMD_WIDTH + 3];
		line[0] = '|';
		for (uint32_t x = 0; x < spike1_devices::DMD_WIDTH; x++) line[1 + x] = shades[dots[y * spike1_devices::DMD_WIDTH + x] & 15];
		line[spike1_devices::DMD_WIDTH + 1] = '|';
		line[spike1_devices::DMD_WIDTH + 2] = 0;
		std::printf("%s\n", line);
	}
	std::fflush(stdout);
}

// The LCD insert's current frame as a PPM file in dir, named after the emulated time
static void save_insert(spike1_linux &os, const std::string &dir)
{
	auto &dev = os.devices();
	const int32_t frame = dev.insert_frame_index(os.now_ns());
	const uint16_t *px = dev.insert_pixels(os.now_ns());
	char name[64];
	std::snprintf(name, sizeof(name), "/insert_%07.3f.ppm", double(os.now_ns()) / 1e9);
	std::printf("insert at %.3f s: frame %d, backlight %u%s\n", double(os.now_ns()) / 1e9, frame, dev.insert_backlight(), px ? (" -> " + dir + name).c_str() : "");
	if (!px) return;
	FILE *f = std::fopen((dir + name).c_str(), "wb");
	if (!f) return;
	std::fprintf(f, "P6\n%u %u\n255\n", spike1_devices::INSERT_WIDTH, spike1_devices::INSERT_HEIGHT);
	for (uint32_t i = 0; i < spike1_devices::INSERT_WIDTH * spike1_devices::INSERT_HEIGHT; i++) {
		const uint8_t rgb[3] = { uint8_t((px[i] >> 11) << 3), uint8_t(((px[i] >> 5) & 63) << 2), uint8_t((px[i] & 31) << 3) };
		std::fwrite(rgb, 1, 3, f);
	}
	std::fclose(f);
}

static u32 unmapped_read(void *, offs_t addr, u32) { if (s_linux) s_linux->fault(addr, false); return 0; }
static void unmapped_write(void *, offs_t addr, u32, u32) { if (s_linux) s_linux->fault(addr, true); }

int main(int argc, char **argv)
{
	spike1_linux::config cfg;
	double seconds = 30.0, dmd_every = 0.0, insert_every = 0.0, slice_ms = 1.0;
	std::vector<std::pair<uint32_t, uint32_t>> peeks;
	std::string state_dir, files_dir;
	std::vector<std::vector<uint8_t>> file_bytes;
	// --switch <seconds>:<name>[:<ms held, default 200>]: close a switch for a while, by its name
	struct press { double at, held_ms; std::string name; bool down = false, up = false; };
	std::vector<press> presses;
	struct home_link { int node, stepper, number; };
	std::vector<home_link> home_links;
	bool list_switches = false, show_outputs = false;
	std::string node_dump, wav_path;
	for (int i = 1; i < argc; i++) {
		const std::string arg = argv[i];
		auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
		if (arg == "--root") cfg.root = next();
		else if (arg == "--files") files_dir = next(); // the game folder's files, held in memory instead of a root
		else if (arg == "--game") cfg.executable = next();
		else if (arg == "--state") state_dir = next();
		else if (arg == "--seconds") seconds = std::atof(next().c_str());
		else if (arg == "--clock") cfg.clock_hz = uint32_t(std::strtoul(next().c_str(), nullptr, 0));
		else if (arg == "--slice") slice_ms = std::atof(next().c_str()); // emulated ms per run() call (PinMAME: one frame, 16.7)
		else if (arg == "--trace") cfg.trace = true;
		else if (arg == "--dmd") dmd_every = std::atof(next().c_str());
		else if (arg == "--insert") insert_every = std::atof(next().c_str()); // the LCD insert, as PPM files in the state directory
		else if (arg == "--list-switches") list_switches = true;
		else if (arg == "--outputs") show_outputs = true; // coil changes as they happen, lit LEDs at the end
		else if (arg == "--node-dump") node_dump = next(); // every node-bus frame except switch reads, to a file
		else if (arg == "--wav") wav_path = next(); // the sound, as a WAV file
		else if (arg == "--switch") {
			const std::string v = next();
			const size_t a = v.find(':'), b = v.rfind(':');
			press p{ std::atof(v.c_str()), 200.0, v.substr(a + 1) };
			if (b != a && b + 1 < v.size() && std::isdigit(uint8_t(v[b + 1]))) { p.held_ms = std::atof(v.c_str() + b + 1); p.name = v.substr(a + 1, b - a - 1); }
			presses.push_back(p);
		}
		else if (arg == "--stepper-home") { // <node>:<stepper>:<switch number>, a home switch on another board
			const std::string v = next();
			const size_t a = v.find(':'), b = v.rfind(':');
			home_links.push_back({ std::atoi(v.c_str()), std::atoi(v.c_str() + a + 1), std::atoi(v.c_str() + b + 1) });
		}
		else if (arg == "--peek") { // <address>:<bytes> of guest memory, printed at the end
			const std::string v = next();
			peeks.emplace_back(uint32_t(std::strtoul(v.c_str(), nullptr, 0)), uint32_t(std::strtoul(v.substr(v.find(':') + 1).c_str(), nullptr, 0)));
		}
		else { std::fprintf(stderr, "unknown argument %s\n", arg.c_str()); return 2; }
	}
	if (!files_dir.empty() && !cfg.executable.empty()) {
		const std::string guest_dir = cfg.executable.substr(0, cfg.executable.rfind('/'));
		std::error_code ec;
		for (auto it = std::filesystem::directory_iterator(std::filesystem::u8path(files_dir), ec); !ec && it != std::filesystem::directory_iterator(); it.increment(ec)) {
			if (!it->is_regular_file(ec)) continue;
			std::ifstream in(it->path(), std::ios::binary | std::ios::ate);
			std::vector<uint8_t> bytes(size_t(in.tellg()));
			in.seekg(0);
			in.read(reinterpret_cast<char *>(bytes.data()), std::streamsize(bytes.size()));
			file_bytes.push_back(std::move(bytes));
			cfg.files.push_back({ guest_dir + "/" + it->path().filename().u8string(), file_bytes.back().data(), file_bytes.back().size() });
		}
		std::printf("files: %zu from %s in memory\n", cfg.files.size(), files_dir.c_str());
	}
	if ((cfg.root.empty() && cfg.files.empty()) || cfg.executable.empty()) {
		std::fprintf(stderr, "usage: spike1boot --root <dir> --game <guest path> [--state <dir>] [--seconds <n>] [--clock <hz>] [--trace] [--dmd <every n s>]\n");
		return 2;
	}
	if (state_dir.empty()) state_dir = "spike1_state";
	// the machine's NVRAM, kept between runs as PinMAME keeps a game's .nv file
	const std::string nvram_path = state_dir + "/spike1.nv";
	if (FILE *nv = std::fopen(nvram_path.c_str(), "rb")) {
		uint8_t chunk[65536];
		for (size_t n; (n = std::fread(chunk, 1, sizeof(chunk), nv)) > 0; ) cfg.nvram.insert(cfg.nvram.end(), chunk, chunk + n);
		std::fclose(nv);
	}
	cfg.environment = { "HOME=/root", "PATH=/bin:/usr/bin:/usr/local/bin", "LANG=C", "LC_ALL=C", "GAMES_PATH=/games" };
	cfg.log = [](const std::string &line) { std::printf("%s\n", line.c_str()); std::fflush(stdout); };

	running_machine machine;
	machine_config config;
	device_t::s_machine = &machine;
	p2k_active_config = &config;

	spike1_cpu_device &cpu = SPIKE1_CPU(config, "maincpu", cfg.clock_hz);
	p2k_bus_callbacks bus{ unmapped_read, unmapped_write, nullptr };
	address_space space(bus);
	spike1_memory memory;
	memory.attach(space);
	cpu.p2k_set_space(AS_PROGRAM, &space);
	for (auto &dev : config.devices) dev->p2k_resolve();
	for (auto &dev : config.devices) dev->p2k_start();
	for (auto &dev : config.devices) dev->p2k_reset();

	spike1_linux linux_os(cpu, memory);
	s_linux = &linux_os;
	std::string error;
	if (!linux_os.start(cfg, error)) { std::fprintf(stderr, "start failed: %s\n", error.c_str()); return 1; }
	for (const auto &h : home_links)
		if (!linux_os.devices().link_stepper_home(uint8_t(h.node), uint8_t(h.stepper), uint16_t(h.number))) {
			std::fprintf(stderr, "no switch %d for the home of node %d stepper %d\n", h.number, h.node, h.stepper);
			return 2;
		}

	if (list_switches) {
		for (const auto &s : linux_os.devices().switches())
			std::printf("switch %2u-%-2u #%-3u %s%s%s\n", s.node, s.position, s.number, s.name.c_str(),
				s.active_high ? " (active high)" : "", linux_os.devices().switch_closed(s.node, s.position) ? " (closed at rest)" : "");
		for (const auto &c : linux_os.devices().coils())
			std::printf("coil   %2u-%-2u #%-3u %s\n", c.node, c.position, c.number, c.name.c_str());
		for (const auto &l : linux_os.devices().leds())
			std::printf("led    %2u-%-3u #%-3u kind %-2u %s\n", l.node, l.position, l.number, l.kind, l.name.c_str());
	}
	for (const auto &p : presses)
		if (!linux_os.devices().find_switch(p.name)) { std::fprintf(stderr, "no switch named \"%s\"\n", p.name.c_str()); return 2; }

	FILE *dump = node_dump.empty() ? nullptr : std::fopen(node_dump.c_str(), "w");
	if (dump)
		linux_os.devices().set_frame_observer([dump](uint64_t now_ns, const uint8_t *frame, uint32_t len) {
			if (len > 2 && (frame[2] == 0x11 || frame[2] == 0xff)) return;
			std::fprintf(dump, "%10.6f %2u %02x", double(now_ns) / 1e9, frame[0] & 0x7f, frame[2]);
			for (uint32_t i = 3; i + 2 < len; i++) std::fprintf(dump, " %02x", frame[i]);
			std::fprintf(dump, "\n");
		});

	std::vector<uint8_t> coil_levels;
	FILE *wav = wav_path.empty() ? nullptr : std::fopen(wav_path.c_str(), "wb");
	uint32_t wav_frames = 0, wav_rate = 0;
	std::vector<int16_t> audio(2 * 4096);
	if (wav) { static const uint8_t blank[44] = {}; std::fwrite(blank, 1, sizeof(blank), wav); } // the header goes in at the end
	const int slice = int(cfg.clock_hz / 1000 * slice_ms); // emulated time per run() call
	const auto t0 = std::chrono::steady_clock::now();
	uint64_t next_dmd_ns = dmd_every > 0 ? uint64_t(dmd_every * 1e9) : UINT64_MAX;
	uint64_t next_insert_ns = insert_every > 0 ? uint64_t(insert_every * 1e9) : UINT64_MAX;
	while (linux_os.run(slice) == spike1_linux::status::running && linux_os.now_ns() < uint64_t(seconds * 1e9)) {
		const double now_s = double(linux_os.now_ns()) / 1e9;
		for (auto &p : presses) {
			const auto *s = linux_os.devices().find_switch(p.name);
			if (!p.down && now_s >= p.at) {
				p.down = true;
				linux_os.devices().set_switch(s->node, s->position, !linux_os.devices().switch_closed(s->node, s->position), linux_os.now_ns());
				std::printf("[%10.6f] switch %s %s\n", now_s, s->name.c_str(), linux_os.devices().switch_closed(s->node, s->position) ? "closed" : "opened");
			} else if (p.down && !p.up && now_s >= p.at + p.held_ms / 1000.0) {
				p.up = true;
				linux_os.devices().set_switch(s->node, s->position, !linux_os.devices().switch_closed(s->node, s->position), linux_os.now_ns());
				std::printf("[%10.6f] switch %s %s\n", now_s, s->name.c_str(), linux_os.devices().switch_closed(s->node, s->position) ? "closed" : "opened");
			}
		}
		if (show_outputs) {
			const auto &coils = linux_os.devices().coils();
			coil_levels.resize(coils.size());
			for (size_t i = 0; i < coils.size(); i++) {
				const uint8_t level = linux_os.devices().coil_level(coils[i].node, coils[i].position, linux_os.now_ns());
				if (level != coil_levels[i])
					std::printf("[%10.6f] coil %u %s %s\n", now_s, coils[i].number, coils[i].name.c_str(),
						level ? ("on " + std::to_string(level * 100 / 255) + "%").c_str() : "off");
				coil_levels[i] = level;
			}
		}
		if (wav) {
			wav_rate = linux_os.devices().audio_rate();
			while (const size_t n = linux_os.devices().audio_take(audio.data(), audio.size() / 2)) {
				std::fwrite(audio.data(), 4, n, wav);
				wav_frames += uint32_t(n);
			}
		}
		if (linux_os.now_ns() >= next_insert_ns) {
			save_insert(linux_os, state_dir);
			next_insert_ns += uint64_t(insert_every * 1e9);
		}
		if (linux_os.now_ns() >= next_dmd_ns) {
			print_dmd(linux_os);
			next_dmd_ns += uint64_t(dmd_every * 1e9);
		}
	}
	if (dmd_every > 0) print_dmd(linux_os);
	if (show_outputs) {
		std::string lit;
		for (const auto &l : linux_os.devices().leds())
			if (const uint8_t level = linux_os.devices().led_level(l.node, l.position))
				lit += " " + l.name + (level < 255 ? " (" + std::to_string(level * 100 / 255) + "%)" : "") + ",";
		std::printf("LEDs lit at the end:%s\n", lit.empty() ? " none" : lit.substr(0, lit.size() - 1).c_str());
	}
	for (const auto &pk : peeks) {
		std::printf("peek %08x:", pk.first);
		for (uint32_t i = 0; i < pk.second; i += 4) { uint32_t w = 0; memory.read(pk.first + i, &w, 4); std::printf(" %08x", w); }
		std::printf("\n");
	}
	{
		// switched off like a machine: the game commits what it keeps in memory first
		if (linux_os.state() == spike1_linux::status::running) linux_os.power_down(5000000000ull, int(cfg.clock_hz / 100));
		std::error_code ec;
		std::filesystem::create_directories(std::filesystem::u8path(state_dir), ec);
		const std::vector<uint8_t> nv = linux_os.nvram();
		if (FILE *f = std::fopen(nvram_path.c_str(), "wb")) { std::fwrite(nv.data(), 1, nv.size(), f); std::fclose(f); }
	}
	if (dump) std::fclose(dump);
	if (wav) { // RIFF header: PCM, 2 channels, 16 bits
		auto u32 = [](uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = uint8_t(v >> (8 * i)); };
		uint8_t h[44] = { 'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E', 'f', 'm', 't', ' ', 16, 0, 0, 0, 1, 0, 2, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 16, 0, 'd', 'a', 't', 'a' };
		u32(h + 4, 36 + wav_frames * 4);
		u32(h + 24, wav_rate);
		u32(h + 28, wav_rate * 4);
		u32(h + 40, wav_frames * 4);
		std::fseek(wav, 0, SEEK_SET);
		std::fwrite(h, 1, sizeof(h), wav);
		std::fclose(wav);
		std::printf("sound: %.3f s at %u Hz -> %s\n", double(wav_frames) / (wav_rate ? wav_rate : 1), wav_rate, wav_path.c_str());
	}
	if (std::getenv("SPIKE1_NODE_STATS")) // node-bus frames per command and board
		for (const auto &c : linux_os.devices().node_bus_counts())
			std::printf("node bus cmd %02x node %2u: %llu\n", c.first >> 8, c.first & 0xff, (unsigned long long)c.second);
	const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

	const char *state = linux_os.state() == spike1_linux::status::running ? "still running" :
		linux_os.state() == spike1_linux::status::exited ? "exited" : "stopped";
	const double idle = linux_os.cycles() ? 100.0 * double(linux_os.idle_cycles()) / double(linux_os.cycles()) : 0.0;
	std::printf("\n%s after %.3f s emulated (%.3f s wall, %llu system calls, %u threads alive, %.1f%% idle)\n", state,
		double(linux_os.now_ns()) / 1e9, wall, (unsigned long long)linux_os.syscall_count(),
		unsigned(linux_os.thread_count()), idle);
	if (!linux_os.stop_reason().empty()) std::printf("reason: %s\n", linux_os.stop_reason().c_str());
	return 0;
}
