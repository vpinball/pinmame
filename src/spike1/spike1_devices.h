// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the CPU board's devices, as the game reaches them through /dev
//
// Each device answers the Linux driver interface the game uses (read/write/ioctl on its node),
// so the game's own driver code runs unchanged above it. A device that would make the caller
// wait on real hardware (the ADC until it has sampled a buffer, the DMD until the panel takes the
// next frame) says until when, in emulated time; the system-call layer blocks the thread.
//
// Bring-up stage: devices are modelled as the game's traffic shows they are needed, and what they
// do not model yet is logged.

#pragma once

#include "spike1_memory.h"

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

class spike1_devices
{
public:
	enum class kind { none, i2c, node_bus, dmd_spi, cpu_spi, adc, i2s, amp, backlight, gpio, rtc, dmd, other };

	// A read/write/ioctl result that means "block until wait_ns, then call again"
	static constexpr int32_t WAIT = INT32_MIN;

	static constexpr uint32_t DMD_WIDTH = 128, DMD_HEIGHT = 32;
	static constexpr uint32_t DMD_FRAME_BYTES = DMD_WIDTH * DMD_HEIGHT / 8 * 4; // four bitplanes

	struct config
	{
		bool trace = false;   // log every transfer, not only new kinds of traffic
		std::function<void (const std::string &)> log;
		// address of a symbol in the game program, 0 if it has none: the node bus reads the
		// game's own board tables, so nothing about a title is hard-coded here
		std::function<uint32_t (const std::string &)> symbol;
		std::function<uint32_t (const std::string &)> symbol_size; // the symbol's size in bytes, 0 if unknown
		// The title's files beside the game program (node firmware, the LCD insert's image): their
		// names, and one file's bytes (false when it is not there)
		std::function<std::vector<std::string> ()> game_files;
		std::function<bool (const std::string &name, std::vector<uint8_t> &out)> read_game_file;
	};

	explicit spike1_devices(spike1_memory &mem);

	void start(const config &cfg);
	// What the board keeps across power cycles - its EEPROMs - as one block; set_nvram() after
	// start() puts a saved block back (and ignores one it does not recognise)
	std::vector<uint8_t> nvram() const;
	void set_nvram(const uint8_t *data, size_t size);

	static kind kind_for_path(const std::string &path);

	// Per open file state the devices keep, owned by the caller's file table
	struct handle
	{
		kind type = kind::none;
		std::string path;
		uint16_t i2c_slave = 0;
	};

