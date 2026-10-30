// license:BSD-3-Clause

// PinMAME SPIKE 1 subsystem - the CPU board's devices. See spike1_devices.h

#include "spike1_devices.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

namespace {

enum : int32_t { E_IO = 5, E_FAULT = 14, E_NXIO = 6, E_INVAL = 22, E_NOTTY = 25 };

// Linux i2c-dev (include/uapi/linux/i2c-dev.h, i2c.h)
constexpr uint32_t I2C_SLAVE = 0x0703, I2C_SLAVE_FORCE = 0x0706, I2C_RDWR = 0x0707;
constexpr uint16_t I2C_M_RD = 0x0001;

// AC line sense. The game's line-sense thread counts a mains cycle as a run of at least 50
// samples at or below 999 followed by higher ones, and divides the rate it is told by the
// samples per cycle. 3600 Hz gives 60 samples per 60 Hz cycle: 52 low, then 8 high - the
// rectified peak the sense circuit passes. The low part must be a valley, not flat: the thread
// tracks 1024 minus both the lowest and the mean low sample, and a mean within 90% of the
// lowest marks the signal as not a real line (the game then refuses to run)
constexpr uint32_t ADC_RATE = 3600;
constexpr uint32_t ADC_CYCLE = 60, ADC_LOW_SAMPLES = 52;
constexpr uint16_t ADC_THRESHOLD = 999, ADC_VALLEY = 849, ADC_HIGH = 2000;

static uint16_t adc_sample(uint32_t phase)
{
	if (phase >= ADC_LOW_SAMPLES) return ADC_HIGH;
	const double pi = 3.1415926535897932384626433832795;
	return uint16_t(ADC_THRESHOLD - std::lround(ADC_VALLEY * std::sin(pi * (phase + 0.5) / ADC_LOW_SAMPLES)));
}

// A board counts time in ticks at the rate its GetVersion reports; the game converts the times
// it sends (coil pulses, motor timeouts) from ms with it: ticks = ms * rate / 1000, at most 0xffff
constexpr uint32_t NODE_TICK_HZ = 1000;

// Reflex switch bytes in a coil configuration: position | used [| inverted]
constexpr uint8_t REFLEX_USED = 0x40, REFLEX_INVERT = 0x80, REFLEX_POSITION = 0x3f;

// The shortest coil pulse a host shows. At power-on every title but WWE fires each driver alone
// for 1 ms, the others switched off, and logs its current - a check for missing and shorted
// coils. The shortest pulse that moves something is 16 ms (Primus' slingshots)
constexpr uint32_t COIL_SHOWN_MIN_MS = 4;

// Motors: the node firmware runs its motor loop once a SysTick tick, 1280 times a second, the rate
// its LED fades step at, and ramps a motor's duty by its table's 51200 / n a tick for a ramp of n
// 16 ms units, as the fades do
constexpr uint64_t MOTOR_TICK_NS = 781250;
constexpr double MOTOR_TICKS_PER_S = 1280.0;

// Sound. The game's amp_set_sample_rate() passes /dev/i2s a code from its own table, and its DAC
// handler writes 200 stereo frames of 16-bit samples (800 bytes) at a time. The driver keeps a
// buffer of 2940 bytes (amp_get_buffer_size()) - 735 frames, about 17 ms at 44.1 kHz - so a write
// waits while the DAC still has more than that to play
constexpr uint32_t AUDIO_RATES[] = { 8000, 11025, 22050, 24000, 32000, 44100, 48000 };
constexpr uint32_t AUDIO_BUFFER_FRAMES = 2940 / 4;
// The volume is analog: the amplifier's MCP4631 at 0x28 divides the DAC's left and right outputs
// by its wipers 0 and 1 (0 to 0x80, full scale). The game's sys_sound_set_main_hdw_volume_pdi()
// maps its volume 0-63 through a table to wiper values 0-127 and writes both wipers
constexpr uint16_t AUDIO_VOLUME_POT = 0x28, POT_FULL_SCALE = 0x80;

// The panel takes a four-bitplane frame per PWM pattern: 1+2+4+8 x 1.05 ms = 15.75 ms
// (PinMAME's docs/dmd.md, Stern SPIKE 1)
constexpr uint64_t DMD_FRAME_NS = 15750000;

// The node bus port's termios c_cflag (asm-generic/termbits.h, which ARM uses): the rate's code in
// CBAUD (with CBAUDEX for the rates above 38400), the data bits in CSIZE, CSTOPB for two stop bits,
// PARENB for a parity bit. A port starts as the serial core sets it up, 9600 baud with 8 data bits
// and one stop bit (B9600 | CS8 | CREAD | HUPCL | CLOCAL), until the game sets its own
constexpr uint32_t TERM_CBAUD = 0x100f, TERM_CBAUDEX = 0x1000, TERM_CSIZE = 0x30, TERM_CSTOPB = 0x40, TERM_PARENB = 0x100;
constexpr uint32_t TERM_INITIAL_CFLAG = 0x0d | 0x30 | 0x80 | 0x400 | 0x800;
constexpr uint32_t TERM_RATES[16] = { 0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400 };
constexpr uint32_t TERM_RATES_EX[16] = { 0, 57600, 115200, 230400, 460800, 500000, 576000, 921600, 1000000, 1152000,
	1500000, 2000000, 2500000, 3000000, 3500000, 4000000 };

static std::string hex(uint32_t v) { char b[16]; std::snprintf(b, sizeof(b), "0x%x", v); return b; }

static std::string bytes_hex(const uint8_t *p, uint32_t n, uint32_t limit = 16)
{
	std::string s;
	char b[4];
	for (uint32_t i = 0; i < n && i < limit; i++) { std::snprintf(b, sizeof(b), "%02x", p[i]); s += b; }
	if (n > limit) s += "...";
	return s;
}

} // anonymous namespace

spike1_devices::spike1_devices(spike1_memory &mem) : m_mem(mem)
{
}

spike1_devices::kind spike1_devices::kind_for_path(const std::string &path)
{
	static const struct { const char *path; kind type; } table[] = {
		{ "/dev/i2c-0", kind::i2c }, { "/dev/ttyS4", kind::node_bus }, { "/dev/ttyS3", kind::node_bus },
		{ "/dev/spi0", kind::dmd_spi }, { "/dev/spi1", kind::cpu_spi }, { "/dev/dmd", kind::dmd },
		{ "/dev/adc", kind::adc }, { "/dev/i2s", kind::i2s }, { "/dev/amp", kind::amp },
		{ "/dev/backlight", kind::backlight }, { "/dev/gpio", kind::gpio }, { "/dev/rtc", kind::rtc },
		{ "/dev/fb0", kind::fb },
	};
	for (const auto &t : table) if (path == t.path) return t.type;
	return path.rfind("/dev/", 0) == 0 ? kind::other : kind::none;
}

void spike1_devices::start(const config &cfg)
{
	m_cfg = cfg;
	m_unmodelled.clear();
	// The bridge runs the title's netbridge firmware: its version is in the file name,
	// <board>-<chip>-<major>_<minor>_<patch>.hex
	std::vector<std::string> chips;
	for (const std::string &name : m_cfg.game_files ? m_cfg.game_files() : std::vector<std::string>()) {
		// the chip token of a node firmware file, <board>-LPCxxxx[_yyy]-<version>.hex
		const size_t lpc = name.find("-LPC");
		if (lpc != std::string::npos && name.size() > 4 && name.compare(name.size() - 4, 4, ".hex") == 0) {
			const std::string chip = name.substr(lpc + 1, name.find('-', lpc + 1) - lpc - 1);
			if (std::find(chips.begin(), chips.end(), chip) == chips.end()) chips.push_back(chip);
		}
		unsigned major, minor, patch;
		const size_t dash = name.rfind('-');
		if (name.rfind("netbridge-", 0) == 0 && dash != std::string::npos &&
		    std::sscanf(name.c_str() + dash + 1, "%u_%u_%u.hex", &major, &minor, &patch) == 3) {
			m_bridge_version[0] = uint8_t(major); m_bridge_version[1] = uint8_t(minor); m_bridge_version[2] = uint8_t(patch);
			SPIKE1_LOG("node bus: bridge firmware " + name);
		}
	}
	// The game numbers the chips it has firmware for in name order (its proc key, from 1), and a
	// board's GetVersion must name its chip by the NXP part ID (the IAP ReadPartID value)
	static const struct { const char *chip; uint32_t part; } nxp_parts[] = {
		{ "LPC1111_101", 0x041e502b }, { "LPC1111_202", 0x2516902b }, { "LPC1112_101", 0x042d502b },
		{ "LPC1112_102", 0x2524d02b }, { "LPC1112_103", 0x00020023 }, { "LPC1112_201", 0x0425502b },
		{ "LPC1112_202", 0x2524902b }, { "LPC1112_203", 0x00020022 }, { "LPC1113_201", 0x0434502b },
		{ "LPC1113_202", 0x2532902b }, { "LPC1113_301", 0x0434102b }, { "LPC1113_302", 0x2532102b },
		{ "LPC1113_303", 0x00030030 }, { "LPC1124_303", 0x00140040 }, { "LPC1313", 0x2c40102b },
		{ "LPC1313_01", 0x1830102b },
	};
	std::sort(chips.begin(), chips.end());
	m_chip_part.clear();
	SPIKE1_DEBUG_ONLY(std::string chip_list;)
	for (const auto &chip : chips) {
		uint32_t part = 0;
		for (const auto &p : nxp_parts) if (chip == p.chip) part = p.part;
		m_chip_part.push_back(part);
		SPIKE1_DEBUG_ONLY(chip_list += ' ' + chip;)
	}
	SPIKE1_LOG("node bus: node chips" + chip_list);
	m_elg = m_cfg.symbol && m_cfg.symbol("_Z12node_query_ti") && m_cfg.symbol("_Z12nodemap_initv");
	load_device_table();
	m_motors.clear();
	for (auto &node : m_coil) for (auto &c : node) c = coil();
	std::memset(m_coil_mask, 0, sizeof(m_coil_mask));
	std::fill(m_led.begin(), m_led.end(), led_channel());
	std::memset(m_led_mask, 0, sizeof(m_led_mask));
	// An LCD insert (a node board with its own display) holds the title's image already; the game
	// identifies an image by the word at offset 8 of lcdinsert.bin. The file: [header size, u8]
	// [frames, u16][width, u16][height, u16][bits per pixel, u8][ID, u32][file name], then the
	// frames, 16 bits a pixel, big endian, blue in the top five bits (BGR565)
	m_lcd_image_id = 0;
	m_insert_frames = 0;
	m_insert_image.clear();
	m_insert_backlight = 0;
	m_fb_addr = m_fb_len = m_fb_yoffset = 0;
	m_fb_frame_ns = 16666667;
	m_fb_pans = 0;
	m_fb_startup_seen = m_fb_background_asked = false;
	m_video_key.clear();
	m_alnum_frames = 0;
	std::memset(m_alnum, 0, sizeof(m_alnum));
	m_nb_reply.clear();
	m_nb_arriving.clear();
	m_nb_line_free_ns = m_nb_sent_ns = m_nb_busy_ns = 0;
	m_nb_cflag = 0;
	std::string initial;
	node_bus_settings(TERM_INITIAL_CFLAG, initial);
	for (auto &c : m_insert) c = insert_channel();
	if (m_cfg.read_game_file) m_cfg.read_game_file("lcdinsert.bin", m_insert_image);
	const uint8_t *head = m_insert_image.data();
	if (m_insert_image.size() >= 32) {
		std::memcpy(&m_lcd_image_id, head + 8, 4);
		const uint32_t frames = head[1] | (head[2] << 8), width = head[3] | (head[4] << 8), height = head[5] | (head[6] << 8);
		if (width == INSERT_HEIGHT && height == INSERT_WIDTH && head[7] == 16 &&
		    head[0] + uint64_t(frames) * INSERT_WIDTH * INSERT_HEIGHT * 2 <= m_insert_image.size())
			m_insert_frames = frames;
		SPIKE1_LOG("node bus: LCD insert image " + hex(m_lcd_image_id) + ", " + std::to_string(frames) + " frames of " +
			std::to_string(width) + 'x' + std::to_string(height) + (m_insert_frames ? "" : " (not a layout the model shows)"));
	}
	m_audio_rate = 44100;
	m_audio_until_ns = 0;
	m_audio.clear();
	m_eeproms.clear();
	// 64 KiB, the most a two-byte word address reaches, blank as on a new board; the trace shows
	// which slaves and how much of them the game uses
	for (uint16_t slave : { uint16_t(0x50), uint16_t(0x51) }) {
		eeprom e;
		e.slave = slave;
		e.data.assign(0x10000, 0xff);
		m_eeproms.push_back(std::move(e));
	}
	if (m_elg) { // the bridge's own EEPROM, which the ELG titles keep their settings in: blank, as an AVR's erased EEPROM
		eeprom e;
		e.slave = ELG_BRIDGE_EEPROM;
		e.data.assign(ELG_BRIDGE_EEPROM_BYTES, 0xff);
		m_eeproms.push_back(std::move(e));
	}
	m_digipots.clear();
	for (uint16_t slave = 0x28; slave <= 0x2a; slave++) {
		digipot pot;
		pot.slave = slave;
		pot.reg[0] = pot.reg[1] = POT_FULL_SCALE / 2; // wipers at mid scale after power-up
		pot.reg[4] = 0x1ff;                           // TCON: all terminals connected
		m_digipots.push_back(pot);
	}
}

void spike1_devices::process_exited()
{
	m_fb_addr = m_fb_len = 0;
	m_nb_reply.clear();
	m_nb_arriving.clear();
	m_nb_poll = 0;
	m_audio.clear();
	m_audio_until_ns = 0;
	m_adc_next_ns = 0; // sampling starts with the next run's first read, as in the first run
}

void spike1_devices::process_started()
{
	m_fb_startup_seen = m_fb_background_asked = false;
	if (m_video_key.size() == 16) provide_video_key(m_video_key.data());
}

// [slave, u16][size, u32][bytes] per EEPROM, little endian
std::vector<uint8_t> spike1_devices::nvram() const
{
	std::vector<uint8_t> out;
	for (const auto &e : m_eeproms) {
		const uint32_t size = uint32_t(e.data.size());
		out.push_back(uint8_t(e.slave));
		out.push_back(uint8_t(e.slave >> 8));
		for (int i = 0; i < 4; i++) out.push_back(uint8_t(size >> (8 * i)));
		out.insert(out.end(), e.data.begin(), e.data.end());
	}
	return out;
}

void spike1_devices::set_nvram(const uint8_t *data, size_t size)
{
	for (size_t p = 0; size - p >= 6; ) {
		const uint16_t slave = uint16_t(data[p] | (data[p + 1] << 8));
		const size_t n = data[p + 2] | (data[p + 3] << 8) | (data[p + 4] << 16) | (size_t(data[p + 5]) << 24);
		p += 6;
		if (size - p < n) return;
		eeprom *e = find_eeprom(slave);
		if (e && n == e->data.size()) std::memcpy(e->data.data(), data + p, n);
		p += n;
	}
}

spike1_devices::eeprom *spike1_devices::find_eeprom(uint16_t slave)
{
	for (auto &e : m_eeproms) if (e.slave == slave) return &e;
	return nullptr;
}

int32_t spike1_devices::read(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note)
{
	if (h.type == kind::i2c) { // a plain read from the selected slave
		uint8_t *p = m_mem.host(buf, len);
		if (len && !p) return -E_FAULT;
		std::string trace;
		const int32_t r = i2c_transfer(h.i2c_slave, true, p, len, trace);
		SPIKE1_DEBUG_ONLY(if (tracing() || r < 0) note = "i2c " + hex(h.i2c_slave) + ':' + trace;)
		return r;
	}
	if (h.type == kind::adc)
		return adc_read(buf, len, now_ns, wait_ns);
	if (elg_display_port(h))
		return 0;
	if (h.type == kind::node_bus) {
		const int32_t r = node_bus_read(buf, len, now_ns, wait_ns);
		if (r == 0 && tracing()) note = "node bus: no reply waiting";
		return r;
	}
	SPIKE1_DEBUG_ONLY(if (first_time("read " + h.path)) note = "read " + std::to_string(len) + " from " + h.path + ": not modelled";)
	return 0;
}

uint64_t spike1_devices::readable_at(const handle &h, uint64_t now_ns) const
{
	if (h.type == kind::adc) return m_adc_next_ns ? m_adc_next_ns : NEVER; // the buffer being sampled
	if (h.type == kind::node_bus && !elg_display_port(h)) {
		if (m_nb_reply.size() > node_bus_in_flight(now_ns)) return now_ns;
		for (const nb_arrival &a : m_nb_arriving) if (a.at_ns > now_ns) return a.at_ns;
	}
	return NEVER;
}

