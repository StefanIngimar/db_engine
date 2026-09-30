#include "disk_manager.h"

#include <algorithm>
#include <stdexcept>

DiskManager::DiskManager(const std::string& filename) {
    // "r+b" requires the file to already exist. If it doesn't, create it
    // fresh, then reopen so subsequent writes/reads use the same mode.
    file_ = std::fopen(filename.c_str(), "r+b");
    if (file_ == nullptr) {
        file_ = std::fopen(filename.c_str(), "w+b");
    }
    if (file_ == nullptr) {
        throw std::runtime_error("DiskManager: unable to open " + filename);
    }
}

DiskManager::~DiskManager() {
    if (file_ != nullptr) {
        std::fclose(file_);
    }
}

void DiskManager::writePage(PageId page_id, const uint8_t* data) {
    long offset = static_cast<long>(page_id) * static_cast<long>(PAGE_SIZE);

    if (std::fseek(file_, offset, SEEK_SET) != 0) {
        throw std::runtime_error("DiskManager: seek failed while writing page");
    }

    std::size_t written = std::fwrite(data, 1, PAGE_SIZE, file_);
    if (written != PAGE_SIZE) {
        throw std::runtime_error("DiskManager: short write while writing page");
    }

    std::fflush(file_);
}

void DiskManager::readPage(PageId page_id, uint8_t* out_data) {
    long offset = static_cast<long>(page_id) * static_cast<long>(PAGE_SIZE);

    if (std::fseek(file_, offset, SEEK_SET) != 0) {
        throw std::runtime_error("DiskManager: seek failed while reading page");
    }

    std::size_t bytes_read = std::fread(out_data, 1, PAGE_SIZE, file_);
    if (bytes_read < PAGE_SIZE) {
        // Past EOF -- this page was never written. Treat it as a fresh,
        // zeroed page rather than an error.
        std::fill(out_data + bytes_read, out_data + PAGE_SIZE, uint8_t{0});
    }
}
