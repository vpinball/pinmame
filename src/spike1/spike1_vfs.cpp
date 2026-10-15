// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the filesystem the game sees. See spike1_vfs.h

#include "spike1_vfs.h"

#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

std::string parent_of(const std::string &path)
{
	const size_t slash = path.rfind('/');
	return slash == 0 || slash == std::string::npos ? "/" : path.substr(0, slash);
}

// The saved block: "S1NV", a format version, then records [kind][path length, u16][path]
// [data length, u32][data], little endian
constexpr char NV_MAGIC[4] = { 'S', '1', 'N', 'V' };
constexpr uint8_t NV_VERSION = 1;
enum : uint8_t { NV_FILE = 1, NV_DIRECTORY = 2, NV_REMOVED = 3 };

} // anonymous namespace

void spike1_vfs::add_file(const std::string &path, const uint8_t *data, size_t size)
{
	m_base[path] = { data, size };
}

bool spike1_vfs::removed(const std::string &path) const
{
	// a path is gone when it, or a directory above it, was deleted and not made again since
	for (std::string p = path; ; p = parent_of(p)) {
		if (m_removed.count(p)) return true;
		if (p == "/") return false;
	}
}

spike1_vfs::type spike1_vfs::base_lookup(const std::string &path, uint64_t *size) const
{
	if (path == "/" || m_base_dirs.count(path)) return type::directory;
	auto it = m_base.lower_bound(path);
	if (it != m_base.end() && it->first == path) {
		if (size) *size = it->second.second;
		return type::file;
	}
	if (it != m_base.end() && it->first.compare(0, path.size() + 1, path + "/") == 0) return type::directory;
	if (!m_root.empty()) {
		std::error_code ec;
		const fs::path h = fs::u8path(m_root + path);
		if (fs::is_directory(h, ec)) return type::directory;
		if (fs::is_regular_file(h, ec)) {
			if (size) *size = uint64_t(fs::file_size(h, ec));
			return type::file;
		}
	}
	return type::none;
}

spike1_vfs::type spike1_vfs::lookup(const std::string &path, uint64_t *size) const
{
	auto w = m_written.find(path);
	if (w != m_written.end()) {
		if (size) *size = w->second->size();
		return type::file;
	}
	if (m_dirs.count(path)) return type::directory;
	if (removed(path)) return type::none;
	return base_lookup(path, size);
}

std::vector<spike1_vfs::entry> spike1_vfs::list(const std::string &dir) const
{
	std::map<std::string, bool> names; // name -> directory
	const std::string prefix = dir == "/" ? "/" : dir + "/";
	auto child = [&](const std::string &path, bool directory) {
		if (path.compare(0, prefix.size(), prefix) != 0 || path.size() == prefix.size()) return;
		const size_t slash = path.find('/', prefix.size());
		const std::string name = path.substr(prefix.size(), slash == std::string::npos ? std::string::npos : slash - prefix.size());
		if (!removed(prefix + name) || m_written.count(prefix + name) || m_dirs.count(prefix + name))
			names[name] = names[name] || directory || slash != std::string::npos;
	};
	for (const auto &b : m_base) child(b.first, false);
	for (const auto &b : m_base_dirs) child(b, true);
	if (!m_root.empty()) {
		std::error_code ec;
		for (auto it = fs::directory_iterator(fs::u8path(m_root + dir), ec); !ec && it != fs::directory_iterator(); it.increment(ec))
			child(prefix + it->path().filename().u8string(), it->is_directory(ec));
	}
	for (const auto &w : m_written) child(w.first, false);
	for (const auto &d : m_dirs) child(d, true);
	std::vector<entry> out;
	for (const auto &n : names) out.push_back({ n.first, n.second });
	return out;
}

bool spike1_vfs::open(const std::string &path, source &out) const
{
	out = source();
	auto w = m_written.find(path);
	if (w != m_written.end()) { out.written = w->second; out.size = w->second->size(); return true; }
	if (m_dirs.count(path) || removed(path)) return false;
	auto b = m_base.find(path);
	if (b != m_base.end()) { out.data = b->second.first; out.size = b->second.second; return true; }
	uint64_t size = 0;
	if (m_root.empty() || base_lookup(path, &size) != type::file) return false;
	out.host_path = m_root + path;
	out.size = size_t(size);
	return true;
}

