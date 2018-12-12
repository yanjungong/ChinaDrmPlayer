#ifndef __CGTS_NAL_ADTS_PARSE_H__
#define __CGTS_NAL_ADTS_PARSE_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define CGTS_ADTS_HEADER_SIZE   7

#define CGTS_NAL_START_CODE_SIZE    (3)
#define CGTS_NAL_HEADER_SIZE_AVC    (3+1)
#define CGTS_NAL_HEADER_SIZE_HEVC   (3+2)

#define CGTS_NAL_TYPE_AVC_IDR_SLICE     0x05
#define CGTS_NAL_TYPE_HEVC_IDR_W_RADL   19
#define CGTS_NAL_TYPE_HEVC_IDR_N_LP     20
#define CGTS_NAL_TYPE_HEVC_CRA_NUT      21

bool cgts_find_nal_unit(uint8_t * buf, uint32_t buf_len, uint32_t buf_start_pos, uint32_t * nal_start_pos, uint32_t * nal_end_pos);
bool cgts_find_adts_unit(uint8_t * buf, uint32_t buf_len, uint32_t buf_start_pos, uint32_t * adts_start_pos, uint32_t * adts_end_pos);

int16_t cgts_find_nal_type_avc(uint8_t * nalu_start_ptr);
int16_t cgts_find_nal_type_hevc(uint8_t * nalu_start_ptr);

#endif
