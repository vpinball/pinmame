// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the CPU board's devices. See spike1_devices.h

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

uint16_t adc_sample(uint32_t phase)
{
	if (phase >= ADC_LOW_SAMPLES) return ADC_HIGH;
	const double pi = 3.14159265358979323846;
	return uint16_t(ADC_THRESHOLD - std::lround(ADC_VALLEY * std::sin(pi * (phase + 0.5) / ADC_LOW_SAMPLES)));
}

// A board counts time in ticks at the rate its GetVersion reports; the game converts the times
// it sends (coil pulses, motor timeouts) from ms with it: ticks = ms * rate / 1000, at most 0xffff
constexpr uint32_t NODE_TICK_HZ = 1000;

// Reflex switch bytes in a coil configuration: position | used [| inverted]
constexpr uint8_t REFLEX_USED = 0x40, REFLEX_INVERT = 0x80, REFLEX_POSITION = 0x3f;

// Motors: how long a home takes, and a move per encoder count. The game polls until the board
// says the move is done, so these only set the pace of the mechanism, not whether it works
constexpr uint64_t MOTOR_HOME_NS = 1500000000;
constexpr uint64_t MOTOR_NS_PER_COUNT = 2000000;

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
// (PinMAME's docs/dmd.md, Stern Spike 1)
constexpr uint64_t DMD_FRAME_NS = 15750000;

std::string hex(uint32_t v) { char b[16]; std::snprintf(b, sizeof(b), "0x%x", v); return b; }

std::string bytes_hex(const uint8_t *p, uint32_t n, uint32_t limit = 16)
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
			log("node bus: bridge firmware " + name);
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
	std::string chip_list;
	for (const auto &chip : chips) {
		uint32_t part = 0;
		for (const auto &p : nxp_parts) if (chip == p.chip) part = p.part;
		m_chip_part.push_back(part);
		chip_list += " " + chip;
	}
	log("node bus: node chips" + chip_list);
	load_device_table();
	m_motors.clear();
	for (auto &node : m_coil) for (auto &c : node) c = coil();
	std::memset(m_coil_mask, 0, sizeof(m_coil_mask));
	std::memset(m_led, 0, sizeof(m_led));
	std::memset(m_led_mask, 0, sizeof(m_led_mask));
	// An LCD insert (a node board with its own display) holds the title's image already; the game
	// identifies an image by the word at offset 8 of lcdinsert.bin. The file: [header size, u8]
	// [frames, u16][width, u16][height, u16][bits per pixel, u8][ID, u32][file name], then the
	// frames, 16 bits a pixel, big endian, blue in the top five bits (BGR565)
	m_lcd_image_id = 0;
	m_insert_frames = 0;
	m_insert_image.clear();
	m_insert_backlight = 0;
	for (auto &c : m_insert) c = insert_channel();
	if (m_cfg.read_game_file) m_cfg.read_game_file("lcdinsert.bin", m_insert_image);
	const uint8_t *head = m_insert_image.data();
	if (m_insert_image.size() >= 32) {
		std::memcpy(&m_lcd_image_id, head + 8, 4);
		const uint32_t frames = head[1] | (head[2] << 8), width = head[3] | (head[4] << 8), height = head[5] | (head[6] << 8);
		if (width == INSERT_HEIGHT && height == INSERT_WIDTH && head[7] == 16 &&
		    head[0] + uint64_t(frames) * INSERT_WIDTH * INSERT_HEIGHT * 2 <= m_insert_image.size())
			m_insert_frames = frames;
		log("node bus: LCD insert image " + hex(m_lcd_image_id) + ", " + std::to_string(frames) + " frames of " +
			std::to_string(width) + "x" + std::to_string(height) + (m_insert_frames ? "" : " (not a layout the model shows)"));
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
	m_digipots.clear();
	for (uint16_t slave = 0x28; slave <= 0x2a; slave++) {
		digipot pot;
		pot.slave = slave;
		pot.reg[0] = pot.reg[1] = POT_FULL_SCALE / 2; // wipers at mid scale after power-up
		pot.reg[4] = 0x1ff;             // TCON: all terminals connected
		m_digipots.push_back(pot);
	}
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
		if (m_cfg.trace || r < 0) note = "i2c " + hex(h.i2c_slave) + ":" + trace;
		return r;
	}
	if (h.type == kind::adc)
		return adc_read(buf, len, now_ns, wait_ns);
	if (h.type == kind::node_bus) {
		const int32_t r = node_bus_read(buf, len);
		if (r == 0 && m_cfg.trace) note = "node bus: no reply waiting";
		return r;
	}
	if (first_time("read " + h.path)) note = "read " + std::to_string(len) + " from " + h.path + ": not modelled";
	return 0;
}

