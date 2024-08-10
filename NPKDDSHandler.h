//
// Created by liu86 on 24-7-26.
//

#ifndef NPKDDSHANDLER_H
#define NPKDDSHANDLER_H

#include <cstdint>
#include <memory>

#include "NPKPublic.h"

namespace neapu_ex_npk {
enum DDSPixelDTXFormat:uint32_t {
    DXT_UNKNOWN = 0x00000000U,
    DXT1 = 0x31545844U,
    DXT2 = 0x32545844U,
    DXT3 = 0x33545844U,
    DXT4 = 0x34545844U,
    DXT5 = 0x35545844U
};
#pragma pack(push, 1)
typedef struct NPKDDSIndex {
    uint32_t unknown1 = 0; // 默认为1
    DDSFormat format = DDSFormat::DDS_UNKNOWN;
    uint32_t index = 0;
    uint32_t compressSize = 0;   // 压缩后大小，也就是在img中实际保存的大小
    uint32_t uncompressSize = 0; // 压缩前大小
    uint32_t width = 0;          // 一般要求被4整除
    uint32_t height = 0;         // 一般要求被4整除
} NPKDDSIndex;
#pragma pack(pop)

class NPKMatrix;

class NPKDDSHandler {
public:
    NPKDDSHandler() = default;
    virtual ~NPKDDSHandler();

    int64_t loadIndex(const uint8_t* data, const uint64_t dataLen);
    int64_t loadData(const uint8_t* data, const uint64_t dataLen);

    std::shared_ptr<NPKMatrix> toMatrix() const;
private:
    static NPKColor RGB565ToNPKColor(const uint16_t color);
    // static std::shared_ptr<NPKMatrix> DXT1ToMatrix(const uint8_t* imgData, const uint64_t dataLen, const uint32_t width, const uint32_t height);
    static void DXT1UnitToNPKColor(const uint8_t* imgData, NPKColor colors[]);
    // static std::shared_ptr<NPKMatrix> DXT3ToMatrix(const uint8_t* imgData, const uint64_t dataLen, const uint32_t width, const uint32_t height);
    static void DXT3UnitToNPKColor(const uint8_t* imgData, NPKColor colors[]);
    // static std::shared_ptr<NPKMatrix> DXT5ToMatrix(const uint8_t* imgData, const uint64_t dataLen, const uint32_t width, const uint32_t height);
    static void DXT5UnitToNPKColor(const uint8_t* imgData, NPKColor colors[]);
    static std::shared_ptr<NPKMatrix> DXTxToMatrix(const uint8_t* imgData, const uint64_t dataLen, const uint32_t width, const uint32_t height, const DDSPixelDTXFormat format);

private:
    NPKDDSIndex m_index;
    uint8_t* m_data{nullptr};
};
} // neapu_ex_npk

#endif //NPKDDSHANDLER_H