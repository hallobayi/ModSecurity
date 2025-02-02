/*
 * ModSecurity, http://www.modsecurity.org/
 * Copyright (c) 2015 - 2021 Trustwave Holdings, Inc. (http://www.trustwave.com/)
 *
 * You may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * If any of the files related to licensing are missing or if you have any
 * other questions related to licensing please contact Trustwave Holdings, Inc.
 * directly using the email address security@modsecurity.org.
 *
 */

#include "src/utils/base64.h"

#include <string.h>

#include <string>
#include <fstream>
#include <iostream>

#include "mbedtls/base64.h"

template<typename Operation>
inline std::string base64Helper(const char *data, const unsigned int len, Operation op) { // cppcheck-suppress syntaxError ; false positive
    size_t out_len = 0;

    op(nullptr, 0, &out_len,
        reinterpret_cast<const unsigned char *>(data), len);

    std::string ret(out_len, {});
    if(out_len > 0) {
        op(reinterpret_cast<unsigned char *>(ret.data()), ret.size(), &out_len,
            reinterpret_cast<const unsigned char *>(data), len);

        ret.resize(out_len);
    }

    return ret;
}

namespace modsecurity {
namespace Utils {


std::string Base64::encode(const std::string& data) {
    return base64Helper(data.c_str(), data.size(), mbedtls_base64_encode);
}


std::string Base64::decode(const std::string& data, bool forgiven) {
    if (forgiven) {
        return decode_forgiven(data);
    }

    return decode(data);
}


std::string Base64::decode(const std::string& data) {
    return base64Helper(data.c_str(), strlen(data.c_str()), mbedtls_base64_decode);
}


std::string Base64::decode_forgiven(const std::string& data) {
    return base64Helper(data.c_str(), data.size(), decode_forgiven_engine);
}


void Base64::decode_forgiven_engine(unsigned char *plain_text,
        size_t plain_text_size, size_t *aiming_size,
        const unsigned char *encoded, size_t input_len) {
    int i = 0, j = 0, k = 0;
    int ch = 0;
    static const char b64_pad = '=';
    static const int b64_reverse_t[256] = {
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
        -2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
        -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
        -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
    };


    while (/*ch = *encoded++ != '\0' && */input_len-- > 0) {
        ch = *encoded++;
        if (ch == b64_pad) {
            if (*encoded != '=' && (i % 4) == 1) {
                *aiming_size = 0;
                return;
            }
            continue;
        }

        ch = b64_reverse_t[ch];
        if (ch < 0) {
            continue;
        /*} else if (ch == -2) {
            *aiming_size = 0;
            return;*/
        }
        switch (i % 4) {
            case 0:
                if (plain_text_size != 0) {
                    plain_text[j] = ch << 2;
                }
                break;
            case 1:
                if (plain_text_size == 0) {
                    j++;
                } else {
                    plain_text[j++] |= ch >> 4;
                    plain_text[j] = (ch & 0x0f) << 4;
                }
                break;
            case 2:
                if (plain_text_size == 0) {
                    j++;
                } else {
                    plain_text[j++] |= ch >>2;
                    plain_text[j] = (ch & 0x03) << 6;
                }
                break;
            case 3:
                if (plain_text_size == 0) {
                    j++;
                } else {
                    plain_text[j++] |= ch;
                }
                break;
        }
        i++;
    }
    k = j;

    if (ch == b64_pad) {
        switch (i % 4) {
            case 1:
                *aiming_size = 0;
                return;
            case 2:
                k++;
            case 3:
                if (plain_text_size != 0) {
                    plain_text[k] = 0;
                }
        }
    }

    if (plain_text_size != 0) {
        plain_text[j] = '\0';
    }

    if (plain_text_size == 0) {
        *aiming_size = k + 1;
    } else {
        *aiming_size = j;
    }
}

}  // namespace Utils
}  // namespace modsecurity
