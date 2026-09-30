#pragma once

#include <cstdint>
#include <cstddef>

using PageId = uint32_t;

constexpr PageId INVALID_PAGE_ID = UINT32_MAX;
constexpr std::size_t PAGE_SIZE = 4096;

class Page {
public:
    explicit Page(PageId id)
        : id_(id) {}

    virtual ~Page() = default;

    PageId id() const {
        return id_;
    }

    virtual void serializeTo(uint8_t* buffer) const{
        (void)buffer;
    }

    virtual void deserializeFrom(const uint8_t* buffer){
        (void)buffer;
    }

private:
    PageId id_;
};