int32_t spike1_devices::write(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note)
{
	if (h.type == kind::dmd_spi && len == DMD_FRAME_BYTES)
		return dmd_write(buf, len, now_ns, wait_ns);
	if (h.type == kind::dmd_spi && len == RGB_FRAME_BYTES)
		return rgb_write(buf, len, now_ns, wait_ns);
	if (h.type == kind::dmd_spi && len == ALNUM_FRAME_BYTES && m_elg)
		return alnum_write(buf, len);
	if (elg_display_port(h)) {
		const uint8_t *p = m_mem.host(buf, len, false);
		if (len && !p) return -E_FAULT;
		SPIKE1_DEBUG_ONLY(if (first_time("elg display port")) note = "display: text port " + h.path + ' ' + bytes_hex(p, len, 32) + " (not shown: the segment frames are)";)
		return int32_t(len);
	}
	if (h.type == kind::node_bus)
		return node_bus_write(buf, len, now_ns, note);
	if (h.type == kind::i2s)
		return audio_write(buf, len, now_ns, wait_ns);
	if (h.type == kind::i2c) { // a plain write to the selected slave
		uint8_t *p = m_mem.host(buf, len, false);
		if (len && !p) return -E_FAULT;
		std::string trace;
		const int32_t r = i2c_transfer(h.i2c_slave, false, p, len, trace);
		SPIKE1_DEBUG_ONLY(if (tracing() || r < 0) note = "i2c " + hex(h.i2c_slave) + ':' + trace;)
		return r;
	}
	const uint8_t *p = m_mem.host(buf, len, false);
	if (len && !p) return -E_FAULT;
	SPIKE1_DEBUG_ONLY(if (first_time("write " + h.path + ' ' + std::to_string(len))) note = "write " + std::to_string(len) + " to " + h.path + ": " + bytes_hex(p, len) + " (not modelled)";)
	return int32_t(len);
}

int32_t spike1_devices::ioctl(handle &h, uint32_t req, uint32_t arg, uint64_t now_ns, uint64_t &wait_ns, std::string &note)
{
	(void)now_ns; (void)wait_ns;
	if (h.type == kind::adc) {
		// The game's line-sense thread: 0x4202 answers the sample rate, 0x4204 configures,
		// 0x4206 reports samples lost since the last read (0: a clean stream)
		switch (req) {
		case 0x4202: return int32_t(ADC_RATE);
		case 0x4204: case 0x4206: return 0;
		}
	}
	if (h.type == kind::fb)
		return fb_ioctl(h, req, arg, now_ns, wait_ns, note);
	if (h.type == kind::dmd && (req == 0x3d01 || req == 0x3d02))
		return 0; // the display thread brackets every frame write with these
	if (h.type == kind::i2s && req == 0x3e00) { // sample rate: [code, u32]
		uint32_t code = 0;
		if (!guest_read(arg, &code, 4)) return -E_FAULT;
		if (code >= sizeof(AUDIO_RATES) / sizeof(AUDIO_RATES[0])) return -E_INVAL;
		m_audio_rate = AUDIO_RATES[code];
		SPIKE1_DEBUG_ONLY(note = "i2s: " + std::to_string(m_audio_rate) + " Hz";)
		return 0;
	}
	if (h.type == kind::i2s && req == 0x3e01) { // the rate in force: [code, u32]. The ELG titles check it after each change; the later titles ask and ignore it
		const uint32_t code = uint32_t(std::find(std::begin(AUDIO_RATES), std::end(AUDIO_RATES), m_audio_rate) - std::begin(AUDIO_RATES));
		return guest_write(arg, &code, 4) ? 0 : -E_FAULT;
	}
	if (h.type == kind::amp && req == 0x3f00)
		return 0; // the amplifier's register set-up; the volume comes through the digipots
	if (h.type == kind::cpu_spi && (req & 0xc000ffff) == 0x40006b00) // SPI_IOC_MESSAGE(n)
		return cpu_spi_message(req, arg, note);
	if (h.type == kind::cpu_spi && (req == 0x40046b04 || req == 0x40016b01 || req == 0x40016b03)) // speed, mode, bits per word
		return 0;
	if (elg_display_port(h))
		return 0; // its line settings, 115200 8N1, and its flushes
	if (h.type == kind::node_bus) {
		switch (req) {
		case 0x540b: // TCFLSH: the game drops stale input before every transfer; a reply still on the wire comes in after it
			if (arg == 0 || arg == 2) m_nb_reply.erase(m_nb_reply.begin(), m_nb_reply.end() - std::ptrdiff_t(node_bus_in_flight(now_ns)));
			return 0;
		case 0x5402: case 0x5403: case 0x5404: { // TCSETS, and TCSETSW and TCSETSF, which first wait for the output to go
			if (req != 0x5402 && now_ns < m_nb_sent_ns) { wait_ns = m_nb_sent_ns; return WAIT; }
			uint32_t cflag = 0;   // struct termios: c_iflag, c_oflag, c_cflag, ...
			if (!guest_read(arg + 8, &cflag, 4)) return -E_FAULT;
			node_bus_settings(cflag, note);
			if (req == 0x5404) m_nb_reply.erase(m_nb_reply.begin(), m_nb_reply.end() - std::ptrdiff_t(node_bus_in_flight(now_ns)));
			return 0;
		}
		case 0x5409: // TCSBRK: tcdrain() when its argument is not 0 - it waits until what the game wrote has gone out
			if (!arg) break; // a break on the line: not modelled
			if (now_ns < m_nb_sent_ns) { wait_ns = m_nb_sent_ns; return WAIT; }
			return 0;
		// TCGETS, TCXONC, TIOCGSERIAL, and TIOCSSERIAL, with which the game sets ASYNC_LOW_LATENCY: the
		// driver passes received bytes to the reader at once
		case 0x5401: case 0x540a: case 0x541e: case 0x541f:
			return 0;
		}
	}
	if (h.type == kind::i2c) {
		switch (req) {
		case I2C_SLAVE: case I2C_SLAVE_FORCE:
			h.i2c_slave = uint16_t(arg);
			if (tracing()) note = "i2c slave " + hex(arg);
			return 0;
		case I2C_RDWR:
			return i2c_rdwr(h, arg, note);
		default:
			SPIKE1_DEBUG_ONLY(if (first_time("i2c ioctl " + hex(req))) note = "i2c ioctl " + hex(req) + " not modelled";)
			return -E_INVAL;
		}
	}
	SPIKE1_DEBUG_ONLY(if (first_time("ioctl " + h.path + ' ' + hex(req))) note = "ioctl " + h.path + " request " + hex(req) + " arg " + hex(arg) + ": not modelled";)
	return 0;
}

// struct i2c_rdwr_ioctl_data { struct i2c_msg *msgs; u32 nmsgs; } and, 32-bit,
// struct i2c_msg { u16 addr; u16 flags; u16 len; u8 *buf; } = 12 bytes
int32_t spike1_devices::i2c_rdwr(handle &h, uint32_t arg, std::string &note)
{
	(void)h;
	uint32_t hdr[2];
	if (!m_mem.read(arg, hdr, 8)) return -E_FAULT;
	if (hdr[1] > 42) return -E_INVAL; // I2C_RDWR_IOCTL_MAX_MSGS
	std::string trace;
	for (uint32_t i = 0; i < hdr[1]; i++) {
		uint8_t m[12];
		if (!m_mem.read(hdr[0] + 12 * i, m, 12)) return -E_FAULT;
		uint16_t addr, flags, len;
		uint32_t buf;
		std::memcpy(&addr, m, 2); std::memcpy(&flags, m + 2, 2); std::memcpy(&len, m + 4, 2); std::memcpy(&buf, m + 8, 4);
		const bool read = (flags & I2C_M_RD) != 0;
		uint8_t *p = m_mem.host(buf, len, read);
		if (len && !p) return -E_FAULT;
		SPIKE1_DEBUG_ONLY(if (i == 0) trace = ' ' + hex(addr) + ':';)
		const int32_t r = i2c_transfer(addr, read, p, len, trace);
		if (r < 0) { SPIKE1_DEBUG_ONLY(note = "i2c" + trace;) return r; }
	}
	if (tracing()) note = "i2c" + trace;
	return int32_t(hdr[1]);
}

// One message to one slave. Returns the bytes moved, or -ENXIO when no device answers
int32_t spike1_devices::i2c_transfer(uint16_t slave, bool read, uint8_t *p, uint32_t len, std::string &trace)
{
	if (eeprom *e = find_eeprom(slave)) {
		const uint32_t size = uint32_t(e->data.size());
		if (read) {
			for (uint32_t k = 0; k < len; k++) p[k] = e->data[(e->pointer + k) % size];
			SPIKE1_DEBUG_ONLY(trace += " rd@" + hex(e->pointer) + '[' + std::to_string(len) + "]=" + bytes_hex(p, len, 8);)
			e->pointer = (e->pointer + len) % size;
		} else if (len >= 2) {
			e->pointer = ((uint32_t(p[0]) << 8) | p[1]) % size;
			if (len > 2) {
				SPIKE1_DEBUG_ONLY(trace += " wr@" + hex(e->pointer) + '[' + std::to_string(len - 2) + "]=" + bytes_hex(p + 2, len - 2, 8);)
				for (uint32_t k = 2; k < len; k++) { e->data[e->pointer] = p[k]; e->pointer = (e->pointer + 1) % size; }
			}
		}
		return int32_t(len);
	}
	for (auto &pot : m_digipots) {
		if (pot.slave != slave) continue;
		if (read) {
			const uint16_t v = pot.reg[pot.pointer & 15];
			for (uint32_t k = 0; k < len; k++) p[k] = k & 1 ? uint8_t(v) : uint8_t(v >> 8);
			SPIKE1_DEBUG_ONLY(trace += " pot rd reg " + std::to_string(pot.pointer) + '=' + hex(v);)
			return int32_t(len);
		}
		for (uint32_t k = 0; k + 1 < len; k += 2) {
			const uint8_t address = p[k] >> 4, command = (p[k] >> 2) & 3;
			pot.pointer = address;
			if (command == 0) pot.reg[address] = uint16_t(((p[k] & 3) << 8) | p[k + 1]); // write data
			SPIKE1_DEBUG_ONLY(trace += " pot reg " + std::to_string(address) + " cmd " + std::to_string(command) + ' ' + hex(((p[k] & 3) << 8) | p[k + 1]);)
		}
		return int32_t(len);
	}
	SPIKE1_DEBUG_ONLY(trace += " no device answers";)
	return -E_NXIO;
}

// The driver hands back a buffer once it has sampled it, so a read waits for the samples' time
int32_t spike1_devices::adc_read(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns)
{
	const uint32_t samples = len / 2;
	if (!samples) return 0;
	const uint64_t span_ns = uint64_t(samples) * 1000000000ull / ADC_RATE;
	if (!m_adc_next_ns) m_adc_next_ns = now_ns + span_ns; // sampling starts with the first read
	if (now_ns < m_adc_next_ns) { wait_ns = m_adc_next_ns; return WAIT; }
	uint8_t *p = m_mem.host(buf, samples * 2);
	if (!p) return -E_FAULT;
	for (uint32_t i = 0; i < samples; i++) {
		const uint16_t v = m_power_off ? 0 : adc_sample((m_adc_phase + i) % ADC_CYCLE);
		p[2 * i] = uint8_t(v);
		p[2 * i + 1] = uint8_t(v >> 8);
	}
	m_adc_phase = (m_adc_phase + samples) % ADC_CYCLE;
	m_adc_next_ns += span_ns;
	if (m_adc_next_ns < now_ns) m_adc_next_ns = now_ns + span_ns; // the reader fell behind: samples were lost
	return int32_t(samples * 2);
}

// A frame is four 512-byte bitplanes, 16 bytes per row; plane n weighs 2^n (its PWM slot is
// 2^n x 1.05 ms long). Dots are taken most significant bit first
int32_t spike1_devices::dmd_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns)
{
	if (now_ns < m_dmd_next_ns) { wait_ns = m_dmd_next_ns; return WAIT; }
	const uint8_t *p = m_mem.host(buf, len, false);
	if (!p) return -E_FAULT;
	constexpr uint32_t plane = DMD_WIDTH * DMD_HEIGHT / 8;
	for (uint32_t y = 0; y < DMD_HEIGHT; y++)
		for (uint32_t x = 0; x < DMD_WIDTH; x++) {
			const uint32_t byte = y * (DMD_WIDTH / 8) + x / 8, bit = 7 - (x & 7);
			uint8_t level = 0;
			for (uint32_t n = 0; n < 4; n++)
				level |= ((p[n * plane + byte] >> bit) & 1) << n;
			m_dmd_dots[y * DMD_WIDTH + x] = level;
		}
	m_dmd_frames++;
	m_dmd_next_ns = (m_dmd_next_ns && m_dmd_next_ns + DMD_FRAME_NS > now_ns ? m_dmd_next_ns : now_ns) + DMD_FRAME_NS;
	return int32_t(len);
}

// The RGB matrix's frame (the game's display_pack_buffer_12_bit_rgb_64x32()): four 768-byte
// bitplanes, plane n holding bit n of every 4-bit colour value. A plane is 16 lines of 48 bytes,
// line l carrying rows l and l + 16 as the panel scans both halves at once: column x takes bits
// 6x to 6x + 5, red, green and blue of the upper row then of the lower one, most significant bit
// of a byte first. The panel's frame time is not known; the DMD's is used
int32_t spike1_devices::rgb_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns)
{
	if (now_ns < m_dmd_next_ns) { wait_ns = m_dmd_next_ns; return WAIT; }
	const uint8_t *p = m_mem.host(buf, len, false);
	if (!p) return -E_FAULT;
	constexpr uint32_t plane = RGB_FRAME_BYTES / 4, line = RGB_WIDTH * 6 / 8;
	for (uint32_t y = 0; y < RGB_HEIGHT; y++)
		for (uint32_t x = 0; x < RGB_WIDTH; x++) {
			uint32_t c[3] = {};
			for (uint32_t k = 0; k < 3; k++) {
				const uint32_t bit = 6 * x + (y >> 4) * 3 + k;
				for (uint32_t n = 0; n < 4; n++)
					c[k] |= ((p[n * plane + (y & 15) * line + bit / 8] >> (7 - bit % 8)) & 1u) << n;
			}
			// 4 bits a colour to RGB565, the top bits repeated below
			m_rgb565[y * RGB_WIDTH + x] = uint16_t((c[0] << 12 | c[0] >> 3 << 11) | (c[1] << 7 | c[1] >> 2 << 5) | (c[2] << 1 | c[2] >> 3));
		}
	m_rgb_frames++;
	m_dmd_next_ns = (m_dmd_next_ns && m_dmd_next_ns + DMD_FRAME_NS > now_ns ? m_dmd_next_ns : now_ns) + DMD_FRAME_NS;
	return int32_t(len);
}

// ---------------------------------------------------------------- node bus
//
// What the game's own transfer code sends and expects (NODEBUS_TransferMessage):
//   poll:   the single byte 00; the reply is one byte, the next board in the poll sequence, 0 = end
//   frame:  [0x80|node][n][cmd][data, n-2 bytes][checksum][reply length], the checksum making
//           addr + n + cmd + data + checksum = 0 mod 256
//   reply:  [data, reply length bytes][checksum][status], data + checksum = 0 mod 256, status
//           bits 2 and 3 clear when the board accepted the frame
//
// The bytes take the wire's time, at the rate and character frame the game sets (WWE and the later
// titles: 460800 baud, 8 data bits, 2 stop bits - 23.9 us a byte). The link is half duplex: a frame
// goes out once the bytes before it, sent or answered, are through, and the answer follows its
// last byte. The game waits for an answer with select() and reads until it has all of it
// (ASYNCSERIAL_ReadBytes), so the wire sets the pace of its node-bus loop. The answer comes to the
// reader whole once its last byte is in - the game sets ASYNC_LOW_LATENCY, so the driver passes
// received bytes on at once. The bridge's relaying and a board's time to answer are not known and
// count as nothing: a transfer takes the least time the wire allows. A board acts on a frame when
// the game writes it, not when the frame's last byte has reached it

// The title's boards, from its node_board_table: {entries, count, entry size}, each entry's
// fourth word holding the bus address - in its upper half on newer SDKs (address << 16 | devices),
// the whole word on the oldest
const std::vector<uint8_t> &spike1_devices::node_bus_nodes()
{
	if (m_nb_nodes_known || !m_cfg.symbol) return m_nb_nodes;
	const uint32_t table = m_cfg.symbol("node_board_table");
	uint32_t desc[3] = {};
	if (!table || !guest_read(table, desc, 12) || !desc[0] || !desc[1] || desc[1] > 32 || desc[2] < 16 || desc[2] > 64)
		return m_nb_nodes;
	std::vector<uint32_t> raw(desc[1]);
	bool upper = false;
	for (uint32_t i = 0; i < desc[1]; i++) {
		guest_read(desc[0] + i * desc[2] + 12, &raw[i], 4);
		if (raw[i] >= 0x10000) upper = true;
	}
	for (uint32_t v : raw) {
		const uint8_t node = uint8_t((upper ? v >> 16 : v) & 0x7f);
		if (node && std::find(m_nb_nodes.begin(), m_nb_nodes.end(), node) == m_nb_nodes.end())
			m_nb_nodes.push_back(node);
	}
	m_nb_nodes_known = true;
	SPIKE1_DEBUG_ONLY(std::string list; for (uint8_t n : m_nb_nodes) list += ' ' + std::to_string(n);)
	SPIKE1_LOG("node bus: boards" + list);
	return m_nb_nodes;
}

void spike1_devices::node_bus_reply(const uint8_t *data, uint32_t len, uint8_t status)
{
	uint8_t sum = 0;
	//m_nb_reply.reserve(m_nb_reply.size() + len + 2);
	for (uint32_t i = 0; i < len; i++) { m_nb_reply.push_back(data[i]); sum += data[i]; }
	m_nb_reply.push_back(uint8_t(0 - sum));
	m_nb_reply.push_back(status);
}

uint32_t spike1_devices::node_bus_in_flight(uint64_t now_ns) const
{
	uint32_t bytes = 0;
	for (const nb_arrival &a : m_nb_arriving) if (a.at_ns > now_ns) bytes += a.bytes;
	return bytes;
}