bool spike1_devices::readable(const handle &h, uint64_t now_ns) const
{
	if (h.type == kind::adc) return m_adc_next_ns && now_ns >= m_adc_next_ns;
	if (h.type == kind::node_bus) return !m_nb_reply.empty();
	return false;
}

int32_t spike1_devices::write(handle &h, uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns, std::string &note)
{
	if (h.type == kind::dmd_spi && len == DMD_FRAME_BYTES)
		return dmd_write(buf, len, now_ns, wait_ns);
	if (h.type == kind::node_bus)
		return node_bus_write(buf, len, now_ns, note);
	if (h.type == kind::i2s)
		return audio_write(buf, len, now_ns, wait_ns);
	if (h.type == kind::i2c) { // a plain write to the selected slave
		uint8_t *p = m_mem.host(buf, len, false);
		if (len && !p) return -E_FAULT;
		std::string trace;
		const int32_t r = i2c_transfer(h.i2c_slave, false, p, len, trace);
		if (m_cfg.trace || r < 0) note = "i2c " + hex(h.i2c_slave) + ":" + trace;
		return r;
	}
	const uint8_t *p = m_mem.host(buf, len, false);
	if (len && !p) return -E_FAULT;
	if (first_time("write " + h.path + " " + std::to_string(len))) note = "write " + std::to_string(len) + " to " + h.path + ": " + bytes_hex(p, len) + " (not modelled)";
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
	if (h.type == kind::dmd && (req == 0x3d01 || req == 0x3d02))
		return 0; // the display thread brackets every frame write with these
	if (h.type == kind::i2s && req == 0x3e00) { // sample rate: [code, u32]
		uint32_t code = 0;
		if (!guest_read(arg, &code, 4)) return -E_FAULT;
		if (code >= sizeof(AUDIO_RATES) / sizeof(AUDIO_RATES[0])) return -E_INVAL;
		m_audio_rate = AUDIO_RATES[code];
		note = "i2s: " + std::to_string(m_audio_rate) + " Hz";
		return 0;
	}
	if (h.type == kind::i2s && req == 0x3e01)
		return 0; // follows every rate change with -1; nothing to model
	if (h.type == kind::amp && req == 0x3f00)
		return 0; // the amplifier's register set-up; the volume comes through the digipots
	if (h.type == kind::cpu_spi && (req & 0xc000ffff) == 0x40006b00) // SPI_IOC_MESSAGE(n)
		return cpu_spi_message(req, arg, note);
	if (h.type == kind::cpu_spi && (req == 0x40046b04 || req == 0x40016b01 || req == 0x40016b03)) // speed, mode, bits per word
		return 0;
	if (h.type == kind::node_bus) {
		switch (req) {
		case 0x540b: // TCFLSH: the game drops stale input before every transfer
			if (arg == 0 || arg == 2) m_nb_reply.clear();
			return 0;
		case 0x5401: case 0x5402: case 0x5403: case 0x5404: // TCGETS, TCSETS[W|F]
		case 0x540a: case 0x541e: case 0x541f:              // TCXONC, TIOCGSERIAL, TIOCSSERIAL
			return 0;
		}
	}
	if (h.type == kind::i2c) {
		switch (req) {
		case I2C_SLAVE: case I2C_SLAVE_FORCE:
			h.i2c_slave = uint16_t(arg);
			if (m_cfg.trace) note = "i2c slave " + hex(arg);
			return 0;
		case I2C_RDWR:
			return i2c_rdwr(h, arg, note);
		default:
			if (first_time("i2c ioctl " + hex(req))) note = "i2c ioctl " + hex(req) + " not modelled";
			return -E_INVAL;
		}
	}
	if (first_time("ioctl " + h.path + " " + hex(req))) note = "ioctl " + h.path + " request " + hex(req) + " arg " + hex(arg) + ": not modelled";
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
		if (i == 0) trace = " " + hex(addr) + ":";
		const int32_t r = i2c_transfer(addr, read, p, len, trace);
		if (r < 0) { note = "i2c" + trace; return r; }
	}
	if (m_cfg.trace) note = "i2c" + trace;
	return int32_t(hdr[1]);
}

