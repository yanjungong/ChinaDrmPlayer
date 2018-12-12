#ifndef __CGTS_STRUCTS_H__
#define __CGTS_STRUCTS_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define CGTS_TS_PACKET_SIZE                 188
#define CGTS_TS_PACKET_HEADER_SIZE          4
#define CGTS_TS_PACKET_PAYLOAD_MAX_SIZE     (CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE)
#define CGTS_PSI_PACKET_HEADER_SIZE         4
#define CGTS_PES_PACKET_HEADER_SIZE         6

#define CGTS_SYNC_BYTE  0x47
#define CGTS_INPUT_TYPE_FILE 1
#define CGTS_INPUT_TYPE_MEMORY 2

#define CGTS_PID_PAT 0x00
#define CGTS_PID_CAT 0x01
#define CGTS_PID_SDT 0x02

#define CGTS_STREAM_TYPE_VIDEO_MPEG1            0x01
#define CGTS_STREAM_TYPE_VIDEO_MPEG2            0x02
#define CGTS_STREAM_TYPE_AUDIO_MPEG1            0x03
#define CGTS_STREAM_TYPE_AUDIO_MPEG2            0x04
#define CGTS_STREAM_TYPE_PRIVATE_SECTION        0x05
#define CGTS_STREAM_TYPE_PRIVATE_DATA           0x06
#define CGTS_STREAM_TYPE_AUDIO_AAC              0x0f
#define CGTS_STREAM_TYPE_AUDIO_AAC_LATM         0x11
#define CGTS_STREAM_TYPE_VIDEO_MPEG4            0x10
#define CGTS_STREAM_TYPE_METADATA               0x15
#define CGTS_STREAM_TYPE_VIDEO_AVC              0x1b
#define CGTS_STREAM_TYPE_VIDEO_HEVC             0x24
#define CGTS_STREAM_TYPE_VIDEO_CAVS             0x42
#define CGTS_STREAM_TYPE_VIDEO_VC1              0xea
#define CGTS_STREAM_TYPE_VIDEO_DIRAC            0xd1
#define CGTS_STREAM_TYPE_AUDIO_AC3              0x81
#define CGTS_STREAM_TYPE_AUDIO_DTS              0x82
#define CGTS_STREAM_TYPE_AUDIO_TRUEHD           0x83
#define CGTS_STREAM_TYPE_AUDIO_EAC3             0x87

bool cgts_stream_type_to_string(uint8_t id, char * str, uint16_t str_len);

// smaller header stream IDs
#define CGTS_STREAM_ID_PADDING_STREAM                       0xbe
#define CGTS_STREAM_ID_PROGRAM_STREAM_MAP                   0xbc
#define CGTS_STREAM_ID_PRIVATE_STREAM_2                     0xbf
#define CGTS_STREAM_ID_ECM                                  0xf0
#define CGTS_STREAM_ID_EMM                                  0xf1
#define CGTS_STREAM_ID_PROGRAM_STREAM_DIRECTORY             0xff
#define CGTS_STREAM_ID_DSMCC_STREAM                         0xf2
#define CGTS_STREAM_ID_H_222_1_TYPE_E                       0xf8
// bigger header stream IDs
#define CGTS_STREAM_ID_PRIVATE_STREAM_1                     0xbd
#define CGTS_STREAM_ID_AUDIO_MPEG1_MPEG2_MPEG4_AAC          0xc0
#define CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC     0xe0

/***********/
/* program */
/***********/
#define MAX_PIDS_PER_PROGRAM 64
struct cgts_program {
    uint16_t program_id;
    uint16_t pmt_pid;

    uint16_t pids[MAX_PIDS_PER_PROGRAM];
    uint16_t pids_stream_type[MAX_PIDS_PER_PROGRAM];
    uint8_t pids_num;
};

struct cgts_program * cgts_program_alloc(uint16_t program_id, uint16_t pmt_pid);
void cgts_program_free(struct cgts_program * program);
bool cgts_program_pid_exist(struct cgts_program * program, uint16_t pid);
bool cgts_program_pid_add(struct cgts_program * program, uint16_t pid, uint16_t stream_type);