// What has come in; with nothing yet but an answer on its way, a read waits for it
int32_t spike1_devices::node_bus_read(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns)
{
	const uint32_t in_flight = node_bus_in_flight(now_ns);
	const uint32_t n = std::min<uint32_t>(len, uint32_t(m_nb_reply.size()) - in_flight);
	if (!n) {
		for (const nb_arrival &a : m_nb_arriving) if (a.at_ns > now_ns) { wait_ns = a.at_ns; return WAIT; }
		return 0;
	}
	uint8_t *p = m_mem.host(buf, n);
	if (!p) return -E_FAULT;
	for (uint32_t i = 0; i < n; i++) { p[i] = m_nb_reply.front(); m_nb_reply.pop_front(); }
	return int32_t(n);
}

int32_t spike1_devices::node_bus_write(uint32_t buf, uint32_t len, uint64_t now_ns, std::string &note)
{
	const uint8_t *p = m_mem.host(buf, len, false);
	if (len && !p) return -E_FAULT;
	const size_t queued = m_nb_reply.size();
	if (m_elg)
		elg_node_bus(p, len, now_ns, note);
	else if (len == 1 && p[0] == 0) { // poll
		const auto &nodes = node_bus_nodes();
		const uint8_t next = m_nb_poll < nodes.size() ? nodes[m_nb_poll++] : 0;
		if (!next) m_nb_poll = 0;
		m_nb_reply.push_back(next);
		if (tracing()) note = "node bus poll -> " + std::to_string(next);
	} else if (len >= 5 && (p[0] & 0x80) && uint32_t(p[1]) + 3 == len)
		node_bus_frame(p, len, now_ns, note);
	else if (len >= 2 && !(p[0] & 0x80) && uint32_t(p[1]) + 2 == len)
		node_bus_bridge(p, len, note);
	else {
		SPIKE1_DEBUG_ONLY(note = "node bus: unframed write " + bytes_hex(p, len);)
	}
	// on the wire once it is free, the answer after it
	while (!m_nb_arriving.empty() && m_nb_arriving.front().at_ns <= now_ns) m_nb_arriving.pop_front();
	const uint32_t answer = uint32_t(m_nb_reply.size() - queued);
	m_nb_sent_ns = std::max(now_ns, m_nb_line_free_ns) + len * m_nb_char_ns;
	m_nb_line_free_ns = m_nb_sent_ns + answer * m_nb_char_ns;
	m_nb_busy_ns += (len + answer) * m_nb_char_ns;
	if (answer) m_nb_arriving.push_back({ m_nb_line_free_ns, answer });
	return int32_t(len);
}

// The game's TCSETS: the line's rate, and its character frame - a start bit, 5 to 8 data bits, a
// parity bit with PARENB, and one or two stop bits
void spike1_devices::node_bus_settings(uint32_t cflag, std::string &note)
{
	if (cflag == m_nb_cflag) return;
	const uint32_t code = cflag & TERM_CBAUD;
	const uint32_t baud = code & TERM_CBAUDEX ? TERM_RATES_EX[code & 0xf] : TERM_RATES[code];
	if (!baud) { // B0 hangs the line up; BOTHER takes a rate only TCSETS2 gives
		SPIKE1_DEBUG_ONLY(note = "node bus: line settings " + hex(cflag) + " not modelled; the line keeps its rate";)
		return;
	}
	m_nb_cflag = cflag;
	const uint32_t data_bits = 5 + ((cflag & TERM_CSIZE) >> 4), stop_bits = cflag & TERM_CSTOPB ? 2 : 1;
	const uint32_t bits = 1 + data_bits + (cflag & TERM_PARENB ? 1 : 0) + stop_bits;
	m_nb_char_ns = (uint64_t(bits) * 1000000000ull + baud / 2) / baud;
#if SPIKE1_DEBUG
	char text[128];
	std::snprintf(text, sizeof(text), "node bus: %u baud, %u data bits, %s, %u stop bit%s - %.1f us a byte", baud, data_bits,
		cflag & TERM_PARENB ? "parity" : "no parity", stop_bits, stop_bits > 1 ? "s" : "", double(m_nb_char_ns) / 1000.0);
	note = text;
#endif
}

// The board block array: sys_node_board_get_next_block_ptr(node) is base + node * stride, the base
// loaded from its literal pool (ldr r1, [pc, #imm]) and the stride an immediate - both differ
// between SDK versions, so read them from the function itself. Newer SDKs load the stride
// (mov r3, #imm); older ones (Whoa Nellie, WWE) compute the address with shifts and step the scan
// loop by it (add r3, r3, #imm). The board type the game settles on for a block sits at the
// offset node_update_runtime_hex_image_id checks first (ldr r3, [r4, #imm]): +88 on most SDKs, +32 on WWE's
spike1_devices::node_image spike1_devices::node_image_for(uint8_t node)
{
	node_image img;
	if (!m_block_base && m_cfg.symbol) {
		const uint32_t fn = m_cfg.symbol("_Z33sys_node_board_get_next_block_ptrh");
		for (uint32_t a = fn; fn && a < fn + 0x54; a += 4) {
			uint32_t insn = 0;
			if (!guest_read(a, &insn, 4) || insn == 0xe12fff1e) break; // bx lr
			if ((insn & 0xfffff000) == 0xe59f1000 && !m_block_base) {
				uint32_t base = 0;
				if (guest_read(a + 8 + (insn & 0xfff), &base, 4)) m_block_base = base;
			} else if (((insn & 0xfffff000) == 0xe3a03000 || (insn & 0xffffff00) == 0xe2833000) && (insn & 0xff) >= 64) {
				m_block_stride = insn & 0xff;
			}
		}
		const uint32_t update = m_cfg.symbol("_Z32node_update_runtime_hex_image_idP18node_board_block_s");
		for (uint32_t a = update; update && a < update + 0x20; a += 4) {
			uint32_t insn = 0;
			if (!guest_read(a, &insn, 4)) break;
			if ((insn & 0xfffff000) == 0xe5943000) { m_block_type_at = insn & 0xfff; break; }
		}
		// the image list's next pointer, from the game's own lookup (ldr r3, [r3, #next]; its
		// ldr r3, [r3] before that reads the list head)
		const uint32_t lookup = m_cfg.symbol("_Z50node_board_runtime_hex_image_manager_get_image_pdijj");
		for (uint32_t a = lookup; lookup && a < lookup + 0x40; a += 4) {
			uint32_t insn = 0;
			if (!guest_read(a, &insn, 4)) break;
			if ((insn & 0xfffff000) == 0xe5933000 && (insn & 0xfff)) { m_image_next_at = insn & 0xfff; break; }
		}
	}
	const uint32_t head = m_cfg.symbol ? m_cfg.symbol("node_board_runtime_hex_image_list_head") : 0;
	if (!m_block_base || !m_block_stride || !head || m_block_type_at + 4 > m_block_stride) return img;
	guest_read(m_block_base + node * m_block_stride + m_block_type_at, &img.board_type, 4);
	if (!img.board_type) return img;
	// image records: [0] board type, [4] proc key; then up to node firmware 0.52 [12] content,
	// [16] offset, [24] checksum, [28] next, the version at content + offset + 9. In 0.67 (Heavy
	// Metal, whose images are encrypted and decrypted by the game into the content buffer) [8]
	// the file name, [12] the image's header, [28] content, [44] offset, [48] size, [56] next, and
	// no checksum (the game does not ask a board for one). The game's version of an image there:
	// none unless [40] is 1; with [32] set, the header's bytes at [16], [18] and [20]; else the
	// bytes at content + offset + 9, as before
	const bool sdk_067 = m_image_next_at == 56;
	uint32_t rec = 0;
	guest_read(head, &rec, 4);
	for (int guard = 0; rec && guard < 64; guard++) {
		uint32_t type = 0;
		guest_read(rec, &type, 4);
		if (type == img.board_type) {
			uint32_t key = 0, content = 0, offset = 0;
			guest_read(rec + 4, &key, 4);
			guest_read(rec + (sdk_067 ? 28 : 12), &content, 4);
			guest_read(rec + (sdk_067 ? 44 : 16), &offset, 4);
			if (!sdk_067) guest_read(rec + 24, &img.checksum, 2);
			uint32_t valid = 1, header = 0, size = ~0u;
			if (sdk_067) {
				guest_read(rec + 40, &valid, 4);
				guest_read(rec + 32, &header, 4);
				guest_read(rec + 48, &size, 4);
			}
			if (valid != 1) {}
			else if (header) for (int i = 0; i < 3; i++) guest_read(rec + 16 + 2 * i, &img.version[i], 1);
			else if (content && size > offset + 11) guest_read(content + offset + 9, img.version, 3);
			img.part = key >= 1 && key <= m_chip_part.size() ? m_chip_part[key - 1] : 0;
			img.found = true;
			return img;
		}
		if (!guest_read(rec + m_image_next_at, &rec, 4)) break;
	}
	return img;
}

void spike1_devices::node_bus_frame(const uint8_t *frame, uint32_t len, uint64_t now_ns, std::string &note)
{
	uint8_t sum = 0;
	for (uint32_t i = 0; i < len - 1; i++) sum += frame[i];
	const uint8_t node = frame[0] & 0x7f, cmd = frame[2], reply_len = frame[len - 1];
	if (sum) { SPIKE1_DEBUG_ONLY(note = "node bus: bad checksum " + bytes_hex(frame, len);) return; }
	const uint32_t data_len = reply_len >= 2 ? reply_len - 2u : 0u;
	m_nb_received[node]++;
	m_nb_counts[(uint32_t(cmd) << 8) | node]++;
	if (m_frame_observer) m_frame_observer(now_ns, frame, len);
	const uint8_t *data = frame + 3;     // the command's own bytes, between cmd and checksum
	const uint32_t n = len - 5;
	if (!m_motors.empty()) motors_run(now_ns); // the boards run their motors all the while
	if (m_led_form < 0) m_led_form = m_cfg.symbol && m_cfg.symbol("_Z23NODEBUS_SetLEDMultiple2hhPhS_S_") ? 0 : 1;
	if ((cmd >= 0x80 && cmd <= 0xbf) || (cmd == 0xc0 && m_led_form)) { // LED update
		if (m_led_form ? led_update_run(node, cmd, data, n, now_ns) : led_update(node, cmd, data, n, now_ns)) {
			if (!m_limit_motors.empty()) limit_motors_run(now_ns);
			return;
		}
		SPIKE1_DEBUG_ONLY(if (m_nb_logged.insert((uint32_t(node) << 8) | cmd).second) note = "node bus: node " + std::to_string(node) + " LED update not understood: " + bytes_hex(frame, len, 32);)
		return;
	}
	switch (cmd) {
	case 0x40: // coil fire
		coil_fire(node, data, n, now_ns, note);
		return;
	case 0x41: // coil configuration with a reflex
		coil_reflex(node, data, n, note);
		return;
	case 0x46: // coil driver mask: [u16], a set bit switches the driver off
		if (n >= 2) m_coil_mask[node] = uint16_t(data[0] | (data[1] << 8));
		return;
	case 0x72: // LED channel mask: a set bit switches the channel off
		std::memcpy(m_led_mask[node], data, std::min<uint32_t>(n, sizeof(m_led_mask[node])));
		return;
	// Board set-up with nothing to show: LED and input counts, over-current behaviour and time,
	// input debounce, the motor's input mask
	case 0x14: case 0x44: case 0x48: case 0x70: case 0x71:
		return;
	// Steppers (the game's StepperMotor, NODEBUS_Stepper*): a board drives up to five
	case 0x32: // configure: [stepper][steps per turn][u16][a driver index][home switch input, s8]
		if (n >= 6 && data[0] <= 4) {
			stepper &s = m_steppers[(uint32_t(node) << 8) | data[0]];
			if (data[1]) s.steps = data[1];
			s.home_input = int8_t(data[5]);
		}
		return;
	case 0x31: { // go: [stepper][target, u16: bit 15 backward][ms per step]
		if (n < 4 || data[0] > 4) break;
		stepper &s = m_steppers[(uint32_t(node) << 8) | data[0]];
		const uint16_t raw = uint16_t(data[1] | (data[2] << 8));
		s.from = stepper_position(s, now_ns);
		s.backward = (raw & 0x8000) != 0;
		s.target = uint16_t((raw & 0x7fff) % s.steps);
		s.distance = uint16_t((s.backward ? s.from + s.steps - s.target : s.target + s.steps - s.from) % s.steps);
		if (!s.distance && (raw & 0x7fff) >= s.steps) s.distance = s.steps; // a target past the turn: once round (Game of Thrones' homing sweep)
		s.start_ns = now_ns;
		s.end_ns = now_ns + uint64_t(s.distance) * std::max<uint8_t>(data[3], 1) * 1000000ull;
		if (tracing()) note = "node bus: node " + std::to_string(node) + " stepper " + std::to_string(data[0]) + " to " + std::to_string(s.target) + (s.backward ? " backward" : "");
		return;
	}
	case 0x34: { // home: [stepper][seek] - in older titles (Whoa Nellie) the stepper sits where the reply length goes, and seeks
		stepper &s = m_steppers[(uint32_t(node) << 8) | (n >= 1 ? data[0] & 7 : reply_len & 7)];
		// Game of Thrones sends it without the seek each time its dragon, turning on through its home
		// switch, passes it during a move, and keeps its own idea of the position: the board counts
		// from home again, which changes nothing here, where step 0 is home
		if (n >= 2 && !data[1]) return;
		s.from = stepper_position(s, now_ns);
		s.backward = false;
		s.target = 0;
		s.distance = uint16_t((s.steps - s.from) % s.steps);
		s.start_ns = now_ns;
		s.end_ns = now_ns + uint64_t(s.distance) * 6000000ull;
		if (data_len) { std::vector<uint8_t> zeros(data_len, 0); node_bus_reply(zeros.data(), data_len); }
		return;
	}
	case 0x38: case 0x39: case 0x3a: case 0x3b: case 0x3c: { // status of stepper cmd - 0x38: [step, u16][target, u16][bits]
		const auto it = m_steppers.find((uint32_t(node) << 8) | (cmd - 0x38));
		uint8_t r[5] = {};
		if (it != m_steppers.end()) {
			const uint16_t at = stepper_position(it->second, now_ns);
			r[0] = uint8_t(at); r[1] = uint8_t(at >> 8);
			r[2] = uint8_t(it->second.target); r[3] = uint8_t(it->second.target >> 8);
			r[4] = it->second.end_ns > now_ns ? 0x02 : 0x04; // bit 1 moving; bit 2 in place (bits 3-4 would be faults)
		}
		if (data_len == sizeof(r)) { node_bus_reply(r, sizeof(r)); return; }
		break;
	}
	// Motors (the game's EncoderMotor): configure, home, away, go, stop and status, as the node
	// firmware runs them (motor_command(), motor_tick())
	case 0x51: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57:
		if (n < 1) break;
		motor_command(node, cmd, data, n, now_ns);
		if (tracing()) note = "node bus: node " + std::to_string(node) + " motor " + std::to_string(data[0]) + " command " + hex(cmd) + ' ' + bytes_hex(data + 1, n - 1);
		return;
	case 0x52: { // status: [motor] -> [position, u16][status, u16]
		if (n < 1 || data_len != 4) break;
		const auto it = m_motors.find((uint32_t(node) << 8) | data[0]);
		const uint16_t pos = it != m_motors.end() ? uint16_t(it->second.position) : 0, status = it != m_motors.end() ? it->second.status : 0;
		const uint8_t r[4] = { uint8_t(pos), uint8_t(pos >> 8), uint8_t(status), uint8_t(status >> 8) };
		node_bus_reply(r, sizeof(r));
		return;
	}
	case 0xff: { // GetStatus: [frames received, u32][status bits, u32]
		uint8_t r[8] = {};
		for (int i = 0; i < 4; i++) r[i] = uint8_t(m_nb_received[node] >> (8 * i));
		if (data_len == sizeof(r)) { node_bus_reply(r, sizeof(r)); return; }
		break;
	}
	case 0xf8: { // GetBootStatus: a board running its application has nothing to report
		const uint8_t r[4] = {};
		if (data_len == sizeof(r)) { node_bus_reply(r, sizeof(r)); return; }
		break;
	}
	case 0xf9: { // GetFullBoardID, in two halves: any stable, non-zero ID registers the board
		uint8_t r[16];
		const uint8_t half = len > 4 ? frame[3] & 1 : 0;
		for (uint32_t i = 0; i < sizeof(r); i++) r[i] = uint8_t(0x40 + node * 4 + half * 0x10 + i);
		if (data_len == sizeof(r)) { node_bus_reply(r, sizeof(r)); return; }
		break;
	}
	case 0xf5: { // GetChecksum: the firmware checksum of the image the game holds for the board
		const node_image img = node_image_for(node);
		const uint8_t r[2] = { uint8_t(img.checksum), uint8_t(img.checksum >> 8) };
		if (data_len == sizeof(r)) {
			node_bus_reply(r, sizeof(r));
			SPIKE1_DEBUG_ONLY(if (tracing() || !img.found) note = "node bus: node " + std::to_string(node) + " GetChecksum -> " + hex(img.checksum) + (img.found ? "" : " (no image)");)
			return;
		}
		break;
	}
	case 0xf2: { // LCD insert: [sub-command][...]
		if (n >= 1 && data[0] == 0x30) { // info: the game compares the image ID at 7..10 with its file
			uint8_t r[12] = {};
			for (int i = 0; i < 4; i++) r[7 + i] = uint8_t(m_lcd_image_id >> (8 * i));
			if (data_len == sizeof(r)) { node_bus_reply(r, sizeof(r)); return; }
		}
		if (n >= 1 && insert_command(data, n, now_ns, note)) { // answered with an all-clear status
			if (data_len) { std::vector<uint8_t> zeros(data_len, 0); node_bus_reply(zeros.data(), data_len); }
			return;
		}
		break;
	}
	case 0x11: { // GetInputState: eight switch bytes (position p = byte p/8, bit p%8), then a u16 the game ignores (WWE's SDK asks for the eight alone)
		uint8_t r[10];
		stepper_home_switches(now_ns);
		if (!m_limit_motors.empty()) limit_motors_run(now_ns);
		wire_bytes(node, r, 8);
		r[8] = r[9] = 0;
		if (data_len == sizeof(r) || data_len == 8) { node_bus_reply(r, data_len); return; }
		break;
	}
	case 0xfe: { // GetVersion: [address | boot << 7][major][minor][patch][part ID, 4][u16]
		const node_image img = node_image_for(node);
		uint8_t r[10] = { node };
		std::memcpy(r + 1, img.version, 3);
		for (int i = 0; i < 4; i++) r[4 + i] = uint8_t(img.part >> (8 * i));
		// the board's tick rate: the game scales coil and motor times with it
		r[8] = uint8_t(NODE_TICK_HZ & 0xff); r[9] = uint8_t(NODE_TICK_HZ >> 8);
		if (data_len == sizeof(r)) {
			node_bus_reply(r, sizeof(r));
			SPIKE1_DEBUG_ONLY(if (tracing() || !img.found)
				note = "node bus: node " + std::to_string(node) + " GetVersion -> " + (img.found ? "" : "no image yet, ") +
					std::to_string(img.version[0]) + '.' + std::to_string(img.version[1]) + '.' + std::to_string(img.version[2]) +
					" part " + hex(img.part) + " board type " + hex(img.board_type);)
			return;
		}
		break;
	}
	}
	// Not modelled yet. A query still gets an answer the transfer code accepts - zeros - so the game
	// carries on; the log names each one once, which is the work list
	if (data_len) {
		std::vector<uint8_t> zeros(data_len, 0);
		node_bus_reply(zeros.data(), data_len);
	}
	SPIKE1_DEBUG_ONLY(if (m_nb_logged.insert((uint32_t(node) << 8) | cmd).second)
		note = "node bus: node " + std::to_string(node) + " command " + hex(cmd) +
			(data_len ? " (a query, answered with " + std::to_string(data_len) + " zeros)" : "") + " not modelled: " + bytes_hex(frame, len, 32);)
}

