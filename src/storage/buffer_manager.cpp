#include "buffer_manager.h"

#include "disk_manager.h"
#include "page.h"

#include <stdexcept>
#include <string>

BufferManager::BufferManager(
    DiskManager& disk_manager,
    std::size_t pool_size
)
    : disk_manager_(disk_manager),
      pool_size_(pool_size),
      frames_(pool_size) {

    free_list_.reserve(pool_size);
    for (FrameId i = 0; i < pool_size; ++i) {
        free_list_.push_back(i);
    }
}

PageId BufferManager::allocatePage() {
    return next_page_id_++;
}

FrameId BufferManager::advanceClock() {
    FrameId hand = clock_hand_;
    clock_hand_ = (clock_hand_ + 1) % pool_size_;
    return hand;
}

std::optional<FrameId> BufferManager::findVictimFrame() {
    for (std::size_t attempts = 0; attempts < 2 * pool_size_; ++attempts) {
        FrameId candidate = advanceClock();
        Frame& frame = frames_[candidate];

        if (!frame.in_use || frame.pin_count > 0) {
            continue;
        }

        if (frame.reference_bit) {
            frame.reference_bit = false;
            continue;
        }

        return candidate;
    }

    return std::nullopt;
}

FrameId BufferManager::allocateFrame() {
    if (!free_list_.empty()) {
        FrameId id = free_list_.back();
        free_list_.pop_back();
        return id;
    }

    std::optional<FrameId> victim = findVictimFrame();
    if (!victim.has_value()) {
        throw std::runtime_error(
            "BufferManager: pool exhausted, every frame is pinned");
    }

    Frame& frame = frames_[*victim];

    if (frame.dirty) {
        // TODO: disk_manager_.writePage(frame.page_id, *frame.page);
    }

    page_table_.erase(frame.page_id);
    frame.page.reset();
    frame.page_id = INVALID_PAGE_ID;
    frame.pin_count = 0;
    frame.dirty = false;
    frame.reference_bit = false;
    frame.in_use = false;

    return *victim;
}

PageId BufferManager::newPage(
    std::unique_ptr<Page> page
) {
    PageId page_id = allocatePage();
    FrameId frame_id = allocateFrame();

    Frame& frame = frames_[frame_id];
    frame.page = std::move(page);
    frame.page_id = page_id;
    frame.pin_count = 1;
    frame.dirty = true;   // a brand-new page has never been written to disk
    frame.reference_bit = true;
    frame.in_use = true;

    page_table_[page_id] = frame_id;

    return page_id;
}

Page* BufferManager::fetchPage(
    PageId page_id
) {
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        return nullptr;
    }

    Frame& frame = frames_[it->second];
    frame.pin_count++;
    frame.reference_bit = true;

    return frame.page.get();
}

void BufferManager::unpinPage(
    PageId page_id,
    bool dirty
) {
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        return;
    }

    Frame& frame = frames_[it->second];

    if (frame.pin_count > 0) {
        frame.pin_count--;
    }

    if (dirty) {
        frame.dirty = true;
    }
}

void BufferManager::disposePage(PageId page_id) {
    auto it = page_table_.find(page_id);

    if (it == page_table_.end()) {
        return;
    }

    FrameId frame_id = it->second;
    Frame& frame = frames_[frame_id];

    if (frame.pin_count > 0) {
        throw std::runtime_error(
            "BufferManager::disposePage: page " + std::to_string(page_id) +
            " is still pinned");
    }

    page_table_.erase(it);

    frame.page.reset();
    frame.page_id = INVALID_PAGE_ID;
    frame.pin_count = 0;
    frame.dirty = false;
    frame.reference_bit = false;
    frame.in_use = false;

    free_list_.push_back(frame_id);
}

void BufferManager::flushPage() {
    for (Frame& frame : frames_) {
        if (frame.in_use && frame.dirty) {
            // TODO: disk_manager_.writePage(frame.page_id, *frame.page);
            frame.dirty = false;
        }
    }
}
