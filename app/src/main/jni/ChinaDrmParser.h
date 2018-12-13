#ifndef CHINA_DRM_PARSER_H
#define CHINA_DRM_PARSER_H

#include <stdint.h>
#include <string>
#include "Log.h"

extern "C" {
#include "./monalisa-android-norach/include/drm.h"
#include "./davinci-mpegts/cgts_structs.h"
#include "./davinci-mpegts/cgts_demux.h"
#include "./davinci-mpegts/cgts_mux.h"
#include "./davinci-mpegts/cgts_nal_adts_parse.h"
}


struct MediaInfo {
    std::string license;
    std::string inputFile;
    std::string outputFile;
};

class ChinaDrmParser {
public:
    ChinaDrmParser();
    ~ChinaDrmParser();

   int32_t setLicense(const std::string& drmLicense, const std::string uid = "", int32_t instanceId = 1);

   bool initalized() const { return mInitialized; }
   int32_t parseTsData(const char *inputFile, const char * outputFile);
   bool parseMediaInfo(std::string infoFile, MediaInfo &info);
    private:
        int32_t parseReturnCode(int retCode);

        /*  解密China DRM数据
         * @srcBuffer：       原始加密数据BUFFER
         * @srcBufferLength： 原始加密数据BUFFER长度
         * @destBuffer：      输出解密数据BUFFER
         * @destBufferLength：传递输出缓冲区长度、输出解密后数据的BUFFER长度
         * return: > 0 if success, otherwise 0.
         */
        int32_t decryptData(uint8_t* srcBuffer, uint32_t srcBufferLength,
                         uint8_t* destBuffer, uint32_t& destBufferLength);

        /*  处理PES数据包
         * @demux_ct:   输入数据包
         * @packet:     输出解析的数据包
         * return:      ChinaDrmCode
         */
        int32_t handleVideoPesPayload(struct cgts_demux_context* demux_ct, struct cgts_pid_buffer* packet);

        /* 解密TS包前的转义，主要是为了解决加密TS段中可能出现NAL分隔符，需要特殊处理。
         * @destBuffer:         转义后输出BUFFER
         * @srcBuffer：         转义前的输入BUFFER
         * @srcBufferLength:    转义前的输入BUFFER长度
         * return: 转义后输出的内容长度。
         */
        uint32_t nalUnescape(uint8_t *destBuffer, uint8_t *srcBuffer, uint32_t srcBufferLength);


    std::string trim(std::string &str);

    private:
        bool  mInitialized;
        std::string mDrmLicense;
        std::string mDrmUid;
        int32_t     mDrmInstanceId;
};

#endif