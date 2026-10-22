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
	enum class kind { none, i2c, node_bus, dmd_spi, cpu_spi, adc, i2s, amp, backlight, gpio, rtc, dmd, fb, other };

	// A read/write/ioctl result that means "block until wait_ns, then call again"
	static constexpr int32_t WAIT = INT32_MIN;

	static constexpr uint32_t DMD_WIDTH = 128, DMD_HEIGHT = 32;
	static constexpr uint32_t DMD_FRAME_BYTES = DMD_WIDTH * DMD_HEIGHT / 8 * 4; // four bitplanes
	// The RGB LED matrix Spider-Man has in the DMD's place (the game's display type rgbdotmatrix):
	// 64x32 dots of 4-bit red, green and blue, as four bitplanes
	static constexpr uint32_t RGB_WIDTH = 64, RGB_HEIGHT = 32;
	static constexpr uint32_t RGB_FRAME_BYTES = RGB_WIDTH * RGB_HEIGHT * 3 / 8 * 4;

	struct config
	{
		bool trace = false;   // log every transfer, not only new kinds of traffic
		std::function<void (const std::string &)> log;
		// address of a symbol in the game program, 0 if it has none: the node bus reads the
		// game's own board tables, so nothing about a title is hard-coded here
		std::function<uint32_t (const std::string &)> symbol;
		std::function<uint32_t (const std::string &)> symbol_size; // the symbol's size in bytes, 0 if unknown
		std::function<std::vector<std::string> (const std::string &prefix)> symbols_with_prefix; // the names that start with prefix
		// The title's files beside the game program (node firmware, the LCD insert's image): their
		// names, and one file's bytes (false when it is not there)
		std::function<std::vector<std::string> ()> game_files;
		std::function<bool (const std::string &name, std::vector<uint8_t> &out)> read_game_file;
	};

	explicit spike1_devices(spike1_memory &mem);

	void start(const config &cfg);
	// The game program has ended: the kernel lets go of what the process held - its mapping of the
	// framebuffer, the serial line's unread input, the sound it queued, the line-sense sampling it
	// started - and the boards carry on; the next run polls them from the first again
	void process_exited();
	// A new run of the program, loaded but not started: it reads the board's video key again
	void process_started();
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
		uint64_t wait_until_ns = 0;   // a wait under way (the framebuffer's vblank wait)
	};
	// Where the game mapped a device's memory: the framebuffer's pixels
	void mapped(const handle &h, uint32_t addr, uint32_t len);

	// Results follow the system-call convention - a byte count or value, or -errno - or WAIT
	int32_t read(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	int32_t write(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	int32_t ioctl(handle &h, uint32_t req, uint32_t arg, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	// When a read finds data, for poll/select: at or before now_ns if it would now, NEVER if nothing
	// is on its way (a later write may still bring something)
	static constexpr uint64_t NEVER = UINT64_MAX;
	uint64_t readable_at(const handle &h, uint64_t now_ns) const;

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
	// The same as a host shows it (PinMAME's solenoids), taken once a look: a pulse that came and
	// went since the last look shows at its power, so one shorter than the look is not lost; the
	// 1 ms pulses of the game's power-on driver check, too short to move a coil, show nothing
	uint8_t coil_output(uint8_t node, uint8_t position, uint64_t now_ns);
	// Whether a board fires a coil on its own now: a reflex with power, on a driver the game has not
	// switched off. The game arms the flippers', slings' and pops' reflexes while a ball is in play
	// and disarms them when it tilts and when a ball or the game ends - it sends them with no power
	bool reflex_armed(uint8_t node, uint8_t position) const;
	// An LED channel's level at now_ns, 0-255: on its way to the level the game last set, over that
	// update's fade time
	uint8_t led_level(uint8_t node, uint8_t position, uint64_t now_ns) const;
	// For a host that animates a mechanism: the i-th motor the game has configured, in board and
	// motor order (its position as the board counts it), then its steppers (a stepper's position is
	// its step within the turn), then the limit motors. Runs the boards' motor loops up to now_ns
	bool motor_at(size_t i, int16_t &position, bool &moving, uint64_t now_ns);
	// The mechanism behind a board's encoder motor (Ghostbusters' Slimer): its end stops, in encoder
	// counts from the edge of its home zone, which reaches from that edge down to the lower stop,
	// and how many counts a second it makes at full drive. Without one the motor runs free at 50
	// counts a second. It stands just inside the zone at power-on
	void link_encoder_motor(uint8_t node, uint8_t motor, int16_t low_stop, int16_t high_stop, uint16_t counts_per_s);
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

	// The CPU board's own LCD (WWE's), a Linux framebuffer at /dev/fb0 that the game maps and draws
	// into: 320x240 RGB565, two buffers one above the other, the shown one picked by a pan. The
	// picture shown now, row by row (false until the game has shown a frame), and the frames shown
	static constexpr uint32_t LCD_WIDTH = 320, LCD_HEIGHT = 240;
	bool lcd_picture(uint16_t *out) const;
	uint64_t lcd_frame_count() const { return m_fb_pans; }
	// The key the title's LCD videos are RC4-encrypted with. The board's factory key store (EEPROM
	// 0x51, encrypted with a key derived from the board's MAC address, and holding a block the
	// model cannot make) gives it to the game, which keeps it in rc4_key once read; the model puts
	// it there instead, while rc4_key still holds the game's 0xCC placeholder. False when the
	// title has no rc4_key
	bool provide_video_key(const uint8_t key[16]);
	// True while rc4_key holds the placeholder: the title has encrypted clips and no key yet
	bool needs_video_key();

	// The last frame the game sent to the DMD as 16 shades per dot (0-15), row by row
	const uint8_t *dmd_frame() const { return m_dmd_dots; }
	// The same for the RGB matrix, as RGB565 dots row by row, and the count of its frames
	const uint16_t *rgb_frame() const { return m_rgb565; }
	uint64_t rgb_frame_count() const { return m_rgb_frames; }
	// Frames the game sent, per command byte and board, for bring-up statistics
	const std::map<uint32_t, uint64_t> &node_bus_counts() const { return m_nb_counts; }
	// The time the node bus wire has carried bytes, sent and answered
	uint64_t node_bus_busy_ns() const { return m_nb_busy_ns; }
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

	std::vector<uint8_t> m_video_key; // what provide_video_key() gave, for each run of the program
	// The LCD's framebuffer: where the game mapped it, the line the shown buffer starts at, the
	// panel's frame time (from the timing the game sets; 60 Hz until it does) and the frames shown
	uint32_t m_fb_addr = 0, m_fb_len = 0, m_fb_yoffset = 0;
	uint64_t m_fb_frame_ns = 16666667, m_fb_pans = 0;
	int32_t fb_ioctl(handle &h, uint32_t req, uint32_t arg, uint64_t now_ns, uint64_t &wait_ns, std::string &note);
	// WWE's video background, asked for again once the node bus start-up is over (lcd_video_background())
	bool m_fb_startup_seen = false, m_fb_background_asked = false;
	void lcd_video_background();

	// AC line sense: a sampled, rectified mains waveform - flat 0 V once the power goes off
	uint64_t m_adc_next_ns = 0;   // when the buffer being sampled is complete
	uint32_t m_adc_phase = 0;
	bool m_power_off = false;

	// Node bus: the RS-485 link to the playfield and cabinet node boards
	std::deque<uint8_t> m_nb_reply;          // reply bytes for the game's read, the last ones maybe still on the wire
	// The wire, half duplex at the rate and character frame the game sets (node_bus_settings()): a
	// frame goes out once the bytes before it are through, and its reply comes back after it
	struct nb_arrival { uint64_t at_ns; uint32_t bytes; };
	std::deque<nb_arrival> m_nb_arriving;    // replies on the wire, oldest first: the tail of m_nb_reply
	uint64_t m_nb_char_ns = 0;               // one character: start bit, data bits, parity and stop bits
	uint64_t m_nb_line_free_ns = 0;          // when the last byte on the wire, sent or answered, is through
	uint64_t m_nb_sent_ns = 0;               // when the last byte the game wrote has gone out (tcdrain)
	uint64_t m_nb_busy_ns = 0;
	uint32_t m_nb_cflag = 0;                 // the settings in force
	std::vector<uint8_t> m_nb_nodes;         // bus addresses of the title's boards
	bool m_nb_nodes_known = false;
	size_t m_nb_poll = 0;
	std::map<uint32_t, uint64_t> m_nb_counts;   // (cmd << 8 | node) -> frames
	std::set<uint32_t> m_nb_logged;          // (node << 8 | command) already reported as not modelled
	uint8_t m_bridge_version[3] = {};        // of the title's netbridge firmware; zero when it has none
	std::vector<uint32_t> m_chip_part;       // [proc key - 1]: NXP part ID of the title's node chips, in the game's order
	uint32_t m_block_base = 0, m_block_stride = 0, m_block_type_at = 88;
	uint32_t m_image_next_at = 28;           // the firmware image list's next pointer: 28, or 56 from node firmware 0.67
	uint32_t m_nb_received[128] = {};        // frames each board has taken, as its GetStatus counts them
	uint32_t m_lcd_image_id = 0;             // the LCD insert holds the title's lcdinsert.bin: its ID word
	frame_observer m_frame_observer;

	// A motor a board runs on its own, on its encoder (the game's EncoderMotor), as the node
	// firmware does it (0.52.0, pinnode-LPC1313: the motor block 0x51-0x57 and the motor loop it
	// runs every SysTick tick). The board drives the motor's H-bridge through two of its LED outputs
	// at a PWM duty; the mechanism behind it moves at the duty's share of its full-drive speed
	struct motor
	{
		// configure (0x51, 0x57). Inputs as the firmware takes them: bit 6 used, bit 7 active when
		// high (else when low), bits 0-5 the board's input
		uint8_t home_in = 0, away_in = 0, watch_in = 0, enc_a = 0, enc_b = 0;
		uint8_t out_down = 0, out_up = 0;   // the outputs driven to count down (home, or a target below) and up
		uint8_t ramp = 0, least = 0;        // the duty ramp in 16 ms units, the least duty near the target
		uint16_t settle[2] = {};            // ticks after a home or away switch is found
		bool encoder = false;
		uint8_t zone_raw = 1;               // the home input's level inside the home zone
		// the firmware's state
		uint8_t state = 1, phase = 0, speed = 0, flags = 0, duty = 0;
		bool down = false;                  // the way it drives: status bit 1
		uint16_t status = 0;
		int16_t position = 0, target = 0, start = 0, mid = 0;
		uint32_t acc = 0, step = 0;         // the duty, times 256, and its change a tick
		uint16_t timeout = 0, timeout_left = 0, settle_left = 0;
		uint8_t last_raw = 0;               // the home, away and watch inputs at the last tick (bits 0-2)
		uint64_t at_ns = 0;                 // the loop has run up to here
		// the mechanism (link_encoder_motor())
		double x = -0.5;                    // counts from the edge of the home zone
		int32_t counted = -1;               // floor(x) as the encoder last counted it
		double low = -30000.0, high = 30000.0, counts_per_s = 50.0;
		int8_t sign_down = -1, sign_up = 1; // which way each drive output moves it
	};
	std::map<uint32_t, motor> m_motors;      // (node << 8 | motor)
	std::map<uint32_t, motor> m_mechanics;   // what link_encoder_motor() set, for motors not configured yet
	void motor_command(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t n, uint64_t now_ns);
	void motors_run(uint64_t now_ns);
	void motor_tick(uint8_t node, motor &m, uint64_t now_ns);
	void motor_drive(motor &m, uint8_t duty, bool down, int32_t timeout);
	bool motor_input(uint8_t node, uint8_t id, bool &active) const; // false for an unused input
	uint8_t motor_pins(uint8_t node, const motor &m) const;          // the home, away and watch pins high, bits 0-2
	void motor_switches(uint8_t node, const motor &m, uint64_t now_ns);

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
		uint8_t trigger[3] = {}, eos = 0; // reflex switches: position | REFLEX_USED [| REFLEX_INVERT]; any trigger fires
		uint8_t reflex_power1 = 0, reflex_power2 = 0;
		uint32_t reflex_pulse_ms = 0, reflex_holdoff_ms = 0;
		uint64_t holdoff_end_ns = 0;
		bool trigger_was_active = false;
		bool check = false;            // the pulse now is a driver check (coil_output())
		uint8_t shown = 0;             // the highest power fired since a host last took coil_output()
	};
	coil m_coil[128][COILS_PER_NODE];
	uint16_t m_coil_mask[128] = {};    // set bits: drivers the game has switched off
	void coil_fire(uint8_t node, const uint8_t *data, uint32_t len, uint64_t now_ns, std::string &note);
	void coil_reflex(uint8_t node, const uint8_t *data, uint32_t len, std::string &note);
	void run_reflexes(uint8_t node, uint64_t now_ns);
	bool reflex_triggered(uint8_t node, const coil &c) const;
	bool reflex_switch_active(uint8_t node, uint8_t sw) const;

	// LEDs: up to 96 channels a board, an RGB LED taking three. An update gives each channel a level
	// and a fade time; the board ramps the channel from where it is to the level in a straight line
	// (led_set())
	static constexpr uint32_t LED_CHANNELS = 96;
	static constexpr uint64_t LED_FADE_UNIT_NS = 16000000;
	struct led_channel {
		uint8_t from = 0, to = 0;      // the ramp's ends
		uint64_t start_ns = 0, end_ns = 0;
		uint8_t at(uint64_t now_ns) const;
	};
	std::vector<led_channel> m_led = std::vector<led_channel>(128 * LED_CHANNELS); // board-major
	uint8_t m_led_mask[128][LED_CHANNELS / 8] = {};   // set bits: channels the game has switched off
	bool led_update(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len, uint64_t now_ns);
	bool led_update_run(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len, uint64_t now_ns);
	void led_set(uint8_t node, uint8_t position, uint8_t level, uint8_t fade, uint64_t now_ns);
	uint8_t led_target(uint8_t node, uint8_t position) const;
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
	uint8_t m_sw_read[128][8] = {};    // closed as the game reads them (wire_bytes())
	uint8_t m_sw_changed[128][8] = {}; // changed since the game last read them
	uint8_t m_sw_active_high[128][8] = {};
	void load_device_table();
	// The board inputs (node, position) the game's compiled switch maps read each switch number at
	std::map<uint16_t, std::vector<std::pair<uint8_t, uint8_t>>> compiled_switch_inputs();
	void apply_manual_numbers(const std::vector<std::pair<int, size_t>> &device);
	void wire_bytes(uint8_t node, uint8_t *out, uint32_t len);
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
	uint64_t m_rgb_frames = 0;
	uint16_t m_rgb565[RGB_WIDTH * RGB_HEIGHT] = {};
	int32_t rgb_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns);

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
	int32_t node_bus_read(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns);
	void node_bus_settings(uint32_t cflag, std::string &note);
	uint32_t node_bus_in_flight(uint64_t now_ns) const;   // bytes of m_nb_reply still on the wire
	void node_bus_frame(const uint8_t *frame, uint32_t len, uint64_t now_ns, std::string &note);
	void node_bus_bridge(const uint8_t *msg, uint32_t len, std::string &note);
	void node_bus_reply(const uint8_t *data, uint32_t len, uint8_t status = 0);
	const std::vector<uint8_t> &node_bus_nodes();
	bool guest_read(uint32_t addr, void *dst, uint32_t len) { return m_mem.read(addr, dst, len); }
	bool guest_write(uint32_t addr, const void *src, uint32_t len) { return m_mem.write(addr, src, len); }
	uint32_t video_key_at() const { return m_cfg.symbol ? m_cfg.symbol("_ZL7rc4_key") : 0; }
};
