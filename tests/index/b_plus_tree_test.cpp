#include <gtest/gtest.h>

#include "index/b_plus_tree/b_plus_tree.h"
#include "storage/buffer_manager.h"
#include "storage/disk_manager.h"

#include <algorithm>
#include <numeric>
#include <random>

class BPlusTreeTest : public ::testing::Test {
protected:
    DiskManager disk_manager;
    BufferManager buffer_manager{disk_manager};
    BPlusTree tree{buffer_manager};
};

TEST_F(BPlusTreeTest, DuplicateInsertionIsRejected) {
    EXPECT_TRUE(tree.insert(42, 100));

    EXPECT_FALSE(tree.insert(42, 999));

    std::optional<Value> value = tree.get(42);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 100);
}

TEST_F(BPlusTreeTest, RemoveDeletesExistingKey) {
    ASSERT_TRUE(tree.insert(7, 70));
    ASSERT_TRUE(tree.contains(7));

    EXPECT_TRUE(tree.remove(7));
    EXPECT_FALSE(tree.contains(7));
    EXPECT_FALSE(tree.get(7).has_value());
}

TEST_F(BPlusTreeTest, RemoveNonExistentKeyReturnsFalse) {
    EXPECT_FALSE(tree.remove(123));
}

TEST_F(BPlusTreeTest, EmptyTreeHasNoKeys) {
    EXPECT_TRUE(tree.empty());
    EXPECT_FALSE(tree.contains(1));
    EXPECT_FALSE(tree.get(1).has_value());
}

// ---------------------------------------------------------------------
// Insertion / splitting
//
// LeafPage::MAX_ENTRIES and InternalPage::MAX_KEYS are both 4, so a
// handful of keys is enough to force a leaf split, and ~30-40 is enough
// to force at least one internal split (and likely a multi-level tree).
// These are black-box tests: they don't assert on tree shape (no
// introspection API exists), just that every inserted key stays
// retrievable and duplicates stay rejected, which is what would break
// first if splitLeaf/splitInternal/createNewRoot had an off-by-one.
// ---------------------------------------------------------------------

TEST_F(BPlusTreeTest, InsertBeyondLeafCapacityTriggersSplit) {
    // MAX_ENTRIES = 4, so the 5th insert overflows the root leaf and must
    // go through splitLeaf -> createNewRoot.
    for (Key key = 0; key < 5; ++key) {
        ASSERT_TRUE(tree.insert(key, key * 10));
    }

    for (Key key = 0; key < 5; ++key) {
        std::optional<Value> value = tree.get(key);
        ASSERT_TRUE(value.has_value()) << "missing key " << key;
        EXPECT_EQ(*value, key * 10);
    }
}

TEST_F(BPlusTreeTest, AscendingInsertsForceInternalSplitAndStayRetrievable) {
    constexpr Key kCount = 40;

    for (Key key = 0; key < kCount; ++key) {
        ASSERT_TRUE(tree.insert(key, key));
    }

    for (Key key = 0; key < kCount; ++key) {
        std::optional<Value> value = tree.get(key);
        ASSERT_TRUE(value.has_value()) << "missing key " << key;
        EXPECT_EQ(*value, key);
    }

    // Keys never inserted must not be found.
    EXPECT_FALSE(tree.contains(-1));
    EXPECT_FALSE(tree.contains(kCount));
}

TEST_F(BPlusTreeTest, DescendingInsertsStayRetrievable) {
    // Ascending inserts always route to the rightmost leaf; descending
    // inserts exercise the opposite edge (always the leftmost leaf,
    // findChild's upper_bound at index 0).
    constexpr Key kCount = 40;

    for (Key key = kCount - 1; key >= 0; --key) {
        ASSERT_TRUE(tree.insert(key, key));
        if (key == 0) break;  // Key is signed; avoid wrapping past 0.
    }

    for (Key key = 0; key < kCount; ++key) {
        std::optional<Value> value = tree.get(key);
        ASSERT_TRUE(value.has_value()) << "missing key " << key;
        EXPECT_EQ(*value, key);
    }
}

