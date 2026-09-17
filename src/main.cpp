#include "storage/buffer_manager.h"
#include "storage/disk_manager.h"

#include "index/b_plus_tree/b_plus_tree.h"

#include <iostream>

int main() {

    DiskManager disk_manager;

    BufferManager buffer_manager(
        disk_manager
    );

    BPlusTree tree(
        buffer_manager
    );

    tree.insert(10, 100);
    tree.insert(20, 200);
    tree.insert(30, 300);
    tree.insert(40, 400);

    auto value = tree.get(20);

    if (value) {
        std::cout
            << "Found: "
            << *value
            << '\n';
    }

    if (!tree.contains(50)) {
        std::cout
            << "50 not found\n";
    }

    return 0;
}
