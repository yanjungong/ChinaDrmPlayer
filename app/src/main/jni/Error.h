#ifndef ERROR_H
#define ERROR_H
#include <stdint.h>
class Error {
public:
//ChinaDRM错误码: 406XXX(monalisa库报错例外, 仍用原始错误码)
    static const int32_t ERROR_CODE_CHINA_DRM_BASE                        = 406000;
    static const int32_t ERROR_CODE_CHINA_DRM_M3U8_FAILED                 = 406001;      // M3U8 parsing failed
    static const int32_t ERROR_CODE_CHINA_DRM_TS_FAILED                   = 406002;      // TS paring failed
    static const int32_t ERROR_CODE_CHINA_DRM_BUFFER_ERR                  = 406003;      // Buffer error
    static const int32_t ERROR_CODE_CHINA_DRM_LICENSE_EMPTY               = 406004;      // License is empty
    /* monalisa error*/
    static const int32_t ERROR_CODE_CHINA_DRM_SUCCESS                     = 1;
    static const int32_t ERROR_CODE_CHINA_DRM_LICENSE_FORMAT              = 4011;        // unsupport license format
    static const int32_t ERROR_CODE_CHINA_DRM_LICENSE_PARSE               = 4012;        // license parse error
    static const int32_t ERROR_CODE_CHINA_DRM_LICENSE_UID                 = 4013;        // uid wrong
    static const int32_t ERROR_CODE_CHINA_DRM_DECRYPT_BUFFER              = 4021;        // buf length too short

};

#endif