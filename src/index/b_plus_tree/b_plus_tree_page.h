#pragma once

#include "../../storage/page.h"

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

private:
    BPlusTreePageType type_;

    bool is_root_ = false;
};