// A command to the bridge itself: [cmd][n][n bytes], and the bridge answers with as many raw bytes
// as the byte after the message asks for - no checksum, no status. The reply length is not part
// of the write, so it comes from the command
void spike1_devices::node_bus_bridge(const uint8_t *msg, uint32_t len, std::string &note)
{
	const uint8_t cmd = msg[0];
	switch (cmd) {
	case 0x03: // version: major, minor, patch
		m_nb_reply.insert(m_nb_reply.end(), m_bridge_version, m_bridge_version + 3);
		break;
	case 0x05: // status word, one byte from old bridges, four from current ones: nothing to report
		m_nb_reply.insert(m_nb_reply.end(), 4, 0);
		break;
	case 0x0a: // state: bit 0 of the first byte is the 48 V supply, which is on
		m_nb_reply.push_back(0x01);
		m_nb_reply.push_back(0x00);
		break;
	case 0x06: // a setting the game sends without waiting for an answer
		if (tracing()) note = "node bus: bridge command 0x6 " + bytes_hex(msg, len);
		return;
	default:
		SPIKE1_DEBUG_ONLY(if (m_nb_logged.insert(0xff00 | cmd).second)
			note = "node bus: bridge command " + hex(cmd) + " not modelled: " + bytes_hex(msg, len);)
		return;
	}
	if (tracing()) note = "node bus: bridge command " + hex(cmd);
}

// ---------------------------------------------------------------- the ELG framework
//
// Transformers The Pin and Avengers The Pin run Stern's earlier framework (its build path:
// system-elg; their game folders tf-elg and avengers-elg). Its node_pdi.cpp talks to node firmware
// 0.5 and 0.6 at 115200 baud, 8 data bits, 2 stop bits, with no checksum and no reply length:
//   poll    the byte 00; one byte back, the address of a board whose inputs changed since its last
//           query, 0 when none did (node_serial_update_t() queries that board, then polls again)
//   frame   [0x80|node][n][cmd][n - 1 data bytes]; only the query, status and quadrature answer
//   0x11    query: the board's eight input bytes (input p = byte p/8, bit p%8)
//   0xff    status: six bytes, error bits in the last two (node_status_t())
//   0x60    quadrature [a][b]: one byte, the encoder's movement since the last ask
//   0xf2    traffic flag
//   0x40|c  coil c: [power 1][ms][power 2][ms] (node_coilmsg())
//   0x48|c  coil c's reflex: a pop or a sling [power 1][ms][power 2][ms][input][hold-off ms]; a
//           flipper [power 1][ms][hold power][EOS input][0x40|button input][3 bytes], held while
//           the button stays closed. The game disarms a reflex by sending it with power 1 at 0
//           (COILSTATE_Update())
//   0x50    motor [3 bytes] (node_motormsg(); neither title has a motor)
//   0x80|f  LED outputs f on: a level (0-255) each, set at once (node_ledmsg())
// The times are taken as ms: the coil table's flipper pulse is 64, its sling pulse 20, and the game
// refreshes a coil it holds on, sent for 255, every 250. The game keeps its settings in the
// bridge's EEPROM, which it reaches in the bridge's command mode (elg_bridge_command())
void spike1_devices::elg_node_bus(const uint8_t *p, uint32_t len, uint64_t now_ns, std::string &note)
{
	if (len >= 3 && p[0] == 0x55) { elg_bridge_command(p, len, note); return; }
	if (len == 1 && p[0] == 0) {
		uint8_t answer = 0;
		for (uint8_t node : m_elg_nodes) if (elg_inputs_changed(node)) { answer = node; break; }
		m_nb_reply.push_back(answer);
		if (tracing()) note = "node bus poll -> " + std::to_string(answer);
		return;
	}
	if (len < 3 || !(p[0] & 0x80) || uint32_t(p[1]) + 2 != len) { SPIKE1_DEBUG_ONLY(note = "node bus: unframed write " + bytes_hex(p, len);) return; }
	const uint8_t node = p[0] & 0x7f, cmd = p[2];
	const uint8_t *data = p + 3;
	const uint32_t n = len - 3;
	m_nb_received[node]++;
	m_nb_counts[(uint32_t(cmd) << 8) | node]++;
	if (m_frame_observer) m_frame_observer(now_ns, p, len);
	if (cmd >= 0x80 && cmd <= 0xbf) {
		for (uint32_t i = 0; i < n && (cmd & 0x3fu) + i < LED_CHANNELS; i++) led_set(node, uint8_t((cmd & 0x3f) + i), data[i], 0, now_ns);
		return;
	}
	if ((cmd & 0xf8) == 0x40 && n == 4) {
		coil_pulse(node, cmd & 7, data[0], data[1], data[2], data[3], now_ns);
		if (tracing()) note = "node bus: node " + std::to_string(node) + " coil " + std::to_string(cmd & 7) + ' ' + bytes_hex(data, n);
		return;
	}
	if ((cmd & 0xf8) == 0x48 && (n == 6 || n == 8)) {
		elg_coil_reflex(node, cmd & 7, data, n, note);
		return;
	}
	switch (cmd) {
	case 0x11: {
		uint8_t r[8];
		wire_bytes(node, r, sizeof(r));
		m_nb_reply.insert(m_nb_reply.end(), r, r + sizeof(r));
		return;
	}
	case 0xff: // nothing to report
		m_nb_reply.insert(m_nb_reply.end(), 6, 0);
		return;
	case 0x60: // the titles have no encoder
		m_nb_reply.push_back(0);
		return;
	case 0xf2:
		return;
	}
	SPIKE1_DEBUG_ONLY(if (m_nb_logged.insert((uint32_t(node) << 8) | cmd).second)
		note = "node bus: node " + std::to_string(node) + " command " + hex(cmd) + " not modelled: " + bytes_hex(p, len, 32);)
}

// The bridge in command mode (sys_command_mode(): the game raises a GPIO line around each
// exchange): [0x55][cmd][n][n bytes][sum], the sum making the bytes after 0x55 add up to 0, and the
// answer [0xaa][n][n bytes][sum] the same way. LL_sys_eep_read(): cmd 0 [address, u16 big endian]
// -> the byte; LL_sys_eep_write(): cmd 1 [address][byte] -> nothing. The game sends a read with a
// stray byte after it, which the length leaves out
void spike1_devices::elg_bridge_command(const uint8_t *p, uint32_t len, std::string &note)
{
	const uint32_t n = p[2];
	uint8_t sum = 0;
	for (uint32_t i = 1; i < 4 + n && i < len; i++) sum += p[i];
	eeprom *e = find_eeprom(ELG_BRIDGE_EEPROM);
	if (len < 4 + n || sum || !e) { SPIKE1_DEBUG_ONLY(note = "node bus: bridge command not understood: " + bytes_hex(p, len);) return; }
	const uint32_t address = n >= 2 ? (uint32_t(p[3]) << 8 | p[4]) : ~0u;
	auto answer = [&](const uint8_t *data, uint8_t count) {
		uint8_t s = count;
		m_nb_reply.push_back(0xaa);
		m_nb_reply.push_back(count);
		for (uint8_t i = 0; i < count; i++) { m_nb_reply.push_back(data[i]); s += data[i]; }
		m_nb_reply.push_back(uint8_t(0 - s));
	};
	if (p[1] == 0 && n == 2 && address < e->data.size()) {
		answer(&e->data[address], 1);
		if (tracing()) note = "bridge EEPROM read " + hex(address) + " -> " + hex(e->data[address]);
	} else if (p[1] == 1 && n == 3 && address < e->data.size()) {
		e->data[address] = p[5];
		answer(nullptr, 0);
		if (tracing()) note = "bridge EEPROM write " + hex(address) + " = " + hex(p[5]);
	} else
		SPIKE1_DEBUG_ONLY(if (m_nb_logged.insert(0xfe00 | p[1]).second) note = "node bus: bridge command " + hex(p[1]) + " not modelled: " + bytes_hex(p, len);)
}

void spike1_devices::elg_coil_reflex(uint8_t node, uint8_t index, const uint8_t *data, uint32_t len, std::string &note)
{
	coil &c = m_coil[node][index];
	const bool flipper = len == 8;
	c.reflex = data[0] != 0;
	c.reflex_power1 = data[0];
	c.reflex_pulse_ms = data[1];
	c.reflex_power2 = flipper ? data[2] : 0;
	c.reflex_holdoff_ms = flipper ? 0 : data[5];
	c.trigger[0] = uint8_t(REFLEX_USED | (data[4] & REFLEX_POSITION)); // the input, or the flipper's button
	c.trigger[1] = c.trigger[2] = 0;
	c.eos = flipper ? uint8_t(REFLEX_USED | (data[3] & REFLEX_POSITION)) : 0;
	c.trigger_was_active = c.reflex && reflex_triggered(node, c);
	if (!c.reflex) c.holding = false;
	SPIKE1_DEBUG_ONLY(if (!flipper && data[2] && data[3] && first_time("elg reflex second phase"))
		note = "node bus: node " + std::to_string(node) + " coil reflex with a second phase (not modelled): " + bytes_hex(data, len);
	else if (tracing())
		note = "node bus: node " + std::to_string(node) + " coil " + std::to_string(index) + (c.reflex ? " reflex on switch " + std::to_string(c.trigger[0] & REFLEX_POSITION) : " reflex off");)
}

// Like wire_bytes(): the next query shows a change
bool spike1_devices::elg_inputs_changed(uint8_t node) const
{
	const uint8_t n = node & 127;
	for (uint32_t i = 0; i < 8; i++) if ((m_sw_closed[n][i] ^ m_sw_read[n][i]) | m_sw_changed[n][i]) return true;
	return false;
}

// The ELG titles' tables:
//   g_smap<n>     a board's switch map: 64 inputs of 56 bytes - [switch byte, u32][bit mask]
//                 [inverted][name, SWITCH_...] - then 0x4c bytes of the map's own state. The game
//                 numbers a switch byte * 8 + bit; an inverted input reads 0 while it is closed
//   __coil_table  60 bytes a coil: [node][output][type: 0 pulse, 1 reflex, 2 flipper][power 1][ms]
//                 [power 2][ms][hold-off ms][input][EOS input][2 bytes][name, "0 - SLINGSHOTS"]
//   __lamp_table  [node][output] a lamp
// The game counts its switches, coils and lamps from 0, as its test menu shows them; here they are
// one more, as PinMAME's numbers start at 1. nodemap_init() registers one map, for the one board
// the coil and lamp tables name (node 8 in both titles); the maps it leaves empty are not in the
// program's data. The board's names lose their SWITCH_ prefix and underscores, as the other titles
// write them. The CPU board's own switches (g_smap_gpio) are empty in both
void spike1_devices::elg_load_tables()
{
	constexpr uint32_t COIL_ENTRY = 60, COIL_NAME_AT = 12, MAP_INPUTS = 64, MAP_ENTRY = 56, MAP_STATE_BYTES = 0x4c;
	auto table = [&](const char *name, std::vector<uint8_t> &out) {
		const uint32_t a = m_cfg.symbol(name), size = m_cfg.symbol_size ? m_cfg.symbol_size(name) : 0;
		out.assign(size, 0);
		return a && size && guest_read(a, out.data(), size);
	};
	m_elg_nodes.clear();
	auto add_node = [&](uint8_t node) { if (node && node < 128 && std::find(m_elg_nodes.begin(), m_elg_nodes.end(), node) == m_elg_nodes.end()) m_elg_nodes.push_back(node); };
	std::vector<uint8_t> t;
	if (table("__coil_table", t))
		for (uint32_t i = 0; (i + 1) * COIL_ENTRY <= t.size(); i++) {
			const uint8_t *e = &t[i * COIL_ENTRY];
			std::string name(reinterpret_cast<const char *>(e + COIL_NAME_AT), strnlen(reinterpret_cast<const char *>(e + COIL_NAME_AT), COIL_ENTRY - COIL_NAME_AT));
			const size_t dash = name.find(" - ");
			if (dash != std::string::npos) name = name.substr(dash + 3);
			if (name.empty() || !e[0] || e[1] >= COILS_PER_NODE) continue; // the table's unused entries, "8 - "
			m_coils.push_back({ uint16_t(i + 1), e[0], e[1], name });
			add_node(e[0]);
		}
	if (table("__lamp_table", t))
		for (uint32_t i = 0; 2 * i + 1 < t.size(); i++) {
			if (!t[2 * i] || t[2 * i + 1] >= LED_CHANNELS) continue; // node 0: unused
			m_leds.push_back({ uint16_t(i + 1), t[2 * i], t[2 * i + 1], "LAMP " + std::to_string(i), 1 }); // as the lamp test names it
			add_node(t[2 * i]);
		}
	uint32_t maps = 0;
	for (const auto &map : m_cfg.symbols_with_prefix ? m_cfg.symbols_with_prefix("g_smap") : std::vector<std::string>()) {
		if (!table(map.c_str(), t) || t.size() != MAP_INPUTS * MAP_ENTRY + MAP_STATE_BYTES) continue;
		std::vector<switch_info> found;
		for (uint32_t position = 0; position < MAP_INPUTS; position++) {
			const uint8_t *e = &t[position * MAP_ENTRY];
			const uint32_t byte = uint32_t(e[0] | (e[1] << 8) | (e[2] << 16) | (uint32_t(e[3]) << 24)), mask = e[4];
			if (!mask || (mask & (mask - 1)) || byte > 7) continue;
			unsigned bit = 0;
			while (!((mask >> bit) & 1)) bit++;
			std::string name(reinterpret_cast<const char *>(e + 6), strnlen(reinterpret_cast<const char *>(e + 6), MAP_ENTRY - 6));
			if (name.rfind("SWITCH_", 0) == 0) name = name.substr(7);
			std::replace(name.begin(), name.end(), '_', ' ');
			switch_info s;
			s.number = uint16_t(byte * 8 + bit + 1);
			s.position = uint8_t(position);
			s.active_high = !e[5];
			s.name = name;
			found.push_back(s);
		}
		if (found.empty()) continue;
		maps++;
		if (maps > 1 || m_elg_nodes.size() != 1) { SPIKE1_LOG("devices: ELG switch map " + map + ": its board is not known (more than one map or board)"); continue; }
		for (auto &s : found) {
			s.node = m_elg_nodes[0];
			if (s.active_high) m_sw_active_high[s.node][s.position >> 3] |= uint8_t(1 << (s.position & 7));
			m_switches.push_back(s);
		}
	}
	// The trough holds a ball at each of its numbered optos (TROUGH 1-3); the jam optos stay open
	for (const auto &s : m_switches)
		if (s.name.rfind("TROUGH ", 0) == 0 && s.name.size() > 7 && std::isdigit(uint8_t(s.name[7]))) set_switch(s.node, s.position, true, 0);
	SPIKE1_DEBUG_ONLY(std::string closed; for (const auto &s : m_switches) if (switch_closed(s.node, s.position)) closed += ' ' + s.name + ',';)
	SPIKE1_LOG("devices: ELG framework: " + std::to_string(m_coils.size()) + " coils, " + std::to_string(m_leds.size()) + " lamps, " +
		std::to_string(m_switches.size()) + " switches from the game's tables; closed at rest:" + (closed.empty() ? " none" : closed.substr(0, closed.size() - 1)));
}

