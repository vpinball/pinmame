// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the PinMAME-facing side of the machine
//
// src/wpc/spike1.c is compiled as part of PinMAME and knows nothing about the subsystem's C++
// types; the functions below (spike1_public.h) are the whole contract between them. The title's
// files arrive as PinMAME ROM regions, which spike1.c passes in: the subsystem opens no files.

#include "spike1_public.h"
#include "spike1_cpu.h"
#include "spike1_linux.h"
#include "spike1_memory.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

extern machine_config *p2k_active_config;

namespace {

// The shim's device world for one machine: its config, the CPU's address space over the guest
// memory, and Linux on top
struct machine
{
	running_machine running;
	machine_config config;
	spike1_memory memory;
	std::unique_ptr<address_space> space;
	spike1_cpu_device *cpu = nullptr;
	std::unique_ptr<spike1_linux> os;
};

std::unique_ptr<machine> g_machine;
bool g_log_all = false;

void copy_text(const std::string &s, char *dst, unsigned size)
{
	if (!dst || !size) return;
	std::snprintf(dst, size, "%s", s.c_str());
}

u32 unmapped_read(void *, offs_t addr, u32) { if (g_machine && g_machine->os) g_machine->os->fault(addr, false); return 0; }
void unmapped_write(void *, offs_t addr, u32, u32) { if (g_machine && g_machine->os) g_machine->os->fault(addr, true); }

spike1_devices *devices() { return g_machine && g_machine->os ? &g_machine->os->devices() : nullptr; }
uint64_t now_ns() { return g_machine && g_machine->os ? g_machine->os->now_ns() : 0; }

} // anonymous namespace

