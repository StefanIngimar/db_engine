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

bool LeafPage::isUnderflow() const{
    std::size_t min_entries = (MAX_ENTRIES + 1) / 2;
    return records_.size() < min_entries;
}

bool LeafPage::canLend() const{
    std::size_t min_entries = (MAX_ENTRIES + 1) / 2;
    return records_.size() > min_entries;
}

void LeafPage::removeAt(int index){
    records_.erase(records_.begin() + index);
}

Key LeafPage::redistributeFrom(LeafPage* sibling, bool sibling_is_left) {

    auto& sibling_records = sibling->records();

    if (sibling_is_left) {
        // Move the sibling's largest record to the front of this node.
        Record borrowed = sibling_records.back();
        sibling_records.pop_back();
        records_.insert(records_.begin(), borrowed);

        // This node is now the "right" side of the pair; its new smallest
        // key is the separator between sibling (left) and this node.
        return records_.front().key;
    }

    // Move the sibling's smallest record to the end of this node.
    Record borrowed = sibling_records.front();
    sibling_records.erase(sibling_records.begin());
    records_.push_back(borrowed);

    // sibling is now the "right" side; its new smallest key is the
    // separator between this node (left) and sibling.
    return sibling_records.front().key;
}

void LeafPage::mergeFrom(LeafPage* sibling) {
    // Assumes sibling is the node to this one's right (see BPlusTree::
    // fixUnderflow, which always merges right into left).
    auto& sibling_records = sibling->records();
    records_.insert(records_.end(), sibling_records.begin(), sibling_records.end());
    sibling_records.clear();
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