// The alphanumeric display's frame (the game's dmd_update_t()): four 64-byte bitplanes, plane n
// holding bit n of every segment's 4-bit level, segment k of character c at bit 16c + k, most
// significant bit of a byte first. The game paces its frames itself
int32_t spike1_devices::alnum_write(uint32_t buf, uint32_t len)
{
	const uint8_t *p = m_mem.host(buf, len, false);
	if (!p) return -E_FAULT;
	constexpr uint32_t plane = ALNUM_FRAME_BYTES / 4;
	for (uint32_t i = 0; i < ALNUM_CHARS * ALNUM_SEGMENTS; i++) {
		uint8_t level = 0;
		for (uint32_t n = 0; n < 4; n++) level |= ((p[n * plane + i / 8] >> (7 - i % 8)) & 1) << n;
		m_alnum[i] = level;
	}
	m_alnum_frames++;
	return int32_t(len);
}

// The game's font: 16 bytes a character, a segment lit where its byte is not 0
std::string spike1_devices::alnum_text() const
{
	constexpr uint32_t GLYPHS = 128;
	const uint32_t font = m_cfg.symbol ? m_cfg.symbol("_ZL4font") : 0;
	uint8_t glyph[GLYPHS * ALNUM_SEGMENTS] = {};
	const bool have_font = font && m_mem.read(font, glyph, sizeof(glyph));
	std::string text;
	for (uint32_t c = 0; c < ALNUM_CHARS; c++) {
		char shown = '?';
		for (uint32_t g = ' '; have_font && g < GLYPHS; g++) {
			uint32_t k = 0;
			while (k < ALNUM_SEGMENTS && (glyph[g * ALNUM_SEGMENTS + k] != 0) == (m_alnum[c * ALNUM_SEGMENTS + k] != 0)) k++;
			if (k == ALNUM_SEGMENTS) { shown = char(g); break; }
		}
		text += shown;
	}
	return text;
}

// ---------------------------------------------------------------- switches
//
// The title's tables, all {entries, count, entry size} descriptors:
//   node_board_table          entry +12: the board's bus address (upper half on newer SDKs)
//   node_board_device_table   24-byte entries: +8 number, +12 names (English first),
//                             +16 type << 16 | board, +20 position << 16; type 2 = coil,
//                             4 = LED channel, 7 = switch
//   switch_table              entry per switch number: +0x10 flags (bit 2: active high,
//   switch_dedicated_table    bit 7: closed at rest), the dedicated ones numbered from 129
// The numbers there are the game's own; apply_manual_numbers() gives switches, coils and LED channels the manual's
void spike1_devices::load_device_table()
{
	m_switches.clear();
	m_coils.clear();
	m_leds.clear();
	std::memset(m_sw_closed, 0, sizeof(m_sw_closed));
	std::memset(m_sw_read, 0, sizeof(m_sw_read));
	std::memset(m_sw_changed, 0, sizeof(m_sw_changed));
	std::memset(m_sw_active_high, 0, sizeof(m_sw_active_high));
	if (!m_cfg.symbol) return;
	if (m_elg) { elg_load_tables(); return; }
	auto desc = [&](const char *name, uint32_t out[3]) {
		out[0] = out[1] = out[2] = 0;
		const uint32_t a = m_cfg.symbol(name);
		return a && guest_read(a, out, 12) && out[0] && out[1] && out[2];
	};
	uint32_t boards[3], devices[3], table[3], dedicated[3];
	if (!desc("node_board_table", boards) || !desc("node_board_device_table", devices)) return;
	const bool have_table = desc("switch_table", table);
	const bool have_dedicated = desc("switch_dedicated_table", dedicated);
	std::vector<uint8_t> node_of(boards[1]);
	bool upper = false;
	std::vector<uint32_t> raw(boards[1]);
	for (uint32_t i = 0; i < boards[1]; i++) { guest_read(boards[0] + i * boards[2] + 12, &raw[i], 4); if (raw[i] >= 0x10000) upper = true; }
	for (uint32_t i = 0; i < boards[1]; i++) node_of[i] = uint8_t((upper ? raw[i] >> 16 : raw[i]) & 0x7f);

	// The device table is what the switch test names a board's inputs from; the switch code reads
	// them through the program's compiled maps (compiled_switch_inputs()), which place a few
	// switches elsewhere: Supreme, Primus, Whoa Nellie and Pabst read SLAM TILT at cabinet input
	// 21 where their tables have 22, KISS the reverse, and WWE's SDK the CPU board's DIP switches
	// 2-8 and service buttons an input lower than its table (DIP 2 at 1, SERVICE SELECT at 8, BACK
	// at 11, as the later titles' tables have them). A switch the maps read at none of its table
	// positions is wired where they read it - on its own board when they read it there
	const std::map<uint16_t, std::vector<std::pair<uint8_t, uint8_t>>> compiled = compiled_switch_inputs();
	std::string moved;
	std::vector<std::pair<switch_info, uint16_t>> found; // with their switch-table flags
	std::vector<std::pair<int, size_t>> device(devices[1], { 0, 0 }); // device index -> (type, index in m_coils/m_leds)
	// An entry's four u16 fields from +16: [board][type][flags][position] up to node firmware 0.52,
	// [board][position][type][flags] in Heavy Metal's SDK (0.67). The order whose type field holds
	// a device type (2 coil, 4 LED, 7 switch) in more entries is the title's
	auto is_device = [](uint32_t t) { return t == 2 || t == 4 || t == 7; };
	int typed_first = 0, typed_second = 0;
	for (uint32_t i = 0; i < devices[1]; i++) {
		uint32_t e[6] = {};
		if (!guest_read(devices[0] + i * devices[2], e, 24)) continue;
		typed_first += is_device(e[4] >> 16);
		typed_second += is_device(e[5] & 0xffff);
	}
	const bool position_first = typed_second > typed_first;
	for (uint32_t i = 0; i < devices[1]; i++) {
		uint32_t e[6] = {};
		if (!guest_read(devices[0] + i * devices[2], e, 24)) continue;
		if (position_first) { // to [board][type][flags][position]
			const uint32_t board_position = e[4], type_flags = e[5];
			e[4] = (board_position & 0xffff) | (type_flags << 16);
			e[5] = (type_flags >> 16) | (board_position & 0xffff0000);
		}
		const uint32_t type = e[4] >> 16, board = e[4] & 0xffff;
		if (type != 2 && type != 4 && type != 7) continue;
		uint32_t name_ptr = 0;
		std::string name;
		if (e[3] && guest_read(e[3], &name_ptr, 4) && name_ptr) m_mem.read_string(name_ptr, name, 64);
		while (!name.empty() && name.back() == ' ') name.pop_back();
		const uint8_t node = board < node_of.size() ? node_of[board] : 0, position = uint8_t(e[5] >> 16);
		if (type != 7) {
			auto &list = type == 2 ? m_coils : m_leds;
			device[i] = { int(type), list.size() };
			list.push_back({ uint16_t(e[2]), node, position, name });
			continue;
		}
		switch_info s;
		s.number = uint16_t(e[2]);
		s.node = node;
		s.position = position;
		s.name = name;
		const auto read_at = compiled.find(s.number);
		if (read_at != compiled.end() &&
		    std::find(read_at->second.begin(), read_at->second.end(), std::make_pair(s.node, s.position)) == read_at->second.end()) {
			auto input = std::find_if(read_at->second.begin(), read_at->second.end(),
				[&](const std::pair<uint8_t, uint8_t> &i) { return i.first == s.node; });
			if (input == read_at->second.end()) input = read_at->second.begin();
			moved += ' ' + name + ' ' + std::to_string(input->first) + '-' + std::to_string(input->second) +
				" (" + std::to_string(s.node) + '-' + std::to_string(s.position) + "),";
			s.node = input->first;
			s.position = input->second;
		}
		uint16_t flags = 0;
		if (have_table && s.number < table[1]) guest_read(table[0] + s.number * table[2] + 0x10, &flags, 2);
		else if (have_dedicated && s.number >= 129 && s.number - 129u < dedicated[1]) guest_read(dedicated[0] + (s.number - 129u) * dedicated[2] + 0x10, &flags, 2);
		s.active_high = (flags & 4) != 0;
		if (s.active_high) m_sw_active_high[s.node][(s.position >> 3) & 7] |= uint8_t(1 << (s.position & 7));
		device[i] = { 7, found.size() };
		found.push_back({ s, flags });
	}

	// What is closed while the machine stands idle with its coin door shut: a trough opto for every
	// installed ball, the firmware's own closed-at-rest marks on other switches, and the door's power sense (DC present)
	auto upper_name = [](std::string n) { for (auto &c : n) c = char(std::toupper(uint8_t(c))); return n; };
	uint8_t installed = 1;
	if (const uint32_t a = m_cfg.symbol("hook_balls_installed_in_game")) guest_read(a, &installed, 1);
	if (installed < 1 || installed > 12) installed = 1;
	std::vector<const switch_info *> troughs;
	for (const auto &f : found) if (upper_name(f.first.name).find("TROUGH") != std::string::npos) troughs.push_back(&f.first);
	auto trough_key = [&](const switch_info *s) {
		const std::string n = upper_name(s->name);
		const size_t at = n.find("TROUGH ");
		if (at != std::string::npos && std::isdigit(uint8_t(n[at + 7]))) return std::atoi(n.c_str() + at + 7);
		return 1000; // the jam opto and unnumbered ones after the ball positions
	};
	std::stable_sort(troughs.begin(), troughs.end(), [&](const switch_info *a, const switch_info *b) { return trough_key(a) < trough_key(b); });
	for (size_t i = 0; i < troughs.size() && i < installed; i++) set_switch(troughs[i]->node, troughs[i]->position, true, 0);
	for (const auto &f : found) {
		const std::string n = upper_name(f.first.name);
		if (((f.second & 0x80) && n.find("TROUGH") == std::string::npos) ||
		    n.find("INTERLOCK") != std::string::npos || n.find("DC SENSE") != std::string::npos)
			set_switch(f.first.node, f.first.position, true, 0);
	}
	if (!moved.empty())
		SPIKE1_LOG("devices: where the game's switch maps read what its device table puts elsewhere:" + moved.substr(0, moved.size() - 1));
	for (auto &f : found) m_switches.push_back(f.first);
	apply_manual_numbers(device);
	SPIKE1_DEBUG_ONLY(std::string closed; for (const auto &s : m_switches) if (switch_closed(s.node, s.position)) closed += ' ' + s.name + ',';)
	SPIKE1_LOG("devices: " + std::to_string(m_coils.size()) + " coils, " + std::to_string(m_leds.size()) + " LED channels, " +
		std::to_string(m_switches.size()) + " switches from the game's tables, " + std::to_string(installed) +
		" balls installed; closed at rest:" + (closed.empty() ? " none" : closed.substr(0, closed.size() - 1)));
}

// Where the game's switch code reads each switch. The program's board records (g_node_data) give
// each board's node address and then its switch map (g_smap_<board>): an entry for each of 64
// inputs, the first word the switch's word in the game's switch bits (0xf0 | word for the
// dedicated switches, numbered from 129) and the u16 after it its bit, then the map's own state,
// 0x54 bytes. The entries are 8 bytes from Supreme on and 56 in WWE's and Spider-Man's SDKs. A
// program without the records has none here: Heavy Metal's SDK builds its maps from the device
// table as it starts, and a map the program fills in itself (Game of Thrones' topper) is empty
std::map<uint16_t, std::vector<std::pair<uint8_t, uint8_t>>> spike1_devices::compiled_switch_inputs()
{
	constexpr uint32_t MAP_INPUTS = 64, MAP_STATE_BYTES = 0x54;
	std::map<uint16_t, std::vector<std::pair<uint8_t, uint8_t>>> inputs;
	const uint32_t records = m_cfg.symbol("g_node_data"), records_size = m_cfg.symbol_size ? m_cfg.symbol_size("g_node_data") : 0;
	if (!records || records_size < 8 || !m_cfg.symbols_with_prefix) return inputs;
	std::vector<uint32_t> words(records_size / 4);
	if (!guest_read(records, words.data(), uint32_t(words.size() * 4))) return inputs;
	for (const auto &name : m_cfg.symbols_with_prefix("g_smap_")) {
		const uint32_t map = m_cfg.symbol(name), size = m_cfg.symbol_size(name);
		if (!map || size <= MAP_STATE_BYTES || (size - MAP_STATE_BYTES) % MAP_INPUTS) continue;
		const uint32_t entry = (size - MAP_STATE_BYTES) / MAP_INPUTS;
		const auto ref = std::find(words.begin() + 1, words.end(), map); // the record's map, after its node address
		if (entry < 8 || ref == words.end() || (*(ref - 1) & 0xff) > 127) continue;
		const uint8_t node = uint8_t(*(ref - 1));
		for (uint32_t position = 0; position < MAP_INPUTS; position++) {
			uint32_t e[2];
			if (!guest_read(map + position * entry, e, 8)) break;
			const uint32_t word = e[0], mask = e[1] & 0xffff;
			if (!mask || (mask & (mask - 1)) || word > 0xff) continue;
			unsigned bit = 0;
			while (!((mask >> bit) & 1)) bit++;
			const uint16_t number = uint16_t(word & 0xf0 ? 129 + (word & 0xf) * 16 + bit : 1 + word * 16 + bit);
			inputs[number].push_back({ node, uint8_t(position) });
		}
	}
	return inputs;
}

const spike1_devices::switch_info *spike1_devices::find_switch(const std::string &name) const
{
	auto upper = [](std::string n) { for (auto &c : n) c = char(std::toupper(uint8_t(c))); return n; };
	const std::string want = upper(name);
	for (const auto &s : m_switches) if (upper(s.name) == want) return &s;
	return nullptr;
}

void spike1_devices::set_switch(uint8_t node, uint8_t position, bool closed, uint64_t now_ns)
{
	uint8_t &b = m_sw_closed[node & 127][(position >> 3) & 7];
	const uint8_t bit = uint8_t(1 << (position & 7)), was = b;
	b = closed ? uint8_t(b | bit) : uint8_t(b & ~bit);
	m_sw_changed[node & 127][(position >> 3) & 7] |= uint8_t((was ^ b) & bit);
	run_reflexes(node & 127, now_ns);
}

// Switches are active low on the wire unless the title marks them active high. The game takes
// every change it reads - sys_node_board_device_switch_update_inputs() counts each edge between two
// reads, and the switch code then handles each one, so a level one read sees is enough (a 2 ms
// Start press starts a game when a read falls inside it) - but it reads a board only every 2 to
// 5 ms, and a ball through a VPX trigger closes it for a few. So each read shows a switch's next
// change: one that changed and changed back since the last read shows that change to this read,
// and its level again to the next. The boards' own reflexes (flippers, slingshots) follow the
// switch itself, at once
void spike1_devices::wire_bytes(uint8_t node, uint8_t *out, uint32_t len)
{
	const uint8_t n = node & 127;
	for (uint32_t i = 0; i < len && i < 8; i++) {
		m_sw_read[n][i] ^= uint8_t((m_sw_closed[n][i] ^ m_sw_read[n][i]) | m_sw_changed[n][i]);
		m_sw_changed[n][i] = 0;
	}
	for (uint32_t i = 0; i < len; i++)
		out[i] = i < 8 ? uint8_t(~(m_sw_read[n][i] ^ m_sw_active_high[n][i])) : 0xff;
}

// The CPU board's own inputs (DIP switches, service buttons, the door's power sense - board 0)
// come back in the receive buffer of every SPI transfer on /dev/spi1. struct spi_ioc_transfer,
// 32 bytes: u64 tx_buf, u64 rx_buf, u32 len, ...
int32_t spike1_devices::cpu_spi_message(uint32_t req, uint32_t arg, std::string &note)
{
	const uint32_t count = ((req >> 16) & 0x3fff) / 32;
	uint32_t total = 0;
	for (uint32_t i = 0; i < count; i++) {
		uint32_t t[5];
		if (!guest_read(arg + 32 * i, t, 20)) return -E_FAULT;
		const uint32_t rx = t[2], len = t[4];
		total += len;
		if (!rx || !len) continue;
		uint8_t *p = m_mem.host(rx, len);
		if (!p) return -E_FAULT;
		wire_bytes(0, p, len);
	}
	if (tracing()) note = "cpu spi: " + std::to_string(count) + " transfer(s)";
	return int32_t(total); // spidev answers with the bytes moved
}

// ---------------------------------------------------------------- motors

// An encoder motor as the node firmware runs it (0.52.0, pinnode-LPC1313, the board Ghostbusters'
// Slimer hangs off). The status bits: 0 driving, 1 driving down, 2 holding, 3 homed (the encoder
// counts from here), 4 away switch found, 5 target switch found, 6 time-out, 7 limit switch, 8 a
// go without an encoder. Home drives down until the home input is active; the game homes twice,
// first with the outputs swapped and the home input's polarity turned (up, until out of the home
// zone), then as configured (down, until into it), so its zero is the zone's edge, reached from
// above. A go ramps the duty up by a step a tick to its speed, holds it, ramps it down from the
// point as far before the target as the ramp up took (or from half way) to the least duty, and
// stops on the exact count

