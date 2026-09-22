#include "internal_page.h"

#include <algorithm>

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

Key InternalPage::redistributeFrom(InternalPage *sibling, bool sibling_is_left, Key parent_separator_key){
    auto& sibling_keys = sibling->keys();
    auto& sibling_children = sibling->children();

    if(sibling_is_left){
        PageId borrowed_child = sibling_children.back();
        Key borowed_key = sibling_keys.back();
        sibling_children.pop_back();
        sibling_keys.pop_back();

        keys_.insert(keys_.begin(), borrowed_child);
        return borowed_key;
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
