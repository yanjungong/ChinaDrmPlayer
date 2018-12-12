#include "cgts_structs.h"

/**********************************/
/************** utils *************/
/**********************************/

bool cgts_stream_type_to_string(uint8_t id, char * str, uint16_t str_len) {
    switch (id) {
        case CGTS_STREAM_TYPE_VIDEO_MPEG1:
            strncpy(str, "mpeg1_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_MPEG2:
            strncpy(str, "mpeg2_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_MPEG1:
            strncpy(str, "mpeg1_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_MPEG2:
            strncpy(str, "mpeg2_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_PRIVATE_SECTION:
            strncpy(str, "private_section", str_len);
            return true;
        case CGTS_STREAM_TYPE_PRIVATE_DATA:
            strncpy(str, "private_data", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_AAC:
            strncpy(str, "aac_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_AAC_LATM:
            strncpy(str, "aaclatm_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_MPEG4:
            strncpy(str, "mpeg4_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_METADATA:
            strncpy(str, "metadata", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_AVC:
            strncpy(str, "avc_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_HEVC:
            strncpy(str, "hevc_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_CAVS:
            strncpy(str, "cavs_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_VC1:
            strncpy(str, "vc1_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_VIDEO_DIRAC:
            strncpy(str, "dirac_video", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_AC3:
            strncpy(str, "ac3_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_DTS:
            strncpy(str, "dts_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_TRUEHD:
            strncpy(str, "truehd_audio", str_len);
            return true;
        case CGTS_STREAM_TYPE_AUDIO_EAC3:
            strncpy(str, "eac3_audio", str_len);
            return true;
        default:
            return false;
    }
}

/**********************************/
/************ program *************/
/**********************************/

struct cgts_program * cgts_program_alloc(uint16_t program_id, uint16_t pmt_pid) {
    struct cgts_program * program = (struct cgts_program *)calloc(1, sizeof(struct cgts_program));
    program->program_id = program_id;
    program->pmt_pid = pmt_pid;
    program->pids_num = 0;
    return program;
}

void cgts_program_free(struct cgts_program * program) {
    free(program);
}

bool cgts_program_pid_exist(struct cgts_program * program, uint16_t pid) {
    if (program->pids_num >= MAX_PIDS_PER_PROGRAM) {
        return false;
    }
    for(int i=0;i<program->pids_num;i++) {
        if (program->pids[i] == pid) {
            return true;
        }
    }
    return false;
}

bool cgts_program_pid_add(struct cgts_program * program, uint16_t pid, uint16_t stream_type) {
    if (program->pids_num >= MAX_PIDS_PER_PROGRAM) {
        return false;
    }
    program->pids[program->pids_num] = pid;
    program->pids_stream_type[program->pids_num] = stream_type;
    program->pids_num = program->pids_num + 1;
    return true;
}

/**********************************/
/*********** pid buffer ***********/
/**********************************/

struct cgts_pid_buffer * cgts_pid_buffer_alloc(uint16_t pid) {
    struct cgts_pid_buffer * pid_buf = (struct cgts_pid_buffer *)calloc(1, sizeof(struct cgts_pid_buffer));
    pid_buf->parsed = false;
    pid_buf->filled_up = false;

    pid_buf->ccounter = 0;

    pid_buf->pid = pid;
    pid_buf->type = PXX_BUF_TYPE_UNKNOWN;
    pid_buf->table_id = 0;
    pid_buf->stream_id = 0;
    pid_buf->reserved0 = 0;
    pid_buf->expect_len = 0;
    pid_buf->pts = 0;
    pid_buf->dts = 0;
    pid_buf->has_pcr = 0;
    pid_buf->pcr = 0;

    pid_buf->payload_offset = 0;

    pid_buf->buf = (uint8_t *)calloc(1, PXX_BUF_LEN_DEFAULT);
    pid_buf->buf_pos = 0;
    pid_buf->buf_cap = PXX_BUF_LEN_DEFAULT;

    pid_buf->parsed = false;
    pid_buf->filled_up = false;
    
    pid_buf->payload_offset = 0;

    return pid_buf;
}

void cgts_pid_buffer_free(struct cgts_pid_buffer * pid_buf) {
    free(pid_buf->buf);
    free(pid_buf);
}

void cgts_pid_buffer_debug(struct cgts_pid_buffer * pid_buf) {
    fprintf(stdout, "parsed: %s, filled_up: %s, "
            , pid_buf->parsed ? "yes": "no"
            , pid_buf->filled_up ? "yes": "no"
            );
    fprintf(stdout, "type: ");
    switch (pid_buf->type) {
        case PXX_BUF_TYPE_UNKNOWN:
            fprintf(stdout, "unknown, ");
            break;
        case PXX_BUF_TYPE_PSI:
            fprintf(stdout, "psi, ");
            break;
        case PXX_BUF_TYPE_PES:
            fprintf(stdout, "pes, ");
            break;
        default:
            fprintf(stdout, "unknown, ");
            break;
    }
    fprintf(stdout, "pid: %5d, table_id: %2d, stream_id: %02x, "
            , pid_buf->pid
            , pid_buf->table_id
            , pid_buf->stream_id
            );
    fprintf(stdout, "expect_len: %6d, pts: %9lld, dts: %9lld, "
            , pid_buf->expect_len
            , pid_buf->pts
            , pid_buf->dts
            );
    fprintf(stdout, "payload_offset: %2d, buf_pos: %6d , buf_cap: %6d, "
            , pid_buf->payload_offset
            , pid_buf->buf_pos
            , pid_buf->buf_cap
            );
    fprintf(stdout, "offset_in_file: %lld\n"
            , pid_buf->offset_in_file
            );
}

void cgts_pid_buffer_print_hex(struct cgts_pid_buffer * pid_buf) {
    int i;
    for(i=0;i<pid_buf->buf_pos;i++) {
        fprintf(stdout, " %02x", pid_buf->buf[i]);
        if (i % 8 == 7) {
            fprintf(stdout, "  ");
        }
        if (i % 16 == 15) {
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, "\n");
}

bool cgts_pid_buffer_append(struct cgts_pid_buffer * pid_buf, const uint8_t * ts_payload, uint32_t ts_payload_len) {
    while (pid_buf->buf_pos + ts_payload_len >= pid_buf->buf_cap) {
        pid_buf->buf = (uint8_t *)realloc(pid_buf->buf, pid_buf->buf_cap * 2);
        if (pid_buf->buf == NULL) {
            return false;
        }
        pid_buf->buf_cap = pid_buf->buf_cap * 2;
    }
    memcpy(pid_buf->buf + pid_buf->buf_pos, ts_payload, ts_payload_len);
    pid_buf->buf_pos +=  ts_payload_len;
    return true;
}

bool cgts_pid_buffer_overwrite(struct cgts_pid_buffer * pid_buf, const uint8_t * ts_payload, uint32_t ts_payload_len) {
    memset(pid_buf->buf, 0, pid_buf->buf_cap);
    pid_buf->buf_pos = 0;
    return cgts_pid_buffer_append(pid_buf, ts_payload, ts_payload_len);
}

void cgts_pid_buffer_reset(struct cgts_pid_buffer * pid_buf) {
    pid_buf->parsed = false;
    pid_buf->filled_up = false;

    //pid_buf->ccounter = 0;

    pid_buf->type = PXX_BUF_TYPE_UNKNOWN;
    pid_buf->table_id = 0;
    pid_buf->stream_id = 0;
    pid_buf->expect_len = 0;
    pid_buf->reserved0 = 0;
    pid_buf->pts = 0;
    pid_buf->dts = 0;
    pid_buf->has_pcr = 0;
    pid_buf->pcr = 0;

    pid_buf->payload_offset = 0;

    memset(pid_buf->buf, 0, pid_buf->buf_cap);
    pid_buf->buf_pos = 0;

    pid_buf->parsed = false;
    pid_buf->filled_up = false;
    
    pid_buf->offset_in_file = 0;
}

bool cgts_pid_buffer_complete(struct cgts_pid_buffer * pid_buf) {
    //printf("len:[%d],expect:[%d]\n", pid_buf->buf_pos, pid_buf->expect_len);
    //cgts_pid_buffer_print_hex(pid_buf);
    if (pid_buf->expect_len > 0 &&
            pid_buf->buf_pos >= pid_buf->expect_len) { // if buf length more than expect, means lots of 0xff fill up at the tail
        pid_buf->filled_up = true;
        return true;
    } else {
        pid_buf->filled_up = false;
        return false;
    }
}

/**********************************/
/********* demux context **********/
/**********************************/

struct cgts_demux_context * cgts_demux_context_alloc_with_memory(uint8_t * buf, uint32_t * buf_len) {
    struct cgts_demux_context * ct = (struct cgts_demux_context *)calloc(1, sizeof(struct cgts_demux_context));
    ct->input_type = CGTS_INPUT_TYPE_MEMORY;
    uint32_t handle_len = (*buf_len / CGTS_TS_PACKET_SIZE) * CGTS_TS_PACKET_SIZE;
    *buf_len = handle_len;
    ct->input_ptr = (uint8_t *)calloc(1, handle_len);
    memcpy(ct->input_ptr, buf, handle_len);
    ct->input_len = handle_len;
    ct->input_offset = 0;

    ct->tsp_counter = 0;

    ct->programs_num = 0;

    ct->pid_buf_num = 1;
    ct->pid_buf[(ct->pid_buf_num - 1)] = cgts_pid_buffer_alloc(0);
    ct->just_parsed_pid_buf_idx = -1;

    ct->pat_found = false;
    ct->pmt_found = false;

    ct->tsp_buffer = (uint8_t *)calloc(1, CGTS_TS_PACKET_SIZE);
    ct->tsp_buffer_is_set = false;
    ct->last_tsp_pid = -1;

    return ct;
}

struct cgts_demux_context * cgts_demux_context_alloc_with_file(const char * filename) {
    struct cgts_demux_context * ct = (struct cgts_demux_context *)calloc(1, sizeof(struct cgts_demux_context));
    ct->input_type = CGTS_INPUT_TYPE_FILE;
    ct->input_fp = fopen(filename, "r");

    ct->tsp_counter = 0;

    ct->programs_num = 0;

    ct->pid_buf_num = 1;
    ct->pid_buf[(ct->pid_buf_num - 1)] = cgts_pid_buffer_alloc(0);
    ct->just_parsed_pid_buf_idx = -1;

    ct->pat_found = false;
    ct->pmt_found = false;

    ct->tsp_buffer = (uint8_t *)calloc(1, CGTS_TS_PACKET_SIZE);
    ct->tsp_buffer_is_set = false;
    ct->last_tsp_pid = -1;

    return ct;
}

void cgts_demux_context_rewind(struct cgts_demux_context * ct) {
    if (ct->input_type == CGTS_INPUT_TYPE_FILE) {
        rewind(ct->input_fp);
    }
    ct->tsp_counter = 0;
    memset(ct->tsp_buffer, 0, CGTS_TS_PACKET_SIZE);
    ct->tsp_buffer_is_set = false;
    ct->last_tsp_pid = -1;
}

void cgts_demux_context_free(struct cgts_demux_context * ct) {
    if (ct->input_type == CGTS_INPUT_TYPE_FILE) {
        fclose(ct->input_fp);
    } else if (ct->input_type == CGTS_INPUT_TYPE_MEMORY) {
        free(ct->input_ptr);
    }
    for(int i=0; i < ct->pid_buf_num; i++) {
        cgts_pid_buffer_free(ct->pid_buf[i]);
    }
    free(ct->tsp_buffer);
    free(ct);
}

void cgts_demux_context_debug(struct cgts_demux_context * ct) {
    fprintf(stdout, "============== mpegts information =============\n");
    if (ct->input_type == CGTS_CONTEXT_INPUT_TYPE_FILE) {
        fprintf(stdout, "| input type: file\n");
    } else {
        fprintf(stdout, "| input type: memory\n");
    }
    fprintf(stdout, "| ts packet conut: %d\n", ct->tsp_counter);
    fprintf(stdout, "|\n");

    fprintf(stdout, "|  ---------- program information ----------   \n");
    for (int i=0;i<ct->programs_num;i++) {
        fprintf(stdout, "|  | program id: %d, pmt id: %d\n", ct->programs[i]->program_id, ct->programs[i]->pmt_pid);
        fprintf(stdout, "|  |     pids:");
        for (int j=0;j<ct->programs[i]->pids_num;j++) {
            char stream_type_string[32];
            cgts_stream_type_to_string(ct->programs[i]->pids_stream_type[j], stream_type_string, 32);
            fprintf(stdout, " %d(%s)", ct->programs[i]->pids[j], stream_type_string);
        }
        fprintf(stdout, "\n");
    }
    fprintf(stdout, "|  -----------------------------------------   \n");
    fprintf(stdout, "|\n");

    fprintf(stdout, "|  ------------ pid information ------------   \n");
    for (int i=0;i<ct->pid_buf_num;i++) {
        fprintf(stdout, "|  | pid: %d, table id: %d", ct->pid_buf[i]->pid, ct->pid_buf[i]->table_id);
        fprintf(stdout, ", stream id: 0x%02x", ct->pid_buf[i]->stream_id);
        if (ct->pid_buf[i]->stream_id == CGTS_STREAM_ID_AUDIO_MPEG1_MPEG2_MPEG4_AAC) {
            fprintf(stdout, "(audio)");
        } else if (ct->pid_buf[i]->stream_id == CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC) {
            fprintf(stdout, "(video)");
        }
        fprintf(stdout, ", capacity: %d, pts: %lld, dts: %lld\n"
                , ct->pid_buf[i]->buf_cap, ct->pid_buf[i]->pts, ct->pid_buf[i]->dts);
    }
    fprintf(stdout, "|  -----------------------------------------   \n");
    fprintf(stdout, "|\n");

    fprintf(stdout, "===============================================\n");
}

//
// Program Functions
//

bool cgts_demux_context_program_exist(struct cgts_demux_context * ct, uint16_t prog_id) {
    for (int i=0;i<ct->programs_num;i++) {
        if (prog_id == ct->programs[i]->program_id) {
            return true;
        }
    }
    return false;
}

int32_t cgts_demux_context_program_index(struct cgts_demux_context * ct, uint16_t prog_id) {
    for (int i=0;i<ct->programs_num;i++) {
        if (prog_id == ct->programs[i]->program_id) {
            return i;
        }
    }
    return -1;
}

bool cgts_demux_context_program_create(struct cgts_demux_context * ct, uint16_t prog_id, uint16_t pmt_pid) {
    if (ct->programs_num >= MAX_PROGRAMS_IN_SIGNLE_MPEGTS) {
        return false;
    }

    ct->programs[ct->programs_num] = cgts_program_alloc(prog_id, pmt_pid);
    ct->programs_num = ct->programs_num + 1;

    return true;
}

bool cgts_demux_context_program_delete(struct cgts_demux_context * ct, uint16_t prog_id, uint16_t pmt_pid) {
    if (ct->programs_num > MAX_PROGRAMS_IN_SIGNLE_MPEGTS) {
        return false;
    }

    bool is_found = false;
    for (int i=0;i<ct->programs_num;i++) {
        if (prog_id == ct->programs[i]->program_id) {
            cgts_program_free(ct->programs[i]);
            is_found = true;
        }
        if (is_found == true && i < ((ct->programs_num) - 1) ) {
            ct->programs[i] = ct->programs[i+1];
        }
    }

    if (is_found == false) {
        return false;
    }

    ct->programs_num = ct->programs_num - 1;
    return true;
}

//
// Pid-buffer Functions
//

bool cgts_demux_context_pid_exist(struct cgts_demux_context * ct, uint16_t pid) {
    for (int i=0;i<ct->pid_buf_num;i++) {
        if (pid == ct->pid_buf[i]->pid) {
            return true;
        }
    }
    return false;
}

int32_t cgts_demux_context_pid_buffer_index(struct cgts_demux_context * ct, uint16_t pid) {
    for (int i=0;i<ct->pid_buf_num;i++) {
        if (pid == ct->pid_buf[i]->pid) {
            return i;
        }
    }
    return -1;
}

bool cgts_demux_context_pid_create(struct cgts_demux_context * ct, uint16_t pid) {
   if (ct->pid_buf_num >= MAX_PIDS_IN_SIGNLE_MPEGTS) {
       return false;
   }

   ct->pid_buf[ct->pid_buf_num] = cgts_pid_buffer_alloc(pid);
   ct->pid_buf_num = ct->pid_buf_num + 1;

   return true;
}

int16_t cgts_demux_context_pid_type(struct cgts_demux_context * ct, uint16_t pid) {
    if (pid == CGTS_PID_PAT) {
        return CGTS_PID_TYPE_PAT;
    }

    if (pid == CGTS_PID_CAT) {
        return CGTS_PID_TYPE_PSI;
    }

    if (pid == CGTS_PID_SDT) {
        return CGTS_PID_TYPE_PSI;
    }

    for (int i=0;i<ct->programs_num;i++) {
        struct cgts_program * prog = ct->programs[i];
        if (pid == prog->pmt_pid) {
            return CGTS_PID_TYPE_PMT;
        }
        for (int j=0;j<prog->pids_num;j++) {
            if (pid == prog->pids[j]) {
                return CGTS_PID_TYPE_PES;
            }
        }
    }
    return CGTS_PID_TYPE_UNKNOWN;
}

int32_t cgts_demux_context_pid_stream_type(struct cgts_demux_context * ct, uint16_t pid) {
    for (int i=0;i<ct->programs_num;i++) {
        for (int j=0;j<ct->programs[i]->pids_num;j++) {
            if (ct->programs[i]->pids[j] == pid) {
                return (int32_t) ct->programs[i]->pids_stream_type[j];
            }
        }
    }
    return -1;
}

/**********************************/
/********** mux context ***********/
/**********************************/

struct cgts_mux_context * cgts_mux_context_alloc_with_memory(uint32_t buf_len) {
    struct cgts_mux_context * ct = (struct cgts_mux_context *)calloc(1, sizeof(struct cgts_mux_context));
    ct->output_type = CGTS_CONTEXT_OUTPUT_TYPE_MEMORY;
    ct->output_ptr = (uint8_t *)calloc(1, buf_len);
    ct->output_buffer_len = buf_len;
    ct->output_offset = 0;

    ct->tsp_counter = 0;

    ct->opt_mode = false;
    ct->pat_wrote = false;
    ct->pmt_wrote = false;
    return ct;
}

struct cgts_mux_context * cgts_mux_context_alloc_with_file(const char * filename) {
    struct cgts_mux_context * ct = (struct cgts_mux_context *)calloc(1, sizeof(struct cgts_mux_context));
    ct->output_type = CGTS_CONTEXT_OUTPUT_TYPE_FILE;
    ct->output_fp = fopen(filename, "w");

    ct->tsp_counter = 0;

    ct->opt_mode = false;
    ct->pat_wrote = false;
    ct->pmt_wrote = false;
    return ct;
}

void cgts_mux_context_free(struct cgts_mux_context * ct) {
    if (ct->output_type == CGTS_CONTEXT_OUTPUT_TYPE_FILE) {
        fclose(ct->output_fp);
    } else if (ct->output_type == CGTS_CONTEXT_OUTPUT_TYPE_MEMORY) {
        free(ct->output_ptr);
    }
    free(ct);
}

void cgts_mux_context_debug(struct cgts_mux_context * ct) {
    fprintf(stdout, "output type: ");
    if (ct->output_type == CGTS_CONTEXT_OUTPUT_TYPE_FILE) {
        fprintf(stdout, "FILE");
    } else if (ct->output_type == CGTS_CONTEXT_OUTPUT_TYPE_MEMORY) {
        fprintf(stdout, "MEMORY");
    }
    fprintf(stdout, ", ");
    fprintf(stdout, "tsp_counter: %d, "
            , ct->tsp_counter
            );
    fprintf(stdout, "opt_mode: %s, pat_wrote: %s, pmt_wrote: %s"
            , ct->opt_mode ? "yes" : "no"
            , ct->pat_wrote ? "yes" : "no"
            , ct->pmt_wrote ? "yes" : "no"
            );
    fprintf(stdout, "\n");
}


/**********************************/
/************ ts packet ***********/
/**********************************/

struct cgts_ts_packet * cgts_ts_packet_alloc() {
    struct cgts_ts_packet * tsp = (struct cgts_ts_packet *) calloc(1, sizeof(struct cgts_ts_packet));
    tsp->has_pcr = 0;
    tsp->pcr = 0;
    tsp->payload_ptr = NULL;
    tsp->payload_len = 0;
    return tsp;
}

void cgts_ts_packet_free(struct cgts_ts_packet * tsp) {
    free(tsp);
}

void cgts_ts_packet_reset(struct cgts_ts_packet * tsp) {
    tsp->sync_byte = 0;
    tsp->unit_start_indicator = 0;
    tsp->pid = 0;
    tsp->scrambling_control = 0;
    tsp->adaption_field_control = 0;
    tsp->continuity_counter = 0;
    tsp->has_adaptation = 0;
    tsp->has_payload = 0;
    tsp->has_pcr = 0;
    tsp->pcr = 0;
    tsp->payload_ptr = NULL;
    tsp->payload_len = 0;
}

void cgts_ts_packet_debug(struct cgts_ts_packet * tsp) {
    fprintf(stdout, "sync_byte:%x, start_flag:%d, pid:%d, cc:%d, payload:%d, adap:%d, ",
            tsp->sync_byte, tsp->unit_start_indicator, 
            tsp->pid, tsp->continuity_counter,
            tsp->has_payload, tsp->has_adaptation
            );

    if (tsp->pid == 0) {
        fprintf(stdout, " PAT ");
    }
    if (tsp->has_pcr != 0) {
        fprintf(stdout, " PCR: %lld ", tsp->pcr);
    }
    fprintf(stdout, "\n");
}

/**********************************/
/********** ts pxx packet *********/
/**********************************/
struct cgts_pxx_packet * cgts_pxx_packet_alloc() {
    struct cgts_pxx_packet * pxx_packet = (struct cgts_pxx_packet *)calloc(1, sizeof(struct cgts_pxx_packet));
    pxx_packet->type = CGTS_PXX_PACKET_TYPE_UNKNOWN;
    pxx_packet->data = NULL;
    pxx_packet->data_len = 0;
    pxx_packet->data_cap = 0;
    return pxx_packet;
}

void cgts_pxx_packet_free(struct cgts_pxx_packet * pxx_packet) {
    if (pxx_packet->data != NULL) {
        free(pxx_packet->data);
    }
    free(pxx_packet);
}

void cgts_pxx_packet_reset(struct cgts_pxx_packet * pxx_packet) {
    if (pxx_packet->data != NULL) {
        memset(pxx_packet->data, 0, pxx_packet->data_cap);
    }
    pxx_packet->type = CGTS_PXX_PACKET_TYPE_UNKNOWN;
    pxx_packet->data_len = 0;
}

void cgts_pxx_packet_debug(struct cgts_pxx_packet * pxx_packet) {
    fprintf(stdout, "pxx_packet_debug\n");
}

bool cgts_pxx_packet_write_data(struct cgts_pxx_packet * pxx_packet, uint8_t * buf, uint32_t buf_len) {
    if (pxx_packet->data == NULL) {
        pxx_packet->data = (uint8_t *)malloc(buf_len);
        pxx_packet->data_cap = buf_len;
    } else if (pxx_packet->data_cap < buf_len) {
        pxx_packet->data = (uint8_t *)realloc(pxx_packet->data, buf_len);
        if (pxx_packet->data == NULL) {
            return false;
        }
        pxx_packet->data_cap = buf_len;
    } 
    memset(pxx_packet->data, 0, pxx_packet->data_cap);

    memcpy(pxx_packet->data, buf, buf_len);
    return true;
}