void spike1_devices::link_encoder_motor(uint8_t node, uint8_t motor_index, int16_t low_stop, int16_t high_stop, uint16_t counts_per_s)
{
	motor &m = m_mechanics[(uint32_t(node) << 8) | motor_index];
	m.low = low_stop;
	m.high = high_stop;
	m.counts_per_s = std::min<double>(std::max<uint16_t>(counts_per_s, 1), 1000.0); // under a count a tick
	m.x = std::max(m.low, -0.5);
}

// An input the firmware names by id: false when the id is unused (bit 6 clear). The board reads its
// own pin, not what the game was last shown (wire_bytes())
bool spike1_devices::motor_input(uint8_t node, uint8_t id, bool &active) const
{
	if (!(id & 0x40)) return false;
	const uint8_t in = id & 0x3f, n = node & 127;
	const bool high = ((~(m_sw_closed[n][in >> 3] ^ m_sw_active_high[n][in >> 3])) >> (in & 7)) & 1;
	active = (id & 0x80) ? high : !high;
	return true;
}

uint8_t spike1_devices::motor_pins(uint8_t node, const motor &m) const
{
	const uint8_t ids[3] = { m.home_in, m.away_in, m.watch_in };
	uint8_t pins = 0;
	for (int k = 0; k < 3; k++) {
		bool active = false;
		if (motor_input(node, ids[k], active) && active == ((ids[k] & 0x80) != 0)) pins |= uint8_t(1 << k);
	}
	return pins;
}

// The mechanism's own switches as it stands: the home input at its zone level below the zone's
// edge (x < 0), the encoder's two phases a count apart
void spike1_devices::motor_switches(uint8_t node, const motor &m, uint64_t now_ns)
{
	auto set_level = [&](uint8_t id, bool high) {
		if (!(id & 0x40)) return;
		const uint8_t in = id & 0x3f, n = node & 127;
		const bool ah = (m_sw_active_high[n][in >> 3] >> (in & 7)) & 1, closed = high == ah;
		if (switch_closed(n, in) != closed) set_switch(n, in, closed, now_ns);
	};
	set_level(m.home_in, (m.x < 0.0) == (m.zone_raw != 0));
	if (m.encoder) {
		const int32_t c = int32_t(std::floor(m.x));
		set_level(m.enc_a, (c >> 1) & 1);
		set_level(m.enc_b, ((c + 1) >> 1) & 1);
	}
}

// The firmware's output routine: one output at the duty, the other off; status bit 0 while it
// drives, bit 1 for down. A time-out of 0 or more restarts the motor's time-out
void spike1_devices::motor_drive(motor &m, uint8_t duty, bool down, int32_t timeout)
{
	m.duty = duty;
	m.down = down;
	m.status = down ? uint16_t(m.status | 0x02) : uint16_t(m.status & ~0x02);
	if (!duty) { m.status &= uint16_t(~0x01); return; }
	m.status = uint16_t((m.status & ~0x81) | 0x01);
	if (timeout >= 0) {
		m.timeout = m.timeout_left = uint16_t(timeout);
		if (timeout) m.status &= uint16_t(~0x40);
	}
}

void spike1_devices::motor_command(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t n, uint64_t now_ns)
{
	const uint32_t key = (uint32_t(node) << 8) | data[0];
	if (cmd == 0x51 || cmd == 0x57) { // configure, configure update: [motor][16 bytes]
		if (n < 17) return;
		auto it = m_motors.find(key);
		if (it == m_motors.end()) { // the mechanism stands where it stood at power-on
			const auto mech = m_mechanics.find(key);
			it = m_motors.emplace(key, mech != m_mechanics.end() ? mech->second : motor()).first;
			it->second.at_ns = now_ns;
		}
		motor &m = it->second;
		m.home_in = data[1]; m.away_in = data[2]; m.watch_in = data[3];
		m.enc_a = data[4]; m.enc_b = data[5];
		m.out_down = data[7]; m.out_up = data[8];
		m.ramp = data[9]; m.least = data[10];
		m.settle[0] = uint16_t(data[13] | data[14] << 8);
		m.settle[1] = uint16_t(data[15] | data[16] << 8);
		if (cmd == 0x51) m.state = 0;
		m.encoder = (m.enc_a & 0x40) && (m.enc_b & 0x40);
		m.counted = int32_t(std::floor(m.x));
		m.status = 0;
		// Which way each output moves the mechanism, by the LED channel's name (Slimer Motor Forward
		// and Backward); else as the firmware's go assumes, counting up on out_up
		auto sign = [&](uint8_t out, int8_t otherwise) -> int8_t {
			for (const auto &l : m_leds) {
				if (l.node != node || l.position != out) continue;
				std::string u = l.name;
				for (auto &ch : u) ch = char(std::toupper(uint8_t(ch)));
				if (u.find("BACKWARD") != std::string::npos) return -1;
				if (u.find("FORWARD") != std::string::npos) return 1;
			}
			return otherwise;
		};
		m.sign_down = sign(m.out_down, -1);
		m.sign_up = sign(m.out_up, 1);
		// the zone is where the home input is active as the game configures it to run - counting
		// down toward the zone - and the opposite in the swapped configuration
		m.zone_raw = uint8_t((m.sign_down < 0) == ((m.home_in & 0x80) != 0));
		motor_switches(node, m, now_ns);
		m.last_raw = motor_pins(node, m);
		return;
	}
	auto it = m_motors.find(key);
	if (it == m_motors.end()) return;
	motor &m = it->second;
	switch (cmd) {
	case 0x53: case 0x54: { // home, away: [motor][speed][fade][time-out, u16]
		if (n < 5) return;
		const bool home = cmd == 0x53;
		const uint8_t in = home ? m.home_in : m.away_in;
		m.speed = data[1];
		m.status &= 0xfe3f;
		bool active = false;
		if (motor_input(node, in, active) && active) { // there already
			m.status |= home ? 0x08 : 0x10;
			if (m.encoder) { if (home) m.position = 0; return; }
			m.position = in & 0x3f;
			m.status &= home ? 0xffcf : 0xffd7;
			return;
		}
		m.flags = 0;
		m.state = home ? 2 : 3;
		if (!m.encoder) m.status &= 0xffc7;
		motor_drive(m, m.speed, home, data[3] | data[4] << 8);
		return;
	}
	case 0x55: { // go: [motor][target, s16][speed][time-out, u16][flags: bit 0 hold there, bit 1 stop at a limit switch]
		if (n < 7) return;
		const int16_t target = int16_t(data[1] | data[2] << 8);
		const uint16_t was = m.status;
		m.status &= 0xfe3f;
		if (m.encoder && !(was & 0x08)) return; // not homed: the board takes no target
		if (!m.encoder) { m.status |= 0x100; m.state = 0; return; } // a target switch: not modelled
		if (m.position == target) { m.state = 0; return; }
		m.acc = 0;
		m.start = m.position;
		m.phase = target <= m.position ? 4 : 0;
		m.mid = int16_t((target + m.position) / 2);
		m.target = target;
		m.speed = data[3];
		m.flags = data[6];
		if (m.ramp) m.step = uint32_t((int32_t(int16_t(51200 / m.ramp)) * int32_t(m.speed)) >> 12);
		else m.acc = uint32_t(m.speed) << 8;
		m.timeout = m.timeout_left = uint16_t(data[4] | data[5] << 8);
		m.state = 4;
		return;
	}
	case 0x56: // stop: [motor]
		m.state = 0;
		m.status &= 0xfe3f;
		return;
	}
}

// One tick of the board's motor loop, then the mechanism moves on for the tick at the duty set
void spike1_devices::motor_tick(uint8_t node, motor &m, uint64_t now_ns)
{
	const uint8_t ids[3] = { m.home_in, m.away_in, m.watch_in };
	bool used[3], active[3] = {};
	for (int k = 0; k < 3; k++) used[k] = motor_input(node, ids[k], active[k]);
	const uint8_t raw = motor_pins(node, m), changed = raw ^ m.last_raw;
	m.last_raw = raw;
	auto found = [&](int k) { return used[k] && ((changed >> k) & 1) && active[k]; };
	// the time-out, which the watch input's edges restart
	if (found(2)) m.timeout_left = m.timeout;
	else if (m.timeout_left && --m.timeout_left == 0) { m.status |= 0x40; m.state = 0; }
	// the encoder counts once the motor is homed
	if (m.encoder && (m.status & 0x08)) {
		const int32_t c = int32_t(std::floor(m.x));
		m.position = int16_t(m.position + (c - m.counted));
		m.counted = c;
	}
	switch (m.state) {
	case 0: // stop, then idle
		motor_drive(m, 0, false, -1);
		m.status &= uint16_t(~0x04);
		m.state = 1;
		m.timeout_left = 0;
		break;
	case 2: case 3: { // home, away: until the switch is found
		const int k = m.state - 2;
		m.status &= uint16_t(~0x04);
		if (!found(k)) break;
		m.status |= k ? 0x10 : 0x08;
		if (!m.encoder) m.position = ids[k] & 0x3f;
		m.settle_left = m.settle[0];
		m.state = 7;
		if (!k) { m.counted = int32_t(std::floor(m.x)); m.position = 0; }
		break;
	}
	case 4: { // go
		m.status &= uint16_t(~0x04);
		bool limit = false;
		if (m.flags & 0x02) { bool a = false; limit = motor_input(node, m.down ? m.home_in : m.away_in, a) && a; }
		if (limit || m.position == m.target) {
			if (limit) m.status |= 0x80;
			m.state = (!limit && (m.flags & 0x01)) ? 6 : 0;
			m.acc = 0;
			motor_drive(m, 0, false, -1);
			break;
		}
		const bool down = m.phase >= 4;
		const bool ahead = down ? m.position > m.mid : m.position < m.mid;
		const uint32_t top = uint32_t(m.speed) << 8, least = uint32_t(m.least) << 8;
		switch (m.phase & 3) {
		case 0: // ramping up
			m.acc += m.step;
			if (m.acc > top) { m.acc = top; m.phase++; m.mid = int16_t(m.start + m.target - m.position); }
			else if (!ahead) m.phase += 2;
			break;
		case 1: // at speed
			if (!ahead) m.phase++;
			break;
		case 2: // ramping down to the least duty
			if (m.acc > m.step) m.acc -= m.step;
			if (m.acc < least) { m.acc = least; m.phase++; }
			break;
		}
		motor_drive(m, uint8_t(m.acc >> 8), down, -1);
		break;
	}
	case 6: // holding at the target (nothing moves it away here)
		m.status |= 0x04;
		break;
	case 7: // settling after a switch was found
		if (m.settle_left) m.settle_left--;
		else { m.settle_left = m.settle[1]; m.state = 8; }
		break;
	case 8: case 9: // then the brake output, which no title configures
		m.state = 0;
		break;
	}
	if (m.duty) {
		const int8_t way = m.down ? m.sign_down : m.sign_up;
		m.x = std::clamp(m.x + way * m.counts_per_s * m.duty / 255.0 / MOTOR_TICKS_PER_S, m.low, m.high);
		motor_switches(node, m, now_ns);
	}
}

void spike1_devices::motors_run(uint64_t now_ns)
{
	for (auto &it : m_motors) {
		motor &m = it.second;
		const uint8_t node = uint8_t(it.first >> 8);
		while (m.at_ns + MOTOR_TICK_NS <= now_ns) {
			if (m.state == 1 && !m.duty && !m.timeout_left) { // nothing moves until a command
				m.at_ns += (now_ns - m.at_ns) / MOTOR_TICK_NS * MOTOR_TICK_NS;
				m.last_raw = motor_pins(node, m);
				break;
			}
			m.at_ns += MOTOR_TICK_NS;
			motor_tick(node, m, m.at_ns);
		}
	}
}

uint16_t spike1_devices::stepper_position(const stepper &s, uint64_t now_ns) const
{
	if (now_ns >= s.end_ns || s.end_ns <= s.start_ns) return s.target;
	const uint32_t done = uint32_t(uint64_t(s.distance) * (now_ns - s.start_ns) / (s.end_ns - s.start_ns));
	return uint16_t((s.backward ? s.from + s.steps - done % s.steps : s.from + done) % s.steps);
}

// A stepper's home switch - its board's own input, or a switch on another board the title links -
// is closed while the stepper stands within two steps of step 0
void spike1_devices::stepper_home_switches(uint64_t now_ns)
{
	for (const auto &it : m_steppers) {
		const stepper &s = it.second;
		const auto link = m_home_links.find(it.first);
		uint8_t node = uint8_t(it.first >> 8), input;
		if (link != m_home_links.end()) { node = link->second.first; input = link->second.second; }
		else if (s.home_input >= 0) input = uint8_t(s.home_input);
		else continue;
		const uint16_t at = stepper_position(s, now_ns);
		const bool home = at < 2 || at + 2 > s.steps;
		if (switch_closed(node, input) != home) set_switch(node, input, home, now_ns);
	}
}

bool spike1_devices::link_stepper_home(uint8_t node, uint8_t stepper, uint16_t switch_number)
{
	for (const switch_info &sw : m_switches)
		if (sw.number == switch_number) {
			m_home_links[(uint32_t(node) << 8) | stepper] = { sw.node, sw.position };
			set_switch(sw.node, sw.position, true, 0);
			return true;
		}
	return false;
}

bool spike1_devices::link_limit_motor(uint16_t forward_led, uint16_t backward_led, uint16_t home_switch, uint16_t away_switch, uint32_t travel_ms)
{
	limit_motor m;
	unsigned found = 0;
	for (const output_info &led : m_leds) {
		if (led.number == forward_led) { m.forward[0] = led.node; m.forward[1] = led.position; found |= 1; }
		if (led.number == backward_led) { m.backward[0] = led.node; m.backward[1] = led.position; found |= 2; }
	}
	for (const switch_info &sw : m_switches) {
		if (sw.number == home_switch) { m.home[0] = sw.node; m.home[1] = sw.position; found |= 4; }
		if (sw.number == away_switch) { m.away[0] = sw.node; m.away[1] = sw.position; found |= 8; }
	}
	if (found != 15 || !travel_ms) return false;
	m.travel_ns = int64_t(travel_ms) * 1000000;
	set_switch(m.home[0], m.home[1], true, 0);
	m_limit_motors.push_back(m);
	return true;
}

int64_t spike1_devices::limit_motor_position(const limit_motor &m, uint64_t now_ns) const
{
	const int64_t at = m.at_ns + m.drive * int64_t(now_ns - m.since_ns);
	return std::clamp<int64_t>(at, 0, m.travel_ns);
}

// Moves each limit motor on to now_ns as the drive left it, takes the drive the LED channels set
// now, and sets its switches, each closed over the last 2% of the way to it - so the game's
// start-up LED test, a few ms on each channel, leaves the motor on its home switch
void spike1_devices::limit_motors_run(uint64_t now_ns)
{
	for (limit_motor &m : m_limit_motors) {
		m.at_ns = limit_motor_position(m, now_ns);
		m.since_ns = now_ns;
		m.drive = (led_target(m.forward[0], m.forward[1]) ? 1 : 0) - (led_target(m.backward[0], m.backward[1]) ? 1 : 0);
		const int64_t zone = m.travel_ns / 50;
		const bool home = m.at_ns <= zone, away = m.at_ns >= m.travel_ns - zone;
		if (switch_closed(m.home[0], m.home[1]) != home) set_switch(m.home[0], m.home[1], home, now_ns);
		if (switch_closed(m.away[0], m.away[1]) != away) set_switch(m.away[0], m.away[1], away, now_ns);
	}
}

// ---------------------------------------------------------------- coils
//
// From the game's NODEBUS_Coil and NODEBUS_CoilConfig: times are board ticks (see NODE_TICK_HZ),
// powers a PWM duty out of 255.
//   fire     [coil][power 1][time 1, u16][power 2][time 2, u16][ramp, u16]
//            ([u16][switch condition, 3] follow on some fires - not modelled yet)
//   reflex   the same seven, five u16 times (the second keeps the coil from firing again too
//            soon), then a list of switches whose place and form the SDK sets, told apart by
//            the frame's length: the trigger switches - the board fires the coil when any of
//            them closes - and after them the end-of-stroke switch
//              23 bytes (WWE, node firmware 0.18.4): one trigger at 17, the EOS at 18
//              28 bytes (Supreme, 0.22):             two at 17, the EOS at 19
//              34 bytes (0.28 to 0.52):              two bytes, then three at 19, the EOS at 22
//              43 bytes (Heavy Metal, 0.67):         four bytes, then three at 21, the EOS at 24
//            (Heavy Metal's slings fire on either of two switches)
// What the traffic shows: flippers pulse at full power, then hold at power 2 while the button
// stays closed; pops and slings pulse once per closure; one-shot coils pulse at power 1 for time 1

static uint32_t ticks_ms(const uint8_t *p) { return uint32_t(p[0] | (p[1] << 8)) * 1000u / NODE_TICK_HZ; }

void spike1_devices::coil_pulse(uint8_t node, uint8_t index, uint8_t power1, uint32_t ms1, uint8_t power2, uint32_t ms2, uint64_t now_ns)
{
	coil &c = m_coil[node][index];
	c.power1 = power1;
	c.power2 = power2;
	c.phase1_end_ns = now_ns + uint64_t(ms1) * 1000000;
	c.phase2_end_ns = c.phase1_end_ns + uint64_t(ms2) * 1000000;
	c.holding = false;
	c.check = (c.power1 ? ms1 : 0) + (c.power2 ? ms2 : 0) < COIL_SHOWN_MIN_MS;
	if (!c.check && !((m_coil_mask[node] >> index) & 1)) c.shown = std::max({ c.shown, c.power1, c.power2 });
}

