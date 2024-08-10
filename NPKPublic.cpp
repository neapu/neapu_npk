#include "NPKPublic.h"

std::string neapu_ex_npk::colorTypeToString(const ColorType type)
{
    switch (type) {
        case ColorType::CL_UNKNOWN:
            return "UNKNOWN";
        case ColorType::CL_V4_FMT:
            return "V4_FMT";
        case ColorType::CL_RGB565:
            return "RGB565";
        case ColorType::CL_ARGB8888:
            return "ARGB8888";
        case ColorType::CL_ARGB4444:
            return "ARGB4444";
        case ColorType::CL_ARGB1555:
            return "ARGB1555";
        case ColorType::CL_LINK:
            return "LINK";
        case ColorType::CL_DDS_DXT1:
            return "DDS_DXT1";
        case ColorType::CL_DDS_DXT3:
            return "DDS_DXT3";
        case ColorType::CL_DDS_DXT5:
            return "DDS_DXT5";
        default:
            return "UNKNOWN";
    }
}