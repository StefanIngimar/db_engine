#pragma once

#include "b_plus_tree_page.h"

#include <cstdint>
#include <vector>

using Key = int64_t;
using Value = int64_t;

struct Record {
    Key key;
    Value value;
};

class LeafPage : public BPlusTreePage {
public:
    explicit LeafPage(PageId id);

    bool insert(Key key, Value value);

    Value* get(Key key);

    bool remove(Key key);

    bool isFull() const;

    std::size_t size() const;

    const std::vector<Record>& records() const;

    std::vector<Record>& records();

    PageId nextPage() const;

    void setNextPage(PageId page_id);

    void removeAt(int index);

    bool isUnderflow() const;

    bool canLend() const;

    Key redistributeFrom(LeafPage* sibling, bool sibling_is_left);

    void mergeFrom(LeafPage* sibling);

private:
    static constexpr std::size_t MAX_ENTRIES = 4;

    std::vector<Record> records_;

    PageId next_page_id_ = INVALID_PAGE_ID;
};
