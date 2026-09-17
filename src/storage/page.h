#pragma once

#include <cstdint>

using PageId = uint32_t;

constexpr PageId INVALID_PAGE_ID = UINT32_MAX;

class Page {
public:
    explicit Page(PageId id)
        : id_(id) {}

    virtual ~Page() = default;

    PageId id() const {
        return id_;
    }

private:
    PageId id_;
};