// One message to one slave. Returns the bytes moved, or -ENXIO when no device answers
int32_t spike1_devices::i2c_transfer(uint16_t slave, bool read, uint8_t *p, uint32_t len, std::string &trace)
{
	if (eeprom *e = find_eeprom(slave)) {
		const uint32_t size = uint32_t(e->data.size());
		if (read) {
			for (uint32_t k = 0; k < len; k++) p[k] = e->data[(e->pointer + k) % size];
			trace += " rd@" + hex(e->pointer) + "[" + std::to_string(len) + "]=" + bytes_hex(p, len, 8);
			e->pointer = (e->pointer + len) % size;
		} else if (len >= 2) {
			e->pointer = ((uint32_t(p[0]) << 8) | p[1]) % size;
			if (len > 2) {
				trace += " wr@" + hex(e->pointer) + "[" + std::to_string(len - 2) + "]=" + bytes_hex(p + 2, len - 2, 8);
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
			trace += " pot rd reg " + std::to_string(pot.pointer) + "=" + hex(v);
			return int32_t(len);
		}
		for (uint32_t k = 0; k + 1 < len; k += 2) {
			const uint8_t address = p[k] >> 4, command = (p[k] >> 2) & 3;
			pot.pointer = address;
			if (command == 0) pot.reg[address] = uint16_t(((p[k] & 3) << 8) | p[k + 1]); // write data
			trace += " pot reg " + std::to_string(address) + " cmd " + std::to_string(command) + " " + hex(((p[k] & 3) << 8) | p[k + 1]);
		}
		return int32_t(len);
	}
	trace += " no device answers";
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

// ---------------------------------------------------------------- node bus
//
// What the game's own transfer code sends and expects (NODEBUS_TransferMessage):
//   poll:   the single byte 00; the reply is one byte, the next board in the poll sequence, 0 = end
//   frame:  [0x80|node][n][cmd][data, n-2 bytes][checksum][reply length], the checksum making
//           addr + n + cmd + data + checksum = 0 mod 256
//   reply:  [data, reply length bytes][checksum][status], data + checksum = 0 mod 256, status
//           bits 2 and 3 clear when the board accepted the frame

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
	std::string list;
	for (uint8_t n : m_nb_nodes) list += " " + std::to_string(n);
	log("node bus: boards" + list);
	return m_nb_nodes;
}

void spike1_devices::node_bus_reply(const uint8_t *data, uint32_t len, uint8_t status)
{
	uint8_t sum = 0;
	for (uint32_t i = 0; i < len; i++) { m_nb_reply.push_back(data[i]); sum += data[i]; }
	m_nb_reply.push_back(uint8_t(0 - sum));
	m_nb_reply.push_back(status);
}

int32_t spike1_devices::node_bus_read(uint32_t buf, uint32_t len)
{
	const uint32_t n = std::min<uint32_t>(len, uint32_t(m_nb_reply.size()));
	if (!n) return 0;
	uint8_t *p = m_mem.host(buf, n);
	if (!p) return -E_FAULT;
	for (uint32_t i = 0; i < n; i++) { p[i] = m_nb_reply.front(); m_nb_reply.pop_front(); }
	return int32_t(n);
}

int32_t spike1_devices::node_bus_write(uint32_t buf, uint32_t len, uint64_t now_ns, std::string &note)
{
	const uint8_t *p = m_mem.host(buf, len, false);
	if (len && !p) return -E_FAULT;
	if (len == 1 && p[0] == 0) { // poll
		const auto &nodes = node_bus_nodes();
		const uint8_t next = m_nb_poll < nodes.size() ? nodes[m_nb_poll++] : 0;
		if (!next) m_nb_poll = 0;
		m_nb_reply.push_back(next);
		if (m_cfg.trace) note = "node bus poll -> " + std::to_string(next);
		return 1;
	}
	if (len >= 5 && (p[0] & 0x80) && uint32_t(p[1]) + 3 == len)
		node_bus_frame(p, len, now_ns, note);
	else if (len >= 2 && !(p[0] & 0x80) && uint32_t(p[1]) + 2 == len)
		node_bus_bridge(p, len, note);
	else
		note = "node bus: unframed write " + bytes_hex(p, len);
	return int32_t(len);
}

// The board block array: sys_node_board_get_next_block_ptr(node) is base + node * stride, the base
// loaded from its literal pool (ldr r1, [pc, #imm]) and the stride an immediate - both differ
// between SDK versions, so read them from the function itself. Newer SDKs load the stride
// (mov r3, #imm); older ones (Whoa Nellie) compute the address with shifts and step the scan loop
// by it (add r3, r3, #imm)
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
			} else if (((insn & 0xfffff000) == 0xe3a03000 || (insn & 0xffffff00) == 0xe2833000) && (insn & 0xff) >= 100) {
				m_block_stride = insn & 0xff;
			}
		}
	}
	const uint32_t head = m_cfg.symbol ? m_cfg.symbol("node_board_runtime_hex_image_list_head") : 0;
	if (!m_block_base || !m_block_stride || !head) return img;
	guest_read(m_block_base + node * m_block_stride + 88, &img.board_type, 4);
	if (!img.board_type) return img;
	// image records: [0] board type, [4] proc key, [12] content, [16] offset, [24] checksum, [28] next;
	// the version sits at content + offset + 9
	uint32_t rec = 0;
	guest_read(head, &rec, 4);
	for (int guard = 0; rec && guard < 64; guard++) {
		uint32_t type = 0;
		guest_read(rec, &type, 4);
		if (type == img.board_type) {
			uint32_t key = 0, content = 0, offset = 0;
			guest_read(rec + 4, &key, 4);
			guest_read(rec + 12, &content, 4);
			guest_read(rec + 16, &offset, 4);
			guest_read(rec + 24, &img.checksum, 2);
			if (content) guest_read(content + offset + 9, img.version, 3);
			img.part = key >= 1 && key <= m_chip_part.size() ? m_chip_part[key - 1] : 0;
			img.found = true;
			return img;
		}
		if (!guest_read(rec + 28, &rec, 4)) break;
	}
	return img;
}

