#include "ChinaDrmParser.h"
#include "Error.h"


ChinaDrmParser::ChinaDrmParser() : mInitFileData(NULL) {
}

int32_t ChinaDrmParser::parse(const char *inputFile, const char *outputFile, int type) {
    if (type == PARSE_FROM_FILE) {
        struct cgts_demux_context * demux_ct = cgts_demux_context_alloc_with_file(inputFile);
        struct cgts_mux_context * mux_ct = cgts_mux_context_alloc_with_file(outputFile);

        UNILOGD("parse chinadrm from file");
        return coreParse(demux_ct, mux_ct);
    } else {
        FILE *f = fopen(inputFile, "rb");
        if (f == NULL) {
            UNILOGD("open file %s failed!", inputFile);
        } else {
            fseek(f, 0, SEEK_END);
            int fileSize = ftell(f);
            fseek(f, 0, SEEK_SET);

            mInitFileData = (uint8_t*)malloc(fileSize);
            uint32_t readSize = fread(mInitFileData, 1, fileSize, f);
            UNILOGD("parse chinadrm from memory, input file size:%d, read size:%d", fileSize, readSize);

            struct cgts_demux_context * demux_ct = cgts_demux_context_alloc_with_memory(mInitFileData, &readSize);
            struct cgts_mux_context * mux_ct = cgts_mux_context_alloc_with_file(outputFile);
            return coreParse(demux_ct, mux_ct);
        }
    }

    return 0;
}

int32_t ChinaDrmParser::setLicense(const char *drmLicense, int licenseLength, const char *uid , int uidLength, int32_t instanceId){
    UNILOGD("setLicense(drmLicense=%s, uid=%s, instanceId=%d)", drmLicense, uid, instanceId);
     if (drmLicense == NULL || licenseLength == 0) {
          return Error::ERROR_CODE_CHINA_DRM_LICENSE_EMPTY;
     }

    memset(mDrmLicense, 0, sizeof(mDrmLicense));
    memcpy(mDrmLicense, drmLicense, licenseLength);

    memset(mDrmUid, 0, sizeof(mDrmUid));
    memcpy(mDrmUid, uid, uidLength);

    mDrmInstanceId = instanceId;

    int32_t ret = monalisa_set_license((char*)(drmLicense), int32_t(licenseLength), instanceId,
                                   (unsigned char* )(uid));
    int32_t errorType = 0;
    if (ret != Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
      mInitialized = false;
    } else {
      mInitialized = true;
    }
    return errorType;
}

int32_t ChinaDrmParser::coreParse(cgts_demux_context * demux_ct, cgts_mux_context * mux_ct){
         UNILOGD("parseTsData, inputFile:%p, outputFile:%p", demux_ct->input_fp, mux_ct->output_fp );

         struct cgts_pid_buffer * packet = NULL;
         while(cgts_read_pxx_packet(demux_ct, &packet) == true) {
             if (NULL == packet->buf) {
                 UNILOGE("parseTsData(), cgts_read_pxx_packet get invalid packet, type=%d, EXIT!", packet->type);
                 return Error::ERROR_CODE_CHINA_DRM_TS_FAILED;
             }
             // Check PES type:
             if (packet->type != PXX_BUF_TYPE_PSI && packet->type != PXX_BUF_TYPE_PES) {
                 UNILOGE("parseTsData() - Unknown mpegts packet type: %d. EXIT!", packet->type);
                 return Error::ERROR_CODE_CHINA_DRM_TS_FAILED;
             }

             // Check codec type:
             if (packet->type == PXX_BUF_TYPE_PES &&
                 (packet->stream_id != CGTS_STREAM_ID_PRIVATE_STREAM_1 &&                  // AAC in some case
                  packet->stream_id != CGTS_STREAM_ID_AUDIO_MPEG1_MPEG2_MPEG4_AAC &&       // AAC
                  packet->stream_id != CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC)) { // AVC
                 UNILOGE("parseTsData() - Audio AAC and video AVC / HEVC supported only, illegal ES format found. type=%d,stream_id=%d",
                     packet->type, packet->stream_id);
                 return Error::ERROR_CODE_CHINA_DRM_TS_FAILED;
             }

             if (packet->type == PXX_BUF_TYPE_PES
                 && packet->stream_id == CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC ) {
                 int32_t retCode = handleVideoPesPayload(demux_ct, packet);
                 if (retCode != Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
                     UNILOGE("parseTsData() - handleVideoPesPayload failed, retCode=%d", retCode);
                     return retCode;
                 }
             }

             cgts_write_pxx_packet(mux_ct, packet);
         }

         // Free
         cgts_mux_context_free(mux_ct);
         cgts_demux_context_free(demux_ct);


         return 0;
}

