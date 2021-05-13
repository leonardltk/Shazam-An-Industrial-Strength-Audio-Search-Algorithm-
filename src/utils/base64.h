#pragma once
#include <stdio.h>
#include <sstream>
#include <vector>

namespace bigoai {

static int Base64Encode(void *_dst, const void *_src, size_t len);
inline std::string base64encode(const std::string &s) {
    std::string dst;
    dst.resize((s.size() / 3 + 1) * 4);
    dst.resize(Base64Encode(const_cast<char *>(dst.data()), s.data(), s.size()));
    return dst;
}

static const unsigned char *EnBase64 =
    (const unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
inline int Base64Encode(void *_dst, const void *_src, size_t len) {
    const unsigned char *src = (const unsigned char *)_src;
    unsigned char *dst = (unsigned char *)_dst;
    unsigned char *odst = dst;

    unsigned long l, m, n;

    for (; len >= 3; len -= 3) {
        l = *src++, m = *src++, n = *src++;
        *dst++ = EnBase64[l >> 2];
        *dst++ = EnBase64[((l << 4) | (m >> 4)) & 63];
        *dst++ = EnBase64[((m << 2) | (n >> 6)) & 63];
        *dst++ = EnBase64[n & 63];
    }

    switch (len) {
        case 1:
            l = *src++;
            *dst++ = EnBase64[l >> 2];
            *dst++ = EnBase64[(l << 4) & 63];
            *dst++ = '=';
            *dst++ = '=';
            break;

        case 2:
            l = *src++, m = *src++;
            *dst++ = EnBase64[l >> 2];
            *dst++ = EnBase64[((l << 4) | (m >> 4)) & 63];
            *dst++ = EnBase64[(m << 2) & 63];
            *dst++ = '=';
            break;
    }
    return (int)(dst - odst);
}

inline std::string Base64Decode(const char *Data, int DataByte, int &OutByte) {
    //解码表
    const char DecodeTable[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        62, // '+'
        0,  0,  0,
        63, // '/'
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, // '0'-'9'
        0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, // 'A'-'Z'
        0,  0,  0,  0,  0,  0,  26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
        42, 43, 44, 45, 46, 47, 48, 49, 50, 51, // 'a'-'z'
    };
    //返回值
    std::string strDecode;
    int nValue;
    int i = 0;
    while (i < DataByte) {
        if (*Data != '\r' && *Data != '\n') {
            nValue = DecodeTable[(unsigned char)(*Data++)] << 18;
            nValue += DecodeTable[(unsigned char)(*Data++)] << 12;
            strDecode += (nValue & 0x00FF0000) >> 16;
            OutByte++;
            if (*Data != '=') {
                nValue += DecodeTable[(unsigned char)(*Data++)] << 6;
                strDecode += (nValue & 0x0000FF00) >> 8;
                OutByte++;
                if (*Data != '=') {
                    nValue += DecodeTable[(unsigned char)(*Data++)];
                    strDecode += nValue & 0x000000FF;
                    OutByte++;
                }
            }
            i += 4;
        } else // 回车换行,跳过
        {
            Data++;
            i++;
        }
    }
    return strDecode;
}

inline std::string Base64Decode(const std::string &data) {
    int _;
    return Base64Decode(data.c_str(), data.size(), _);
}

static int safeBase64Encode(void *_dst, const void *_src, size_t len);
inline std::string safeBase64encode(const std::string &s) {
    std::string dst;
    dst.resize((s.size() / 3 + 1) * 4);
    dst.resize(safeBase64Encode(const_cast<char *>(dst.data()), s.data(), s.size()));
    return dst;
}

static const unsigned char *safeEnBase64 =
    (const unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
inline int safeBase64Encode(void *_dst, const void *_src, size_t len) {
    const unsigned char *src = (const unsigned char *)_src;
    unsigned char *dst = (unsigned char *)_dst;
    unsigned char *odst = dst;

    unsigned long l, m, n;

    for (; len >= 3; len -= 3) {
        l = *src++, m = *src++, n = *src++;
        *dst++ = safeEnBase64[l >> 2];
        *dst++ = safeEnBase64[((l << 4) | (m >> 4)) & 63];
        *dst++ = safeEnBase64[((m << 2) | (n >> 6)) & 63];
        *dst++ = safeEnBase64[n & 63];
    }

    switch (len) {
        case 1:
            l = *src++;
            *dst++ = safeEnBase64[l >> 2];
            *dst++ = safeEnBase64[(l << 4) & 63];
            *dst++ = '=';
            *dst++ = '=';
            break;

        case 2:
            l = *src++, m = *src++;
            *dst++ = safeEnBase64[l >> 2];
            *dst++ = safeEnBase64[((l << 4) | (m >> 4)) & 63];
            *dst++ = safeEnBase64[(m << 2) & 63];
            *dst++ = '=';
            break;
    }
    return (int)(dst - odst);
}


}
