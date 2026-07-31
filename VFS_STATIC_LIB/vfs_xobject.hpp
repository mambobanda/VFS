#pragma once
#include <vector>
#include <span>
#include <cstddef>
#include <memory>

namespace vfs {

class XObject {
public:
    virtual ~XObject() = default;

    virtual std::vector<std::byte> getByteStream() const = 0;
    virtual void initializeObject(std::span<const std::byte> data) = 0;

    // For XObjectFile copy constructor
    virtual std::unique_ptr<XObject> clone() const = 0;
};

} // namespace vfs

// XObject is an abstract interface used by XObjectFile to contain
// arbitrary user-defined serializable objects. The VFS only requires
// two operations: produce a byte stream (getByteStream) and initialize
// from a byte stream (initializeObject). clone() is used to support
// copying XObjectFile instances without exposing concrete types.