	// Results follow the system-call convention - a byte count or value, or -errno - or WAIT
	int32_t read(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	int32_t write(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	int32_t ioctl(handle &h, uint32_t req, uint32_t arg, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	// Whether a read would return data now, for poll/select
	bool readable(const handle &h, uint64_t now_ns) const;

	// Switches, as the title's own tables define them: the board (node) and position each one is
	// wired to, its number in the game and its English name
	struct switch_info
	{
		uint16_t number = 0;        // the manual's Switch Reference number; the CPU board's C1-C16 are 101-116
		uint8_t node = 0, position = 0;
		bool active_high = false;   // the wire reads 1 when the switch is closed
		std::string name;
	};
	const std::vector<switch_info> &switches() const { return m_switches; }
	const switch_info *find_switch(const std::string &name) const;
	// A switch change at now_ns: the boards' reflexes (flippers, pops, slings) answer it at once
	void set_switch(uint8_t node, uint8_t position, bool closed, uint64_t now_ns);
	bool switch_closed(uint8_t node, uint8_t position) const { return (m_sw_closed[node & 127][(position >> 3) & 7] >> (position & 7)) & 1; }

	// Coils and LED channels, as the title's own device table names them
	struct output_info
	{
		// The factory manual's number - a coil's Driver Reference, an LED channel's Light Reference -
		// from the title's coil and LED tables; the device table's own number for one they leave out
		uint16_t number = 0;
		uint8_t node = 0, position = 0;
		std::string name;
		// LED channels: the LED table's class bits. By the channels' names: 1 lamp, 2 GI string,
		// 4 flasher, 8 motor drive (an H-bridge on LED drivers), 0x10 and 0x20 cabinet lights
		uint16_t kind = 0;
	};
	static constexpr uint16_t LED_KIND_MOTOR = 0x08;
	static constexpr uint16_t CPU_SWITCH_BASE = 100;
	static constexpr uint16_t TOPPER_SWITCH_BASE = 120;
	const std::vector<output_info> &coils() const { return m_coils; }
	const std::vector<output_info> &leds() const { return m_leds; }
	// What a coil driver puts out at now_ns, 0 (off) to 255 (full on): the board's PWM duty
	uint8_t coil_level(uint8_t node, uint8_t position, uint64_t now_ns) const;
	// An LED channel's level, 0-255, as the game last set it
	uint8_t led_level(uint8_t node, uint8_t position) const;
	// For a host that animates a mechanism: the position of a board's motor
	bool motor_state(uint8_t node, uint8_t index, int16_t &position, bool &moving, uint64_t now_ns) const;
	// The same for the i-th motor the game has configured, in board and motor order, then its
	// steppers (a stepper's position is its step within the turn), then the limit motors
	bool motor_at(size_t i, int16_t &position, bool &moving, uint64_t now_ns) const;
	// A stepper whose home switch is wired to another board, which the game watches itself (Game of
	// Thrones' dragons): that switch, by its manual number, is closed while the stepper stands at
	// step 0, as it does at power-on. False when the title has no such switch
	bool link_stepper_home(uint8_t node, uint8_t stepper, uint16_t switch_number);
	// A motor the game runs itself between two limit switches through an H-bridge on two LED
	// channels (KISS's Starchild): while the forward channel is lit it goes toward the away switch,
	// while the backward one is, toward home, taking travel_ms from one switch to the other. The
	// channels and switches by their manual numbers; the motor stands at home at power-on, and
	// comes after the steppers in motor_at(), as 0 (home) to 100 (away). False when the title has
	// no such channel or switch
	bool link_limit_motor(uint16_t forward_led, uint16_t backward_led, uint16_t home_switch, uint16_t away_switch, uint32_t travel_ms);

	// Sound: what the game sends its DAC, 16-bit stereo at audio_rate() Hz. The device takes the
	// game's writes at the pace the DAC plays them in emulated time; a host takes the samples
	// (interleaved left/right) as it plays them. Unclaimed samples beyond a second are dropped
	uint32_t audio_rate() const { return m_audio_rate; }
	size_t audio_take(int16_t *out, size_t frames);

	// The LCD insert - a node board with its own 160x128 display - when the title has one: the
	// frame of the title's lcdinsert.bin it shows at now_ns (-1: none), its backlight (0-255), and
	// that frame as RGB565 pixels, row by row as the player sees it (nullptr: nothing to show)
	static constexpr uint32_t INSERT_WIDTH = 160, INSERT_HEIGHT = 128;
	bool insert_present() const { return m_insert_frames != 0; }
	int32_t insert_frame_index(uint64_t now_ns) const;
	uint8_t insert_backlight() const { return m_insert_backlight; }
	const uint16_t *insert_pixels(uint64_t now_ns);

	// The last frame the game sent to the DMD as 16 shades per dot (0-15), row by row
	const uint8_t *dmd_frame() const { return m_dmd_dots; }
	// Frames the game sent, per command byte and board, for bring-up statistics
	const std::map<uint32_t, uint64_t> &node_bus_counts() const { return m_nb_counts; }
	uint64_t dmd_frame_count() const { return m_dmd_frames; }
	// Sees every addressed node-bus frame the game sends, for diagnostics
	using frame_observer = std::function<void (uint64_t now_ns, const uint8_t *frame, uint32_t len)>;
	void set_frame_observer(frame_observer observer) { m_frame_observer = std::move(observer); }

	// The mains go away: the line sense reads 0 V from now on, which the game takes as a power
	// failure - the moment it commits what it keeps in memory to its NVRAM files
	void power_off() { m_power_off = true; }

private:
	// A 24xx-series serial EEPROM: a two-byte word address, then sequential bytes from there
	struct eeprom
	{
		uint16_t slave = 0;
		std::vector<uint8_t> data;
		uint32_t pointer = 0;
	};

	// An MCP4631 dual digital potentiometer (the amplifier's volume controls): sixteen 9-bit
	// registers, a command byte [address << 4 | command << 2 | data bits 9:8] and a data byte
	struct digipot
	{
		uint16_t slave = 0;
		uint16_t reg[16] = {};
		uint8_t pointer = 0;
	};

	spike1_memory &m_mem;
	config m_cfg;
	std::vector<eeprom> m_eeproms;
	std::vector<digipot> m_digipots;

	// AC line sense: a sampled, rectified mains waveform - flat 0 V once the power goes off
	uint64_t m_adc_next_ns = 0;   // when the buffer being sampled is complete
	uint32_t m_adc_phase = 0;
	bool m_power_off = false;

	// Node bus: the RS-485 link to the playfield and cabinet node boards
	std::deque<uint8_t> m_nb_reply;          // reply bytes waiting for the game's read
	std::vector<uint8_t> m_nb_nodes;         // bus addresses of the title's boards
	bool m_nb_nodes_known = false;
	size_t m_nb_poll = 0;
	std::map<uint32_t, uint64_t> m_nb_counts;   // (cmd << 8 | node) -> frames
	std::set<uint32_t> m_nb_logged;          // (node << 8 | command) already reported as not modelled
	uint8_t m_bridge_version[3] = {};        // of the title's netbridge firmware; zero when it has none
	std::vector<uint32_t> m_chip_part;       // [proc key - 1]: NXP part ID of the title's node chips, in the game's order
	uint32_t m_block_base = 0, m_block_stride = 0;
	uint32_t m_nb_received[128] = {};        // frames each board has taken, as its GetStatus counts them
	uint32_t m_lcd_image_id = 0;             // the LCD insert holds the title's lcdinsert.bin: its ID word
	frame_observer m_frame_observer;

	// A motor a board runs on its own, closed loop on its encoder: the game commands home or a
	// target position and polls the status
	struct motor
	{
		int16_t position = 0, from = 0, target = 0;
		uint64_t start_ns = 0, end_ns = 0;   // the move under way, if end_ns > now
		bool homed = false;
	};
	std::map<uint32_t, motor> m_motors;      // (node << 8 | motor)
	void motor_move(motor &m, int16_t target, uint64_t now_ns, uint64_t duration_ns);
	int16_t motor_position(const motor &m, uint64_t now_ns) const;

	// A stepper a board runs on its own (Whoa Nellie's score and credit reels, Game of Thrones'
	// dragons): the game sets its steps per turn and home switch input, then sends targets - the
	// shorter way round never, but forward, or backward when the target has bit 15
	struct stepper
	{
		uint16_t steps = 200;                // per turn
		int8_t home_input = -1;              // the board's switch input that is closed at step 0
		uint16_t from = 0, target = 0, distance = 0;
		bool backward = false;
		uint64_t start_ns = 0, end_ns = 0;   // the move under way, if end_ns > now
	};
	std::map<uint32_t, stepper> m_steppers;  // (node << 8 | stepper)
	std::map<uint32_t, std::pair<uint8_t, uint8_t>> m_home_links; // (node << 8 | stepper) -> its home switch's board and position
	uint16_t stepper_position(const stepper &s, uint64_t now_ns) const;
	void stepper_home_switches(uint64_t now_ns);

	// A motor between two limit switches (link_limit_motor())
	struct limit_motor
	{
		uint8_t forward[2] = {}, backward[2] = {};  // the LED channels: board, position
		uint8_t home[2] = {}, away[2] = {};         // the switches: board, position
		int64_t travel_ns = 1;
		int64_t at_ns = 0;                          // its way from home, 0 to travel_ns
		int drive = 0;                              // since since_ns: 1 toward away, -1 toward home
		uint64_t since_ns = 0;
	};
	std::vector<limit_motor> m_limit_motors;
	int64_t limit_motor_position(const limit_motor &m, uint64_t now_ns) const;
	void limit_motors_run(uint64_t now_ns);         // after every LED update and switch read

	// Coils. A board drives a coil on the game's command - a pulse at one power, then optionally a
	// second phase at another - or on its own when a switch it watches closes (a reflex), so
	// flippers, pops and slings do not wait for the game
	static constexpr uint32_t COILS_PER_NODE = 16;
	struct coil
	{
		uint8_t power1 = 0, power2 = 0;
		uint64_t phase1_end_ns = 0, phase2_end_ns = 0;
		bool holding = false;          // at power2 for as long as the reflex switch stays closed
		bool reflex = false;
		uint8_t trigger = 0, eos = 0;  // reflex switches: position | REFLEX_USED [| REFLEX_INVERT]
		uint8_t reflex_power1 = 0, reflex_power2 = 0;
		uint32_t reflex_pulse_ms = 0, reflex_holdoff_ms = 0;
		uint64_t holdoff_end_ns = 0;
		bool trigger_was_active = false;
	};
	coil m_coil[128][COILS_PER_NODE];
	uint16_t m_coil_mask[128] = {};    // set bits: drivers the game has switched off
	void coil_fire(uint8_t node, const uint8_t *data, uint32_t len, uint64_t now_ns, std::string &note);
	void coil_reflex(uint8_t node, const uint8_t *data, uint32_t len, std::string &note);
	void run_reflexes(uint8_t node, uint64_t now_ns);
	bool reflex_switch_active(uint8_t node, uint8_t sw) const;

	// LEDs: up to 96 channels a board, an RGB LED taking three
	static constexpr uint32_t LED_CHANNELS = 96;
	uint8_t m_led[128][LED_CHANNELS] = {};
	uint8_t m_led_mask[128][LED_CHANNELS / 8] = {};   // set bits: channels the game has switched off
	bool led_update(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len);
	bool led_update_run(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len);
	int m_led_form = -1;   // 1: the older SDK's runs of channels (led_update_run), 0: packed, -1: not known yet

	std::vector<output_info> m_coils, m_leds;

	// The LCD insert holds its image in its own flash; the game tells it which frames to show
	struct insert_channel
	{
		bool shown = false;
		bool running = false;            // a run steps through frames; otherwise one frame stays
		uint32_t frame = 0;              // the frame that stays, or a run's first frame
		uint32_t last = 0, loop_first = 0, loop_last = 0;
		uint16_t loops = 0;
		bool loop = false;
		uint64_t start_ns = 0, period_ns = 0;
		uint8_t flags = 0;
	};
	insert_channel m_insert[4];
	std::vector<uint8_t> m_insert_image;      // lcdinsert.bin
	uint32_t m_insert_frames = 0;
	uint8_t m_insert_backlight = 0;
	std::vector<uint16_t> m_insert_rgb;
	bool insert_command(const uint8_t *data, uint32_t len, uint64_t now_ns, std::string &note);
	uint32_t insert_channel_frame(const insert_channel &c, uint64_t now_ns) const;

	// What the game loaded for one board: its firmware image record
	struct node_image { bool found = false; uint8_t version[3] = {}; uint32_t part = 0; uint16_t checksum = 0; uint32_t board_type = 0; };
	node_image node_image_for(uint8_t node);

	// Switches: closed bits per board and position, and the title's switch list
	std::vector<switch_info> m_switches;
	uint8_t m_sw_closed[128][8] = {};
	uint8_t m_sw_active_high[128][8] = {};
	void load_device_table();
	void apply_manual_numbers(const std::vector<std::pair<int, size_t>> &device);
	void wire_bytes(uint8_t node, uint8_t *out, uint32_t len) const;
	int32_t cpu_spi_message(uint32_t req, uint32_t arg, std::string &note);

	// Sound
	uint32_t m_audio_rate = 44100;
	uint64_t m_audio_until_ns = 0;   // when the DAC will have played what the game has written
	std::deque<int16_t> m_audio;
	int32_t audio_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns);

	// DMD
	uint64_t m_dmd_next_ns = 0;   // when the panel takes the next frame
	uint64_t m_dmd_frames = 0;
	uint8_t m_dmd_dots[DMD_WIDTH * DMD_HEIGHT] = {};

	void log(const std::string &line) const { if (m_cfg.log) m_cfg.log(line); }
	// True the first time a kind of unmodelled traffic is seen (always when tracing), so the log
	// names each once instead of flooding
	bool first_time(const std::string &what) { return m_cfg.trace || m_unmodelled.insert(what).second; }
	std::set<std::string> m_unmodelled;
	eeprom *find_eeprom(uint16_t slave);
	int32_t i2c_rdwr(handle &h, uint32_t arg, std::string &note);
	int32_t i2c_transfer(uint16_t slave, bool read, uint8_t *p, uint32_t len, std::string &trace);
	int32_t adc_read(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns);
	int32_t dmd_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns);
	int32_t node_bus_write(uint32_t buf, uint32_t len, uint64_t now_ns, std::string &note);
	int32_t node_bus_read(uint32_t buf, uint32_t len);
	void node_bus_frame(const uint8_t *frame, uint32_t len, uint64_t now_ns, std::string &note);
	void node_bus_bridge(const uint8_t *msg, uint32_t len, std::string &note);
	void node_bus_reply(const uint8_t *data, uint32_t len, uint8_t status = 0);
	const std::vector<uint8_t> &node_bus_nodes();
	bool guest_read(uint32_t addr, void *dst, uint32_t len) { return m_mem.read(addr, dst, len); }
};