std::shared_ptr<std::vector<uint8_t>> spike1_vfs::open_write(const std::string &path, bool truncate)
{
	auto w = m_written.find(path);
	if (w != m_written.end()) {
		if (truncate) w->second->clear();
		return w->second;
	}
	if (lookup(parent_of(path)) != type::directory || lookup(path) == type::directory) return nullptr;
	auto bytes = std::make_shared<std::vector<uint8_t>>();
	source base;
	if (!truncate && open(path, base)) { // copy the base file up
		if (base.data) bytes->assign(base.data, base.data + base.size);
		else {
			std::ifstream in(fs::u8path(base.host_path), std::ios::binary);
			bytes->resize(base.size);
			in.read(reinterpret_cast<char *>(bytes->data()), std::streamsize(bytes->size()));
			bytes->resize(size_t(in.gcount()));
		}
	}
	m_written[path] = bytes;
	return bytes;
}

bool spike1_vfs::make_directory(const std::string &path)
{
	if (lookup(path) != type::none || lookup(parent_of(path)) != type::directory) return false;
	m_dirs.insert(path);
	return true;
}

bool spike1_vfs::remove(const std::string &path)
{
	const type t = lookup(path);
	if (t == type::none) return false;
	if (t == type::directory && !list(path).empty()) return false;
	m_written.erase(path);
	m_dirs.erase(path);
	if (base_lookup(path, nullptr) != type::none) m_removed.insert(path);
	return true;
}

std::vector<uint8_t> spike1_vfs::save() const
{
	std::vector<uint8_t> out(NV_MAGIC, NV_MAGIC + 4);
	out.push_back(NV_VERSION);
	auto record = [&](uint8_t kind, const std::string &path, const uint8_t *data, size_t size) {
		out.push_back(kind);
		out.push_back(uint8_t(path.size()));
		out.push_back(uint8_t(path.size() >> 8));
		out.insert(out.end(), path.begin(), path.end());
		for (int i = 0; i < 4; i++) out.push_back(uint8_t(uint32_t(size) >> (8 * i)));
		if (size) out.insert(out.end(), data, data + size);
	};
	for (const auto &d : m_dirs) record(NV_DIRECTORY, d, nullptr, 0); // sorted, so parents come first
	for (const auto &r : m_removed) record(NV_REMOVED, r, nullptr, 0);
	for (const auto &w : m_written) record(NV_FILE, w.first, w.second->data(), w.second->size());
	return out;
}

bool spike1_vfs::load(const uint8_t *data, size_t size)
{
	m_written.clear();
	m_dirs.clear();
	m_removed.clear();
	if (size < 5 || std::memcmp(data, NV_MAGIC, 4) != 0 || data[4] != NV_VERSION) return false;
	size_t p = 5;
	while (p < size) {
		if (size - p < 3) break;
		const uint8_t kind = data[p];
		const size_t path_len = data[p + 1] | (data[p + 2] << 8);
		p += 3;
		if (size - p < path_len + 4) break;
		const std::string path(reinterpret_cast<const char *>(data + p), path_len);
		p += path_len;
		const size_t len = data[p] | (data[p + 1] << 8) | (data[p + 2] << 16) | (size_t(data[p + 3]) << 24);
		p += 4;
		if (size - p < len || path.empty() || path[0] != '/') break;
		if (kind == NV_DIRECTORY) m_dirs.insert(path);
		else if (kind == NV_REMOVED) m_removed.insert(path);
		else if (kind == NV_FILE) m_written[path] = std::make_shared<std::vector<uint8_t>>(data + p, data + p + len);
		else break;
		p += len;
	}
	if (p == size) return true;
	m_written.clear();
	m_dirs.clear();
	m_removed.clear();
	return false;
}
