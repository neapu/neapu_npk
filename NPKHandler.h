//
// Created by liu86 on 24-7-20.
//

#ifndef NPKLOADER_H
#define NPKLOADER_H
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <string>

namespace neapu_ex_npk {
using funcSHA256 = std::function<bool(const uint8_t* source, const uint64_t sourceLen, uint8_t* dst, const uint64_t dstLen)>;
#pragma pack(push, 1)
typedef struct NPKHeader {
    char magic[16];
    uint32_t imgCount;
} NPKHeader;
#pragma pack(pop)

class NPKHandler {
    friend class NPKImageHandler;
public:
    NPKHandler() = default;
    virtual ~NPKHandler() = default;
    /**
     * @brief 加载NPK文件
     * @param path NPK文件路径
     * @return 成功返回true，失败返回false
     */
    bool loadNPK(const std::string& path);
    uint32_t getImageCount() const;
    std::shared_ptr<NPKImageHandler> getImage(uint32_t index) const;
    const std::vector<std::shared_ptr<NPKImageHandler>>& getImages() const { return m_images; }
    std::string getNpkName() const { return m_fileName; }

    static funcSHA256 sha256;
private:
    std::string m_fileName;

    NPKHeader m_header{0};
    std::vector<std::shared_ptr<NPKImageHandler>> m_images;
};
}


#endif //NPKLOADER_H