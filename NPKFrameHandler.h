//
// Created by liu86 on 24-7-21.
//

#ifndef NPKFRAMEHANDLER_H
#define NPKFRAMEHANDLER_H
#include <cstdint>
#include <memory>

#include "NPKMatrix.h"

namespace neapu_ex_npk {
#pragma pack(push, 1)
typedef struct NPKFrameIndex {
    ColorType colorType = CL_UNKNOWN; // 颜色系统 0x10 (ARGB8888) Ox0F (ARGB4444) 0x0E (ARGB1555); 0x11 (链接帧)
    union {
        CompressType compressType; // 压缩类型 0x05 (无压缩) 0x06 (zlib压缩) 0x07 (zlib2压缩)
        uint32_t linkTo = 0;       // 链接到的帧
    };

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t dataSize = 0; // 原始数据大小(如果有压缩，是压缩后的大小)
    uint32_t posX = 0;
    uint32_t posY = 0;
    uint32_t frameWidth = 0;  // 帧域宽度，一般等于帧宽度+posX, 也可能更大
    uint32_t frameHeight = 0; // 帧域高度

    // V5新增
    uint32_t unknown1 = 0;      // 保留
    uint32_t ddsIndex = 0;      // DDS索引，表示是从哪个DDS图像上剪裁出来的
    uint32_t ddsLeftEdge = 0;   // 剪裁左边界
    uint32_t ddsTopEdge = 0;    // 剪裁上边界
    uint32_t ddsRightEdge = 0;  // 剪裁右边界
    uint32_t ddsBottomEdge = 0; // 剪裁下边界
    uint32_t unknown2 = 0;      // 保留
} NPKFrameIndex;


#pragma pack(pop)

constexpr auto LINK_FRAME_INDEX_SIZE = 2 * sizeof(uint32_t);
constexpr auto MATRIX_FRAME_INDEX_SIZE = 9 * sizeof(uint32_t);
constexpr auto DDS_FRAME_INDEX_SIZE = sizeof(NPKFrameIndex);

class NPKImageHandler;
class NPKMatrix;
class NPKPaletteManager;

class NPKFrameHandler {
public:
    explicit NPKFrameHandler(std::shared_ptr<NPKPaletteManager> paletteManager);
    virtual ~NPKFrameHandler();

    int loadIndex(const uint8_t* data, uint64_t dataLen);
    int loadData(const uint8_t* data, const uint64_t dataLen);
    bool isLinkFrame() const { return m_index.colorType == CL_LINK; }
    bool isMatrixFrame() const { return m_index.colorType < CL_LINK && m_index.colorType != CL_UNKNOWN; }
    bool isDDSFrame() const { return m_index.colorType > CL_LINK; }
    uint32_t linkTo() const { return m_index.linkTo; }
    uint32_t ddsIndex() const { return m_index.ddsIndex; }
    std::string ddsClipInfo() const;
    std::shared_ptr<NPKMatrix> toMatrix(int paletteIndex = 0);
    std::shared_ptr<NPKMatrix> ddsClipMatrix(std::shared_ptr<NPKMatrix>&& matrix);

    ColorType colorType() const { return m_index.colorType; }
    uint32_t width() const { return m_index.width; }
    uint32_t height() const { return m_index.height; }

private:
    std::shared_ptr<NPKMatrix> toMatrixV2(const uint8_t* data);
    std::shared_ptr<NPKMatrix> toMatrixV4V6(const uint8_t* data, int paletteIndex);

private:
    NPKFrameIndex m_index{};
    uint8_t* m_data{nullptr}; // 为了加载时不等待太久，在真正读取帧画面时才解压缩
    std::shared_ptr<NPKPaletteManager> m_paletteManager{nullptr};
};
} // neapu_ex_npk

#endif //NPKFRAMEHANDLER_H