void spike1_devices::coil_fire(uint8_t node, const uint8_t *data, uint32_t len, uint64_t now_ns, std::string &note)
{
	if (len < 9 || data[0] >= COILS_PER_NODE) { SPIKE1_DEBUG_ONLY(note = "node bus: node " + std::to_string(node) + " coil fire not understood: " + bytes_hex(data, len);) return; }
	coil_pulse(node, data[0], data[1], ticks_ms(data + 2), data[4], ticks_ms(data + 5), now_ns);
	SPIKE1_DEBUG_ONLY(if (len > 9 && std::any_of(data + 9, data + len, [](uint8_t b) { return b != 0; }) && first_time("coil fire extras"))
		note = "node bus: node " + std::to_string(node) + " coil fire with a time and switch condition (not modelled): " + bytes_hex(data, len);
	else if (tracing())
		note = "node bus: node " + std::to_string(node) + " coil " + std::to_string(data[0]) + " fire";)
}

void spike1_devices::coil_reflex(uint8_t node, const uint8_t *data, uint32_t len, std::string &note)
{
	uint32_t first = 0, triggers = 0;
	switch (len) {
	case 23: first = 17; triggers = 1; break;
	case 28: first = 17; triggers = 2; break;
	case 34: first = 19; triggers = 3; break;
	case 43: first = 21; triggers = 3; break;
	}
	if (!first || data[0] >= COILS_PER_NODE) { SPIKE1_DEBUG_ONLY(note = "node bus: node " + std::to_string(node) + " coil reflex not understood: " + bytes_hex(data, len);) return; }
	coil &c = m_coil[node][data[0]];
	c.reflex = false;
	for (uint32_t i = 0; i < 3; i++) {
		c.trigger[i] = i < triggers ? data[first + i] : 0;
		if (c.trigger[i] & REFLEX_USED) c.reflex = true;
	}
	c.eos = data[first + triggers];
	c.reflex_power1 = data[1];
	c.reflex_power2 = data[4];
	c.reflex_pulse_ms = ticks_ms(data + 2);
	c.reflex_holdoff_ms = ticks_ms(data + 11);
	c.trigger_was_active = c.reflex && reflex_triggered(node, c);
	if (!c.reflex) c.holding = false;
	if (tracing()) {
		std::string on;
		for (uint8_t t : c.trigger) if (t & REFLEX_USED) on += (on.empty() ? " reflex on switch " : " or ") + std::to_string(t & REFLEX_POSITION);
		note = "node bus: node " + std::to_string(node) + " coil " + std::to_string(data[0]) + (c.reflex ? on : " reflex off");
	}
}

bool spike1_devices::reflex_switch_active(uint8_t node, uint8_t sw) const
{
	return (sw & REFLEX_USED) && (switch_closed(node, sw & REFLEX_POSITION) != ((sw & REFLEX_INVERT) != 0));
}

bool spike1_devices::reflex_triggered(uint8_t node, const coil &c) const
{
	for (uint8_t t : c.trigger) if (reflex_switch_active(node, t)) return true;
	return false;
}

void spike1_devices::run_reflexes(uint8_t node, uint64_t now_ns)
{
	for (coil &c : m_coil[node]) {
		if (!c.reflex) continue;
		const bool active = reflex_triggered(node, c);
		if (active && !c.trigger_was_active && now_ns >= c.holdoff_end_ns) {
			c.power1 = c.reflex_power1;
			c.power2 = c.reflex_power2;
			c.phase1_end_ns = now_ns + uint64_t(c.reflex_pulse_ms) * 1000000;
			c.phase2_end_ns = 0;
			c.holding = c.reflex_power2 != 0;
			c.holdoff_end_ns = c.phase1_end_ns + uint64_t(c.reflex_holdoff_ms) * 1000000;
			c.check = false;
			if (!((m_coil_mask[node] >> unsigned(&c - m_coil[node])) & 1)) c.shown = std::max(c.shown, c.power1);
		} else if (!active && c.trigger_was_active && c.holding) {
			c.holding = false;                                     // a released flipper drops at once
			c.phase1_end_ns = std::min(c.phase1_end_ns, now_ns);
		}
		if (c.holding && now_ns < c.phase1_end_ns && reflex_switch_active(node, c.eos))
			c.phase1_end_ns = now_ns;                              // end of stroke: on to the hold power
		c.trigger_was_active = active;
	}
}

uint8_t spike1_devices::coil_level(uint8_t node, uint8_t position, uint64_t now_ns) const
{
	if (node > 127 || position >= COILS_PER_NODE || (m_coil_mask[node] >> position) & 1) return 0;
	const coil &c = m_coil[node][position];
	if (now_ns < c.phase1_end_ns) return c.power1;
	if (c.holding || now_ns < c.phase2_end_ns) return c.power2;
	return 0;
}

bool spike1_devices::reflex_armed(uint8_t node, uint8_t position) const
{
	if (node > 127 || position >= COILS_PER_NODE) return false;
	const coil &c = m_coil[node][position];
	return c.reflex && (c.reflex_power1 || c.reflex_power2) && !((m_coil_mask[node] >> position) & 1);
}

uint8_t spike1_devices::coil_output(uint8_t node, uint8_t position, uint64_t now_ns)
{
	if (node > 127 || position >= COILS_PER_NODE) return 0;
	coil &c = m_coil[node][position];
	const uint8_t shown = std::max(c.check ? uint8_t(0) : coil_level(node, position, now_ns), c.shown);
	c.shown = 0;
	return shown;
}

// ---------------------------------------------------------------- LEDs
//
// The game's NODEBUS_SetLEDMultiple2 packs a set of channel changes as small as it can; the
// command byte says how: 0x80 | 0x20 (index list / extended bitmap) | 0x1c time form | 0x03 level form.
//   channels  first byte below 0x80: one index, or with 0x20 a list ending at the index with bit 7
//             set; first byte with bit 7: a bitmap - [0x80 | 0x40 if absent bytes are 0xff |
//             presence bits][first byte << 4 | last byte] ([presence of bytes 4-11] with 0x20),
//             then the bitmap bytes present
//   levels    0: all 0, 1: all 255, 2: one per channel, 3: one for all
//   times     0: one per channel; else 0x10: a common time follows (0 when absent), and the
//             0x0c field: 4 all common, 8 a second time for the channels a selection bitmap
//             marks, 12 one time for each marked channel. The selection bitmap (a bit per
//             channel in order) follows the channel bitmap
// The time is the channel's fade time (led_set())
bool spike1_devices::led_update(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len, uint64_t now_ns)
{
	const uint8_t list = cmd & 0x20, times = cmd & 0x1c, levels = cmd & 0x03;
	uint32_t p = 0;
	auto next = [&](uint8_t &v) { if (p >= len) return false; v = data[p++]; return true; };
	uint8_t index[LED_CHANNELS];
	uint32_t count = 0;
	uint8_t b = 0;
	if (!next(b)) return false;
	if (b & 0x80) { // bitmap
		const uint8_t fill = (b & 0x40) ? 0xff : 0x00, present = b & 0x3f;
		uint8_t range = 0, upper = 0xff;
		if (!next(range) || (list && !next(upper))) return false;
		const uint32_t first = range >> 4, last = range & 15;
		if (last < first || last >= LED_CHANNELS / 8) return false;
		const uint32_t span = last - first + 1;
		for (uint32_t k = 0; k < span; k++) {
			bool here;
			if (span <= 8) here = k == 0 || k == span - 1 || ((present >> (k - 1)) & 1);
			else here = k <= 3 ? ((present >> k) & 1) : ((upper >> (k - 4)) & 1);
			uint8_t bits = fill;
			if (here && !next(bits)) return false;
			for (uint32_t i = 0; i < 8; i++)
				if ((bits >> i) & 1) index[count++] = uint8_t((first + k) * 8 + i);
		}
	} else if (list) {
		index[count++] = b;
		do {
			if (!next(b) || count >= LED_CHANNELS) return false;
			index[count++] = b & 0x7f;
		} while (!(b & 0x80));
	} else {
		index[count++] = b;
	}
	uint8_t select[LED_CHANNELS / 8] = {};
	if ((data[0] & 0x80) && (times & 0x08))
		for (uint32_t i = 0; i < (count + 7) / 8; i++) if (!next(select[i])) return false;
	uint8_t level[LED_CHANNELS];
	for (uint32_t i = 0; i < count; i++) {
		if (levels == 2) { if (!next(level[i])) return false; }
		else if (levels == 3) { if (i == 0 && !next(level[0])) return false; level[i] = level[0]; }
		else level[i] = levels ? 0xff : 0x00;
	}
	uint8_t time[LED_CHANNELS];
	if (!times) { for (uint32_t i = 0; i < count; i++) if (!next(time[i])) return false; }
	else {
		uint8_t common = 0, second = 0;
		if ((times & 0x10) && !next(common)) return false;
		if ((times & 0x0c) == 0x08 && !next(second)) return false;
		for (uint32_t i = 0; i < count; i++) {
			time[i] = common;
			if (!((select[i >> 3] >> (i & 7)) & 1)) continue;
			if ((times & 0x0c) == 0x08) time[i] = second;
			else if ((times & 0x0c) == 0x0c && !next(time[i])) return false;
		}
	}
	if (p != len) return false;
	for (uint32_t i = 0; i < count; i++)
		if (index[i] < LED_CHANNELS) led_set(node, index[i], level[i], time[i], now_ns);
	return true;
}

// The SDKs before NODEBUS_SetLEDMultiple2 (Whoa Nellie, Game of Thrones: NODEBUS_SetLED,
// NODEBUS_SetLEDMultiple, NODEBUS_SetLEDMultipleTime) set a run of channels:
//   command   0x80 | the first channel, for a run within channels 0-63; or 0xc0 and the first
//             channel as a u16 (Game of Thrones, for channels from 64 on - its start-up test
//             sweeps all 96 a board drives)
//   [time][level, one per channel]          one fade time for all (0xff never: the game sends 0xfe)
//   [0xff][time][level] for each channel    a fade time each
// The packed SDK's own command bytes overlap these, so the form is the program's: the packed one
// where it has NODEBUS_SetLEDMultiple2
bool spike1_devices::led_update_run(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len, uint64_t now_ns)
{
	uint32_t first = cmd & 0x3f, limit = 64;
	if (cmd == 0xc0) {
		if (len < 2) return false;
		first = data[0] | (uint32_t(data[1]) << 8);
		data += 2;
		len -= 2;
		limit = LED_CHANNELS;
	}
	if (len < 2) return false;
	const bool pairs = data[0] == 0xff;
	if (pairs && (len - 1) % 2) return false;
	const uint32_t count = pairs ? (len - 1) / 2 : len - 1;
	if (first + count > limit || first + count > LED_CHANNELS) return false;
	for (uint32_t i = 0; i < count; i++)
		led_set(node, uint8_t(first + i), pairs ? data[2 + 2 * i] : data[1 + i], pairs ? data[1 + 2 * i] : data[0], now_ns);
	return true;
}

// The node boards' firmware (every version the titles carry, 0.18.4 to 0.52.0) steps each channel
// 1280 times a second (SysTick: 37500 clocks at 48 MHz on the LPC1112s, 56250 at 72 MHz on the
// LPC1313s), each step (target - level) * (51200 / time) / 2^20 of the way the update set out
// with - so a fade takes 20.48 steps a unit: 16 ms. It starts from where the channel is, a fade
// in progress included. Time 0 sets the level at once, and so in effect does 1: its table entry
// (51200) overflows 16 bits, the step points away from the target, and the firmware then sets
// the target at the first step
void spike1_devices::led_set(uint8_t node, uint8_t position, uint8_t level, uint8_t fade, uint64_t now_ns)
{
	led_channel &c = m_led[size_t(node & 127) * LED_CHANNELS + position];
	c.from = c.at(now_ns);
	c.to = level;
	c.start_ns = now_ns;
	c.end_ns = fade > 1 ? now_ns + fade * LED_FADE_UNIT_NS : now_ns;
}

uint8_t spike1_devices::led_channel::at(uint64_t now_ns) const
{
	if (now_ns >= end_ns) return to;
	if (now_ns <= start_ns) return from;
	return uint8_t(from + (int64_t(to) - from) * int64_t(now_ns - start_ns) / int64_t(end_ns - start_ns));
}

uint8_t spike1_devices::led_level(uint8_t node, uint8_t position, uint64_t now_ns) const
{
	if (node > 127 || position >= LED_CHANNELS || (m_led_mask[node][position >> 3] >> (position & 7)) & 1) return 0;
	return m_led[size_t(node) * LED_CHANNELS + position].at(now_ns);
}

// The level an update last set, whatever its fade: a motor's H-bridge drive
uint8_t spike1_devices::led_target(uint8_t node, uint8_t position) const
{
	if (node > 127 || position >= LED_CHANNELS || (m_led_mask[node][position >> 3] >> (position & 7)) & 1) return 0;
	return m_led[size_t(node) * LED_CHANNELS + position].to;
}

// ---------------------------------------------------------------- LCD insert
//
// From the game's NODEBUS_LCDInsert* functions. The sub-command's low two bits pick one of four
// channels (Ghostbusters uses channel 0); the rest of the frame follows the sub-command:
//   0x00           status, a byte: bit 1 = busy (flash erase/write)
//   0x80 | ch      backlight: [level][fade]
//   0x88 | ch      fill: three colour bytes (not modelled)
//   0x90           animation status of [channel]: 12 bytes (their meaning is not established;
//                  zeros, which the game accepts)
//   0x98 | ch      by length: [flags] / [0][frame, u32] (show one frame) / [mode][first, u32]
//                  [last, u32][period, u16] (run) / the run's fields, then [loop first, u32]
//                  [loop last, u32][loops, u16] (run with a loop)
//   0xb8 | ch      stop
// A run steps one frame per period, the period in 1/1280 s (the game's table: 84 for 15 fps,
// 43 for 30), and stays on its last frame. How a loop's count is meant is not established: the
// model plays to the loop's end, repeats the loop as often as the count says (0: until the next
// command), then plays on to the last frame.
bool spike1_devices::insert_command(const uint8_t *data, uint32_t len, uint64_t now_ns, std::string &note)
{
	const uint8_t sub = data[0];
	insert_channel &c = m_insert[sub & 3];
	auto u32 = [&](uint32_t at) { return uint32_t(data[at] | (data[at + 1] << 8) | (data[at + 2] << 16) | (uint32_t(data[at + 3]) << 24)); };
	auto u16 = [&](uint32_t at) { return uint16_t(data[at] | (data[at + 1] << 8)); };
	if (sub == 0x00 || sub == 0x90) return true;
	if ((sub & 0xfc) == 0x80 && len >= 2) { m_insert_backlight = data[1]; return true; }
	if ((sub & 0xfc) == 0xb8) {
		if (c.running) { c.frame = insert_channel_frame(c, now_ns); c.running = false; }
		return true;
	}
	if ((sub & 0xfc) != 0x98) return false;
	SPIKE1_DEBUG_ONLY(if ((sub & 3) && first_time("insert channel")) note = "node bus: LCD insert channel " + std::to_string(sub & 3) + " used; the model shows channel 0";)
	switch (len) {
	case 2:
		c.flags = data[1];
		return true;
	case 6:
		c = insert_channel();
		c.shown = true;
		c.frame = u32(2);
		return true;
	case 12: case 22:
		c = insert_channel();
		c.shown = c.running = true;
		c.frame = u32(2);
		c.last = u32(6);
		c.period_ns = uint64_t(u16(10)) * 1000000000ull / 1280;
		c.start_ns = now_ns;
		if (len == 22) {
			c.loop = true;
			c.loop_first = u32(12);
			c.loop_last = u32(16);
			c.loops = u16(20);
		}
		return true;
	}
	return false;
}

uint32_t spike1_devices::insert_channel_frame(const insert_channel &c, uint64_t now_ns) const
{
	if (!c.running || !c.period_ns || now_ns < c.start_ns) return c.frame;
	uint64_t step = (now_ns - c.start_ns) / c.period_ns;
	if (c.loop && c.loop_first >= c.frame && c.loop_last >= c.loop_first) {
		const uint64_t to_loop_end = c.loop_last - c.frame + 1, span = c.loop_last - c.loop_first + 1;
		if (step >= to_loop_end) {
			const uint64_t into = step - to_loop_end;
			if (!c.loops || into < span * c.loops) return c.loop_first + uint32_t(into % span);
			step -= span * c.loops;
		}
	}
	return uint32_t(std::min<uint64_t>(c.frame + step, std::max(c.last, c.frame)));
}

int32_t spike1_devices::insert_frame_index(uint64_t now_ns) const
{
	const insert_channel &c = m_insert[0];
	if (!m_insert_frames || !c.shown) return -1;
	const uint32_t f = insert_channel_frame(c, now_ns);
	return f < m_insert_frames ? int32_t(f) : -1;
}