/**************/
/* pid buffer */
/**************/
#define PXX_BUF_LEN_DEFAULT 1024

#define PXX_BUF_TYPE_UNKNOWN    0
#define PXX_BUF_TYPE_PSI        1
#define PXX_BUF_TYPE_PES        2
struct cgts_pid_buffer {
    bool parsed;
    bool filled_up;

    int8_t ccounter;

    uint16_t pid;
    uint8_t type;
    uint8_t table_id;
    uint8_t stream_id;
    uint8_t reserved0;
    uint32_t expect_len;
    uint64_t pts;
    uint64_t dts;
    uint8_t has_pcr;
    uint64_t pcr;

    uint32_t payload_offset;

    uint8_t * buf;
    uint32_t buf_pos; // means data_length in *buf
    uint32_t buf_cap;
    
    uint64_t offset_in_file;
};

struct cgts_pid_buffer * cgts_pid_buffer_alloc(uint16_t pid);
void cgts_pid_buffer_free(struct cgts_pid_buffer * pid_buf);
void cgts_pid_buffer_debug(struct cgts_pid_buffer * pid_buf);
void cgts_pid_buffer_print_hex(struct cgts_pid_buffer * pid_buf);
bool cgts_pid_buffer_append(struct cgts_pid_buffer * pid_buf, const uint8_t * ts_payload, uint32_t ts_payload_len);
bool cgts_pid_buffer_overwrite(struct cgts_pid_buffer * pid_buf, const uint8_t * ts_payload, uint32_t ts_payload_len);
void cgts_pid_buffer_reset(struct cgts_pid_buffer * pid_buf);
bool cgts_pid_buffer_complete(struct cgts_pid_buffer * pid_buf);

/*************/
/* ts packet */
/*************/
struct cgts_ts_packet {
    uint8_t sync_byte;
    uint8_t unit_start_indicator;
    uint16_t pid;
    uint8_t scrambling_control;
    uint8_t adaption_field_control;
    uint8_t continuity_counter;

    uint8_t has_adaptation;
    uint8_t has_payload;

    uint8_t has_pcr;
    uint64_t pcr;

    uint8_t * payload_ptr;
    int32_t payload_len;
};

struct cgts_ts_packet * cgts_ts_packet_alloc();
void cgts_ts_packet_free(struct cgts_ts_packet * tsp);
void cgts_ts_packet_reset(struct cgts_ts_packet * tsp);
void cgts_ts_packet_debug(struct cgts_ts_packet * tsp);

struct cgts_pat {
    uint8_t table_id;
    uint16_t stream_id;
    uint8_t version;
    uint8_t sec_num;
    uint8_t last_sec_num;
    struct cgts_program *prg;
};

/*****************/
/* ts pxx packet */
/*****************/
#define CGTS_PXX_PACKET_DATA_CAP_DEFAULT    4096
#define CGTS_PXX_PACKET_TYPE_UNKNOWN    0
#define CGTS_PXX_PACKET_TYPE_PSI        1
#define CGTS_PXX_PACKET_TYPE_PES        2
struct cgts_pxx_packet {
    uint8_t type;
    uint8_t * data;
    uint8_t data_len;
    uint8_t data_cap;
};
struct cgts_pxx_packet * cgts_pxx_packet_alloc();
void cgts_pxx_packet_free(struct cgts_pxx_packet * pxx_packet);
void cgts_pxx_packet_reset(struct cgts_pxx_packet * pxx_packet);
void cgts_pxx_packet_debug(struct cgts_pxx_packet * pxx_packet);
bool cgts_pxx_packet_parse_data(struct cgts_pxx_packet * pxx_packet, uint8_t * buf, uint32_t buf_len);
bool cgts_pxx_packet_write_data(struct cgts_pxx_packet * pxx_packet, uint8_t * buf, uint32_t buf_len);

