//
// Created by liu86 on 24-7-22.
//

#include "NPKPaletteManager.h"

#include "logger.h"

namespace neapu_ex_npk {
NPKPaletteManager::NPKPaletteManager()
= default;

int NPKPaletteManager::loadPalettes(const uint8_t* data, const uint64_t dataLen, uint32_t version)
{
    if (version == 4 || version == 5) {
        m_paletteCount = 1;
        auto palette = std::make_shared<NPKPalette>();
        const int ret = palette->loadPalette(data, dataLen);
        if (ret == 0) {
            return 0;
        }
        m_palette.push_back(palette);
        return ret;
    } else if (version == 6) {
        if (dataLen < sizeof(uint32_t)) {
            LOG_WARNING << "Data length is too short.";
            return 0;
        }

        int ret = memcpy_s(&m_paletteCount, sizeof(uint32_t), data, sizeof(uint32_t));
        if (ret != 0) {
            LOG_WARNING << "Failed to read palette count.";
            return 0;
        }
        int offset = sizeof(uint32_t);
        for (int i = 0; i < m_paletteCount; ++i) {
            auto palette = std::make_shared<NPKPalette>();
            ret = palette->loadPalette(data + offset, dataLen - offset);
            if (ret == 0) {
                return 0;
            }
            m_palette.push_back(palette);
            offset += ret;
        }
        return offset;
    }
    LOG_WARNING << "Unsupported version." << version;
    return 0;
}

NPKColor NPKPaletteManager::getColor(int paletteIndex, int colorIndex) const
{
    if (paletteIndex < 0 || paletteIndex >= m_paletteCount) {
        LOG_ERROR << "Invalid palette index." << paletteIndex;
        return {};
    }

    if (colorIndex < 0 || colorIndex >= m_palette[paletteIndex]->m_colorCount) {
        LOG_WARNING << "Invalid color index." << colorIndex;
        return {};
    }

    return m_palette[paletteIndex]->m_colors[colorIndex];
}

NPKPaletteManager::NPKPalette::~NPKPalette()
{
    if (m_colors) {
        delete[] m_colors;
    }
}

int NPKPaletteManager::NPKPalette::loadPalette(const uint8_t* data, int dataLen)
{
    if (dataLen < sizeof(uint32_t)) {
        LOG_WARNING << "Data length is too short.";
        return 0;
    }

    const int ret = memcpy_s(&m_colorCount, sizeof(uint32_t), data, sizeof(uint32_t));
    if (ret != 0) {
        LOG_WARNING << "Failed to read color count.";
        return 0;
    }

    if (dataLen < (sizeof(uint32_t) + m_colorCount * 4)) {
        LOG_WARNING << "Data length is too short.";
        return 0;
    }

    m_colors = new NPKColor[m_colorCount];
    for (int i = 0; i < m_colorCount; ++i) {
        m_colors[i].a = data[i * 4 + sizeof(uint32_t)];
        m_colors[i].b = data[i * 4 + 1 + sizeof(uint32_t)];
        m_colors[i].g = data[i * 4 + 2 + sizeof(uint32_t)];
        m_colors[i].r = data[i * 4 + 3 + sizeof(uint32_t)];
    }

    return sizeof(uint32_t) + m_colorCount * 4;
}
} // neapu_ex_npk