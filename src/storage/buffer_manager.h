#pragma once

#include "page.h"

#include <memory>
#include <unordered_map>
#include <cstddef>
#include <optional>
#include <vector>

class DiskManager;
class BPlusTreePage;

using FrameId = std::size_t;

struct Frame{
    std::unique_ptr<Page> page;
    PageId page_id = INVALID_PAGE_ID;
    size_t pin_count = 0;
    bool dirty = false;
    bool reference_bit = false;
    bool in_use = false;
};

class BufferManager {
public:
    explicit BufferManager(
        DiskManager& disk_manager,
        std::size_t pool_size = 16
    );

    Page* fetchPage(PageId page_id);

    PageId newPage(
        std::unique_ptr<Page> page
    );

    void unpinPage(
        PageId page_id,
        bool dirty
    );

    void disposePage(PageId page_id);

    void flushPage();

private:
    FrameId advanceClock();
    std::optional<FrameId> findVictimFrame();
    FrameId allocateFrame();
    PageId allocatePage();

    DiskManager& disk_manager_;

    std::size_t pool_size_;
    std::vector<Frame> frames_;
    std::vector<FrameId> free_list_;
    FrameId clock_hand_ = 0;

    PageId next_page_id_ = 0;

    std::unordered_map<PageId, FrameId> page_table_;
};
