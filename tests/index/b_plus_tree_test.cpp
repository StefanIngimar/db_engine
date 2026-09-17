#include <gtest/gtest.h>

#include "index/b_plus_tree/b_plus_tree.h"
#include "storage/buffer_manager.h"
#include "storage/disk_manager.h"

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