const uint16_t *spike1_devices::insert_pixels(uint64_t now_ns)
{
	const int32_t f = insert_frame_index(now_ns);
	if (f < 0) return nullptr;
	const uint8_t *p = m_insert_image.data() + m_insert_image[0] + size_t(f) * INSERT_WIDTH * INSERT_HEIGHT * 2;
	// The file's frames are 128 wide and 160 high in the panel's scan order: the picture with rows
	// and columns swapped (the Stern logo reads right only this way round)
	m_insert_rgb.resize(INSERT_WIDTH * INSERT_HEIGHT);
	for (uint32_t y = 0; y < INSERT_HEIGHT; y++)
		for (uint32_t x = 0; x < INSERT_WIDTH; x++) {
			const uint8_t *px = p + 2 * (x * INSERT_HEIGHT + y);
			const uint16_t bgr = uint16_t((px[0] << 8) | px[1]); // red and blue swap places for RGB565
			m_insert_rgb[y * INSERT_WIDTH + x] = uint16_t(((bgr & 0x1f) << 11) | (bgr & 0x07e0) | (bgr >> 11));
		}
	return m_insert_rgb.data();
}

// ---------------------------------------------------------------- the CPU board's LCD
//
// WWE's FB_Init, FB_SetTiming and lcd_display_begin/end_frame: the game reads the fixed and
// variable screen info, maps smem_len bytes, sets its timing (FBIOPUT_VSCREENINFO), then for each
// frame draws into the buffer not shown, pans to it (yoffset 0 or 240), and waits out the rest of
// the frame's vblanks - counted from Stern's 0x80204612 (32 bytes, the vblank count at +4) and
// waited with FBIO_WAITFORVSYNC, whose argument is the count to wait
namespace {
constexpr uint32_t FB_LINE_BYTES = 320 * 2, FB_BUFFERS = 2;
constexpr uint32_t FBIOGET_VSCREENINFO = 0x4600, FBIOPUT_VSCREENINFO = 0x4601, FBIOGET_FSCREENINFO = 0x4602;
constexpr uint32_t FBIOPAN_DISPLAY = 0x4606, FBIO_WAITFORVSYNC = 0x40044620, FB_STERN_VBLANK = 0x80204612;
}

void spike1_devices::mapped(const handle &h, uint32_t addr, uint32_t len)
{
	if (h.type != kind::fb) return;
	m_fb_addr = addr;
	m_fb_len = len;
	SPIKE1_LOG("lcd: the game maps the framebuffer at " + hex(addr) + " (" + std::to_string(len) + " bytes)");
}

// WWE picks its LCD's attract video when its effect lists are evaluated at start-up, which happens
// while its node bus start-up (a power cycle of the boards and their checks, some 5 s) still holds
// video effects back (node_bus_control_thread_flags bits 0 and 3), and nothing evaluates them again
// in attract mode. On the board the program's slower loading puts that first evaluation after the
// start-up; here loading takes no time. So once the start-up is over, the model asks for the video
// background again the way the game does itself when a video ends
// (video_effect_update_background_flag) - once, from the LCD thread's vblank waits
void spike1_devices::lcd_video_background()
{
	if (m_fb_background_asked) return;
	const uint32_t flags_at = m_cfg.symbol ? m_cfg.symbol("node_bus_control_thread_flags") : 0;
	const uint32_t ask_at = m_cfg.symbol ? m_cfg.symbol("_ZL35video_effect_update_background_flag") : 0;
	uint32_t flags = 0;
	if (!flags_at || !ask_at || !guest_read(flags_at, &flags, 4)) { m_fb_background_asked = true; return; }
	if (flags & 9) { m_fb_startup_seen = true; return; }
	if (!m_fb_startup_seen) return;
	const uint8_t ask = 1;
	m_fb_background_asked = guest_write(ask_at, &ask, 1);
	SPIKE1_LOG("lcd: node bus start-up over - the game picks its video background again");
}

int32_t spike1_devices::fb_ioctl(handle &h, uint32_t req, uint32_t arg, uint64_t now_ns, uint64_t &wait_ns, std::string &note)
{
	if (req == FBIO_WAITFORVSYNC && !h.wait_until_ns) lcd_video_background();
	switch (req) {
	case FBIOGET_VSCREENINFO: { // struct fb_var_screeninfo, 160 bytes
		uint32_t var[40] = {};
		var[0] = LCD_WIDTH; var[1] = LCD_HEIGHT;              // xres, yres
		var[2] = LCD_WIDTH; var[3] = LCD_HEIGHT * FB_BUFFERS; // xres_virtual, yres_virtual
		var[5] = m_fb_yoffset;
		var[6] = 16;                                          // bits per pixel: RGB565
		var[8] = 11; var[9] = 5; var[11] = 5; var[12] = 6; var[14] = 0; var[15] = 5; // red, green, blue: offset, length
		return guest_write(arg, var, sizeof(var)) ? 0 : -E_FAULT;
	}
	case FBIOPUT_VSCREENINFO: { // the game's timing: the frame time from the pixel clock (ps) and the margins
		uint32_t var[40] = {};
		if (!guest_read(arg, var, sizeof(var))) return -E_FAULT;
		const uint64_t htotal = uint64_t(var[0]) + var[26] + var[27] + var[30], vtotal = uint64_t(var[1]) + var[28] + var[29] + var[31];
		if (var[25] && htotal && vtotal) {
			m_fb_frame_ns = std::max<uint64_t>(uint64_t(var[25]) * htotal * vtotal / 1000, 1000000);
			SPIKE1_DEBUG_ONLY(note = "lcd: " + std::to_string(var[0]) + 'x' + std::to_string(var[1]) + " at " + std::to_string(1e9 / double(m_fb_frame_ns)) + " Hz";)
		}
		return 0;
	}
	case FBIOGET_FSCREENINFO: { // struct fb_fix_screeninfo, 68 bytes
		uint8_t fix[68] = {};
		std::memcpy(fix, "spike1 lcd", 10);
		const uint32_t smem_len = FB_LINE_BYTES * LCD_HEIGHT * FB_BUFFERS, visual_truecolor = 2, line_length = FB_LINE_BYTES;
		const uint16_t ypanstep = 1;
		std::memcpy(fix + 20, &smem_len, 4);
		std::memcpy(fix + 32, &visual_truecolor, 4);
		std::memcpy(fix + 38, &ypanstep, 2);
		std::memcpy(fix + 44, &line_length, 4);
		return guest_write(arg, fix, sizeof(fix)) ? 0 : -E_FAULT;
	}
	case FBIOPAN_DISPLAY: {
		uint32_t yoffset = 0;
		if (!guest_read(arg + 20, &yoffset, 4)) return -E_FAULT;
		if (yoffset > LCD_HEIGHT * (FB_BUFFERS - 1)) return -E_INVAL;
		m_fb_yoffset = yoffset;
		m_fb_pans++;
		return 0;
	}
	case FB_STERN_VBLANK: {
		uint32_t info[8] = {};
		info[1] = uint32_t(now_ns / m_fb_frame_ns);
		return guest_write(arg, info, sizeof(info)) ? 0 : -E_FAULT;
	}
	case FBIO_WAITFORVSYNC: { // until the count of vblanks has passed
		if (h.wait_until_ns) {
			if (now_ns < h.wait_until_ns) { wait_ns = h.wait_until_ns; return WAIT; }
			h.wait_until_ns = 0;
			return 0;
		}
		uint32_t count = 0;
		if (!guest_read(arg, &count, 4)) return -E_FAULT;
		if (!count) return 0;
		h.wait_until_ns = (now_ns / m_fb_frame_ns + std::min<uint32_t>(count, 60)) * m_fb_frame_ns;
		wait_ns = h.wait_until_ns;
		return WAIT;
	}
	}
	SPIKE1_DEBUG_ONLY(if (first_time("ioctl " + h.path + ' ' + hex(req))) note = "ioctl " + h.path + " request " + hex(req) + " arg " + hex(arg) + ": not modelled";)
	return 0;
}

bool spike1_devices::lcd_picture(uint16_t *out) const
{
	const uint32_t bytes = FB_LINE_BYTES * LCD_HEIGHT;
	if (!m_fb_pans || !m_fb_addr || uint64_t(m_fb_yoffset) * FB_LINE_BYTES + bytes > m_fb_len) return false;
	const uint8_t *p = m_mem.host(m_fb_addr + m_fb_yoffset * FB_LINE_BYTES, bytes);
	if (!p) return false;
	std::memcpy(out, p, bytes); // RGB565, little endian like the host
	return true;
}

bool spike1_devices::needs_video_key()
{
	const uint32_t at = video_key_at();
	uint8_t placeholder[2] = {};
	return at && guest_read(at, placeholder, 2) && placeholder[0] == 0xcc && placeholder[1] == 0xcc;
}

bool spike1_devices::provide_video_key(const uint8_t key[16])
{
	const uint32_t at = video_key_at();
	if (!at) return false;
	m_video_key.assign(key, key + 16);
	if (!needs_video_key()) return true; // the game has its key already
	if (!guest_write(at, key, 16)) return false;
	SPIKE1_LOG("lcd: video key provided at " + hex(at));
	return true;
}

// ---------------------------------------------------------------- sound

int32_t spike1_devices::audio_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns)
{
	const uint64_t buffer_ns = uint64_t(AUDIO_BUFFER_FRAMES) * 1000000000ull / m_audio_rate;
	if (m_audio_until_ns > now_ns + buffer_ns) {
		wait_ns = m_audio_until_ns - buffer_ns;
		return WAIT;
	}
	const uint32_t frames = len / 4;
	const uint8_t *p = m_mem.host(buf, frames * 4, false);
	if (frames && !p) return -E_FAULT;
	int32_t wiper[2] = { POT_FULL_SCALE, POT_FULL_SCALE };
	for (const auto &pot : m_digipots)
		if (pot.slave == AUDIO_VOLUME_POT)
			for (int c = 0; c < 2; c++) wiper[c] = std::min<int32_t>(pot.reg[c], POT_FULL_SCALE);
	//m_audio.reserve(m_audio.size() + frames*2);
	for (uint32_t i = 0; i < frames * 2; i++)
		m_audio.push_back(int16_t(int32_t(int16_t(p[2 * i] | (p[2 * i + 1] << 8))) * wiper[i & 1] / POT_FULL_SCALE));
	const size_t limit = size_t(m_audio_rate) * 2;
	if (m_audio.size() > limit) m_audio.erase(m_audio.begin(), m_audio.begin() + (m_audio.size() - limit));
	m_audio_until_ns = std::max(m_audio_until_ns, now_ns) + uint64_t(frames) * 1000000000ull / m_audio_rate;
	return int32_t(len);
}

size_t spike1_devices::audio_take(int16_t *out, size_t frames)
{
	const size_t n = std::min(frames, m_audio.size() / 2);
	std::copy(m_audio.begin(), m_audio.begin() + n * 2, out);
	m_audio.erase(m_audio.begin(), m_audio.begin() + n * 2);
	return n;
}

// ---------------------------------------------------------------- the manual's numbers
//
// The device table numbers coils its own way; the service menu and the manual use the Driver
// Reference and Light Reference numbers, which the title's coil and LED tables hold:
//   node_board_device_cl_table_data   entries of 56 bytes (sys_nbd_cl_get_table_ptr()): +32 the
//                                     device's index in the low half (Heavy Metal's SDK keeps flags
//                                     in the high half), +48 bits 16-23 its Driver Reference number;
//                                     in WWE's SDK 28 bytes: +20 the index in the low half, +24
//                                     bits 16-23 the number
//   node_board_device_led_table       {entries, count, size}, 24-byte entries: +12 the Light
//                                     Reference number in the low half, the device's index in the
//                                     high half; +16 the class bits
//   node_board_device_sw_table        {entries, count, size}: 20 bytes before an entry's end the
//                                     Switch Reference number in the low half, the device's index in
//                                     the high half (+32 of 52 bytes in Ghostbusters, +44 of 64 in
//                                     Whoa Nellie, whose entries carry three more pointers); in
//                                     WWE's (64 bytes) +40 the index, +60 the number in the low half;
//                                     in Spider-Man's (68 bytes) the number and index as the first
//                                     form but 24 bytes before the end, at +44
// The manual numbers the CPU board's own switches (DIP switches, service buttons, the door's power
// sense) apart, as C1-C16: they become 101-116 here, so that every switch has one number
// Entry 0 of each is a blank. An entry that names no device of the right type is skipped
void spike1_devices::apply_manual_numbers(const std::vector<std::pair<int, size_t>> &device)
{
	unsigned coils = 0, leds = 0;
	const uint32_t cl = m_cfg.symbol("node_board_device_cl_table_data");
	const uint32_t cl_count_at = m_cfg.symbol("NODE_BOARD_DEVICE_CL_TABLE_DATA_ENTRY_COUNT");
	uint32_t cl_count = 0;
	if (cl && cl_count_at && guest_read(cl_count_at, &cl_count, 4) && cl_count) {
		const uint32_t size = m_cfg.symbol_size ? m_cfg.symbol_size("node_board_device_cl_table_data") : 0;
		const uint32_t stride = size / cl_count;
		// where an entry keeps the device's index and the Driver Reference number (bits 16-23)
		const uint32_t index_at = stride >= 52 ? 32 : stride == 28 ? 20 : 0, number_at = stride >= 52 ? 48 : 24;
		for (uint32_t i = 1; index_at && i < cl_count; i++) {
			uint32_t index = 0, number = 0;
			if (!guest_read(cl + i * stride + index_at, &index, 4) || !guest_read(cl + i * stride + number_at, &number, 4)) continue;
			index &= 0xffff;
			if (index >= device.size() || device[index].first != 2) continue;
			m_coils[device[index].second].number = uint16_t((number >> 16) & 0xff);
			coils++;
		}
	}
	uint32_t lt[3] = {};
	const uint32_t lt_at = m_cfg.symbol("node_board_device_led_table");
	if (lt_at && guest_read(lt_at, lt, 12) && lt[0] && lt[2] >= 20) {
		for (uint32_t i = 1; i < lt[1]; i++) {
			uint32_t w[2] = {};
			if (!guest_read(lt[0] + i * lt[2] + 12, w, 8)) continue;
			const uint32_t index = w[0] >> 16;
			if (index >= device.size() || device[index].first != 4) continue;
			output_info &led = m_leds[device[index].second];
			led.number = uint16_t(w[0] & 0xffff);
			led.kind = uint16_t(w[1] & 0xffff);
			leds++;
		}
	}
	unsigned switches = 0;
	uint32_t st[3] = {};
	const uint32_t st_at = m_cfg.symbol("node_board_device_sw_table");
	if (st_at && guest_read(st_at, st, 12) && st[0] && st[2] >= 36 && st[2] <= 256) {
		// Three layouts: index << 16 | number 20 bytes before an entry's end; or (WWE's SDK) the
		// index 24 bytes before the end and the number in the last word's low half; or (Spider-Man's)
		// index << 16 | number 24 bytes before the end. The first that names switch devices is the
		// title's
		for (int layout = 0; layout < 3 && !switches; layout++)
			for (uint32_t i = 1; i < st[1]; i++) {
				const uint32_t entry = st[0] + i * st[2];
				uint32_t index = 0, number = 0;
				if (layout != 1) {
					uint32_t w = 0;
					if (!guest_read(entry + st[2] - (layout == 0 ? 20 : 24), &w, 4)) continue;
					index = w >> 16;
					number = w & 0xffff;
				} else {
					if (!guest_read(entry + st[2] - 24, &index, 4) || !guest_read(entry + st[2] - 4, &number, 4)) continue;
					number &= 0xffff;
				}
				if (index >= device.size() || device[index].first != 7 || device[index].second >= m_switches.size()) continue;
				switch_info &sw = m_switches[device[index].second];
				sw.number = uint16_t(number + (sw.node == 0 ? CPU_SWITCH_BASE : 0));
				switches++;
			}
	}
	// An optional topper's board (the game names its boards lang_text_nb_<node>) numbers its
	// switches from 1 again, apart from the manual: they become 121 and up, clear of the playfield's
	std::set<uint8_t> toppers;
	for (const switch_info &sw : m_switches) {
		const uint32_t at = m_cfg.symbol("lang_text_nb_" + std::to_string(sw.node));
		uint32_t text = 0;
		std::string name;
		if (at && guest_read(at, &text, 4) && text) m_mem.read_string(text, name, 16);
		if (name.compare(0, 6, "TOPPER") == 0) toppers.insert(sw.node);
	}
	for (switch_info &sw : m_switches)
		if (toppers.count(sw.node)) sw.number = uint16_t(sw.number + TOPPER_SWITCH_BASE);
	SPIKE1_LOG("devices: the manual's numbers for " + std::to_string(switches) + " of " + std::to_string(m_switches.size()) + " switches, " +
		std::to_string(coils) + " of " + std::to_string(m_coils.size()) + " coils and " +
		std::to_string(leds) + " of " + std::to_string(m_leds.size()) + " LED channels");
}

bool spike1_devices::motor_at(size_t i, int16_t &position, bool &moving, uint64_t now_ns)
{
	if (i < m_motors.size()) {
		motors_run(now_ns);
		auto it = m_motors.begin();
		std::advance(it, i);
		position = it->second.position;
		moving = it->second.status & 0x01;
		return true;
	}
	i -= m_motors.size();
	if (i < m_steppers.size()) {
		auto it = m_steppers.begin();
		std::advance(it, i);
		position = int16_t(stepper_position(it->second, now_ns));
		moving = it->second.end_ns > now_ns;
		return true;
	}
	i -= m_steppers.size();
	if (i >= m_limit_motors.size()) return false;
	const limit_motor &m = m_limit_motors[i];
	const int64_t at = limit_motor_position(m, now_ns);
	position = int16_t(at * 100 / m.travel_ns);
	moving = (m.drive > 0 && at < m.travel_ns) || (m.drive < 0 && at > 0);
	return true;
}
