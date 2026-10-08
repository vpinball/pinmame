// license:BSD-3-Clause

// PinMAME Spike 1 subsystem - the filesystem the game sees
//
// Two layers. The base is the title's own files, read-only: a host directory holding the machine's
// extracted root filesystem (the standalone harness), or files the host holds in memory (PinMAME's
// ROM regions). Above it, everything the game creates or writes - its NVRAM files - lives in
// memory and is saved and restored as one block, so the host keeps a machine's state the way it
// keeps any other game's NVRAM.

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

class spike1_vfs
{
public:
	enum class type { none, file, directory };
	struct entry { std::string name; bool directory; };

	// What an open file reads from: a base file in memory (data), a base file on the host
	// (host_path), or a file of the written layer (written)
	struct source
	{
		const uint8_t *data = nullptr;
		size_t size = 0;
		std::string host_path;
		std::shared_ptr<std::vector<uint8_t>> written;
	};

	// The base. Paths are absolute guest paths; memory added with add_file() stays the caller's
	// and must outlive the filesystem
	void set_root(const std::string &host_dir) { m_root = host_dir; }
	void add_file(const std::string &path, const uint8_t *data, size_t size);
	void add_directory(const std::string &path) { m_base_dirs.insert(path); }

	type lookup(const std::string &path, uint64_t *size = nullptr) const;
	std::vector<entry> list(const std::string &dir) const;
	bool open(const std::string &path, source &out) const;

	// The written layer. open_write() gives the file's bytes to change in place, copying a base file
	// up first unless `truncate`; nullptr when the path is a directory or its parent is missing
	std::shared_ptr<std::vector<uint8_t>> open_write(const std::string &path, bool truncate);
	bool make_directory(const std::string &path);
	bool remove(const std::string &path);

	// The written layer as one block: what the host saves as the machine's NVRAM. load() replaces
	// the layer and fails, leaving it empty, on a block it does not recognise
	std::vector<uint8_t> save() const;
	bool load(const uint8_t *data, size_t size);
	bool written_empty() const { return m_written.empty() && m_dirs.empty() && m_removed.empty(); }

private:
	std::string m_root;
	std::map<std::string, std::pair<const uint8_t *, size_t>> m_base;
	std::set<std::string> m_base_dirs;   // empty base directories
	std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> m_written;
	std::set<std::string> m_dirs;     // directories the game made
	std::set<std::string> m_removed;  // base files and directories the game deleted

	type base_lookup(const std::string &path, uint64_t *size) const;
	bool removed(const std::string &path) const;
};
