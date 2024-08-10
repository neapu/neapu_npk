//
// Created by liu86 on 24-7-21.
//

#include "NPKFrameHandler.h"
#include "NPKPaletteManager.h"
#include "logger.h"
#include <zlib.h>
#include <format>

namespace neapu_ex_npk {
NPKFrameHandler::NPKFrameHandler(std::shared_ptr<NPKPaletteManager> paletteManager)
    : m_paletteManager(paletteManager)
{
}

NPKFrameHandler::~NPKFrameHandler()
{
    if (m_data != nullptr) {
        delete[] m_data;
    }
}

int NPKFrameHandler::loadIndex(const uint8_t* data, const uint64_t dataLen)
{
    if (dataLen < LINK_FRAME_INDEX_SIZE) { // 最小的index是link类型，为8字节
        LOG_ERROR << "Data length is too short.";
        return -1;
    }

    uint32_t indexType = 0;
    int ret = memcpy_s(&indexType, sizeof(uint32_t), data, sizeof(uint32_t));
    if (ret != 0) {
        LOG_ERROR << "Failed to copy index type.";
        return -1;
    }

    int copyLen = 0;
    if (indexType == ColorType::CL_LINK) { // link类型, 8字节
        copyLen = LINK_FRAME_INDEX_SIZE;
    } else if (indexType < ColorType::CL_LINK) {   // 点阵图片索引(V2 V4 V6), 36字节
        if (dataLen < MATRIX_FRAME_INDEX_SIZE) {
            LOG_ERROR << "Data length is too short.";
            return -1;
        }
        copyLen = MATRIX_FRAME_INDEX_SIZE;
    } else { // DDS图片索引(V5), 60字节
        if (dataLen < sizeof(NPKFrameIndex)) {
            LOG_ERROR << "Data length is too short.";
            return -1;
        }
        copyLen = DDS_FRAME_INDEX_SIZE;
    }

    ret = memcpy_s(&m_index, copyLen, data, copyLen);
    if (ret != 0) {
        LOG_ERROR << "Failed to copy index.";
        return -1;
    }

    return copyLen;
}

int NPKFrameHandler::loadData(const uint8_t* data, const uint64_t dataLen)
{
    if (m_data != nullptr) {
        delete[] m_data;
    }

    if (m_index.dataSize == 0) {
        return 0;
    }

    if (dataLen < m_index.dataSize) {
        // 有时候DNF的IMG中最后一帧的数据长度不足（辣鸡DNF），直接当做无效帧处理
        LOG_WARNING << "Data length is too short. [size:" << dataLen << ", dataSize:" << m_index.dataSize << "]";
        return dataLen;
    }

    m_data = new uint8_t[m_index.dataSize];
    const int ret = memcpy_s(m_data, m_index.dataSize, data, m_index.dataSize);
    if (ret != 0) {
        LOG_ERROR << "Failed to copy origin data.";
        delete[] m_data;
        m_data = nullptr;
        return -1;
    }
    return m_index.dataSize;
}

std::string NPKFrameHandler::ddsClipInfo() const
{
    if (!isDDSFrame()) {
        return "none";
    }
    return std::format("{}.{}:{}.{}", m_index.ddsLeftEdge, m_index.ddsTopEdge, m_index.ddsRightEdge, m_index.ddsBottomEdge);
}

std::shared_ptr<NPKMatrix> NPKFrameHandler::toMatrix(int paletteIndex)
{
    if (isLinkFrame()) {
        return nullptr;
    }

    if (m_index.width == 0 || m_index.height == 0 || m_data == nullptr) {
        return nullptr;
    }

    uint32_t colorSize = 4;
    if (m_index.colorType == CL_ARGB4444 || m_index.colorType == CL_ARGB1555 || m_index.colorType == CL_RGB565) {
        colorSize = 2;
    }

    std::shared_ptr<NPKMatrix> retMatrix;
    if (m_index.compressType == CP_ZLIB || m_index.compressType == CP_ZLIB2) {
        // 根据颜色类型，尺寸计算解压后的大小
        unsigned long dataSize = m_index.width * m_index.height * colorSize;
        // 解压
        auto* data = new uint8_t[dataSize];

        int ret = uncompress(data, &dataSize, m_data, m_index.dataSize);
        if (ret != Z_OK) {
            if (ret == Z_BUF_ERROR) {
                LOG_WARNING << "Failed to uncompress data. Buffer is too small.";
            } else if (ret == Z_DATA_ERROR) {
                LOG_WARNING << "Failed to uncompress data. Data is corrupted.";
            } else {
                LOG_WARNING << "Failed to uncompress data.";
            }
            delete[] data;
            return nullptr;
        }

        if (m_paletteManager == nullptr) {
            retMatrix = toMatrixV2(data);
        } else {
            retMatrix = toMatrixV4V6(data, paletteIndex);
        }
        delete[] data;
    } else {
        if (m_paletteManager == nullptr) {
            retMatrix = toMatrixV2(m_data);
        } else {
            retMatrix = toMatrixV4V6(m_data, paletteIndex);
        }
    }
    return retMatrix;
}

std::shared_ptr<NPKMatrix> NPKFrameHandler::ddsClipMatrix(std::shared_ptr<NPKMatrix>&& matrix)
{
    return matrix->clip(m_index.ddsLeftEdge, m_index.ddsTopEdge, m_index.ddsRightEdge, m_index.ddsBottomEdge);
}

std::shared_ptr<NPKMatrix> NPKFrameHandler::toMatrixV2(const uint8_t* data)
{
    uint32_t colorSize = 4;
    if (m_index.colorType == CL_ARGB4444 || m_index.colorType == CL_ARGB1555 || m_index.colorType == CL_RGB565) {
        colorSize = 2;
    }
    auto matrix = NPKMatrix::createMatrix(m_index.width, m_index.height, m_index.frameWidth, m_index.frameHeight, m_index.posX, m_index.posY);
    for (int x = 0; x < m_index.width; ++x) {
        for (int y = 0; y < m_index.height; ++y) {
            int index = (y * m_index.width + x) * colorSize;
            NPKColor color{0};
            if (m_index.colorType == CL_ARGB8888) {
                color.r = data[index + 2];
                color.g = data[index + 1];
                color.b = data[index];
                color.a = data[index + 3];
            } else if (m_index.colorType == CL_ARGB4444) {
                unsigned short temp = (data[index + 1] << 8) | data[index];
                color.a = ((temp & 0xF000) >> 12) * 0x11;
                color.r = (temp & 0x0F00) >> 8 << 4;
                color.g = (temp & 0x00F0) >> 4 << 4;
                color.b = temp & 0x000F << 4;
            } else if (m_index.colorType == CL_ARGB1555) {
                unsigned short temp = (data[index + 1] << 8) | data[index];
                color.r = (temp & 0x7C00) >> 10 << 3;
                color.g = (temp & 0x03E0) >> 5 << 3;
                color.b = (temp & 0x001F) << 3;
                color.a = ((temp & 0x8000) >> 15) * 0xFF;
            } else if (m_index.colorType == CL_RGB565) {
                unsigned short temp = (data[index + 1] << 8) | data[index];
                color.r = (temp & 0xF800) >> 11 << 3;
                color.g = (temp & 0x07E0) >> 5 << 2;
                color.b = temp & 0x001F << 3;
                color.a = 0xFF;
            } else {
                LOG_ERROR << "Unsupported color type: " << m_index.colorType;
                return nullptr;
            }
            matrix->setPixel(x, y, color);
        }
    }
    return matrix;
}

std::shared_ptr<NPKMatrix> NPKFrameHandler::toMatrixV4V6(const uint8_t* data, int paletteIndex)
{
    // 对于V4和V6版本，为1字节的索引，索引到调色板中的颜色
    auto matrix = NPKMatrix::createMatrix(m_index.width, m_index.height, m_index.frameWidth, m_index.frameHeight, m_index.posX, m_index.posY);
    for (uint32_t y = 0; y < m_index.height; ++y) {
        for (uint32_t x = 0; x < m_index.width; ++x) {
            uint8_t index = data[y * m_index.width + x];
            NPKColor color = m_paletteManager->getColor(paletteIndex, index);
            matrix->setPixel(x, y, color);
        }
    }
    return matrix;
}
} // neapu_ex_npk