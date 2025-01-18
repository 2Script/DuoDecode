#pragma once
#include <array>
#include <string>
extern "C" {
#include <libavutil/avutil.h>
}

namespace dd {
    struct device_info {
        std::string name;
        std::string description;
        std::array<bool, AVMEDIA_TYPE_NB> media_types;
    };
}