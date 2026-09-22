#include "buffer_manager.h"

#include "disk_manager.h"
#include "page.h"

BufferManager::BufferManager(
    DiskManager& disk_manager
)
    : disk_manager_(disk_manager) {
}

PageId BufferManager::newPage(
    std::unique_ptr<Page> page
) {
    PageId page_id = next_page_id_++;

    Frame frame;
    frame.page = std::move(page);
    frame.pin_count = 1;
    frame.dirty = true;

    pages_[page_id] = std::move(frame);

    return page_id;
}

Page* BufferManager::fetchPage(
    PageId page_id
) {
    auto it = pages_.find(page_id);

    if (it == pages_.end()) {
        return nullptr;
    }

    it->second.pin_count++;

    return it->second.page.get();
}

void BufferManager::unpinPage(
    PageId page_id,
    bool dirty
) {
    auto it = pages_.find(page_id);

    if(it == pages_.end()){
        return;
    }

    Frame& frame = it->second;

    if(frame.pin_count > 0){
        frame.pin_count--;
    }

    if(dirty){
        frame.dirty = true;
    }
}

void BufferManager::flushFile(){
    for(auto& [page_id, frame]:pages_){
        if(frame.dirty){
            // TODO disk_manager_.writepage(...)
            frame.dirty = false;
        }
    }
}
