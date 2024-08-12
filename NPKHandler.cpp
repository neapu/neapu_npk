//
// Created by liu86 on 24-7-20.
//

#include "NPKHandler.h"
#include <cstdint>
#include <logger.h>
#include "NPKImageHandler.h"
#ifndef _WIN32
#define fopen_s(pFile, filename, mode) (((*(pFile)) = fopen((filename), (mode))) == NULL)
#endif
#ifdef USE_OPENSSL
#include <openssl/evp.h>
#endif

namespace neapu {
#ifdef USE_OPENSSL
funcSHA256 NPKHandler::sha256 = [](const uint8_t* source, const uint64_t sourceLen, uint8_t* dst, const uint64_t dstLen) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (ctx == nullptr) {
        LOG_ERROR << "Failed to create EVP_MD_CTX";
        return false;
    }

    const EVP_MD* md = EVP_sha256();
    if (md == nullptr) {
        LOG_ERROR << "Failed to get EVP_sha256";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    int ret = EVP_DigestInit_ex(ctx, md, nullptr);
    if (ret != 1) {
        LOG_ERROR << "Failed to init digest";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    ret = EVP_DigestUpdate(ctx, source, sourceLen);
    if (ret != 1) {
        LOG_ERROR << "Failed to update digest";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    auto mdLen = static_cast<unsigned int>(dstLen);
    ret = EVP_DigestFinal_ex(ctx, dst, &mdLen);
    if (ret != 1) {
        LOG_ERROR << "Failed to final digest";
        EVP_MD_CTX_free(ctx);
        return false;
    }

    EVP_MD_CTX_free(ctx);
    return true;
};
#else
funcSHA256 NPKHandler::sha256 = nullptr;
#endif

bool NPKHandler::loadNPK(const std::string& path)
{
    if (sha256 == nullptr) {
        LOG_ERROR << "SHA256 function is not set";
        return false;
    }
    FILE* file = nullptr;
    int ret = fopen_s(&file, path.c_str(), "rb");
    if (ret != 0) {
        LOG_ERROR << "Failed to open file: " << path;
        return false;
    }

    uint64_t fileSize = 0;
    _fseeki64(file, 0, SEEK_END);
    fileSize = _ftelli64(file);
    _fseeki64(file, 0, SEEK_SET);
    auto* buffer = new uint8_t[fileSize];
    ret = fread(buffer, 1, fileSize, file);
    fclose(file);
    if (ret != fileSize) {
        LOG_ERROR << "Failed to read file: " << path;
        delete[] buffer;
        return false;
    }

    ret = memcpy_s(&m_header, sizeof(m_header), buffer, sizeof(m_header));
    if (ret != 0) {
        LOG_ERROR << "Failed to copy header";
        delete[] buffer;
        return false;
    }

    static constexpr char magic[] = "NeoplePack_Bill";
    if (memcmp(m_header.magic, magic, sizeof(magic)) != 0) {
        LOG_ERROR << "Magic is not correct";
        delete[] buffer;
        return false;
    }

    uint64_t verifyOffset = sizeof(NPKHeader) + m_header.imgCount * sizeof(NPKImageIndex);
    uint64_t verifySize = (verifyOffset / 17) * 17;
    uint8_t verify[32];
    ret = memcpy_s(verify, sizeof(verify), buffer + verifyOffset, sizeof(verify));
    if (ret != 0) {
        LOG_ERROR << "Failed to copy verify";
        delete[] buffer;
        return false;
    }

    uint8_t sha256[32];
    if (!NPKHandler::sha256(buffer, verifySize, sha256, sizeof(sha256))) {
        LOG_ERROR << "Failed to calculate sha256";
        delete[] buffer;
        return false;
    }

    if (memcmp(sha256, verify, sizeof(sha256)) != 0) {
        LOG_ERROR << "SHA256 verify failed";
        delete[] buffer;
        return false;
    }

    // 读取img
    uint64_t offset = sizeof(NPKHeader);
    for (uint32_t i = 0; i < m_header.imgCount; ++i) {
        auto image = std::make_shared<NPKImageHandler>();
        ret = image->loadIndex(buffer + offset, fileSize - offset);
        if (ret < 0) {
            LOG_ERROR << "Failed to load index";
            delete[] buffer;
            return false;
        }
        offset += ret;
        ret = image->loadData(buffer, fileSize);
        if (ret < 0) {
            LOG_ERROR << "Failed to load data. index: " << i;
            delete[] buffer;
            return false;
        }
        m_images.push_back(image);
    }

    delete[] buffer;
    m_fileName = path.substr(path.find_last_of('/') + 1);
    return true;
}

uint32_t neapu::NPKHandler::getImageCount() const
{
    return m_header.imgCount;
}

std::shared_ptr<neapu::NPKImageHandler> neapu::NPKHandler::getImage(uint32_t index) const
{
    if (index >= m_images.size()) {
        return nullptr;
    }

    return m_images[index];
}
}