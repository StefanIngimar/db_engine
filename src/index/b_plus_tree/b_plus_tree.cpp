#include "b_plus_tree.h"

#include "b_plus_tree_page.h"
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

    auto* leaf =
        static_cast<LeafPage*>(buffer_manager_.fetchPage(leaf_id));

    int idx = -1;
    const auto& recs = leaf->records();
    for(std::size_t i = 0; i < recs.size(); ++i){
        if(recs[i].key == key){
            idx = static_cast<int>(i);
            break;
        }
    }
    if(idx < 0){
        return false;
    }

    leaf->removeAt(idx);
    fixUnderflow(path, leaf_id);
    return true;
}

void BPlusTree::fixUnderflow(std::vector<PageId>& path, PageId node_id) {

    auto* page =
        static_cast<BPlusTreePage*>(buffer_manager_.fetchPage(node_id));

    bool underflow = page->isLeafPage()
        ? static_cast<LeafPage*>(page)->isUnderflow()
        : static_cast<InternalPage*>(page)->isUnderflow();

    // path holds this node's ancestors only (see findLeaf), so an empty
    // path means node_id is the root. The root is allowed to be underfull;
    // it only collapses when an internal root is left with a single child.
    if (path.empty()) {
        if (!page->isLeafPage()) {
            auto* root = static_cast<InternalPage*>(page);
            if (root->children().size() == 1) {
                root_page_id_ = root->children()[0];
            }
        }
        return;
    }

    if (!underflow) {
        return;
    }

    PageId parent_id = path.back();
    auto* parent =
        static_cast<InternalPage*>(buffer_manager_.fetchPage(parent_id));

    auto& children = parent->children();
    int idx = -1;
    for (std::size_t i = 0; i < children.size(); ++i) {
        if (children[i] == node_id) {
            idx = static_cast<int>(i);
            break;
        }
    }
    assert(idx >= 0);

    bool has_left = idx > 0;
    PageId sibling_id = has_left ? children[idx - 1] : children[idx + 1];
    bool sibling_is_left = has_left;
    // keys()[separator_idx] is the key that sits between the left and right
    // node of this pair, regardless of which side is "sibling" vs "node".
    int separator_idx = sibling_is_left ? idx - 1 : idx;

    auto* sibling_page =
        static_cast<BPlusTreePage*>(buffer_manager_.fetchPage(sibling_id));

    bool sibling_can_lend = page->isLeafPage()
        ? static_cast<LeafPage*>(sibling_page)->canLend()
        : static_cast<InternalPage*>(sibling_page)->canLend();

    if (sibling_can_lend) {
        Key new_separator = page->isLeafPage()
            ? static_cast<LeafPage*>(page)->redistributeFrom(
                  static_cast<LeafPage*>(sibling_page), sibling_is_left)
            : static_cast<InternalPage*>(page)->redistributeFrom(
                  static_cast<InternalPage*>(sibling_page),
                  sibling_is_left,
                  parent->keys()[separator_idx]);

        parent->keys()[separator_idx] = new_separator;
        return;
    }

    PageId left_id = sibling_is_left ? sibling_id : node_id;
    PageId right_id = sibling_is_left ? node_id : sibling_id;
    auto* left = buffer_manager_.fetchPage(left_id);
    auto* right = buffer_manager_.fetchPage(right_id);

    if (page->isLeafPage()) {
        auto* left_leaf = static_cast<LeafPage*>(left);
        auto* right_leaf = static_cast<LeafPage*>(right);
        left_leaf->mergeFrom(right_leaf);
        left_leaf->setNextPage(right_leaf->nextPage());
    } else {
        static_cast<InternalPage*>(left)->mergeFrom(
            static_cast<InternalPage*>(right),
            parent->keys()[separator_idx]);
    }

    // Removing the separator must also drop the pointer to right_id from
    // parent->children() -- confirm InternalPage::removeAt does both.
    parent->removeAt(separator_idx);

    path.pop_back();
    fixUnderflow(path, parent_id);
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