/*****************/
/* demux context */
/*****************/
#define MAX_PIDS_IN_SIGNLE_MPEGTS       512
#define MAX_PROGRAMS_IN_SIGNLE_MPEGTS   512
#define CGTS_CONTEXT_INPUT_TYPE_FILE    1
#define CGTS_CONTEXT_INPUT_TYPE_MEMORY  2
struct cgts_demux_context {
    uint8_t input_type; // 1-file, 2-memory
    FILE * input_fp;
    uint8_t* input_ptr;
    uint32_t input_len;
    uint32_t input_offset;
    uint32_t tsp_counter;
    //int8_t ccounter;

    struct cgts_program * programs[MAX_PROGRAMS_IN_SIGNLE_MPEGTS];
    uint16_t programs_num;

    struct cgts_pid_buffer * pid_buf[MAX_PIDS_IN_SIGNLE_MPEGTS];
    uint16_t pid_buf_num;
    int16_t just_parsed_pid_buf_idx;

    bool pat_found;
    bool pmt_found;

    uint8_t * tsp_buffer;
    bool tsp_buffer_is_set;
    int32_t last_tsp_pid;
};

// base functions
struct cgts_demux_context * cgts_demux_context_alloc_with_memory(uint8_t * buf, uint32_t * buf_len);
struct cgts_demux_context * cgts_demux_context_alloc_with_file(const char * filename);
void cgts_demux_context_free(struct cgts_demux_context * ct);
void cgts_demux_context_debug(struct cgts_demux_context * ct);
void cgts_demux_context_rewind(struct cgts_demux_context * ct);

// program functions
int32_t cgts_demux_context_program_index(struct cgts_demux_context * ct, uint16_t prog_id);
bool cgts_demux_context_program_exist(struct cgts_demux_context * ct, uint16_t prog_id);
bool cgts_demux_context_program_create(struct cgts_demux_context * ct, uint16_t prog_id, uint16_t pmt_pid);
bool cgts_demux_context_program_delete(struct cgts_demux_context * ct, uint16_t prog_id, uint16_t pmt_pid);

// pid-buf functions
#define CGTS_PID_TYPE_PAT       0x10
#define CGTS_PID_TYPE_PMT       0x11
#define CGTS_PID_TYPE_PSI       0x12
#define CGTS_PID_TYPE_PES       0x13
#define CGTS_PID_TYPE_UNKNOWN   0x19
bool cgts_demux_context_pid_exist(struct cgts_demux_context * ct, uint16_t pid);
bool cgts_demux_context_pid_create(struct cgts_demux_context * ct, uint16_t pid);
int16_t cgts_demux_context_pid_type(struct cgts_demux_context * ct, uint16_t pid);
int32_t cgts_demux_context_pid_buffer_index(struct cgts_demux_context * ct, uint16_t pid);
int32_t cgts_demux_context_pid_stream_type(struct cgts_demux_context * ct, uint16_t pid);

/***************/
/* mux context */
/***************/
#define CGTS_CONTEXT_OUTPUT_TYPE_FILE    1
#define CGTS_CONTEXT_OUTPUT_TYPE_MEMORY  2
struct cgts_mux_context {
    uint8_t output_type;    // 1-file, 2-memory
    FILE * output_fp;
    uint8_t * output_ptr;
    uint32_t  output_buffer_len;
    uint32_t  output_offset;
    uint32_t tsp_counter;
    //int8_t ccounter;

    bool opt_mode;  // todo: opt mpegts file size - decrease PAT and PMT packet number, increase PES packet size.
    bool pat_wrote;
    bool pmt_wrote;
};
struct cgts_mux_context * cgts_mux_context_alloc_with_memory(uint32_t buf_len);
struct cgts_mux_context * cgts_mux_context_alloc_with_file(const char * filename);
void cgts_mux_context_free(struct cgts_mux_context * ct);
void cgts_mux_context_debug(struct cgts_mux_context * ct);

#endif

