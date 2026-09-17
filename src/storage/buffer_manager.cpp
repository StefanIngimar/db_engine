#include "buffer_manager.h"

#include "disk_manager.h"

BufferManager::BufferManager(
    DiskManager& disk_manager
)
    : disk_manager_(disk_manager) {
}

PageId BufferManager::newPage(
    std::unique_ptr<Page> page
) {
    PageId page_id = next_page_id_++;

    pages_[page_id] = std::move(page);

    return page_id;
}

Page* BufferManager::fetchPage(
    PageId page_id
) {
    auto it = pages_.find(page_id);

    if (it == pages_.end()) {
        return nullptr;
    }

    return it->second.get();
}

void BufferManager::unpinPage(
    PageId page_id,
    bool dirty
) {
    // TODO
}
