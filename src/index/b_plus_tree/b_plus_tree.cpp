#include "b_plus_tree.h"

#include "internal_page.h"
#include "leaf_page.h"

#include <cassert>

BPlusTree::BPlusTree(
    BufferManager& buffer_manager
)
    : buffer_manager_(buffer_manager) {
}

bool BPlusTree::empty() const {
    return root_page_id_ == INVALID_PAGE_ID;
}

bool BPlusTree::insert(
    Key key,
    Value value
) {

    // Tree doesn't have a root yet.
    if (root_page_id_ == INVALID_PAGE_ID) {

        auto root =
            std::make_unique<LeafPage>(0);

        root->setRootPage(true);

        PageId root_id =
            buffer_manager_.newPage(
                std::move(root)
            );

        root_page_id_ = root_id;

        auto* root_page =
            static_cast<LeafPage*>(
                buffer_manager_.fetchPage(root_id)
            );

        return root_page->insert(key, value);
    }

    std::vector<PageId> path;

    PageId leaf_id =
        findLeaf(key, path);

    Page* page =
        buffer_manager_.fetchPage(leaf_id);

    auto* leaf =
        static_cast<LeafPage*>(page);

    bool inserted =
        leaf->insert(key, value);

    if (!inserted) {
        return false;
    }

    if (leaf->isFull()) {
        splitLeaf(*leaf, path);
    }

    return true;
}

std::optional<Value> BPlusTree::get(Key key) {

    if (empty()) {
        return std::nullopt;
    }

    std::vector<PageId> path;

    PageId leaf_id =
        findLeaf(key, path);

    Page* page =
        buffer_manager_.fetchPage(leaf_id);

    auto* leaf =
        static_cast<LeafPage*>(page);

    Value* value =
        leaf->get(key);

    if (value == nullptr) {
        return std::nullopt;
    }

    return *value;
}

bool BPlusTree::contains(Key key) {
    return get(key).has_value();
}

bool BPlusTree::remove(Key key) {

    if (empty()) {
        return false;
    }

    std::vector<PageId> path;

    PageId leaf_id =
        findLeaf(key, path);

    Page* page =
        buffer_manager_.fetchPage(leaf_id);

    auto* leaf =
        static_cast<LeafPage*>(page);

    return leaf->remove(key);
}

PageId BPlusTree::findLeaf(
    Key key,
    std::vector<PageId>& path
) {

    PageId current =
        root_page_id_;

    while (true) {

        Page* page =
            buffer_manager_.fetchPage(current);

        auto* btree_page =
            static_cast<BPlusTreePage*>(page);

        if (btree_page->isLeafPage()) {
            return current;
        }

        path.push_back(current);

        auto* internal =
            static_cast<InternalPage*>(page);

        current =
            internal->findChild(key);
    }
}

void BPlusTree::splitLeaf(
    LeafPage& leaf,
    std::vector<PageId>& path
) {
    // TODO: implement leaf splitting
}