int32_t ChinaDrmParser::parseReturnCode(int retCode){
    int32_t codeType = Error::ERROR_CODE_CHINA_DRM_BASE;
      //std::string codeTips = "ChinaDrmErrorType_UNKNOWN";
      switch (retCode) {
      case 1:
          codeType = Error::ERROR_CODE_CHINA_DRM_SUCCESS;
          //codeTips = "ChinaDrmCode_SUCCESS";
          break;
      case 4011:
          codeType = Error::ERROR_CODE_CHINA_DRM_LICENSE_FORMAT;
          //codeTips = "ChinaDrmCode_LICENSE_FORMAT";
          break;
      case 4012:
          codeType = Error::ERROR_CODE_CHINA_DRM_LICENSE_PARSE;
          //codeTips = "ChinaDrmCode_LICENSE_PARSE";
          break;
      case 4013:
          codeType = Error::ERROR_CODE_CHINA_DRM_LICENSE_UID;
          //codeTips = "ChinaDrmCode_LICENSE_UID";
          break;
      case 4021:
          codeType = Error::ERROR_CODE_CHINA_DRM_DECRYPT_BUFFER;
          //codeTips = "ChinaDrmCode_DECRYPT_BUFFER";
          break;
      default:
          break;
      }

      //UNILOGD("parseReturnCode(%d) return %s", retCode, codeTips.c_str());
      return codeType;
}
int32_t ChinaDrmParser::decryptData(uint8_t* srcBuffer, uint32_t srcBufferLength, uint8_t* destBuffer, uint32_t& destBufferLength){
    UNILOGD("decryptData(%p, %u, %p, %u, %d)", srcBuffer, srcBufferLength, destBuffer, destBufferLength, mDrmInstanceId);
       int32_t inBufferSize = (int32_t)srcBufferLength;
       int32_t outBufferSize = (int32_t)destBufferLength;
       int32_t ret = monalisa_decrypt_data(srcBuffer, inBufferSize, destBuffer, &outBufferSize, mDrmInstanceId);
       int32_t errorType = parseReturnCode(ret);
       if (errorType == Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
           destBufferLength = (uint32_t)outBufferSize;
       }
       return errorType;
}
int32_t ChinaDrmParser::handleVideoPesPayload(struct cgts_demux_context* demux_ct, struct cgts_pid_buffer* packet){
   uint32_t nalu_start_pos = 0;
   uint32_t nalu_end_pos = 0;

   uint8_t * new_es = (uint8_t *)calloc(1, (2 * packet->buf_pos + 1024) );
   uint32_t new_es_pos = 0;

   // PES头不加密
   memcpy(new_es + new_es_pos, packet->buf, packet->payload_offset);
   new_es_pos = packet->payload_offset;

   uint32_t payload_start_pos = packet->payload_offset;

   // 遍历NAL单元
   while( true == cgts_find_nal_unit(packet->buf, packet->buf_pos, payload_start_pos, &nalu_start_pos, &nalu_end_pos) ) {
       bool is_keyframe = false;
       uint8_t nal_header_size = CGTS_NAL_HEADER_SIZE_HEVC;
       int32_t stream_type = cgts_demux_context_pid_stream_type(demux_ct, packet->pid);

       // AVC和HEVC不同的处理逻辑，找到KeyFrame:
       if (stream_type == CGTS_STREAM_TYPE_VIDEO_AVC) {
           nal_header_size = CGTS_NAL_HEADER_SIZE_AVC;
           uint8_t nalu_type = cgts_find_nal_type_avc(packet->buf + nalu_start_pos);
           if (nalu_type == CGTS_NAL_TYPE_AVC_IDR_SLICE ) {
               is_keyframe = true;
           }
       }
       else if (stream_type == CGTS_STREAM_TYPE_VIDEO_HEVC) {
           nal_header_size = CGTS_NAL_HEADER_SIZE_HEVC;
           uint8_t nalu_type = cgts_find_nal_type_hevc(packet->buf + nalu_start_pos);
           if (nalu_type == CGTS_NAL_TYPE_HEVC_IDR_W_RADL
               || nalu_type == CGTS_NAL_TYPE_HEVC_IDR_N_LP
               || nalu_type == CGTS_NAL_TYPE_HEVC_CRA_NUT ) {
               is_keyframe = true;
           }
       }

       if (is_keyframe) {      // --- IDR data ---
           // NAL header
           memcpy(new_es + new_es_pos, packet->buf + nalu_start_pos, nal_header_size);
           new_es_pos = new_es_pos + nal_header_size;

           // NAL payload bytes
           uint32_t payload_len = nalu_end_pos - nalu_start_pos + 1 - nal_header_size;
           uint8_t * buf4enc = (uint8_t *)calloc(1, payload_len + 16 /* AES128 block size */);
           uint32_t buf4enc_len = payload_len + 16;

           // NAL unescape
           uint8_t * escapedBuffer = (uint8_t *)calloc(1, payload_len * 2);
           uint32_t escapedLength = nalUnescape(escapedBuffer, packet->buf + nalu_start_pos + nal_header_size, payload_len);

           int32_t ret = decryptData(escapedBuffer, escapedLength, buf4enc, buf4enc_len);
           if (ret != Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
               UNILOGW("handleVideoPesPayload() - decryptData ERROR: %d", ret);
               free(escapedBuffer);
               free(buf4enc);
               free(new_es);
               return ret;
           }

           memcpy(new_es + new_es_pos, buf4enc, buf4enc_len);
           new_es_pos = new_es_pos + buf4enc_len;

           free(escapedBuffer);
           free(buf4enc);
       } else {
           // --- IDR data ---
           memcpy(new_es + new_es_pos, packet->buf + nalu_start_pos, nalu_end_pos - nalu_start_pos + 1);
           new_es_pos = new_es_pos + nalu_end_pos - nalu_start_pos + 1;
       }
       payload_start_pos = nalu_end_pos;
   }

   cgts_pid_buffer_overwrite(packet, new_es, new_es_pos);
   packet->buf_pos = new_es_pos;

   free(new_es);
   return Error::ERROR_CODE_CHINA_DRM_SUCCESS;
}
uint32_t ChinaDrmParser::nalUnescape(uint8_t *destBuffer, uint8_t *srcBuffer, uint32_t srcBufferLength){
    uint32_t di = 0;
    uint32_t si = 0;
    while ( si + 3 < srcBufferLength ) {
       if ( srcBuffer[si] == 0x00 && srcBuffer[si+1] == 0x00 && srcBuffer[si+2] == 0x03 && srcBuffer[si+3] <= 0x03) {
           destBuffer[di++] = srcBuffer[si++];
           destBuffer[di++] = srcBuffer[si++];
           si++;
           destBuffer[di++] = srcBuffer[si++];
       } else {
           destBuffer[di++] = srcBuffer[si++];
       }
    }
    while (si < srcBufferLength) {
       destBuffer[di++] = srcBuffer[si++];
    }
    return di;
}

