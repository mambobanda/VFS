#pragma once
#include <vector>
#include <string>
#include <span>
#include <fstream>
#include <type_traits>
#include <cstring>
#include <filesystem>
#include <any>

#include "vfs_vfs.hpp"

namespace vfs::util {

// Convert a trivially-copyable value to vector<std::byte>
template<typename T>
std::vector<std::byte> to_bytes_trivial(const T& value) {
	static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
	std::vector<std::byte> out(sizeof(T));
	std::memcpy(out.data(), &value, sizeof(T));
	return out;
}

// Deserialize a trivially-copyable value from a span of bytes.
template<typename T>
T from_bytes_trivial(std::span<const std::byte> s) {
	static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
	if (s.size() != sizeof(T)) throw std::runtime_error("from_bytes_trivial: size mismatch");
	T t;
	std::memcpy(&t, s.data(), sizeof(T));
	return t;
}

inline std::vector<std::byte> string_to_bytes(const std::string& s) {
	std::vector<std::byte> v(s.size());
	if (!s.empty()) std::memcpy(v.data(), s.data(), s.size());
	return v;
}

inline std::string bytes_to_string(std::span<const std::byte> s) {
	return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}

inline std::vector<std::byte> read_file_bytes(const std::filesystem::path& p) {
	std::ifstream in(p, std::ios::binary);
	if (!in) throw std::runtime_error("read_file_bytes: cannot open file");
	in.seekg(0, std::ios::end);
	auto size = static_cast<std::size_t>(in.tellg());
	in.seekg(0, std::ios::beg);
	std::vector<std::byte> buf(size);
	if (size > 0) in.read(reinterpret_cast<char*>(buf.data()), size);
	if (!in && size > 0) throw std::runtime_error("read_file_bytes: read error");
	return buf;
}

inline void write_file_bytes(const std::filesystem::path& p, std::span<const std::byte> data) {
	std::ofstream out(p, std::ios::binary);
	if (!out) throw std::runtime_error("write_file_bytes: cannot open file");
	if (!data.empty()) out.write(reinterpret_cast<const char*>(data.data()), data.size());
	if (!out && !data.empty()) throw std::runtime_error("write_file_bytes: write error");
}

inline vfs::File* create_memory_file_from_string(vfs::VirtualFileSystem& vfs,
												 const vfs::Path& dir,
												 const std::string& name,
												 const std::string& content) {
	auto* f = vfs.create_memory_file(dir, name);
	if (!f) return nullptr;
	if (auto* mf = dynamic_cast<vfs::MemoryFile*>(f)) {
		mf->from_bytes(std::span<const std::byte>(reinterpret_cast<const std::byte*>(content.data()), content.size()));
	}
	return f;
}

inline vfs::File* create_memory_file_from_bytes(vfs::VirtualFileSystem& vfs,
												const vfs::Path& dir,
												const std::string& name,
												std::span<const std::byte> bytes) {
	auto* f = vfs.create_memory_file(dir, name);
	if (!f) return nullptr;
	if (auto* mf = dynamic_cast<vfs::MemoryFile*>(f)) {
		mf->from_bytes(bytes);
	}
	return f;
}

template<typename T>
inline void set_dictionary_entry(vfs::DictionaryFile* df, const std::string& key, T value, const std::string& typeName) {
	if (!df) throw std::invalid_argument("set_dictionary_entry: df is null");
	df->set(key, std::any(std::move(value)), typeName);
}

class SimpleByteXObject : public vfs::XObject {
public:
	explicit SimpleByteXObject(std::vector<std::byte> bytes) : _bytes(std::move(bytes)) {}
	std::vector<std::byte> getByteStream() const override { return _bytes; }
	void initializeObject(std::span<const std::byte> data) override { _bytes.assign(data.begin(), data.end()); }
	std::unique_ptr<vfs::XObject> clone() const override { return std::make_unique<SimpleByteXObject>(_bytes); }
private:
	std::vector<std::byte> _bytes;
};

inline vfs::File* create_xobject_file_from_bytes(vfs::VirtualFileSystem& vfs,
												 const vfs::Path& dir,
												 const std::string& name,
												 std::span<const std::byte> bytes) {
	auto obj = std::make_unique<SimpleByteXObject>(std::vector<std::byte>(bytes.begin(), bytes.end()));
	return vfs.create_xobject_file(dir, name, std::move(obj));
}

} // namespace vfs::util
