//
// Created by liu86 on 24-7-20.
//

#include "NPKImageHandler.h"
#include "NPKPaletteManager.h"
#include "NPKFrameHandler.h"
#include "NPKDDSHandler.h"

#include "logger.h"

namespace neapu_ex_npk {
int NPKImageHandler::loadIndex(const uint8_t* data, const uint64_t dataLen)
{
    if (dataLen < sizeof(NPKImageHeader)) {
        LOG_ERROR << "Data length is too short.";
        return -1;
    }

    int ret = memcpy_s(&m_index, sizeof(m_index), data, sizeof(m_index));
    if (ret != 0) {
        LOG_ERROR << "Failed to copy header.";
        return -1;
    }

    return sizeof(m_index);
}

int NPKImageHandler::loadData(const uint8_t* npkSourceData, const uint64_t dataLen)
{
    if (m_index.offset + m_index.size > dataLen) {
        LOG_ERROR << "Data length is too short.";
        return -1;
    }

    const uint8_t* data = npkSourceData + m_index.offset;
    return loadNPKImage(data, m_index.size);
}

std::string NPKImageHandler::getName() const
{
    return m_name;
}

std::string NPKImageHandler::getShortName() const
{
    return m_shortName;
}

std::shared_ptr<NPKFrameHandler> NPKImageHandler::getFrame(const uint32_t index) const
{
    if (index >= m_frames.size()) {
        return nullptr;
    }
    return m_frames[index];
}

int NPKImageHandler::getPalletCount() const
{
    if (m_paletteManager) {
        return m_paletteManager->paletteCount();
    }
    return 0;
}

ColorType NPKImageHandler::getFrameColorType(const uint32_t index) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return ColorType::CL_UNKNOWN;
    }

    return frame->colorType();
}

int NPKImageHandler::getFrameWidth(const uint32_t index) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return 0;
    }

    return frame->width();
}

int NPKImageHandler::getFrameHeight(const uint32_t index) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return 0;
    }

    return frame->height();
}

std::shared_ptr<NPKMatrix> NPKImageHandler::getFrameMatrix(const uint32_t index, const int paletteIndex) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return nullptr;
    }

    if (frame->isMatrixFrame()) {
        return m_frames[index]->toMatrix(paletteIndex);
    } else if (frame->isDDSFrame()) {
        auto ddsIndex = frame->ddsIndex();
        if (ddsIndex >= m_ddsHandlers.size()) {
            LOG_ERROR << "Invalid DDS index. [index:" << ddsIndex << "][size:" << m_ddsHandlers.size() << "]";
            return nullptr;
        }
        auto ddsMatrix = m_ddsHandlers[ddsIndex]->toMatrix();
        return frame->ddsClipMatrix(std::move(ddsMatrix));
    }
    return nullptr;
}

bool NPKImageHandler::getFrameIsLink(const uint32_t index) const
{
    if (index >= m_frames.size()) {
        return false;
    }

    return m_frames[index]->isLinkFrame();
}

std::string NPKImageHandler::getFrameLinkInfo(const uint32_t index) const
{
    if (index >= m_frames.size()) {
        return "";
    }
    uint32_t pos = index;
    std::string ret = std::to_string(pos);
    for (uint32_t i = 0; i < 2; i++) {
        if (m_frames[pos]->isLinkFrame()) {
            pos = m_frames[pos]->linkTo();
            ret += " -> " + std::to_string(pos);
        }
    }
    return ret;
}

bool NPKImageHandler::getFrameIsDDS(uint32_t index) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return false;
    }

    return frame->isDDSFrame();
}

uint32_t NPKImageHandler::getFrameDDSIndex(const uint32_t index) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return 0;
    }

    if (!frame->isDDSFrame()) {
        return 0;
    }

    return frame->ddsIndex();
}

std::string NPKImageHandler::getFrameDDSClipInfo(const uint32_t index) const
{
    const auto frame = traceFrame(index, 0);
    if (!frame) {
        return "";
    }

    return frame->ddsClipInfo();
}

