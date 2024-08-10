//
// Created by liu86 on 24-7-26.
//

#include "NPKDDSHandler.h"

#include <zlib.h>

#include "logger.h"
#include "NPKMatrix.h"

namespace {
std::string DDSPixelDTXFormatToString(const neapu_ex_npk::DDSPixelDTXFormat format)
{
    switch (format) {
    case neapu_ex_npk::DDSPixelDTXFormat::DXT1: return "DXT1";
    case neapu_ex_npk::DDSPixelDTXFormat::DXT2: return "DXT2";
    case neapu_ex_npk::DDSPixelDTXFormat::DXT3: return "DXT3";
    case neapu_ex_npk::DDSPixelDTXFormat::DXT4: return "DXT4";
    case neapu_ex_npk::DDSPixelDTXFormat::DXT5: return "DXT5";
    default: return "Unknown";
    }
}
}

namespace neapu_ex_npk {
constexpr auto DXT1_UNIT_LENGTH = 8;
constexpr auto DXT3_UNIT_LENGTH = 16;
constexpr auto DXT5_UNIT_LENGTH = 16;
constexpr auto UNIT_COLOR_COUNT = 16;

//枚举
enum DDSHeaderFlag:uint32_t {
    DDSD_UNKNOWN = 0x00000000U,
    DDSD_CAPS = 0x00000001U,        // Required in every .dds file.
    DDSD_HEIGHT = 0x00000002U,      // Required in every .dds file.
    DDSD_WIDTH = 0x00000004U,       // Required in every .dds file.
    DDSD_PITCH = 0x00000008U,       // Required when pitch is provided for an uncompressed texture.
    DDSD_PIXELFORMAT = 0x00001000U, // Required in every .dds file.
    DDSD_MIPMAPCOUNT = 0x00020000U, // Required in a mipmapped texture.
    DDSD_LINEARSIZE = 0x00080000U,  // Required when pitch is provided for a compressed texture.
    DDSD_DEPTH = 0x00800000U        // Required in a depth texture.
};

enum DDSCap:uint32_t {
    DDSCAPS_UNKNOWN = 0x00000000U,
    DDSCAPS_COMPLEX = 0x00000008U,
    // Optional; must be used on any file that contains more than one surface (a mipmap, a cubic environment map, or mipmapped volume texture).
    DDSCAPS_MIPMAP = 0x00400000U,  // Optional; should be used for a mipmap.
    DDSCAPS_TEXTURE = 0x00001000U, // Required
};

enum DDSCap2:uint32_t {
    DDSCAPS2_UNKNOWN = 0x00000000U,
    DDSCAPS2_CUBEMAP = 0x00000200U,
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x00000400U,
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x00000800U,
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x00001000U,
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x00002000U,
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x00004000U,
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x00008000U,
    DDSCAPS2_VOLUME = 0x00200000U
};

enum DDSPixelFormatFlag:uint32_t {
    DDPF_UNKNOWN = 0x00000000U,
    DDPF_ALPHAPIXELS = 0x00000001U,
    DDPF_ALPHA = 0x00000002U,
    DDPF_FOURCC = 0x00000004U,
    DDPF_RGB = 0x00000040U,
    DDPF_YUV = 0x00000200U,
    DDPF_LUMINANCE = 0x00002000U
};


#pragma pack(push, 1)
typedef struct NPKDDSPixelFormat {
    uint32_t size;            //像素格式数据大小，固定为32(0x20)
    DDSPixelFormatFlag flags; //标志位，DNF里一般用DDPF_FOURCC（0x04）
    DDSPixelDTXFormat fourCC; // DXT格式，flag里有DDPF_FOURCC有效，DNF里一般为字符串DXT1-5（0x31545844 - 0x35545844）
    uint32_t rgbBitCount;     //一个RGB(A)包含的位数，flag里有DDPF_RGB、DDPF_YUV、DDPF_LUMINANCE有效，DNF里一般无效
    uint32_t rBitMask;        // R通道(Y通道或亮度通道)掩码，DNF用不上
    uint32_t gBitMask;        // G通道(U通道)掩码，DNF用不上
    uint32_t bBitMask;        // B通道(V通道)掩码，DNF用不上
    uint32_t aBitMask;        // A通道掩码，DNF用不上
} NPKDDSPixelFormat;

//像素头
typedef struct NPKDDSHeader {
    uint32_t magic;                //标识"DDS "，固定为542327876（0x20534444）
    uint32_t size;                 //首部大小，固定为124（0x7C）
    uint32_t flags;                //标志位（见DDSHeaderFlags），DNF里经常采用的是“压缩纹理”，一般为0x81007
    uint32_t height;               //高度
    uint32_t width;                //宽度
    uint32_t pitchOrLinearSize;    //间距，对于压缩纹理，为整个图像数据的字节数，对于非压缩纹理，为一行像素数据的字节数，DNF为前者
    uint32_t depth;                // DNF里没用，标志位DDSD_DEPTH有效时有用
    uint32_t mipMapCount;          // DNF里没用，标志位DDSD_MIPMAPCOUNT有效时有用
    uint32_t reserved1[11];        //保留位，DNF里第十双字一般为"NVTT"（0x5454564E），第十一双字一般为0x20008，其他为零
    NPKDDSPixelFormat pixelFormat; //像素格式数据，见上文
    uint32_t caps1;                //曲面的复杂性（见DDSCaps），DNF里一般为0x1000
    uint32_t caps2;                //曲面的其他信息（主要是三维性），DNF里不用，为零
    uint32_t caps3;                //默认零
    uint32_t caps4;                //默认零
    uint32_t reserved2;            //默认零
} NPKDDSHeader;
#pragma pack(pop)
NPKDDSHandler::~NPKDDSHandler()
{
    if (m_data) {
        delete[] m_data;
    }
}

int64_t NPKDDSHandler::loadIndex(const uint8_t* data, const uint64_t dataLen)
{
    if (dataLen < sizeof(NPKDDSIndex)) {
        return -1;
    }

    int ret = memcpy_s(&m_index, sizeof(m_index), data, sizeof(m_index));
    if (ret != 0) {
        LOG_ERROR << "Failed to copy index.";
        return -1;
    }

    return sizeof(m_index);
}

int64_t NPKDDSHandler::loadData(const uint8_t* data, const uint64_t dataLen)
{
    if (dataLen < m_index.compressSize) {
        LOG_WARNING << "Data length is too short.";
        return dataLen;
    }

    m_data = new uint8_t[m_index.compressSize];
    int ret = memcpy_s(m_data, m_index.compressSize, data, m_index.compressSize);
    if (ret != 0) {
        LOG_ERROR << "Failed to copy data.";
        return -1;
    }

    return m_index.compressSize;
}

std::shared_ptr<NPKMatrix> NPKDDSHandler::toMatrix() const
{
    unsigned long imgDataSize = m_index.uncompressSize;
    auto* uncompressedData = new uint8_t[imgDataSize];
    int ret = uncompress(uncompressedData, &imgDataSize, m_data, m_index.compressSize);
    if (ret != Z_OK) {
        LOG_ERROR << "Failed to uncompress data.";
        delete[] uncompressedData;
        return nullptr;
    }

    NPKDDSHeader header;
    ret = memcpy_s(&header, sizeof(header), uncompressedData, sizeof(header));
    if (ret != 0) {
        LOG_ERROR << "Failed to copy header.";
        delete[] uncompressedData;
        return nullptr;
    }

    if (header.magic != 0x20534444) {
        LOG_ERROR << "Magic is not correct.";
        delete[] uncompressedData;
        return nullptr;
    }

    if (header.size != sizeof(NPKDDSHeader) - 4) {
        LOG_ERROR << "Size is not correct.";
        delete[] uncompressedData;
        return nullptr;
    }

    if (header.flags != 0x00081007) {
        LOG_ERROR << "Flags is not correct.";
        delete[] uncompressedData;
        return nullptr;
    }
    if (header.pitchOrLinearSize < m_index.uncompressSize - sizeof(NPKDDSHeader)) {
        LOG_ERROR << "Data length is too short.";
        delete[] uncompressedData;
        return nullptr;
    }

    const auto* imgData = uncompressedData + sizeof(NPKDDSHeader);
    std::shared_ptr<NPKMatrix> retMatrix = DXTxToMatrix(imgData, m_index.uncompressSize - sizeof(NPKDDSHeader), header.width, header.height,
                                                        header.pixelFormat.fourCC);
    LOG_DEBUG << "DTX Format: " << DDSPixelDTXFormatToString(header.pixelFormat.fourCC) << ", Width: " << header.width << ", Height: " <<
        header.height;

    delete[] uncompressedData;
    return retMatrix;
}

NPKColor NPKDDSHandler::RGB565ToNPKColor(const uint16_t color)
{
    NPKColor ret;
    ret.r = (color & 0xF800) >> 8;
    ret.g = (color & 0x07E0) >> 3;
    ret.b = (color & 0x001F) << 3;
    ret.a = 0xFF;
    return ret;
}

void NPKDDSHandler::DXT1UnitToNPKColor(const uint8_t* imgData, NPKColor colors[])
{
    const uint32_t clrPart = imgData[0] | imgData[1] << 8 | imgData[2] << 16 | imgData[3] << 24;
    const uint32_t idxPart = imgData[4] | imgData[5] << 8 | imgData[6] << 16 | imgData[7] << 24;
    const uint16_t clr0 = clrPart & 0xFFFF;
    const uint16_t clr1 = clrPart >> 16;
    NPKColor clrList[4];
    clrList[0] = RGB565ToNPKColor(clr0);
    clrList[1] = RGB565ToNPKColor(clr1);
    if (clr0 > clr1) {
        clrList[2].r = (2 * clrList[0].r + clrList[1].r) / 3;
        clrList[2].g = (2 * clrList[0].g + clrList[1].g) / 3;
        clrList[2].b = (2 * clrList[0].b + clrList[1].b) / 3;
        clrList[2].a = 0xFF;
        clrList[3].r = (clrList[0].r + 2 * clrList[1].r) / 3;
        clrList[3].g = (clrList[0].g + 2 * clrList[1].g) / 3;
        clrList[3].b = (clrList[0].b + 2 * clrList[1].b) / 3;
        clrList[3].a = 0xFF;
    } else {
        clrList[2].r = (clrList[0].r + clrList[1].r) / 2;
        clrList[2].g = (clrList[0].g + clrList[1].g) / 2;
        clrList[2].b = (clrList[0].b + clrList[1].b) / 2;
        clrList[2].a = 0xFF;
        clrList[3].r = 0;
        clrList[3].g = 0;
        clrList[3].b = 0;
        clrList[3].a = 0;
    }

    for (uint32_t i = 0; i < UNIT_COLOR_COUNT; ++i) {
        const uint32_t idx = (idxPart >> (i * 2)) & 0x03;
        colors[i] = clrList[idx];
    }
}

void NPKDDSHandler::DXT3UnitToNPKColor(const uint8_t* imgData, NPKColor colors[])
{
    const uint16_t clr0 = imgData[8] | imgData[9] << 8;
    const uint16_t clr1 = imgData[10] | imgData[11] << 8;
    const uint32_t idxPart = imgData[12] | imgData[13] << 8 | imgData[14] << 16 | imgData[15] << 24;
    NPKColor clrList[4];
    clrList[0] = RGB565ToNPKColor(clr0);
    clrList[1] = RGB565ToNPKColor(clr1);
    clrList[2].r = (2 * clrList[0].r + clrList[1].r) / 3;
    clrList[2].g = (2 * clrList[0].g + clrList[1].g) / 3;
    clrList[2].b = (2 * clrList[0].b + clrList[1].b) / 3;
    clrList[2].a = 0xFF;
    clrList[3].r = (clrList[0].r + 2 * clrList[1].r) / 3;
    clrList[3].g = (clrList[0].g + 2 * clrList[1].g) / 3;
    clrList[3].b = (clrList[0].b + 2 * clrList[1].b) / 3;
    clrList[3].a = 0xFF;

    for (uint32_t i = 0; i < UNIT_COLOR_COUNT; ++i) {
        const uint32_t idx = (idxPart >> (i * 2)) & 0x03;
        colors[i] = clrList[idx];
        if (i % 2 == 0) {
            colors[i].a = (imgData[i / 2] & 0x0F) * 0x11;
        } else {
            colors[i].a = (imgData[i / 2] >> 4) * 0x11;
        }
    }
}

void NPKDDSHandler::DXT5UnitToNPKColor(const uint8_t* imgData, NPKColor colors[])
{
    const uint16_t clr0 = imgData[8] | imgData[9] << 8;
    const uint16_t clr1 = imgData[10] | imgData[11] << 8;
    const uint32_t idxPart = imgData[12] | imgData[13] << 8 | imgData[14] << 16 | imgData[15] << 24;
    NPKColor clrList[4];
    clrList[0] = RGB565ToNPKColor(clr0);
    clrList[1] = RGB565ToNPKColor(clr1);
    clrList[2].r = (2 * clrList[0].r + clrList[1].r) / 3;
    clrList[2].g = (2 * clrList[0].g + clrList[1].g) / 3;
    clrList[2].b = (2 * clrList[0].b + clrList[1].b) / 3;
    clrList[2].a = 0xFF;
    clrList[3].r = (clrList[0].r + 2 * clrList[1].r) / 3;
    clrList[3].g = (clrList[0].g + 2 * clrList[1].g) / 3;
    clrList[3].b = (clrList[0].b + 2 * clrList[1].b) / 3;
    clrList[3].a = 0xFF;

    uint8_t alpha[8];
    alpha[0] = imgData[0];
    alpha[1] = imgData[1];
    if (alpha[0] > alpha[1]) {
        alpha[2] = (6 * alpha[0] + 1 * alpha[1]) / 7;
        alpha[3] = (5 * alpha[0] + 2 * alpha[1]) / 7;
        alpha[4] = (4 * alpha[0] + 3 * alpha[1]) / 7;
        alpha[5] = (3 * alpha[0] + 4 * alpha[1]) / 7;
        alpha[6] = (2 * alpha[0] + 5 * alpha[1]) / 7;
        alpha[7] = (1 * alpha[0] + 6 * alpha[1]) / 7;
    } else {
        alpha[2] = (4 * alpha[0] + 1 * alpha[1]) / 5;
        alpha[3] = (3 * alpha[0] + 2 * alpha[1]) / 5;
        alpha[4] = (2 * alpha[0] + 3 * alpha[1]) / 5;
        alpha[5] = (1 * alpha[0] + 4 * alpha[1]) / 5;
        alpha[6] = 0;
        alpha[7] = 0xFF;
    }

    uint64_t alphaIdxPart = 0;
    for (uint32_t i = 7; i >= 2; --i) {
        alphaIdxPart <<= 8;
        alphaIdxPart |= imgData[i];
    }
    uint16_t alphaIdx[16];
    for (unsigned short& i : alphaIdx) {
        i = alpha[alphaIdxPart & 0x07];
        alphaIdxPart >>= 3;
    }

    for (uint32_t i = 0; i < UNIT_COLOR_COUNT; ++i) {
        const uint32_t idx = (idxPart >> (i * 2)) & 0x03;
        colors[i] = clrList[idx];
        colors[i].a = alphaIdx[i];
    }
}

std::shared_ptr<NPKMatrix> NPKDDSHandler::DXTxToMatrix(const uint8_t* imgData, const uint64_t dataLen, const uint32_t width,
                                                       const uint32_t height, const DDSPixelDTXFormat format)
{
    uint64_t offset = 0;
    const uint32_t blockWidth = width / 4;
    const uint32_t blockHeight = height / 4;
    uint32_t unitCount = 0;
    switch (format) {
    case DDSPixelDTXFormat::DXT1: unitCount = DXT1_UNIT_LENGTH;
        break;
    case DDSPixelDTXFormat::DXT3: unitCount = DXT3_UNIT_LENGTH;
        break;
    case DDSPixelDTXFormat::DXT5: unitCount = DXT5_UNIT_LENGTH;
        break;
    default: return nullptr;
    }
    if (dataLen < blockWidth * blockHeight * unitCount) {
        return nullptr;
    }
    uint8_t unitData[DXT5_UNIT_LENGTH]; // 按大的来
    NPKColor colors[UNIT_COLOR_COUNT];
    auto matrex = NPKMatrix::createMatrix(width, height);
    for (uint32_t y = 0; y < blockHeight; ++y) {
        for (uint32_t x = 0; x < blockWidth; ++x) {
            memcpy(unitData, imgData + offset, unitCount);
            switch (format) {
            case DDSPixelDTXFormat::DXT1: DXT1UnitToNPKColor(unitData, colors);
                break;
            case DDSPixelDTXFormat::DXT3: DXT3UnitToNPKColor(unitData, colors);
                break;
            case DDSPixelDTXFormat::DXT5: DXT5UnitToNPKColor(unitData, colors);
                break;
            default: return nullptr;
            }
            for (uint32_t i = 0; i < UNIT_COLOR_COUNT; ++i) {
                const uint32_t unitX = x * 4 + i % 4;
                const uint32_t unitY = y * 4 + i / 4;
                matrex->setPixel(unitX, unitY, colors[i]);
            }
            offset += unitCount;
        }
    }
    return matrex;
}
} // neapu_ex_npk