extern "C" {

int spike1_pinmame_start(const char *game, const spike1_file *files, unsigned count,
                         const unsigned char *nvram, unsigned nvram_size, char *error, unsigned error_size)
{
	g_machine.reset();
	g_log_all = std::getenv("SPIKE1_LOG") != nullptr; // SPIKE1_LOG=1: the subsystem's whole log on stderr
	auto m = std::make_unique<machine>();
	device_t::s_machine = &m->running;
	p2k_active_config = &m->config;

	m->cpu = &SPIKE1_CPU(m->config, "maincpu", SPIKE1_CPU_HZ);
	p2k_bus_callbacks bus{ unmapped_read, unmapped_write, nullptr };
	m->space = std::make_unique<address_space>(bus);
	m->memory.attach(*m->space);
	m->cpu->p2k_set_space(AS_PROGRAM, m->space.get());
	for (auto &dev : m->config.devices) dev->p2k_resolve();
	for (auto &dev : m->config.devices) dev->p2k_start();
	for (auto &dev : m->config.devices) dev->p2k_reset();

	spike1_linux::config cfg;
	const std::string dir = std::string("/games/") + (game ? game : "");
	for (unsigned i = 0; i < count; i++)
		if (files[i].name && files[i].data)
			cfg.files.push_back({ dir + "/" + files[i].name, files[i].data, files[i].size });
	cfg.executable = dir + "/game";
	cfg.environment = { "HOME=/root", "PATH=/bin:/usr/bin:/usr/local/bin", "LANG=C", "LC_ALL=C", "GAMES_PATH=/games" };
	cfg.clock_hz = SPIKE1_CPU_HZ;
	if (nvram && nvram_size) cfg.nvram.assign(nvram, nvram + nvram_size);
	cfg.log = [](const std::string &line) {
		// a stop is worth seeing in any build; the rest only when asked for
		if (g_log_all || line.rfind("stop:", 0) == 0 || line.rfind("nvram:", 0) == 0) std::fprintf(stderr, "[spike1] %s\n", line.c_str());
	};
	m->os = std::make_unique<spike1_linux>(*m->cpu, m->memory);
	g_machine = std::move(m); // before start(): a fault during it reports through g_machine
	std::string reason;
	if (!g_machine->os->start(cfg, reason)) {
		copy_text(reason, error, error_size);
		g_machine.reset();
		return 0;
	}
	return 1;
}

void spike1_pinmame_stop(void)
{
	if (g_machine && p2k_active_config == &g_machine->config) p2k_active_config = nullptr;
	if (g_machine && device_t::s_machine == &g_machine->running) device_t::s_machine = nullptr;
	g_machine.reset();
}

int spike1_pinmame_running(char *reason, unsigned reason_size)
{
	if (!g_machine) { copy_text("not started", reason, reason_size); return 0; }
	if (g_machine->os->state() == spike1_linux::status::running) return 1;
	copy_text(g_machine->os->stop_reason(), reason, reason_size);
	return 0;
}

int spike1_pinmame_run(int cycles)
{
	if (g_machine && cycles > 0 && g_machine->os->state() == spike1_linux::status::running)
		g_machine->os->run(cycles);
	return cycles;
}

unsigned spike1_pinmame_nvram(unsigned char *dst, unsigned capacity)
{
	if (!g_machine) return 0;
	const std::vector<uint8_t> block = g_machine->os->nvram();
	if (dst && block.size() <= capacity) std::memcpy(dst, block.data(), block.size());
	return unsigned(block.size());
}

int spike1_pinmame_power_down(unsigned max_ms)
{
	if (!g_machine || g_machine->os->state() != spike1_linux::status::running) return 0;
	return g_machine->os->power_down(uint64_t(max_ms) * 1000000ull, SPIKE1_CPU_HZ / 100) ? 1 : 0;
}

int spike1_pinmame_switch(unsigned index, int *number, int *closed_at_rest, const char **name)
{
	const spike1_devices *d = devices();
	if (!d || index >= d->switches().size()) return 0;
	const auto &s = d->switches()[index];
	if (number) *number = s.number;
	if (closed_at_rest) *closed_at_rest = d->switch_closed(s.node, s.position);
	if (name) *name = s.name.c_str();
	return 1;
}

int spike1_pinmame_find_switch(const char *name)
{
	const spike1_devices *d = devices();
	const spike1_devices::switch_info *s = d && name ? d->find_switch(name) : nullptr;
	return s ? s->number : -1;
}

void spike1_pinmame_set_switch(int number, int closed)
{
	spike1_devices *d = devices();
	if (!d) return;
	for (const auto &s : d->switches())
		if (s.number == number && d->switch_closed(s.node, s.position) != (closed != 0))
			d->set_switch(s.node, s.position, closed != 0, now_ns());
}

int spike1_pinmame_coil(unsigned index, int *number, const char **name)
{
	const spike1_devices *d = devices();
	if (!d || index >= d->coils().size()) return 0;
	if (number) *number = d->coils()[index].number;
	if (name) *name = d->coils()[index].name.c_str();
	return 1;
}

unsigned spike1_pinmame_coil_level(int number)
{
	const spike1_devices *d = devices();
	if (!d) return 0;
	for (const auto &c : d->coils())
		if (c.number == number) return d->coil_level(c.node, c.position, now_ns());
	return 0;
}

unsigned spike1_pinmame_led_count(void)
{
	const spike1_devices *d = devices();
	return d ? unsigned(d->leds().size()) : 0;
}

int spike1_pinmame_led(unsigned index, int *number, int *kind, const char **name)
{
	const spike1_devices *d = devices();
	if (!d || index >= d->leds().size()) return 0;
	const auto &led = d->leds()[index];
	if (number) *number = led.number;
	if (kind) *kind = led.kind;
	if (name) *name = led.name.c_str();
	return 1;
}

int spike1_pinmame_motor(unsigned index, int *position, int *moving)
{
	const spike1_devices *d = devices();
	int16_t p = 0;
	bool m = false;
	if (!d || !d->motor_at(index, p, m, now_ns())) return 0;
	if (position) *position = p;
	if (moving) *moving = m;
	return 1;
}

int spike1_pinmame_stepper_home(unsigned node, unsigned stepper, int switch_number)
{
	spike1_devices *d = devices();
	return d && node < 128 && stepper <= 4 && switch_number >= 0 &&
		d->link_stepper_home(uint8_t(node), uint8_t(stepper), uint16_t(switch_number));
}

unsigned spike1_pinmame_led_level(unsigned index)
{
	const spike1_devices *d = devices();
	if (!d || index >= d->leds().size()) return 0;
	return d->led_level(d->leds()[index].node, d->leds()[index].position);
}

unsigned spike1_pinmame_dmd(unsigned char *dots)
{
	const spike1_devices *d = devices();
	if (!d) return 0;
	if (dots) std::memcpy(dots, d->dmd_frame(), SPIKE1_DMD_WIDTH * SPIKE1_DMD_HEIGHT);
	return unsigned(d->dmd_frame_count());
}

int spike1_pinmame_insert_present(void)
{
	const spike1_devices *d = devices();
	return d && d->insert_present();
}

int spike1_pinmame_insert(unsigned short *rgb565, unsigned *backlight)
{
	spike1_devices *d = devices();
	const uint16_t *px = d ? d->insert_pixels(now_ns()) : nullptr;
	if (backlight) *backlight = d ? d->insert_backlight() : 0;
	if (!px) return 0;
	if (rgb565) std::memcpy(rgb565, px, SPIKE1_INSERT_WIDTH * SPIKE1_INSERT_HEIGHT * sizeof(uint16_t));
	return 1;
}

unsigned spike1_pinmame_audio_rate(void)
{
	const spike1_devices *d = devices();
	return d ? d->audio_rate() : 44100;
}

unsigned spike1_pinmame_audio(short *dst, unsigned frames)
{
	spike1_devices *d = devices();
	return d && dst ? unsigned(d->audio_take(dst, frames)) : 0;
}

} // extern "C"

static_assert(SPIKE1_LED_MOTOR == spike1_devices::LED_KIND_MOTOR, "the LED class bits differ");
static_assert(SPIKE1_DMD_WIDTH == spike1_devices::DMD_WIDTH && SPIKE1_DMD_HEIGHT == spike1_devices::DMD_HEIGHT, "the DMD sizes differ");
static_assert(SPIKE1_INSERT_WIDTH == spike1_devices::INSERT_WIDTH && SPIKE1_INSERT_HEIGHT == spike1_devices::INSERT_HEIGHT, "the insert sizes differ");