bool ChinaDrmParser::parseMediaInfo(const char *infoFile, MediaInfo &info){
    FILE *f = fopen(infoFile, "r");
    if (f == NULL) {
        UNILOGD("open media file failed");
        return false;
    }

    char szBuffer[512] = {0};
    const char *tagLicense = "license=";
    const char *tagInputFile = "input_file=";
    const char *tagOutputFile = "output_file=";

     memset(info.license, 0, sizeof(info.license));
     memset(info.inputFile, 0, sizeof(info.inputFile));
     memset(info.outputFile, 0, sizeof(info.outputFile));

    while (fgets(szBuffer, sizeof(szBuffer), f)){
        char *strFind = strstr(szBuffer, tagLicense);

        //UNILOGD("current line:%s", szBuffer);
        if (strFind != NULL) {
            int sz = strlen(strFind + strlen(tagLicense));
            memcpy(info.license,  strFind + strlen(tagLicense), sz);
            trim(info.license);
            UNILOGD("license:%s, length:%d", info.license, strlen(info.license));
        } else if ( (strFind = strstr(szBuffer, tagInputFile)) != NULL){
            int sz = strlen(strFind + strlen(tagInputFile));
             memcpy(info.inputFile,  strFind + strlen(tagInputFile), sz);

             trim(info.inputFile);
             UNILOGD("inputFile:%s, size:%d", info.inputFile, strlen(info.inputFile));
        } else if ((strFind = strstr(szBuffer, tagOutputFile)) != NULL) {
              int sz = strlen(strFind + strlen(tagOutputFile));
              memcpy(info.outputFile,  strFind + strlen(tagOutputFile), sz);
              trim(info.outputFile);
            UNILOGD("outputFile:%s, size:%d", info.outputFile, strlen(info.outputFile));
        }
    }

    if (strlen(info.outputFile) == 0) {
        const char *defaultName = "output.ts";
        memcpy(info.outputFile, defaultName, strlen(defaultName));
    }

    if (strlen(info.license) == 0 || strlen(info.inputFile) == 0) {
        UNILOGD("parse media file failed");
        return false;
    } else {
        UNILOGD("parse media file success");
        return true;
    }
}


