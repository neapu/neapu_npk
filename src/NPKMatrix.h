//
// Created by liu86 on 24-7-22.
//

#ifndef NPKMATRIX_H
#define NPKMATRIX_H

#include <memory>
#include <vector>
#include "NPKPublic.h"

namespace neapu {
class NPKMatrix {
public:
    NPKMatrix() = default;
    virtual ~NPKMatrix();
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    uint32_t canvasWidth() const { return m_canvasWidth; }
    uint32_t canvasHeight() const { return m_canvasHeight; }
    const NPKColor* data() const { return m_data; }
    bool isEmpty() const { return m_width == 0 || m_height == 0; }
    std::vector<uint8_t> toPng() const;

    void reset(uint32_t width, uint32_t height, uint32_t canvasWidth = 0, uint32_t canvasHeight = 0, uint32_t offsetX = 0,
               uint32_t offsetY = 0);
    void setPixel(const uint32_t x, const uint32_t y, NPKColor color);

    std::shared_ptr<NPKMatrix> clip(const uint32_t left, const uint32_t top, const uint32_t right, const uint32_t bottom,
        const uint32_t canvasWidth = 0, const uint32_t canvasHeight = 0, const uint32_t offsetX = 0, const uint32_t offsetY = 0) const;

    static std::shared_ptr<NPKMatrix> createMatrix(const uint32_t width, const uint32_t height, const uint32_t canvasWidth = 0,
                                                   const uint32_t canvasHeight = 0, const uint32_t offsetX = 0, const uint32_t offsetY = 0);

private:
    uint32_t m_width{0};
    uint32_t m_height{0};
    uint32_t m_canvasWidth{0};
    uint32_t m_canvasHeight{0};
    uint32_t m_offsetX{0};
    uint32_t m_offsetY{0};
    NPKColor* m_data{nullptr};
};
} // neapu

#endif //NPKMATRIX_H