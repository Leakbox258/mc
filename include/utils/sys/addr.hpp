#ifndef UTILS_SYS_ADDR
#define UTILS_SYS_ADDR

#include "utils/numeric.hpp"
#include <cassert>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

namespace utils {

namespace sys {

struct VMMapInfo {
  private:
    std::size_t HeapBegin = 0XFFFFFFFF;
    std::size_t HeapEnd = 0;
    std::size_t MaxStackTop = 0XFFFFFFFF;
    std::size_t StackEnd = 0;

    struct MapLine {
        std::size_t start, end;
        bool permissions[3]; // rwx
        std::size_t elf_offset;
        std::pair<unsigned, unsigned> dev_id; // major:minor
        std::size_t inode;
        std::string extar_attr;

        MapLine(const std::string& line) {
            std::istringstream iss(line);

            std::string range;
            iss >> range;
            std::size_t splitor; // "-"
            this->start = std::stoull(range.c_str(), &splitor, 16);
            this->end = std::stoull(range.substr(splitor + 1), &splitor, 16);
            assert(splitor + 1 == line.size());

            std::string perm;
            iss >> perm;
            for (int i = 0; i != 3; ++i) {
                this->permissions[i] = perm[i] != '-';
            }

            std::string offset;
            iss >> offset;
            this->elf_offset = std::stoull(offset.c_str(), nullptr, 16);

            std::string dev;
            iss >> dev;
            this->dev_id.first = std::stoul(dev);
            this->dev_id.second = std::stoul(dev.c_str() + 3);

            std::string inode;
            iss >> inode;
            this->inode = std::stoull(inode.c_str());

            iss >> this->extar_attr;
        }
    };

  private:
    VMMapInfo() {
        std::filesystem::path vmmap = "/proc/self/maps";
        std::ifstream file(vmmap);
        std::string line;

        while (std::getline(file, line)) {
            MapLine infos(line);

            if (infos.extar_attr.contains("heap")) {
                this->HeapBegin = std::min(this->HeapBegin, infos.start);
                this->HeapEnd = infos.end;
            } else if (infos.extar_attr.contains("stack")) {
                this->MaxStackTop = std::min(this->MaxStackTop, infos.start);
                this->StackEnd = infos.end;
            }
        }
    };

  public:
    static VMMapInfo& instance() {
        static VMMapInfo info;
        return info;
    }
    VMMapInfo(const VMMapInfo&) = delete;
    VMMapInfo(VMMapInfo&&) = delete;
    VMMapInfo& operator=(const VMMapInfo&) = delete;

    enum class Segment {
        Stack,
        Heap,
        NoRecord,
    };

    Segment check(void* addr) const {
        if (in_interval_t(this->HeapBegin, this->HeapEnd,
                          reinterpret_cast<std::size_t>(addr))) {
            return Segment::Heap;
        }
        if (in_interval_t(this->MaxStackTop, this->StackEnd,
                          reinterpret_cast<std::size_t>(addr))) {
            return Segment::Stack;
        }

        return Segment::NoRecord;
    }
    bool checkif(void* addr, Segment seg) const {
        return this->check(addr) == seg;
    }
};

}; // namespace sys

}; // namespace utils

#endif