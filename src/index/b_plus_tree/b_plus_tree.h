#pragma once

#include "b_plus_tree_page.h"

#include "../../storage/buffer_manager.h"

#include <cstdint>
#include <optional>
#include <vector>

using Key = int64_t;
using Value = int64_t;

class LeafPage;
class InternalPage;

class BPlusTree {
public:
    explicit BPlusTree(BufferManager& buffer_manager);

    bool insert(Key key, Value value);

    std::optional<Value> get(Key key);

    bool remove(Key key);

    bool contains(Key key);

    bool empty() const;

private:
    BufferManager& buffer_manager_;

    PageId root_page_id_ = INVALID_PAGE_ID;

private:

    PageId findLeaf(
        Key key,
        std::vector<PageId>& path
    );

    void insertIntoLeaf(
        LeafPage& leaf,
        Key key,
        Value value
    );

    void splitLeaf(
        LeafPage& leaf,
        std::vector<PageId>& path
    );

    void splitInternal(
        InternalPage& internal,
        std::vector<PageId>& path
    );

    void createNewRoot(
        PageId left,
        PageId right,
        Key separator
    );
};
