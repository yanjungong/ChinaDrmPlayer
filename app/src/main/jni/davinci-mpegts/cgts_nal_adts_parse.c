#include "cgts_nal_adts_parse.h"

bool cgts_find_nal_unit(uint8_t * buf, uint32_t buf_len, uint32_t buf_start_pos, uint32_t * nal_start_pos, uint32_t * nal_end_pos) {
    bool nalu_start_found = false;
    bool nalu_end_found = false;
    uint32_t nalu_start_pos = 0;
    uint32_t nalu_end_pos = 0;

    for (uint32_t i=buf_start_pos; i<buf_len-CGTS_NAL_START_CODE_SIZE; i++) {
        if ( buf[i] == 0x00 && buf[i+1] == 0x00 && buf[i+2] == 0x01 ) {
            if ( nalu_start_found == false) {
                nalu_start_pos = i;
                nalu_start_found = true;
            } else if ( nalu_end_found == false ) {
                nalu_end_pos = i - 1;
                nalu_end_found = true;
            }
            i = i + CGTS_NAL_START_CODE_SIZE; // pass the start code
        }

        if ( i == buf_len - CGTS_NAL_START_CODE_SIZE - 1 ) { // max i in this `for`
            if ( nalu_start_found == true ) {
                nalu_end_pos = buf_len - 1;
                nalu_end_found = true;
            }
        }

        if ( nalu_start_found == true && nalu_end_found == true ) {
            (* nal_start_pos) = nalu_start_pos;
            (* nal_end_pos) = nalu_end_pos;
            return true;
        }
    }
    return false;
}

bool cgts_find_adts_unit(uint8_t * buf, uint32_t buf_len, uint32_t buf_start_pos, uint32_t * adts_start_pos, uint32_t * adts_end_pos) {
    bool adtsu_start_found = false;
    bool adtsu_end_found = false;
    uint32_t adtsu_start_pos = 0;
    uint32_t adtsu_end_pos = 0;

    for (uint32_t i=buf_start_pos; i<buf_len-1; i++) {
        if ( buf[i] == 0xff
                && buf[i+1] == 0xf1)
        {
            if ( adtsu_start_found == false) {
                adtsu_start_pos = i;
                adtsu_start_found = true;
            } else if ( adtsu_end_found == false ) {
                adtsu_end_pos = i - 1;
                adtsu_end_found = true;
            }
            i = i + 2;
        }

        if ( i == buf_len
                - 1 /* length of start code - 1 (2-1)*/
                - 1 /* last index equal length - 1 */ )
        {
            if ( adtsu_start_found == true ) {
                adtsu_end_pos = i + 1;
                adtsu_end_found = true;
            }
        }

        if ( adtsu_start_found == true && adtsu_end_found == true ) {
            (* adts_start_pos) = adtsu_start_pos;
            (* adts_end_pos) = adtsu_end_pos;
            return true;
        }
    }
    return false;
}

int16_t cgts_find_nal_type_avc(uint8_t * nalu_start_ptr) {
    uint8_t nalu_header = *(nalu_start_ptr + CGTS_NAL_START_CODE_SIZE);  // AVC has one-byte NAL header
    return nalu_header & 0x1f;
}

int16_t cgts_find_nal_type_hevc(uint8_t * nalu_start_ptr) {
    uint8_t nalu_header_first_byte = *(nalu_start_ptr + CGTS_NAL_START_CODE_SIZE);  // HEVC has two-byte NAL header
    return (nalu_header_first_byte >> 1) & 0x3f;
}
