//
// Created by liu86 on 24-7-22.
//

#include "NPKMatrix.h"
#include "logger.h"

namespace neapu_ex_npk {
NPKMatrix::~NPKMatrix()
{
    if (m_data) {
        delete[] m_data;
    }
}

void NPKMatrix::reset(const uint32_t width, const uint32_t height, const uint32_t canvasWidth, const uint32_t canvasHeight, const uint32_t
                       offsetX,
                       const uint32_t offsetY)
{
    if (m_data) {
        delete[] m_data;
    }
    m_width = width;
    m_height = height;
    m_canvasWidth = canvasWidth == 0 ? width : canvasWidth;
    m_canvasHeight = canvasHeight == 0 ? height : canvasHeight;
    m_offsetX = offsetX;
    m_offsetY = offsetY;
    m_data = new NPKColor[m_canvasWidth * m_canvasHeight];
}

void NPKMatrix::setPixel(const uint32_t x, const uint32_t y, const NPKColor color)
{
    if (x >= m_width || y >= m_height) {
        return;
    }

    int pos = (y + m_offsetY) * m_canvasWidth + (x + m_offsetX);
    if (pos < 0 || pos >= m_canvasWidth * m_canvasHeight) {
        return;
    }
    m_data[pos] = color;
}

std::shared_ptr<NPKMatrix> NPKMatrix::clip(const uint32_t left, const uint32_t top, const uint32_t right, const uint32_t bottom,
    const uint32_t canvasWidth, const uint32_t canvasHeight, const uint32_t offsetX, const uint32_t offsetY) const
{
    if (left >= right || top >= bottom || right > m_width || bottom > m_height) {
        LOG_ERROR << "Invalid clip area.";
        return nullptr;
    }

    auto matrix = createMatrix(right - left, bottom - top, canvasWidth, canvasHeight, offsetX, offsetY);
    for (uint32_t y = top; y < bottom; ++y) {
        for (uint32_t x = left; x < right; ++x) {
            matrix->setPixel(x - left, y - top, m_data[y * m_canvasWidth + x]);
        }
    }
    return matrix;
}

std::shared_ptr<NPKMatrix> NPKMatrix::createMatrix(const uint32_t width, const uint32_t height, const uint32_t canvasWidth, const uint32_t canvasHeight,
                                                   const uint32_t offsetX, const uint32_t offsetY)
{
    auto matrix = std::make_shared<NPKMatrix>();
    matrix->reset(width, height, canvasWidth, canvasHeight, offsetX, offsetY);
    return matrix;
}
} // neapu_ex_npk