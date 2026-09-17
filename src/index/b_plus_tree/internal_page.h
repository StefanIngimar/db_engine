#pragma once

#include "b_plus_tree_page.h"

#include <cstdint>
#include <vector>

using Key = int64_t;

class InternalPage : public BPlusTreePage {
public:
    explicit InternalPage(PageId id);

    PageId findChild(Key key) const;

    void insertChild(
        Key key,
        PageId child
    );

    bool isFull() const;

    std::size_t size() const;

    const std::vector<Key>& keys() const;

    const std::vector<PageId>& children() const;

    std::vector<Key>& keys();

    std::vector<PageId>& children();

private:
    static constexpr std::size_t MAX_KEYS = 4;

    std::vector<Key> keys_;

    std::vector<PageId> children_;
};
