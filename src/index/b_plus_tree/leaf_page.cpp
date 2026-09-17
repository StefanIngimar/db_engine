#include "leaf_page.h"

#include <algorithm>

LeafPage::LeafPage(PageId id)
    : BPlusTreePage(id, BPlusTreePageType::LEAF) {
}

bool LeafPage::insert(Key key, Value value) {

    auto it = std::lower_bound(
        records_.begin(),
        records_.end(),
        key,
        [](const Record& record, Key key) {
            return record.key < key;
        }
    );

    if (it != records_.end() && it->key == key) {
        return false;
    }

    records_.insert(it, {key, value});

    return true;
}

Value* LeafPage::get(Key key) {

    auto it = std::lower_bound(
        records_.begin(),
        records_.end(),
        key,
        [](const Record& record, Key key) {
            return record.key < key;
        }
    );

    if (it == records_.end() || it->key != key) {
        return nullptr;
    }

    return &it->value;
}

bool LeafPage::remove(Key key) {

    auto it = std::lower_bound(
        records_.begin(),
        records_.end(),
        key,
        [](const Record& record, Key key) {
            return record.key < key;
        }
    );

    if (it == records_.end() || it->key != key) {
        return false;
    }

    records_.erase(it);

    return true;
}

bool LeafPage::isFull() const {
    return records_.size() > MAX_ENTRIES;
}

size_t LeafPage::size() const {
    return records_.size();
}

const std::vector<Record>& LeafPage::records() const {
    return records_;
}

std::vector<Record>& LeafPage::records() {
    return records_;
}

PageId LeafPage::nextPage() const {
    return next_page_id_;
}

void LeafPage::setNextPage(PageId page_id) {
    next_page_id_ = page_id;
}
