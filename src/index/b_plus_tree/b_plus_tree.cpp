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
        splitLeaf(leaf_id, *leaf, path);
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
    PageId leaf_id,
    LeafPage& leaf,
    std::vector<PageId>& path
) {
    // leaf currently holds MAX_ENTRIES + 1 records -- isFull() only fires
    // after the insert that pushed it one over the limit.
    auto new_leaf_owner = std::make_unique<LeafPage>(0);
    PageId new_leaf_id = buffer_manager_.newPage(std::move(new_leaf_owner));
    auto* new_leaf =
        static_cast<LeafPage*>(buffer_manager_.fetchPage(new_leaf_id));

    auto& left_records = leaf.records();
    auto& right_records = new_leaf->records();

    std::size_t split_point = left_records.size() / 2;

    right_records.assign(
        left_records.begin() + static_cast<long>(split_point),
        left_records.end()
    );
    left_records.erase(
        left_records.begin() + static_cast<long>(split_point),
        left_records.end()
    );

    // Keep the leaf chain (used for range scans) intact.
    new_leaf->setNextPage(leaf.nextPage());
    leaf.setNextPage(new_leaf_id);

    // The smallest key that moved right becomes the separator: any search
    // key >= this value routes to the new leaf (see InternalPage::findChild).
    Key separator_key = right_records.front().key;

    if (path.empty()) {
        // leaf was the root it needs a new parent.
        createNewRoot(leaf_id, new_leaf_id, separator_key);
        return;
    }

    PageId parent_id = path.back();
    auto* parent =
        static_cast<InternalPage*>(buffer_manager_.fetchPage(parent_id));

    parent->insertChild(separator_key, new_leaf_id);

    if (parent->isFull()) {
        path.pop_back();
        splitInternal(parent_id, *parent, path);
    }
}

void BPlusTree::createNewRoot(
    PageId left,
    PageId right,
    Key separator
) {
    auto new_root_owner = std::make_unique<InternalPage>(0);
    PageId new_root_id = buffer_manager_.newPage(std::move(new_root_owner));
    auto* new_root =
        static_cast<InternalPage*>(buffer_manager_.fetchPage(new_root_id));

    new_root->setRootPage(true);
    new_root->keys().push_back(separator);
    new_root->children().push_back(left);
    new_root->children().push_back(right);

    auto* old_root =
        static_cast<BPlusTreePage*>(buffer_manager_.fetchPage(left));
    old_root->setRootPage(false);

    root_page_id_ = new_root_id;
}

void BPlusTree::splitInternal(
    PageId internal_id,
    InternalPage& internal,
    std::vector<PageId>& path
) {
    // internal currently holds MAX_KEYS + 1 keys / MAX_KEYS + 2 children --
    // isFull() only fires right after the insertChild that overflowed it.
    auto& keys = internal.keys();
    auto& children = internal.children();

    std::size_t mid = keys.size() / 2;
    Key promoted_key = keys[mid];

    auto new_internal_owner = std::make_unique<InternalPage>(0);
    PageId new_internal_id = buffer_manager_.newPage(std::move(new_internal_owner));
    auto* new_internal =
        static_cast<InternalPage*>(buffer_manager_.fetchPage(new_internal_id));

    // Everything right of the promoted key goes to the new node.
    new_internal->keys().assign(
        keys.begin() + static_cast<long>(mid) + 1,
        keys.end()
    );
    new_internal->children().assign(
        children.begin() + static_cast<long>(mid) + 1,
        children.end()
    );

    // The promoted key moves up it isn't copied, so it's dropped from
    // both sides here.
    keys.erase(keys.begin() + static_cast<long>(mid), keys.end());
    children.erase(children.begin() + static_cast<long>(mid) + 1, children.end());

    if (path.empty()) {
        // internal was the root it needs a new parent.
        createNewRoot(internal_id, new_internal_id, promoted_key);
        return;
    }

    PageId parent_id = path.back();
    auto* parent =
        static_cast<InternalPage*>(buffer_manager_.fetchPage(parent_id));

    parent->insertChild(promoted_key, new_internal_id);

    if (parent->isFull()) {
        path.pop_back();
        splitInternal(parent_id, *parent, path);
    }
}