bool ChinaDrmParser::trim(char *szBuffer) {
   if (szBuffer == NULL){
        return false;
   }

    int i = 0;
    int headerIndex = 0;
    int notEmptyIndex = 0;
    bool processHeader = true;
    while(i < strlen(szBuffer)) {
        if (processHeader && szBuffer[i] == ' '){
            notEmptyIndex++;
            i++;
            continue;
        } else {
            processHeader = false;
            char cur = szBuffer[i];
            if (szBuffer[i] == '\r' || szBuffer[i] == '\n'){
                //UNILOGD("reset char: %d", szBuffer[i]);
                szBuffer[i] = '\0';
            }
            szBuffer[headerIndex++] == szBuffer[i++];
        }
    }
    // TODO

    return true;
}

////////////////////////////////////////
/*
ChinaDrmParser::ChinaDrmParser() : mInitialized(false), mDrmInstanceId(0) {
}

ChinaDrmParser::~ChinaDrmParser() {

}

int32_t ChinaDrmParser::setLicense(const char *drmLicense, int licenseLength, const char *uid , int uidLength, int32_t instanceId){
    //UNILOGD("setLicense(drmLicense=%s, uid=%s, instanceId=%d)", drmLicense, uid, instanceId);

    //mDrmLicense = drmLicense;
    //mDrmUid = uid;
     if (drmLicense == NULL || licenseLength == 0) {
          return Error::ERROR_CODE_CHINA_DRM_LICENSE_EMPTY;
     }

    memset(mDrmLicense, 0, sizeof(mDrmLicense));
    memcpy(mDrmLicense, drmLicense, licenseLength);

    memset(mDrmUid, 0, sizeof(mDrmUid));
    memcpy(mDrmUid, uid, uidLength);

    mDrmInstanceId = instanceId;

    int32_t ret = monalisa_set_license((char*)(drmLicense), int32_t(licenseLength), instanceId,
                                   (unsigned char* )(uid));
    int32_t errorType = parseReturnCode(ret);
    if (ret != Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
      mInitialized = false;
    } else {
      mInitialized = true;
    }
    return errorType;
}

int32_t ChinaDrmParser::parseTsData(const char *inputFile, const char * outputFile){
    struct cgts_demux_context * demux_ct = cgts_demux_context_alloc_with_file(inputFile);
    struct cgts_mux_context * mux_ct = cgts_mux_context_alloc_with_file(outputFile);
    UNILOGD("parseTsData, inputFile:%p, outputFile:%p", demux_ct->input_fp, mux_ct->output_fp );

    if (demux_ct->input_fp == NULL || mux_ct->output_fp == NULL) {
        return 0;
    }

    struct cgts_pid_buffer * packet = NULL;
    while(cgts_read_pxx_packet(demux_ct, &packet) == true) {
        if (NULL == packet->buf) {
            UNILOGE("parseTsData(), cgts_read_pxx_packet get invalid packet, type=%d, EXIT!", packet->type);
            return Error::ERROR_CODE_CHINA_DRM_TS_FAILED;
        }
        // Check PES type:
        if (packet->type != PXX_BUF_TYPE_PSI && packet->type != PXX_BUF_TYPE_PES) {
            UNILOGE("parseTsData() - Unknown mpegts packet type: %d. EXIT!", packet->type);
            return Error::ERROR_CODE_CHINA_DRM_TS_FAILED;
        }

        // Check codec type:
        if (packet->type == PXX_BUF_TYPE_PES &&
            (packet->stream_id != CGTS_STREAM_ID_PRIVATE_STREAM_1 &&                  // AAC in some case
             packet->stream_id != CGTS_STREAM_ID_AUDIO_MPEG1_MPEG2_MPEG4_AAC &&       // AAC
             packet->stream_id != CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC)) { // AVC
            UNILOGE("parseTsData() - Audio AAC and video AVC / HEVC supported only, illegal ES format found. type=%d,stream_id=%d",
                packet->type, packet->stream_id);
            return Error::ERROR_CODE_CHINA_DRM_TS_FAILED;
        }

        if (packet->type == PXX_BUF_TYPE_PES
            && packet->stream_id == CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC ) {
            int32_t retCode = handleVideoPesPayload(demux_ct, packet);
            if (retCode != Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
                UNILOGE("parseTsData() - handleVideoPesPayload failed, retCode=%d", retCode);
                return retCode;
            }
        }

        cgts_write_pxx_packet(mux_ct, packet);
    }

    // Free
    cgts_mux_context_free(mux_ct);
    cgts_demux_context_free(demux_ct);


    return 0;
}

int32_t ChinaDrmParser::parseReturnCode(int retCode){
    int32_t codeType = Error::ERROR_CODE_CHINA_DRM_BASE;
   //std::string codeTips = "ChinaDrmErrorType_UNKNOWN";
   switch (retCode) {
   case 1:
       codeType = Error::ERROR_CODE_CHINA_DRM_SUCCESS;
       //codeTips = "ChinaDrmCode_SUCCESS";
       break;
   case 4011:
       codeType = Error::ERROR_CODE_CHINA_DRM_LICENSE_FORMAT;
       //codeTips = "ChinaDrmCode_LICENSE_FORMAT";
       break;
   case 4012:
       codeType = Error::ERROR_CODE_CHINA_DRM_LICENSE_PARSE;
       //codeTips = "ChinaDrmCode_LICENSE_PARSE";
       break;
   case 4013:
       codeType = Error::ERROR_CODE_CHINA_DRM_LICENSE_UID;
       //codeTips = "ChinaDrmCode_LICENSE_UID";
       break;
   case 4021:
       codeType = Error::ERROR_CODE_CHINA_DRM_DECRYPT_BUFFER;
       //codeTips = "ChinaDrmCode_DECRYPT_BUFFER";
       break;
   default:
       break;
   }

   //UNILOGD("parseReturnCode(%d) return %s", retCode, codeTips.c_str());
   return codeType;
}

int32_t ChinaDrmParser::decryptData(uint8_t* srcBuffer, uint32_t srcBufferLength, uint8_t* destBuffer, uint32_t& destBufferLength){
    UNILOGD("decryptData(%p, %u, %p, %u, %d)", srcBuffer, srcBufferLength, destBuffer, destBufferLength, mDrmInstanceId);
    int32_t inBufferSize = (int32_t)srcBufferLength;
    int32_t outBufferSize = (int32_t)destBufferLength;
    int32_t ret = monalisa_decrypt_data(srcBuffer, inBufferSize, destBuffer, &outBufferSize, mDrmInstanceId);
    int32_t errorType = parseReturnCode(ret);
    if (errorType == Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
        destBufferLength = (uint32_t)outBufferSize;
    }
    return errorType;
}

 int32_t ChinaDrmParser::handleVideoPesPayload(struct cgts_demux_context* demux_ct, struct cgts_pid_buffer* packet){
    uint32_t nalu_start_pos = 0;
        uint32_t nalu_end_pos = 0;

        uint8_t * new_es = (uint8_t *)calloc(1, (2 * packet->buf_pos + 1024) );
        uint32_t new_es_pos = 0;

        // PES头不加密
        memcpy(new_es + new_es_pos, packet->buf, packet->payload_offset);
        new_es_pos = packet->payload_offset;

        uint32_t payload_start_pos = packet->payload_offset;

        // 遍历NAL单元
        while( true == cgts_find_nal_unit(packet->buf, packet->buf_pos, payload_start_pos, &nalu_start_pos, &nalu_end_pos) ) {
            bool is_keyframe = false;
            uint8_t nal_header_size = CGTS_NAL_HEADER_SIZE_HEVC;
            int32_t stream_type = cgts_demux_context_pid_stream_type(demux_ct, packet->pid);

            // AVC和HEVC不同的处理逻辑，找到KeyFrame:
            if (stream_type == CGTS_STREAM_TYPE_VIDEO_AVC) {
                nal_header_size = CGTS_NAL_HEADER_SIZE_AVC;
                uint8_t nalu_type = cgts_find_nal_type_avc(packet->buf + nalu_start_pos);
                if (nalu_type == CGTS_NAL_TYPE_AVC_IDR_SLICE ) {
                    is_keyframe = true;
                }
            }
            else if (stream_type == CGTS_STREAM_TYPE_VIDEO_HEVC) {
                nal_header_size = CGTS_NAL_HEADER_SIZE_HEVC;
                uint8_t nalu_type = cgts_find_nal_type_hevc(packet->buf + nalu_start_pos);
                if (nalu_type == CGTS_NAL_TYPE_HEVC_IDR_W_RADL
                    || nalu_type == CGTS_NAL_TYPE_HEVC_IDR_N_LP
                    || nalu_type == CGTS_NAL_TYPE_HEVC_CRA_NUT ) {
                    is_keyframe = true;
                }
            }

            if (is_keyframe) {      // --- IDR data ---
                // NAL header
                memcpy(new_es + new_es_pos, packet->buf + nalu_start_pos, nal_header_size);
                new_es_pos = new_es_pos + nal_header_size;

                // NAL payload bytes
                uint32_t payload_len = nalu_end_pos - nalu_start_pos + 1 - nal_header_size;
                uint8_t * buf4enc = (uint8_t *)calloc(1, payload_len + 16 );
                uint32_t buf4enc_len = payload_len + 16;

                // NAL unescape
                uint8_t * escapedBuffer = (uint8_t *)calloc(1, payload_len * 2);
                uint32_t escapedLength = nalUnescape(escapedBuffer, packet->buf + nalu_start_pos + nal_header_size, payload_len);

                int32_t ret = decryptData(escapedBuffer, escapedLength, buf4enc, buf4enc_len);
                if (ret != Error::ERROR_CODE_CHINA_DRM_SUCCESS) {
                    UNILOGW("handleVideoPesPayload() - decryptData ERROR: %d", ret);
                    free(escapedBuffer);
                    free(buf4enc);
                    free(new_es);
                    return ret;
                }

                memcpy(new_es + new_es_pos, buf4enc, buf4enc_len);
                new_es_pos = new_es_pos + buf4enc_len;

                free(escapedBuffer);
                free(buf4enc);
            } else {
                // --- IDR data ---
                memcpy(new_es + new_es_pos, packet->buf + nalu_start_pos, nalu_end_pos - nalu_start_pos + 1);
                new_es_pos = new_es_pos + nalu_end_pos - nalu_start_pos + 1;
            }
            payload_start_pos = nalu_end_pos;
        }

        cgts_pid_buffer_overwrite(packet, new_es, new_es_pos);
        packet->buf_pos = new_es_pos;

        free(new_es);
        return Error::ERROR_CODE_CHINA_DRM_SUCCESS;
 }

uint32_t ChinaDrmParser::nalUnescape(uint8_t *destBuffer, uint8_t *srcBuffer, uint32_t srcBufferLength){
    uint32_t di = 0;
       uint32_t si = 0;
       while ( si + 3 < srcBufferLength ) {
           if ( srcBuffer[si] == 0x00 && srcBuffer[si+1] == 0x00 && srcBuffer[si+2] == 0x03 && srcBuffer[si+3] <= 0x03) {
               destBuffer[di++] = srcBuffer[si++];
               destBuffer[di++] = srcBuffer[si++];
               si++;
               destBuffer[di++] = srcBuffer[si++];
           } else {
               destBuffer[di++] = srcBuffer[si++];
           }
       }
       while (si < srcBufferLength) {
           destBuffer[di++] = srcBuffer[si++];
       }
       return di;
}
*/
/*
bool ChinaDrmParser::parseMediaInfo(std::string infoFile, MediaInfo &info){
    FILE *f = fopen(infoFile.c_str(), "r");
    if (f == NULL) {
        return false;
    }

    char szBuffer[512] = {0};
    std::string tagLicense = "license=";
    std::string tagInputFile = "input_file=";
    std::string tagOutputFile = "output_file=";
    while (fgets(szBuffer, sizeof(szBuffer), f)){
        std::string currentLine(szBuffer);
        size_t pos = currentLine.find(tagLicense);
        //UNILOGD("current line:%s", szBuffer);

        if (pos != std::string::npos) {
            info.license = currentLine.substr(pos + tagLicense.length(), currentLine.length());
            info.license = trim(info.license);
            //UNILOGD("license:%s", info.license.c_str());
        } else if ( (pos = currentLine.find(tagInputFile)) != std::string::npos){
            info.inputFile = currentLine.substr(pos + tagInputFile.length(), currentLine.length());
            info.inputFile = trim(info.inputFile);
            //UNILOGD("inputFile:%s, size:%d", info.inputFile.c_str(), info.inputFile.length());
        } else if ( (pos = currentLine.find(tagOutputFile)) != std::string::npos) {
            info.outputFile = currentLine.substr(pos + tagOutputFile.length(), currentLine.length());
            info.outputFile = trim(info.outputFile);
            //UNILOGD("outputFile:%s, size:%d", info.outputFile.c_str(), info.outputFile.length());
        }
    }

    if (info.license.empty() || info.inputFile.empty() ) {
        return false;
    } else {
        return true;
    }
}
*/

/*std::string ChinaDrmParser::trim(std::string &str) {
   if (str.empty()){
    return "";
   }

    std::string dest;
   for (int i = 0; i < str.length(); i++){
        char cur = str.at(i);
        if (cur == ' ' || cur == '\r' || cur == '\n'){
            continue;
        }

        dest += cur;
   }

    return dest;
}*/