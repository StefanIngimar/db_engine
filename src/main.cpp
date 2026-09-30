#include "storage/buffer_manager.h"
#include "storage/disk_manager.h"

#include "index/b_plus_tree/b_plus_tree.h"

#include <iomanip>
#include <iostream>

namespace {

void section(const std::string& title) {
    std::cout << "\n=== " << title << " ===\n";
}

void expect(bool condition, const std::string& description) {
    std::cout << (condition ? "[ok]   " : "[FAIL] ") << description << '\n';
}

}  // namespace

int main() {

    DiskManager disk_manager;
    BufferManager buffer_manager(disk_manager);
    BPlusTree tree(buffer_manager);

    // ------------------------------------------------------------------
    section("Basic insert / get / contains");
    // ------------------------------------------------------------------

    expect(tree.insert(10, 100), "insert(10, 100) succeeds");
    expect(tree.insert(20, 200), "insert(20, 200) succeeds");
    expect(tree.insert(30, 300), "insert(30, 300) succeeds");
    expect(tree.insert(40, 400), "insert(40, 400) succeeds");

    auto value = tree.get(20);
    expect(value.has_value() && *value == 200, "get(20) returns 200");

    expect(!tree.contains(50), "contains(50) is false (never inserted)");

    // ------------------------------------------------------------------
    section("Duplicate keys are rejected");
    // ------------------------------------------------------------------

    expect(!tree.insert(20, 999), "insert(20, 999) is rejected (duplicate key)");
    value = tree.get(20);
    expect(value.has_value() && *value == 200,
           "get(20) still returns the original 200, not 999");

    // ------------------------------------------------------------------
    section("Remove");
    // ------------------------------------------------------------------

    expect(tree.remove(30), "remove(30) succeeds");
    expect(!tree.contains(30), "contains(30) is false after removal");
    expect(!tree.remove(30), "remove(30) again fails (already gone)");
    expect(!tree.remove(999), "remove(999) fails (never existed)");

    expect(tree.insert(30, 333), "insert(30, 333) succeeds again after removal");
    value = tree.get(30);
    expect(value.has_value() && *value == 333,
           "get(30) returns the new value, not blocked as a duplicate");

    // ------------------------------------------------------------------
    section("Enough inserts to force leaf and internal splits");
    // ------------------------------------------------------------------
    // LeafPage::MAX_ENTRIES and InternalPage::MAX_KEYS are both 4, so this
    // comfortably exercises splitLeaf, createNewRoot, and splitInternal.

    constexpr Key kSplitTestCount = 100;
    bool all_split_inserts_ok = true;
    for (Key key = 1000; key < 1000 + kSplitTestCount; ++key) {
        if (!tree.insert(key, key * 2)) {
            all_split_inserts_ok = false;
        }
    }
    expect(all_split_inserts_ok, "all 100 ascending inserts succeeded");

    bool all_split_gets_ok = true;
    for (Key key = 1000; key < 1000 + kSplitTestCount; ++key) {
        auto v = tree.get(key);
        if (!v.has_value() || *v != key * 2) {
            all_split_gets_ok = false;
            break;
        }
    }
    expect(all_split_gets_ok, "all 100 keys are retrievable after the splits");

    // ------------------------------------------------------------------
    section("Enough removals to force redistribution and merges");
    // ------------------------------------------------------------------

    bool all_removes_ok = true;
    for (Key key = 1000; key < 1000 + kSplitTestCount; key += 2) {
        if (!tree.remove(key)) {
            all_removes_ok = false;
        }
    }
    expect(all_removes_ok, "removed every other key (50 removals) successfully");

    bool merge_check_ok = true;
    for (Key key = 1000; key < 1000 + kSplitTestCount; ++key) {
        bool should_exist = (key % 2 != 0);
        bool does_exist = tree.contains(key);
        if (should_exist != does_exist) {
            merge_check_ok = false;
            break;
        }
    }
    expect(merge_check_ok,
           "surviving keys are all still present, removed keys are all gone");

    // Original keys from earlier sections should be untouched by any of
    // the merge/redistribute activity above.
    expect(tree.contains(10) && tree.contains(20) && tree.contains(30) && tree.contains(40),
           "original keys (10/20/30/40) survived unrelated tree restructuring");

    // ------------------------------------------------------------------
    section("Flushing to disk");
    // ------------------------------------------------------------------

    buffer_manager.flushPage();
    std::cout << "flushPage() called -- every dirty page has now been "
                 "written via DiskManager::writePage.\n";
    std::cout << "Note: this does NOT mean the tree survives a restart yet. "
                 "root_page_id_ lives only in BPlusTree's memory and is "
                 "never written to disk anywhere, so a fresh process has no "
                 "way to find the root even though the pages themselves are "
                 "on disk. That's the next real gap to close (a small "
                 "metadata/superblock page reserved for it).\n";

    std::cout << "\nDone.\n";
    return 0;
}
