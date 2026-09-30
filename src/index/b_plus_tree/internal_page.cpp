#include "internal_page.h"

#include <algorithm>
#include <cstring>
#include <assert.h>

InternalPage::InternalPage(PageId id)
    : BPlusTreePage(
          id,
          BPlusTreePageType::INTERNAL
      ) {
}

PageId InternalPage::findChild(Key key) const {

    auto it = std::upper_bound(
        keys_.begin(),
        keys_.end(),
        key
    );

    size_t index =
        static_cast<size_t>(
            it - keys_.begin()
        );

    assert(children_.size() == keys_.size()+1);
    assert(index < children_.size());

    return children_[index];
}

void InternalPage::insertChild(
    Key key,
    PageId child
) {

    auto it = std::upper_bound(
        keys_.begin(),
        keys_.end(),
        key
    );

    size_t index =
        static_cast<size_t>(
            it - keys_.begin()
        );

    keys_.insert(
        keys_.begin() + index,
        key
    );

    children_.insert(
        children_.begin() + index + 1,
        child
    );
}

bool InternalPage::isFull() const {
    return keys_.size() > MAX_KEYS;
}

bool InternalPage::isUnderflow() const{
    std::size_t min_keys = (MAX_KEYS + 1) / 2;
    return keys_.size() < min_keys;
}

bool InternalPage::canLend() const{
    std::size_t min_keys = (MAX_KEYS + 1) / 2;
    return keys_.size() > min_keys;
}

void InternalPage::removeAt(int index){
    keys_.erase(keys_.begin()+index);
    children_.erase(children_.begin()+index+1);
}

Key InternalPage::redistributeFrom(InternalPage* sibling, bool sibling_is_left, Key parent_separator_key) {

    auto& sibling_keys = sibling->keys();
    auto& sibling_children = sibling->children();

    if (sibling_is_left) {
        PageId borrowed_child = sibling_children.back();
        Key borrowed_key = sibling_keys.back();

        sibling_children.pop_back();
        sibling_keys.pop_back();

        keys_.insert(keys_.begin(), parent_separator_key);
        children_.insert(children_.begin(), borrowed_child);
        return borrowed_key;
    }

    PageId borrowed_child = sibling_children.front();
    Key borrowed_key = sibling_keys.front();

    sibling_children.erase(sibling_children.begin());
    sibling_keys.erase(sibling_keys.begin());

    keys_.push_back(parent_separator_key);
    children_.push_back(borrowed_child);

    return borrowed_key;
}

void InternalPage::mergeFrom(InternalPage *sibling, Key parent_separator_key){
    keys_.push_back(parent_separator_key);
    keys_.insert(keys_.end(), sibling->keys().begin(), sibling->keys().end());
    children_.insert(children_.end(), sibling->children().begin(), sibling->children().end());

    sibling->keys().clear();
    sibling->children().clear();
}

void InternalPage::serializePayload(uint8_t* buffer) const {
    std::size_t offset = 0;

    uint32_t key_count = static_cast<uint32_t>(keys_.size());
    std::memcpy(buffer + offset, &key_count, sizeof(key_count));
    offset += sizeof(key_count);

    for (Key key : keys_) {
        std::memcpy(buffer + offset, &key, sizeof(Key));
        offset += sizeof(Key);
    }

    uint32_t child_count = static_cast<uint32_t>(children_.size());
    std::memcpy(buffer + offset, &child_count, sizeof(child_count));
    offset += sizeof(child_count);

    for (PageId child : children_) {
        std::memcpy(buffer + offset, &child, sizeof(PageId));
        offset += sizeof(PageId);
    }
}

void InternalPage::deserializePayload(const uint8_t* buffer) {
    std::size_t offset = 0;

    uint32_t key_count = 0;
    std::memcpy(&key_count, buffer + offset, sizeof(key_count));
    offset += sizeof(key_count);

    keys_.clear();
    keys_.reserve(key_count);
    for (uint32_t i = 0; i < key_count; ++i) {
        Key key = 0;
        std::memcpy(&key, buffer + offset, sizeof(Key));
        offset += sizeof(Key);
        keys_.push_back(key);
    }

    uint32_t child_count = 0;
    std::memcpy(&child_count, buffer + offset, sizeof(child_count));
    offset += sizeof(child_count);

    children_.clear();
    children_.reserve(child_count);
    for (uint32_t i = 0; i < child_count; ++i) {
        PageId child = 0;
        std::memcpy(&child, buffer + offset, sizeof(PageId));
        offset += sizeof(PageId);
        children_.push_back(child);
    }
}

size_t InternalPage::size() const {
    return keys_.size();
}

const std::vector<Key>& InternalPage::keys() const {
    return keys_;
}

const std::vector<PageId>& InternalPage::children() const {
    return children_;
}

std::vector<Key>& InternalPage::keys() {
    return keys_;
}

std::vector<PageId>& InternalPage::children() {
    return children_;
}
