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