int NPKImageHandler::loadNPKImage(const uint8_t* data, const uint32_t dataLen)
{
    uint32_t offset = 0;

    if (dataLen < sizeof(NPKImageHeader)) {
        LOG_ERROR << "Data length is too short. " << getName();
        return -1;
    }
    int ret = memcpy_s(&m_header, sizeof(NPKImageHeader), data, sizeof(NPKImageHeader));
    if (ret != 0) {
        LOG_ERROR << "Failed to read image header. " << getName();
        return -1;
    }
    offset += sizeof(NPKImageHeader);

    const char mask[] =
        "puchikon@neople dungeon and fighter DNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNFDNF";
    char temp[256];
    for (int i = 0; i < 256; i++) {
        temp[i] = m_index.name[i] ^ mask[i];
    }
    m_name = temp;
    m_shortName = m_name.substr(m_name.find_last_of('/') + 1);

    if (version() == 5) {
        ret = memcpy_s(&m_v5Info, sizeof(m_v5Info), data + offset, sizeof(m_v5Info));
        if (ret != 0) {
            LOG_ERROR << "Failed to read image v5 info. " << getName();
            return -1;
        }
        offset += sizeof(m_v5Info);
    }
    if (version() == 4 || version() == 5 || version() == 6) {
        m_paletteManager = std::make_shared<NPKPaletteManager>();
        uint32_t paletteSize = m_paletteManager->loadPalettes(data + offset, m_index.size - offset, version());
        if (paletteSize == 0) {
            LOG_WARNING << "Palette size is 0. " << getName();
        }
        offset += paletteSize;
    } else if (version() != 2) {
        LOG_WARNING << "Unsupported version: " << version() << " " << getName();
        return offset; // 不支持的Image版本当做空Image处理
    }

    // DDS索引表
    if (version() == 5) {
        for (uint32_t i = 0; i < m_v5Info.ddsIndexCount; i++) {
            auto dds = std::make_shared<NPKDDSHandler>();
            const int64_t len = dds->loadIndex(data + offset, m_index.size - offset);
            if (len < 0) {
                LOG_ERROR << "Failed to load DDS index. " << getName();
                return -1;
            }
            offset += len;
            m_ddsHandlers.push_back(dds);
        }
    }

    // 读取索引表
    for (uint32_t i = 0; i < m_header.frameIndexCount; i++) {
        auto frame = std::make_shared<NPKFrameHandler>(m_paletteManager);
        const int len = frame->loadIndex(data + offset, m_index.size - offset);
        if (len < 0) {
            LOG_ERROR << "Failed to load frame index. " << getName();
            return -1;
        }
        offset += len;
        m_frames.push_back(frame);
    }

    if (version() == 5) {
        for (uint32_t i = 0; i < m_v5Info.ddsIndexCount; ++i) {
            if (offset >= m_index.size) {
                LOG_WARNING << "Data length is too short. " << getName();
                break;
            }
            const auto& dds = m_ddsHandlers[i];
            const int64_t len = dds->loadData(data + offset, m_index.size - offset);
            if (len < 0) {
                LOG_ERROR << "Failed to load DDS data. " << getName();
                return -1;
            }
            offset += len;
        }
    }


        // 点阵图片
        for (uint32_t i = 0; i < m_header.frameIndexCount; i++) {
            if (offset >= m_index.size) {
                LOG_WARNING << "Data length is too short. " << getName();
                break;
            }
            const auto frame = m_frames[i];
            if (!frame->isMatrixFrame()) {
                continue;
            }
            const int len = frame->loadData(data + offset, m_index.size - offset);
            if (len < 0) {
                LOG_ERROR << "Failed to load frame data. [name:" << getName() << "][frame:" << i
                    << "][version:" << version() << "][size:" << offset << "/" << m_index.size << "]";
                return -1;
            }
            offset += len;
        }


    return true;
}

std::shared_ptr<NPKFrameHandler> NPKImageHandler::traceFrame(uint32_t index, uint32_t deep) const
{
    // DNF中，链接帧的深度最多为2
    if (index >= m_frames.size() || deep > 2) {
        return nullptr;
    }

    auto frame = m_frames[index];
    if (frame->isLinkFrame()) {
        return traceFrame(frame->linkTo(), deep + 1);
    }
    return frame;
}
} // neapu_ex_npk