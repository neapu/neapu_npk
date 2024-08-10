//
// Created by liu86 on 24-7-20.
//

#ifndef NPKIMAGEHANDLER_H
#define NPKIMAGEHANDLER_H
#include <cstdint>
#include <memory>
#include <vector>

#include "NPKPublic.h"

namespace neapu_ex_npk {
#pragma pack(push, 1)
typedef struct NPKImageIndex {
    uint32_t offset;
    uint32_t size;
    char name[256];
} NPKImageIndex;

typedef struct NPKImageHeader {
    char magic[16];
    uint32_t frameIndexSize; // 索引表大小
    uint32_t unknown1;
    uint32_t version;
    uint32_t frameIndexCount; // 索引表数量
} NPKImageHeader;

typedef struct NPKImageV5Info {
    uint32_t ddsIndexCount;
    uint32_t imageSize;
} NPKImageV5Info;
#pragma pack(pop)
class NPKFrameHandler;
class NPKPaletteManager;
class NPKMatrix;
class NPKDDSHandler;
class NPKImageHandler {
    friend class NPKHandler;
public:
    NPKImageHandler() = default;
    virtual ~NPKImageHandler() = default;

    /**
     * @brief loadIndex
     * @param data 索引表数据，需偏移到索引表开始位置
     * @param dataLen 数据最大长度
     * @return 成功返回索引表大小，失败返回-1
     */
    int loadIndex(const uint8_t* data, uint64_t dataLen);
    /**
     * @brief loadData
     * @param npkSourceData NPK原始数据，因为所以中包含了img偏移量，所以输入为从0偏移开始的NPK数据
     * @param dataLen 数据最大长度
     * @return 成功返回读取的长度，失败返回-1
     */
    int loadData(const uint8_t* npkSourceData, uint64_t dataLen);

    std::string getName() const;
    std::string getShortName() const;

    int version() const { return m_header.version; }

    uint32_t getFrameCount() const { return m_frames.size(); }

    [[deprecated("Use getFrame*() instead")]]
    std::shared_ptr<NPKFrameHandler> getFrame(uint32_t index) const;

    int getPalletCount() const;

    ColorType getFrameColorType(uint32_t index) const;
    int getFrameWidth(uint32_t index) const;
    int getFrameHeight(uint32_t index) const;
    std::shared_ptr<NPKMatrix> getFrameMatrix(uint32_t index, int paletteIndex = 0) const;
    bool getFrameIsLink(uint32_t index) const;
    std::string getFrameLinkInfo(uint32_t index) const;
    bool getFrameIsDDS(uint32_t index) const;
    uint32_t getFrameDDSIndex(uint32_t index) const;
    std::string getFrameDDSClipInfo(uint32_t index) const;

private:
    int loadNPKImage(const uint8_t* data, uint32_t dataLen);
    std::shared_ptr<NPKFrameHandler> traceFrame(uint32_t index, uint32_t deep) const;

private:
    NPKImageIndex m_index{0};
    NPKImageHeader m_header{0};
    NPKImageV5Info m_v5Info{0};
    std::vector<std::shared_ptr<NPKFrameHandler>> m_frames;
    std::vector<std::shared_ptr<NPKDDSHandler>> m_ddsHandlers;

    std::string m_name{};
    std::string m_shortName{};
    std::shared_ptr<NPKPaletteManager> m_paletteManager{nullptr};
};

} // neapu_ex_npk

#endif //NPKIMAGEHANDLER_H