TEST_F(BPlusTreeTest, ShuffledInsertsStayRetrievable) {
    // A fixed seed keeps this deterministic across runs while still
    // exercising splits/merges from unpredictable positions in the tree,
    // rather than only ever the leftmost/rightmost leaf.
    constexpr Key kCount = 50;

    std::vector<Key> keys(kCount);
    std::iota(keys.begin(), keys.end(), 0);
    std::mt19937 rng(1234);
    std::shuffle(keys.begin(), keys.end(), rng);

    for (Key key : keys) {
        ASSERT_TRUE(tree.insert(key, key * 2));
    }

    for (Key key = 0; key < kCount; ++key) {
        std::optional<Value> value = tree.get(key);
        ASSERT_TRUE(value.has_value()) << "missing key " << key;
        EXPECT_EQ(*value, key * 2);
    }
}

// ---------------------------------------------------------------------
// Deletion / redistribution / merging
// ---------------------------------------------------------------------

TEST_F(BPlusTreeTest, RemoveCausesLeafUnderflowKeepsOtherKeysIntact) {
    // min_entries for MAX_ENTRIES=4 is (4+1)/2 = 2, so dropping a leaf to
    // 1 entry triggers underflow handling (redistribute or merge)
    // immediately.
    for (Key key = 0; key < 10; ++key) {
        ASSERT_TRUE(tree.insert(key, key));
    }

    // Remove enough keys from the low end to force at least one leaf
    // through underflow -> redistribute/merge.
    ASSERT_TRUE(tree.remove(0));
    ASSERT_TRUE(tree.remove(1));
    ASSERT_TRUE(tree.remove(2));

    EXPECT_FALSE(tree.contains(0));
    EXPECT_FALSE(tree.contains(1));
    EXPECT_FALSE(tree.contains(2));

    for (Key key = 3; key < 10; ++key) {
        std::optional<Value> value = tree.get(key);
        ASSERT_TRUE(value.has_value()) << "missing key " << key;
        EXPECT_EQ(*value, key);
    }
}

TEST_F(BPlusTreeTest, RemoveAllKeysThenReinsertWorks) {
    constexpr Key kCount = 30;

    for (Key key = 0; key < kCount; ++key) {
        ASSERT_TRUE(tree.insert(key, key));
    }

    // Remove in a different order than insertion so both merges from the
    // left and merges from the right of a given node get exercised, and
    // any cascading merge up multiple internal levels gets triggered.
    std::vector<Key> remove_order(kCount);
    std::iota(remove_order.begin(), remove_order.end(), 0);
    std::mt19937 rng(5678);
    std::shuffle(remove_order.begin(), remove_order.end(), rng);

    for (Key key : remove_order) {
        ASSERT_TRUE(tree.remove(key)) << "failed to remove key " << key;
    }

    for (Key key = 0; key < kCount; ++key) {
        EXPECT_FALSE(tree.contains(key)) << "key " << key << " still present after removal";
    }

    // The tree must still work correctly after being emptied out via
    // repeated merges/root-collapses -- this is the scenario most likely
    // to surface a corrupted root or a dangling child pointer.
    EXPECT_TRUE(tree.insert(999, 42));
    std::optional<Value> value = tree.get(999);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 42);
}

TEST_F(BPlusTreeTest, RemoveThenReinsertSameKeySucceeds) {
    ASSERT_TRUE(tree.insert(5, 50));
    ASSERT_TRUE(tree.remove(5));
    EXPECT_FALSE(tree.contains(5));

    // Removing the key must un-block it as a duplicate.
    EXPECT_TRUE(tree.insert(5, 999));
    std::optional<Value> value = tree.get(5);
    ASSERT_TRUE(value.has_value());
    EXPECT_EQ(*value, 999);
}

TEST_F(BPlusTreeTest, RemoveEveryOtherKeyLeavesRemainderIntact) {
    constexpr Key kCount = 40;

    for (Key key = 0; key < kCount; ++key) {
        ASSERT_TRUE(tree.insert(key, key));
    }

    for (Key key = 0; key < kCount; key += 2) {
        ASSERT_TRUE(tree.remove(key));
    }

    for (Key key = 0; key < kCount; ++key) {
        if (key % 2 == 0) {
            EXPECT_FALSE(tree.contains(key)) << "key " << key << " should have been removed";
        } else {
            std::optional<Value> value = tree.get(key);
            ASSERT_TRUE(value.has_value()) << "missing key " << key;
            EXPECT_EQ(*value, key);
        }
    }
}
