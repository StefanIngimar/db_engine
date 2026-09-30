#pragma once

#include "page.h"
#include <cstdio>
#include <string>

class DiskManager {
public:
    explicit DiskManager(const std::string& fileame = "database.db");
    ~DiskManager();

    DiskManager(const DiskManager&) = delete;
    DiskManager& operator=(const DiskManager&) = delete;

    void writePage(PageId page_id, const uint8_t* data);

    void readPage(PageId page_id, uint8_t* out_data);

private:
    FILE* file_;
};
