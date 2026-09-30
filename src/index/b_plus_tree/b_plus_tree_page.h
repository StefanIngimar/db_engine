#pragma once

#include "../../storage/page.h"
#include <cstdint>

enum class BPlusTreePageType {
    INTERNAL,
    LEAF
};

class BPlusTreePage : public Page {
public:
    BPlusTreePage(
        PageId id,
        BPlusTreePageType type
    )
        : Page(id),
          type_(type) {
    }

    BPlusTreePageType type() const {
        return type_;
    }

    bool isLeafPage() const {
        return type_ == BPlusTreePageType::LEAF;
    }

    bool isRootPage() const {
        return is_root_;
    }

    void setRootPage(bool is_root) {
        is_root_ = is_root;
    }

    static constexpr std::size_t kHeaderSize = 2;

    void serializeTo(uint8_t* buffer) const override{
        buffer[0] = static_cast<uint8_t>(type_);
        buffer[1] = is_root_ ? 1 : 0;
        serializePayload(buffer + kHeaderSize);
    }

    void deserializeFrom(const uint8_t* buffer) override{
        is_root_ = buffer[1] != 0;
        deserializePayload(buffer + kHeaderSize);
    }

protected:
    virtual void serializePayload(uint8_t* buffer) const = 0;
    virtual void deserializePayload(const uint8_t* buffer) = 0;
private:
    BPlusTreePageType type_;

    bool is_root_ = false;
};
