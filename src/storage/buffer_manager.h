#pragma once

#include "page.h"

#include <memory>
#include <unordered_map>

class DiskManager;
class BPlusTreePage;

class BufferManager {
public:
    explicit BufferManager(DiskManager& disk_manager);

    Page* fetchPage(PageId page_id);

    PageId newPage(
        std::unique_ptr<Page> page
    );

    void unpinPage(
        PageId page_id,
        bool dirty
    );

    void advanceClock();

    void allocateFrame();

    void readPage(
        PageId page_id,
        bool dirty
    );

    PageId allocatePage(
        PageId page_id,
        bool dirty
    );

    void disposePage(
        PageId page_id,
        bool dirty
    );

    void flushFile();

private:
    DiskManager& disk_manager_;

    PageId next_page_id_ = 0;

    std::unordered_map<
        PageId,
        std::unique_ptr<Page>
    > pages_;
};
