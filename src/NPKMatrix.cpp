//
// Created by liu86 on 24-7-22.
//

#include "NPKMatrix.h"
#include "logger.h"
#include <png.h>

namespace neapu {
NPKMatrix::~NPKMatrix()
{
    if (m_data) {
        delete[] m_data;
    }
}

std::vector<uint8_t> NPKMatrix::toPng() const
{
    FUNC_TRACE;
    std::vector<uint8_t> pngData;
    if (isEmpty()) {
        return pngData;
    }

    auto pngPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!pngPtr) {
        LOG_ERROR << "Failed to create png write struct.";
        return pngData;
    }

    auto infoPtr = png_create_info_struct(pngPtr);
    if (!infoPtr) {
        png_destroy_write_struct(&pngPtr, nullptr);
        LOG_ERROR << "Failed to create png info struct.";
        return pngData;
    }

    if (setjmp(png_jmpbuf(pngPtr))) {
        png_destroy_write_struct(&pngPtr, &infoPtr);
        LOG_ERROR << "Failed to setjmp.";
        return pngData;
    }

    // PNG文件头
    png_set_IHDR(pngPtr, infoPtr, m_canvasWidth, m_canvasHeight, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

    // 绑定输出流到内存
    png_set_write_fn(pngPtr, &pngData, [](png_structp pngPtr, png_bytep data, png_size_t length) {
        auto pngData = reinterpret_cast<std::vector<uint8_t>*>(png_get_io_ptr(pngPtr));
        std::copy(data, data + length, std::back_inserter(*pngData));
    }, nullptr);

    // 将图像数据写入内存缓冲区
    png_write_info(pngPtr, infoPtr);
    for (uint32_t y = 0; y < m_canvasHeight; ++y) {
        png_write_row(pngPtr, reinterpret_cast<png_bytep>(m_data + y * m_canvasWidth));
    }
    png_write_end(pngPtr, infoPtr);

    // 释放资源
    png_destroy_write_struct(&pngPtr, &infoPtr);
    return pngData;
}

void NPKMatrix::reset(uint32_t width, uint32_t height, uint32_t canvasWidth, uint32_t canvasHeight, uint32_t offsetX, uint32_t offsetY)
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
                                           const uint32_t canvasWidth, const uint32_t canvasHeight, const uint32_t offsetX,
                                           const uint32_t offsetY) const
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

std::shared_ptr<NPKMatrix> NPKMatrix::createMatrix(const uint32_t width, const uint32_t height, const uint32_t canvasWidth,
                                                   const uint32_t canvasHeight, const uint32_t offsetX, const uint32_t offsetY)
{
    auto matrix = std::make_shared<NPKMatrix>();
    matrix->reset(width, height, canvasWidth, canvasHeight, offsetX, offsetY);
    return matrix;
}
} // namespace neapu