void spike1_devices::node_bus_frame(const uint8_t *frame, uint32_t len, uint64_t now_ns, std::string &note)
{
	uint8_t sum = 0;
	for (uint32_t i = 0; i < len - 1; i++) sum += frame[i];
	const uint8_t node = frame[0] & 0x7f, cmd = frame[2], reply_len = frame[len - 1];
	if (sum) { note = "node bus: bad checksum " + bytes_hex(frame, len); return; }
	const uint32_t data_len = reply_len >= 2 ? reply_len - 2u : 0u;
	m_nb_received[node]++;
	m_nb_counts[(uint32_t(cmd) << 8) | node]++;
	if (m_frame_observer) m_frame_observer(now_ns, frame, len);
	const uint8_t *data = frame + 3;     // the command's own bytes, between cmd and checksum
	const uint32_t n = len - 5;
	if (cmd >= 0x80 && cmd <= 0xbf) {    // LED update
		if (m_led_form < 0) m_led_form = m_cfg.symbol && m_cfg.symbol("_Z14NODEBUS_SetLEDhhhh") ? 1 : 0;
		if (m_led_form ? led_update_run(node, cmd, data, n) : led_update(node, cmd, data, n)) return;
		const uint32_t key = (uint32_t(node) << 8) | cmd;
		if (m_nb_logged.insert(key).second) note = "node bus: node " + std::to_string(node) + " LED update not understood: " + bytes_hex(frame, len, 32);
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
		s.start_ns = now_ns;
		s.end_ns = now_ns + uint64_t(s.distance) * std::max<uint8_t>(data[3], 1) * 1000000ull;
		if (m_cfg.trace) note = "node bus: node " + std::to_string(node) + " stepper " + std::to_string(data[0]) + " to " + std::to_string(s.target) + (s.backward ? " backward" : "");
		return;
	}
	case 0x34: { // home - the stepper sits where the reply length goes: to step 0, forward
		stepper &s = m_steppers[(uint32_t(node) << 8) | (reply_len & 7)];
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
	// Motors (the game's EncoderMotor): status bit 3 = at rest and ready (homed after a home),
	// bit 0 = moving, bit 6 = fault
	case 0x51: case 0x57: // configure, configure update: [motor][configuration]
		if (len > 4) m_motors[(uint32_t(node) << 8) | frame[3]];
		return;
	case 0x53: case 0x54: { // home, away: [motor][...][timeout in board ticks, u16]
		if (len <= 4) break;
		motor &m = m_motors[(uint32_t(node) << 8) | frame[3]];
		const int16_t to = cmd == 0x53 ? 0 : m.position;
		motor_move(m, to, now_ns, MOTOR_HOME_NS);
		if (cmd == 0x53) m.homed = true;
		if (m_cfg.trace) note = "node bus: node " + std::to_string(node) + " motor " + std::to_string(frame[3]) + (cmd == 0x53 ? " home" : " away");
		return;
	}
	case 0x55: { // go: [motor][target, s16][speed][...]
		if (len <= 6) break;
		motor &m = m_motors[(uint32_t(node) << 8) | frame[3]];
		const int16_t target = int16_t(frame[4] | (frame[5] << 8));
		const int16_t now_at = motor_position(m, now_ns);
		motor_move(m, target, now_ns, uint64_t(std::abs(int(target) - int(now_at))) * MOTOR_NS_PER_COUNT);
		if (m_cfg.trace) note = "node bus: node " + std::to_string(node) + " motor " + std::to_string(frame[3]) + " go " + std::to_string(target);
		return;
	}
	case 0x56: { // stop: [motor]
		if (len <= 4) break;
		motor &m = m_motors[(uint32_t(node) << 8) | frame[3]];
		m.position = motor_position(m, now_ns);
		m.end_ns = 0;
		return;
	}
	case 0x52: { // status: [motor] -> [position, u16][status, u16]
		if (len <= 4 || data_len != 4) break;
		const motor &m = m_motors[(uint32_t(node) << 8) | frame[3]];
		const bool moving = m.end_ns > now_ns;
		const uint16_t pos = uint16_t(motor_position(m, now_ns)), status = moving ? 0x01 : 0x08;
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
			if (m_cfg.trace || !img.found) note = "node bus: node " + std::to_string(node) + " GetChecksum -> " + hex(img.checksum) + (img.found ? "" : " (no image)");
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
	case 0x11: { // GetInputState: eight switch bytes (position p = byte p/8, bit p%8), then a u16 the game ignores
		uint8_t r[10];
		stepper_home_switches(node, now_ns);
		wire_bytes(node, r, 8);
		r[8] = r[9] = 0;
		if (data_len == sizeof(r)) { node_bus_reply(r, sizeof(r)); return; }
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
			if (m_cfg.trace || !img.found)
				note = "node bus: node " + std::to_string(node) + " GetVersion -> " + (img.found ? "" : "no image yet, ") +
					std::to_string(img.version[0]) + "." + std::to_string(img.version[1]) + "." + std::to_string(img.version[2]) +
					" part " + hex(img.part) + " board type " + hex(img.board_type);
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
	const uint32_t key = (uint32_t(node) << 8) | cmd;
	if (!m_nb_logged.count(key)) {
		m_nb_logged.insert(key);
		note = "node bus: node " + std::to_string(node) + " command " + hex(cmd) +
			(data_len ? " (a query, answered with " + std::to_string(data_len) + " zeros)" : "") + " not modelled: " + bytes_hex(frame, len, 32);
	}
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
		if (m_cfg.trace) note = "node bus: bridge command 0x6 " + bytes_hex(msg, len);
		return;
	default:
		if (m_nb_logged.insert(0xff00 | cmd).second)
			note = "node bus: bridge command " + hex(cmd) + " not modelled: " + bytes_hex(msg, len);
		return;
	}
	if (m_cfg.trace) note = "node bus: bridge command " + hex(cmd);
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
// The numbers there are the game's own; apply_manual_numbers() gives switches, coils and LED channels
// the manual's
void spike1_devices::load_device_table()
{
	m_switches.clear();
	m_coils.clear();
	m_leds.clear();
	std::memset(m_sw_closed, 0, sizeof(m_sw_closed));
	std::memset(m_sw_active_high, 0, sizeof(m_sw_active_high));
	if (!m_cfg.symbol) return;
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

	std::vector<std::pair<switch_info, uint16_t>> found; // with their switch-table flags
	std::vector<std::pair<int, size_t>> device(devices[1], { 0, 0 }); // device index -> (type, index in m_coils/m_leds)
	for (uint32_t i = 0; i < devices[1]; i++) {
		uint32_t e[6] = {};
		if (!guest_read(devices[0] + i * devices[2], e, 24)) continue;
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
		uint16_t flags = 0;
		if (have_table && s.number < table[1]) guest_read(table[0] + s.number * table[2] + 0x10, &flags, 2);
		else if (have_dedicated && s.number >= 129 && s.number - 129u < dedicated[1]) guest_read(dedicated[0] + (s.number - 129u) * dedicated[2] + 0x10, &flags, 2);
		s.active_high = (flags & 4) != 0;
		if (s.active_high) m_sw_active_high[s.node][(s.position >> 3) & 7] |= uint8_t(1 << (s.position & 7));
		device[i] = { 7, found.size() };
		found.push_back({ s, flags });
	}

	// What is closed while the machine stands idle with its coin door shut: a trough opto for every
	// installed ball, the firmware's own closed-at-rest marks on other switches, and the door's
	// power sense (DC present)
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
	for (auto &f : found) m_switches.push_back(f.first);
	apply_manual_numbers(device);
	std::string closed;
	for (const auto &s : m_switches) if (switch_closed(s.node, s.position)) closed += " " + s.name + ",";
	log("devices: " + std::to_string(m_coils.size()) + " coils, " + std::to_string(m_leds.size()) + " LED channels, " +
		std::to_string(m_switches.size()) + " switches from the game's tables, " + std::to_string(installed) +
		" balls installed; closed at rest:" + (closed.empty() ? " none" : closed.substr(0, closed.size() - 1)));
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
	const uint8_t bit = uint8_t(1 << (position & 7));
	b = closed ? uint8_t(b | bit) : uint8_t(b & ~bit);
	run_reflexes(node & 127, now_ns);
}

// Switches are active low on the wire unless the title marks them active high
void spike1_devices::wire_bytes(uint8_t node, uint8_t *out, uint32_t len) const
{
	for (uint32_t i = 0; i < len; i++)
		out[i] = i < 8 ? uint8_t(~(m_sw_closed[node & 127][i] ^ m_sw_active_high[node & 127][i])) : 0xff;
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
	if (m_cfg.trace) note = "cpu spi: " + std::to_string(count) + " transfer(s)";
	return int32_t(total); // spidev answers with the bytes moved
}

// ---------------------------------------------------------------- motors

void spike1_devices::motor_move(motor &m, int16_t target, uint64_t now_ns, uint64_t duration_ns)
{
	m.from = motor_position(m, now_ns);
	m.position = m.target = target;
	m.start_ns = now_ns;
	m.end_ns = now_ns + duration_ns;
}

int16_t spike1_devices::motor_position(const motor &m, uint64_t now_ns) const
{
	if (now_ns >= m.end_ns || m.end_ns <= m.start_ns) return m.target;
	const double f = double(now_ns - m.start_ns) / double(m.end_ns - m.start_ns);
	return int16_t(m.from + std::lround((m.target - m.from) * f));
}

uint16_t spike1_devices::stepper_position(const stepper &s, uint64_t now_ns) const
{
	if (now_ns >= s.end_ns || s.end_ns <= s.start_ns) return s.target;
	const uint32_t done = uint32_t(uint64_t(s.distance) * (now_ns - s.start_ns) / (s.end_ns - s.start_ns));
	return uint16_t((s.backward ? s.from + s.steps - done % s.steps : s.from + done) % s.steps);
}

// A reel's home switch is closed while the reel stands within two steps of step 0
void spike1_devices::stepper_home_switches(uint8_t node, uint64_t now_ns)
{
	for (auto it = m_steppers.lower_bound(uint32_t(node) << 8); it != m_steppers.end() && (it->first >> 8) == node; ++it) {
		const stepper &s = it->second;
		if (s.home_input < 0) continue;
		const uint16_t at = stepper_position(s, now_ns);
		const bool home = at < 2 || at + 2 > s.steps;
		if (switch_closed(node, uint8_t(s.home_input)) != home) set_switch(node, uint8_t(s.home_input), home, now_ns);
	}
}

bool spike1_devices::motor_state(uint8_t node, uint8_t index, int16_t &position, bool &moving, uint64_t now_ns) const
{
	auto it = m_motors.find((uint32_t(node) << 8) | index);
	if (it == m_motors.end()) return false;
	position = motor_position(it->second, now_ns);
	moving = it->second.end_ns > now_ns;
	return true;
}

// ---------------------------------------------------------------- coils
//
// From the game's NODEBUS_Coil and NODEBUS_CoilConfig: times are board ticks (see NODE_TICK_HZ),
// powers a PWM duty out of 255.
//   fire     [coil][power 1][time 1, u16][power 2][time 2, u16][ramp, u16]
//            ([u16][switch condition, 3] follow on some fires - not modelled yet)
//   reflex   the same seven, five u16 times, eight switch bytes, four bytes, three bytes. The
//            board fires the coil when the switch in byte 0 closes; byte 3 is the end-of-stroke
//            switch, and the second time keeps the coil from firing again too soon
// What the traffic shows: flippers pulse at full power, then hold at power 2 while the button
// stays closed; pops and slings pulse once per closure; one-shot coils pulse at power 1 for time 1

static uint32_t ticks_ms(const uint8_t *p) { return uint32_t(p[0] | (p[1] << 8)) * 1000u / NODE_TICK_HZ; }

void spike1_devices::coil_fire(uint8_t node, const uint8_t *data, uint32_t len, uint64_t now_ns, std::string &note)
{
	if (len < 9 || data[0] >= COILS_PER_NODE) { note = "node bus: node " + std::to_string(node) + " coil fire not understood: " + bytes_hex(data, len); return; }
	coil &c = m_coil[node][data[0]];
	c.power1 = data[1];
	c.power2 = data[4];
	c.phase1_end_ns = now_ns + uint64_t(ticks_ms(data + 2)) * 1000000;
	c.phase2_end_ns = c.phase1_end_ns + uint64_t(ticks_ms(data + 5)) * 1000000;
	c.holding = false;
	if (len > 9 && std::any_of(data + 9, data + len, [](uint8_t b) { return b != 0; }) && first_time("coil fire extras"))
		note = "node bus: node " + std::to_string(node) + " coil fire with a time and switch condition (not modelled): " + bytes_hex(data, len);
	else if (m_cfg.trace)
		note = "node bus: node " + std::to_string(node) + " coil " + std::to_string(data[0]) + " fire";
}

void spike1_devices::coil_reflex(uint8_t node, const uint8_t *data, uint32_t len, std::string &note)
{
	if (len < 27 || data[0] >= COILS_PER_NODE) { note = "node bus: node " + std::to_string(node) + " coil reflex not understood: " + bytes_hex(data, len); return; }
	coil &c = m_coil[node][data[0]];
	c.trigger = data[19];
	c.eos = data[22];
	c.reflex = (c.trigger & REFLEX_USED) != 0;
	c.reflex_power1 = data[1];
	c.reflex_power2 = data[4];
	c.reflex_pulse_ms = ticks_ms(data + 2);
	c.reflex_holdoff_ms = ticks_ms(data + 11);
	c.trigger_was_active = c.reflex && reflex_switch_active(node, c.trigger);
	if (!c.reflex) c.holding = false;
	if (m_cfg.trace) note = "node bus: node " + std::to_string(node) + " coil " + std::to_string(data[0]) + (c.reflex ? " reflex on switch " + std::to_string(c.trigger & REFLEX_POSITION) : " reflex off");
}

bool spike1_devices::reflex_switch_active(uint8_t node, uint8_t sw) const
{
	return (sw & REFLEX_USED) && (switch_closed(node, sw & REFLEX_POSITION) != ((sw & REFLEX_INVERT) != 0));
}

void spike1_devices::run_reflexes(uint8_t node, uint64_t now_ns)
{
	for (coil &c : m_coil[node]) {
		if (!c.reflex) continue;
		const bool active = reflex_switch_active(node, c.trigger);
		if (active && !c.trigger_was_active && now_ns >= c.holdoff_end_ns) {
			c.power1 = c.reflex_power1;
			c.power2 = c.reflex_power2;
			c.phase1_end_ns = now_ns + uint64_t(c.reflex_pulse_ms) * 1000000;
			c.phase2_end_ns = 0;
			c.holding = c.reflex_power2 != 0;
			c.holdoff_end_ns = c.phase1_end_ns + uint64_t(c.reflex_holdoff_ms) * 1000000;
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
// The time is the channel's fade time; its unit is not established, so levels change at once
bool spike1_devices::led_update(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len)
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
	// times: read past them, so the length check below still proves the decoding
	uint8_t t;
	if (!times) { for (uint32_t i = 0; i < count; i++) if (!next(t)) return false; }
	else {
		if ((times & 0x10) && !next(t)) return false;
		if ((times & 0x0c) == 0x08 && !next(t)) return false;
		if ((times & 0x0c) == 0x0c)
			for (uint32_t i = 0; i < count; i++) if (((select[i >> 3] >> (i & 7)) & 1) && !next(t)) return false;
	}
	if (p != len) return false;
	for (uint32_t i = 0; i < count; i++)
		if (index[i] < LED_CHANNELS) m_led[node][index[i]] = level[i];
	return true;
}

// The older SDK (Whoa Nellie: NODEBUS_SetLED with an 8-bit channel, NODEBUS_SetLEDMultiple and
// NODEBUS_SetLEDMultipleTime) sets a run of channels, at most up to channel 63:
//   command   0x80 | the first channel
//   [time][level, one per channel]          one fade time for all (0xff never: the game sends 0xfe)
//   [0xff][time][level] for each channel    a fade time each
bool spike1_devices::led_update_run(uint8_t node, uint8_t cmd, const uint8_t *data, uint32_t len)
{
	const uint32_t first = cmd & 0x7f;
	if (len < 2) return false;
	const bool pairs = data[0] == 0xff;
	if (pairs && (len - 1) % 2) return false;
	const uint32_t count = pairs ? (len - 1) / 2 : len - 1;
	if (first + count > 64 || first + count > LED_CHANNELS) return false;
	for (uint32_t i = 0; i < count; i++) m_led[node][first + i] = pairs ? data[2 + 2 * i] : data[1 + i];
	return true;
}

uint8_t spike1_devices::led_level(uint8_t node, uint8_t position) const
{
	if (node > 127 || position >= LED_CHANNELS || (m_led_mask[node][position >> 3] >> (position & 7)) & 1) return 0;
	return m_led[node][position];
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
	if ((sub & 3) && first_time("insert channel")) note = "node bus: LCD insert channel " + std::to_string(sub & 3) + " used; the model shows channel 0";
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
		c.period_ns = uint64_t(u16(10)) * 1000000000 / 1280;
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

// ---------------------------------------------------------------- sound

int32_t spike1_devices::audio_write(uint32_t buf, uint32_t len, uint64_t now_ns, uint64_t &wait_ns)
{
	const uint64_t buffer_ns = uint64_t(AUDIO_BUFFER_FRAMES) * 1000000000 / m_audio_rate;
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
	for (uint32_t i = 0; i < frames * 2; i++)
		m_audio.push_back(int16_t(int32_t(int16_t(p[2 * i] | (p[2 * i + 1] << 8))) * wiper[i & 1] / POT_FULL_SCALE));
	const size_t limit = size_t(m_audio_rate) * 2;
	if (m_audio.size() > limit) m_audio.erase(m_audio.begin(), m_audio.begin() + (m_audio.size() - limit));
	m_audio_until_ns = std::max(m_audio_until_ns, now_ns) + uint64_t(frames) * 1000000000 / m_audio_rate;
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
//                                     device's index, +48 bits 16-23 its Driver Reference number
//   node_board_device_led_table       {entries, count, size}, 24-byte entries: +12 the Light
//                                     Reference number in the low half, the device's index in the
//                                     high half; +16 the class bits
//   node_board_device_sw_table        {entries, count, size}: 20 bytes before an entry's end the
//                                     Switch Reference number in the low half, the device's index in
//                                     the high half (+32 of 52 bytes in Ghostbusters, +44 of 64 in
//                                     Whoa Nellie, whose entries carry three more pointers)
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
		for (uint32_t i = 1; stride >= 52 && i < cl_count; i++) {
			uint32_t index = 0, number = 0;
			if (!guest_read(cl + i * stride + 32, &index, 4) || !guest_read(cl + i * stride + 48, &number, 4)) continue;
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
		for (uint32_t i = 1; i < st[1]; i++) {
			uint32_t w = 0;
			if (!guest_read(st[0] + i * st[2] + st[2] - 20, &w, 4)) continue;
			const uint32_t index = w >> 16;
			if (index >= device.size() || device[index].first != 7 || device[index].second >= m_switches.size()) continue;
			switch_info &sw = m_switches[device[index].second];
			sw.number = uint16_t((w & 0xffff) + (sw.node == 0 ? CPU_SWITCH_BASE : 0));
			switches++;
		}
	}
	log("devices: the manual's numbers for " + std::to_string(switches) + " of " + std::to_string(m_switches.size()) + " switches, " +
		std::to_string(coils) + " of " + std::to_string(m_coils.size()) + " coils and " +
		std::to_string(leds) + " of " + std::to_string(m_leds.size()) + " LED channels");
}

bool spike1_devices::motor_at(size_t i, int16_t &position, bool &moving, uint64_t now_ns) const
{
	if (i < m_motors.size()) {
		auto it = m_motors.begin();
		std::advance(it, i);
		position = motor_position(it->second, now_ns);
		moving = it->second.end_ns > now_ns;
		return true;
	}
	i -= m_motors.size();
	if (i >= m_steppers.size()) return false;
	auto it = m_steppers.begin();
	std::advance(it, i);
	position = int16_t(stepper_position(it->second, now_ns));
	moving = it->second.end_ns > now_ns;
	return true;
}
