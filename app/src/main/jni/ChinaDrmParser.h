#ifndef CHINA_DRM_PARSER_H
#define CHINA_DRM_PARSER_H

#include <stdint.h>
#include "Log.h"

extern "C" {
#include "./monalisa-android-norach/include/drm.h"
#include "./davinci-mpegts/cgts_structs.h"
#include "./davinci-mpegts/cgts_demux.h"
#include "./davinci-mpegts/cgts_mux.h"
#include "./davinci-mpegts/cgts_nal_adts_parse.h"
}

struct MediaInfo {
    char  license[512];
    char inputFile[512];
    char outputFile[512];
};

enum ParseType { PARSE_FROM_FILE, PARSE_FROM_MEM };

class ChinaDrmParser {
public :
    ChinaDrmParser();

    int32_t parse(const char *inputFile, const char *outputFile, int type = PARSE_FROM_FILE);
    int32_t setLicense(const char *drmLicense, int licenseLength, const char *uid , int uidLength, int32_t instanceId = 1);
    int32_t parseTsData(const char *inputFile, const char * outputFile);
    int32_t parseTsData(uint8_t *inputBuffer, uint32_t bufferLength, const char *outputFile);
    bool parseMediaInfo(const char *infoFile, MediaInfo &info);
    bool trim(char *szBuffer);
private:

    int32_t coreParse(cgts_demux_context * demux_ct, cgts_mux_context * mux_ct);
    int32_t parseReturnCode(int retCode);
    int32_t decryptData(uint8_t* srcBuffer, uint32_t srcBufferLength, uint8_t* destBuffer, uint32_t& destBufferLength);
    int32_t handleVideoPesPayload(struct cgts_demux_context* demux_ct, struct cgts_pid_buffer* packet);
    uint32_t nalUnescape(uint8_t *destBuffer, uint8_t *srcBuffer, uint32_t srcBufferLength);

private:
    bool  mInitialized;
    char mDrmLicense[512];
    char mDrmUid[256];
    uint8_t *mInitFileData;
    int32_t     mDrmInstanceId;
};

/*
class ChinaDrmParser {
public:
    ChinaDrmParser();
    ~ChinaDrmParser();

   int32_t setLicense(const char *drmLicense, int licenseLength, const char *uid , int uidLength, int32_t instanceId = 1);

   int32_t parseTsData(const char *inputFile, const char * outputFile);
   //bool parseMediaInfo(const char *infoFile, MediaInfo &info);
    private:
        int32_t parseReturnCode(int retCode);


        int32_t decryptData(uint8_t* srcBuffer, uint32_t srcBufferLength,
                         uint8_t* destBuffer, uint32_t& destBufferLength);


        int32_t handleVideoPesPayload(struct cgts_demux_context* demux_ct, struct cgts_pid_buffer* packet);


        uint32_t nalUnescape(uint8_t *destBuffer, uint8_t *srcBuffer, uint32_t srcBufferLength);


    //std::string trim(std::string &str);

    private:
        bool  mInitialized;
        char mDrmLicense[512];
        char mDrmUid[256];
        int32_t     mDrmInstanceId;
};
*/
#endif