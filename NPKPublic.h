#ifndef NPKCOLOR_H
#define NPKCOLOR_H
#include <cstdint>
#include <string>

namespace neapu_ex_npk {
enum ColorType: uint32_t {
    CL_UNKNOWN = 0x00,
    CL_V4_FMT = 0x01,
    CL_RGB565 = 0x02,
    CL_ARGB8888 = 0x10,
    CL_ARGB4444 = 0x0F,
    CL_ARGB1555 = 0x0E,
    CL_LINK = 0x11,
    CL_DDS_DXT1 = 0x12,
    CL_DDS_DXT3 = 0x13,
    CL_DDS_DXT5 = 0x14
};

enum CompressType: uint32_t {
    CP_UNKNOWN = 0x00,
    CP_NONE = 0x05,
    CP_ZLIB = 0x06,
    CP_ZLIB2 = 0x07
};

enum DDSFormat: uint32_t {
    DDS_UNKNOWN = 0x00,
    DDS_FXT1 = 0x12,
    DDS_FXT3 = 0x13,
    DDS_FXT5 = 0x14
};

typedef struct NPKColor {
    uint8_t b{0};
    uint8_t g{0};
    uint8_t r{0};
    uint8_t a{0};
} NPKColor;

std::string colorTypeToString(ColorType type);
}

#endif //NPKCOLOR_H