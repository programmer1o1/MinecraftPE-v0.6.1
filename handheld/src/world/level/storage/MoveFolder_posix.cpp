#if defined(LINUX) || (defined(POSIX) && !defined(__APPLE__))

#include "MoveFolder.h"
#include <cstdio>

void moveFolder(const std::string& src, const std::string& dst) {
    rename(src.c_str(), dst.c_str());
}

#endif
