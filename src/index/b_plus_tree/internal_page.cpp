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
