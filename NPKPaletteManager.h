//
// Created by liu86 on 24-7-22.
//

#ifndef NPKPALETTE_H
#define NPKPALETTE_H

#include "NPKPublic.h"
#include <vector>
#include <memory>

namespace neapu_ex_npk {
class NPKPaletteManager {
public:
    NPKPaletteManager();
    virtual ~NPKPaletteManager() = default;

    // void setPalette(int index, const uint8_t* colorData, int colorCount);
    int loadPalettes(const uint8_t* data, uint64_t dataLen, uint32_t version);
    NPKColor getColor(int paletteIndex, int colorIndex) const;

    int paletteCount() const { return m_paletteCount; }

private:
    typedef struct NPKPalette {
        NPKPalette() = default;
        ~NPKPalette();

        int loadPalette(const uint8_t* data, int dataLen);

        int m_colorCount{0};
        NPKColor* m_colors{nullptr};
    } NPKPalette;

private:
    int m_paletteCount{0};
    std::vector<std::shared_ptr<NPKPalette>> m_palette;
};
} // neapu_ex_npk

#endif //NPKPALETTE_H