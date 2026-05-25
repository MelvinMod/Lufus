#include "hash.h"
#include <openssl/evp.h>
#include <openssl/md5.h>

static int do_hash(EVP_MD_CTX* ctx, const EVP_MD* md, FILE* f, uint8_t* sum)
{
    unsigned char buf[65536];
    size_t n;
    unsigned int len = 0;

    EVP_DigestInit_ex(ctx, md, NULL);
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        EVP_DigestUpdate(ctx, buf, n);
    }
    EVP_DigestFinal_ex(ctx, sum, &len);
    return len;
}

int HashFile(unsigned type, const char* path, uint8_t* sum)
{
    FILE* f;
    EVP_MD_CTX* ctx;
    int len = 0;

    f = fopen(path, "rb");
    if (!f) return 0;

    ctx = EVP_MD_CTX_new();
    if (!ctx) {
        fclose(f);
        return 0;
    }

    switch (type) {
    case HASH_MD5:
        len = do_hash(ctx, EVP_md5(), f, sum);
        break;
    case HASH_SHA1:
        len = do_hash(ctx, EVP_sha1(), f, sum);
        break;
    case HASH_SHA256:
        len = do_hash(ctx, EVP_sha256(), f, sum);
        break;
    case HASH_SHA512:
        len = do_hash(ctx, EVP_sha512(), f, sum);
        break;
    default:
        break;
    }

    EVP_MD_CTX_free(ctx);
    fclose(f);
    return len;
}

int HashBuffer(unsigned type, const uint8_t* buf, const size_t len, uint8_t* sum)
{
    EVP_MD_CTX* ctx;
    unsigned int outlen = 0;

    ctx = EVP_MD_CTX_new();
    if (!ctx) return 0;

    switch (type) {
    case HASH_MD5:
        EVP_DigestInit_ex(ctx, EVP_md5(), NULL);
        EVP_DigestUpdate(ctx, buf, len);
        EVP_DigestFinal_ex(ctx, sum, &outlen);
        break;
    case HASH_SHA1:
        EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
        EVP_DigestUpdate(ctx, buf, len);
        EVP_DigestFinal_ex(ctx, sum, &outlen);
        break;
    case HASH_SHA256:
        EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
        EVP_DigestUpdate(ctx, buf, len);
        EVP_DigestFinal_ex(ctx, sum, &outlen);
        break;
    case HASH_SHA512:
        EVP_DigestInit_ex(ctx, EVP_sha512(), NULL);
        EVP_DigestUpdate(ctx, buf, len);
        EVP_DigestFinal_ex(ctx, sum, &outlen);
        break;
    default:
        break;
    }

    EVP_MD_CTX_free(ctx);
    return outlen;
}

uint8_t* StringToHash(const char* str)
{
    static uint8_t hash[64];
    size_t len = strlen(str);
    size_t i;
    if (len % 2 != 0) return NULL;
    for (i = 0; i < len / 2; i++) {
        unsigned int b;
        if (sscanf(str + i * 2, "%2x", &b) != 1)
            return NULL;
        hash[i] = (uint8_t)b;
    }
    return hash;
}

int FileMatchesHash(const char* path, const char* str)
{
    uint8_t sum[64];
    uint8_t* ref = StringToHash(str);
    int len;

    if (!ref) return 0;
    len = HashFile(HASH_SHA256, path, sum);
    if (len <= 0) return 0;
    return (memcmp(sum, ref, len) == 0);
}

int BufferMatchesHash(const uint8_t* buf, const size_t len, const char* str)
{
    uint8_t sum[64];
    uint8_t* ref = StringToHash(str);
    int outlen;

    if (!ref) return 0;
    outlen = HashBuffer(HASH_SHA256, buf, len, sum);
    if (outlen <= 0) return 0;
    return (memcmp(sum, ref, outlen) == 0);
}

int IsFileInDB(const char* path)
{
    return 0;
}

int IsBufferInDB(const unsigned char* buf, const size_t len)
{
    return 0;
}

void* HashThread(void* param)
{
    char* path = (char*)param;
    uint8_t sum[64];
    char hex[129];
    int len, i;

    if (!path) return NULL;

    uprintf("Computing hash for %s...", path);
    len = HashFile(HASH_SHA256, path, sum);
    if (len > 0) {
        for (i = 0; i < len; i++)
            snprintf(hex + i * 2, sizeof(hex) - i * 2, "%02x", sum[i]);
        uprintf("SHA256: %s", hex);
    } else {
        uprintf("Failed to compute hash");
    }

    safe_free(path);
    return NULL;
}
