#include "cgts_demux.h"

#define DEBUG_TS_PACKET_LAYER       0
#define DEBUF_PES_PACKET_LAYER      0

bool cgts_read_pxx_packet(struct cgts_demux_context * ct, struct cgts_pid_buffer ** pxx_packet) {
    struct cgts_ts_packet * tsp = cgts_ts_packet_alloc();
    while(true) {
        if (ct->tsp_buffer_is_set == true) {
            ct->tsp_buffer_is_set = false;
        } else {
            if (cgts_get188(ct, ct->tsp_buffer) == false) {
                // flush pxx packet left in cgts_demux_context
                for (int16_t i=0;i<ct->pid_buf_num;i++) {
                    struct cgts_pid_buffer * left_pid_buf = ct->pid_buf[i];
                    if (left_pid_buf->buf_pos > 0 && left_pid_buf->filled_up == false && left_pid_buf->parsed == false) {
                        left_pid_buf->filled_up = true;
                        cgts_pxx_parse(ct, left_pid_buf);
                        (*pxx_packet) = left_pid_buf;
                        return true;
                    }
                }
                return false;
            }
        }

        cgts_ts_packet_parse(ct, tsp, ct->tsp_buffer);

        // append ts packet to pxx packet
        if (cgts_demux_context_pid_exist(ct, tsp->pid) == false) {
            cgts_demux_context_pid_create(ct, tsp->pid);
        }

        int32_t pid_buffer_index = cgts_demux_context_pid_buffer_index(ct, tsp->pid);
        int16_t pid_type = cgts_demux_context_pid_type(ct, tsp->pid);
        if (pid_type == CGTS_PID_TYPE_UNKNOWN) {
            continue;
        }

        if (tsp->unit_start_indicator) {
            if (ct->last_tsp_pid != -1 ) {
                struct cgts_pid_buffer * prev_pid_buf = ct->pid_buf[cgts_demux_context_pid_buffer_index(ct, tsp->pid)];
                if( prev_pid_buf->filled_up == false
                        && prev_pid_buf->buf_pos > 0 ) {
                    prev_pid_buf->filled_up = true;

                    cgts_pxx_parse(ct, prev_pid_buf);
                    ct->tsp_buffer_is_set = true;

                    (*pxx_packet) = prev_pid_buf;
                    break;
                }
            }

            cgts_pid_buffer_reset(ct->pid_buf[pid_buffer_index]);
            if (ct->input_type == CGTS_INPUT_TYPE_FILE) {
                ct->pid_buf[pid_buffer_index]->offset_in_file = (uint64_t) ftell(ct->input_fp) - CGTS_TS_PACKET_SIZE;
            }
            if (pid_type == CGTS_PID_TYPE_PAT
                    || pid_type == CGTS_PID_TYPE_PMT
                    || pid_type == CGTS_PID_TYPE_PSI ) {
                cgts_pid_buffer_append_psi_header(ct->pid_buf[pid_buffer_index], tsp);
            } else {
                cgts_pid_buffer_append_pes_header(ct->pid_buf[pid_buffer_index], tsp);
            }
        } else {
            cgts_pid_buffer_append(ct->pid_buf[pid_buffer_index], tsp->payload_ptr, tsp->payload_len);
        }

        // check this TS packet complete the PXX packet, if not skip following processes
        if (cgts_pid_buffer_complete(ct->pid_buf[pid_buffer_index]) == false) {
            ct->pid_buf[pid_buffer_index]->parsed = false;
            ct->pid_buf[pid_buffer_index]->filled_up = false;
            ct->just_parsed_pid_buf_idx = -1;
        } else {
            ct->pid_buf[pid_buffer_index]->filled_up = true;
            cgts_pxx_parse(ct, ct->pid_buf[pid_buffer_index]);

            ct->just_parsed_pid_buf_idx = cgts_demux_context_pid_buffer_index(ct, ct->pid_buf[pid_buffer_index]->pid);
        }

        ct->last_tsp_pid = tsp->pid;

        // cleanup works
        cgts_ts_packet_reset(tsp);
        memset(ct->tsp_buffer, 0, CGTS_TS_PACKET_SIZE);

        if (ct->just_parsed_pid_buf_idx != -1) {
            struct cgts_pid_buffer * pid_buf = ct->pid_buf[ct->just_parsed_pid_buf_idx];
            (*pxx_packet) = pid_buf;
            break;
        }
    }
    cgts_ts_packet_free(tsp);
    return true;
}

bool cgts_pxx_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf) {
    switch (cgts_demux_context_pid_type(ct, pid_buf->pid)) {
        case CGTS_PID_TYPE_PAT:
            cgts_pat_parse(ct, pid_buf);
            break;
        case CGTS_PID_TYPE_PMT:
            cgts_pmt_parse(ct, pid_buf);
            break;
        case CGTS_PID_TYPE_PES:
            cgts_pes_parse(ct, pid_buf);
            break;
    }
    pid_buf->parsed = true;
    return true;
}

int64_t cgts_pes_parse_pts_dts(uint8_t * buf) {
    // following SPEC ISO-13818-1 page 31
    int64_t ts_32_30 = (int64_t)(*buf & 0x0e) << 29;
    int64_t ts_29_15 = ( ( (buf[1] << 8) | buf[2] ) >> 1) << 15;
    int64_t ts_14_0  = ( (buf[3] << 8) | buf[4] ) >> 1;
    int64_t ts = ts_32_30 | ts_29_15 | ts_14_0;
    return ts;
}

bool cgts_pes_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf) {
    //printf("hihi, i am pes!, my stream id is [%02x]\n", pid_buf->stream_id);
    if (pid_buf->stream_id == CGTS_STREAM_ID_PADDING_STREAM) {
        /* skip padding stream */
        return true;
    }

    if (pid_buf->stream_id == CGTS_STREAM_ID_PROGRAM_STREAM_MAP
            || pid_buf->stream_id == CGTS_STREAM_ID_PROGRAM_STREAM_MAP
            || pid_buf->stream_id == CGTS_STREAM_ID_PRIVATE_STREAM_2
            || pid_buf->stream_id == CGTS_STREAM_ID_ECM
            || pid_buf->stream_id == CGTS_STREAM_ID_EMM
            || pid_buf->stream_id == CGTS_STREAM_ID_PROGRAM_STREAM_DIRECTORY
            || pid_buf->stream_id == CGTS_STREAM_ID_DSMCC_STREAM
            || pid_buf->stream_id == CGTS_STREAM_ID_H_222_1_TYPE_E
            ) {
        /* Maybe: pass the payload to upper-layer application */
        return true;
    }

    // go on HERE!
    // start parse the longest main PES part in SPEC

    uint8_t * p = pid_buf->buf;
    uint8_t useless_flags = p[0];
    uint8_t flags = p[1];
    uint8_t pes_header_length = p[2];

    /*****************************************/
    /**                                     **/
    /**            FLAGS`s STRUCT           **/
    /**                                     **/
    /**  PTS_DTS_flags               2 bit  **/
    /**  ES Clock Refrence flag      1 bit  **/   
    /**  ES_rate flag                1 bit  **/   
    /**  DSM_trick_mode_flag         1 bit  **/   
    /**  additional_copy_info_flag   1 bit  **/   
    /**  PES_CRC_flag                1 bit  **/   
    /**  PES_extension_flag          1 bit  **/   
    /**                                     **/
    /*****************************************/

    if ((flags & 0xc0) == 0x80) { 
        /* pts_dts_flag == 10 -- only pts shall be present */
        pid_buf->pts = pid_buf->dts = cgts_pes_parse_pts_dts(p + 3 /* 2 bytes flags + 1 byte pes header length */);
    } else if ((flags & 0xc0) == 0xc0) {
        /* pts_dts_flag == 11 -- pts and dts shall be present */
        pid_buf->pts = cgts_pes_parse_pts_dts(p + 3 /* 2 bytes flags + 1 byte pes header length */);
        pid_buf->dts = cgts_pes_parse_pts_dts(p 
                + 3 /* 2 bytes flags + 1 byte pes header length */
                + 5 /* the previous pts value used 5 bytes */);
    } else {
        /* pts_dts_flag == 00 -- no pts and dts shall be present */
        /* pts_dts_flag == 01 -- forbidden */
    }
    //fprintf(stdout, "pts: %lld, dts: %lld\n", pid_buf->pts, pid_buf->dts);

    pid_buf->payload_offset = pes_header_length 
        + 1 /* [0]:useless flags            */
        + 1 /* [1]:flags                    */
        + 1 /* [2]:pes_header_length        */;
#if DEBUF_PES_PACKET_LAYER
    printf("pid: %d, payload_offset: %d, stream_id: %02x\n", pid_buf->pid, pid_buf->payload_offset, pid_buf->stream_id);
    //print_hex(pid_buf->buf + pid_buf->payload_offset, 8);
    print_hex(pid_buf->buf + pid_buf->payload_offset, pid_buf->buf_pos - pid_buf->payload_offset);
    printf("\n");
#endif
    return true;
}

bool cgts_pmt_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf) {
    //printf("PMT found\n");
    uint8_t * p = pid_buf->buf;
    uint16_t program_id = (p[0] << 8) | p[1];
    uint8_t version = (p[2] >> 1) & 0x1f;
    uint8_t sec_num = p[3];
    uint8_t last_sec_num = p[4];
    //printf("program_id:[%d], version: [%d], sec_num: [%d], last_sec_num:[%d]\n", program_id, version, sec_num, last_sec_num);

    int16_t pcr_pid = ((p[5] << 8) | p[6] ) & 0x1fff;
    int16_t program_info_length = ((p[7] << 8) | p[8] ) & 0x0fff;
    //printf("pcr_pid:[%d], program_info_length: [%d]\n", pcr_pid, program_info_length);
    //exit(0);

    int16_t ptr_moved = 7 + program_info_length;
    if (ptr_moved >= pid_buf->buf_pos) {
        return false;
    }

    //cgts_pid_buffer_print_hex(pid_buf);
    uint8_t * remain_buf = p + 9 + program_info_length; /* skip program info length */
    int32_t remain_buf_len = pid_buf->buf_pos - 9 /* bytes before pid map in pmt */ - program_info_length;
    while(remain_buf_len >= 5 /* min pid desc length is 40 bit = 5 byte */ ) {
        /********************************************/
        /* stream type  -- 8  bit                   */
        /* reserved     -- 3  bit                   */
        /* pid          -- 13 bit                   */
        /* reserved     -- 4  bit                   */
        /* info length  -- 12 bit                   */
        /* info         -- (info length) * 8  bit   */
        /********************************************/

        uint16_t read_bytes = 0;
        int16_t stream_type = remain_buf[0];
        if (stream_type < 0) {
            break;
        } else {
            read_bytes += 1;
        }

        int32_t pid = (remain_buf[1] << 8) | remain_buf[2];
        if (pid < 0) {
            break;
        } else {
            read_bytes += 2;
            pid &= 0x1fff;
        }

        int32_t es_info_length = (remain_buf[3] << 8) | remain_buf[4];
        if (es_info_length < 0) {
            break;
        } else {
            read_bytes += 2;
            es_info_length &= 0x0fff;
        }

        int32_t program_index = cgts_demux_context_program_index(ct, program_id);
        if (cgts_program_pid_exist(ct->programs[program_index], pid) == false) {
            cgts_program_pid_add(ct->programs[program_index], pid, stream_type);
        }

        remain_buf = remain_buf + read_bytes + es_info_length;
        remain_buf_len = remain_buf_len - read_bytes - es_info_length;
    }

    ct->pmt_found = true;
    return true;
}

bool cgts_pat_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf) {
    if (pid_buf->pid != 0) {
        return false;
    }
    uint8_t * p = pid_buf->buf;
    uint16_t stream_id = (p[0] << 8) | p[1];
    uint8_t version = (p[2] >> 1) & 0x1f;
    uint8_t sec_num = p[3];
    uint8_t last_sec_num = p[4];
    //printf("table_id:[%d], sec_len:[%d]\n", table_id, section_length);

    uint16_t i = 0;
    while(true) {
        if (5 /* header(8) - bytes before section length(3) */ 
                + i /* prev id pair */ 
                + 4 /* this id pair*/ 
                + 4 /* crc32 */ 
                >= pid_buf->expect_len + 3) {
            break;
        }
        uint16_t program_id = 
            (p[5 /* header */ +i] << 8)             /* first byte */ 
            | p[6+i]                                /* second byte */ ; 
        uint16_t pmt_pid = 
            ((p[5+i+2] << 8)                        /* third byte */ 
             | p[6+i+2])                            /* fourth byte */ & 0x1fff; 
        //printf("progid:[%d], pid:[%d]\n", program_id, pmt_pid);
        i += 4;

        // fill PMT`s pid into context
        if (cgts_demux_context_program_exist(ct, program_id) == false) {
            cgts_demux_context_program_create(ct, program_id, pmt_pid);
        } else {
            cgts_demux_context_program_delete(ct, program_id, pmt_pid);
            cgts_demux_context_program_create(ct, program_id, pmt_pid);
        }
    }

    ct->pat_found = true;
    return true;
}

bool cgts_ts_packet_parse(struct cgts_demux_context * ct, struct cgts_ts_packet * tsp, uint8_t * buf) {
    ct->tsp_counter++;

    tsp->sync_byte = buf[0];
    tsp->unit_start_indicator = (buf[1] & 0x40) >> 6;
    tsp->pid = (buf[1] & 0x1f) * 256 + buf[2];
    tsp->scrambling_control = (buf[3] >> 4) & 0xc;
    tsp->adaption_field_control = (buf[3] >> 4) & 0x3;
    tsp->continuity_counter = (buf[3] & 0xf);

    tsp->has_adaptation = (tsp->adaption_field_control & 2) >> 1;
    tsp->has_payload = tsp->adaption_field_control & 1;

    tsp->payload_len = CGTS_TS_PACKET_SIZE - 4;
    tsp->payload_ptr = buf + 4; /*ts header*/

#if DEBUG_TS_PACKET_LAYER
    //print_hex(buf, 188);
#endif
    if (tsp->has_adaptation) {
        uint8_t adaptation_len = buf[4];
        uint8_t adaptation_flags = buf[5];

        if (adaptation_flags & 0x10) {   // PCR_flag, means a PCR is in this tspacket
            if (adaptation_len >= 7) { // PCR need 6btye + 1byte flags
                int64_t pcr_high = buf[6] * 256 * 256 * 256 + buf[7] * 256 * 256 + buf[8] * 256 + buf[9];
                pcr_high = (pcr_high << 1) | (buf[10] >> 7);
                int64_t pcr_low = ((buf[10] & 1) << 8) | buf[11];
                tsp->pcr = pcr_high * 300 + pcr_low;
                tsp->has_pcr = 1;
            }
        }
        //fprintf(stdout, "pcr: %lld\n", tsp->pcr);

        tsp->payload_ptr = tsp->payload_ptr + adaptation_len /* afc length */ + 1 /* afc header */;
        tsp->payload_len = tsp->payload_len - adaptation_len - 1;
    }
#if DEBUG_TS_PACKET_LAYER
    cgts_ts_packet_debug(tsp);
#endif

    if (!(tsp->has_payload)) {
        return false;
    }
    if (tsp->payload_ptr >= buf + CGTS_TS_PACKET_SIZE 
            || tsp->payload_len <= 0) {
        return false;
    }

    /* TS Packet Header Finished */

    return true;
}

bool cgts_pid_buffer_append_pes_header(struct cgts_pid_buffer * pid_buf, struct cgts_ts_packet * tsp) {
    uint8_t * ts_payload = tsp->payload_ptr;
    uint32_t ts_payload_len = tsp->payload_len;

    if (! (ts_payload[0] == 0x00 && ts_payload[1] == 0x00 && ts_payload[2] == 0x01 ) ) {
        fprintf(stdout, "PES header error\n");
        return false;
    }
    pid_buf->type = PXX_BUF_TYPE_PES;
    pid_buf->stream_id = ts_payload[3];
    pid_buf->expect_len = (ts_payload[4] << 8) | ts_payload[5];

    if (tsp->has_pcr != 0) {
        pid_buf->has_pcr = tsp->has_pcr;
        pid_buf->pcr = tsp->pcr;
    }

    //printf("expect length in header:[%d]\n", pid_buf->expect_len);
    return cgts_pid_buffer_append(pid_buf
            , ts_payload + 6 /* 6 Btyes = 3 (packet_start_code_prefix) + 1 (stream_id) + 2 (PES_packet_length) */
            , ts_payload_len - 6 );
}

bool cgts_pid_buffer_append_psi_header(struct cgts_pid_buffer * pid_buf, struct cgts_ts_packet * tsp) {
    uint8_t * ts_payload = tsp->payload_ptr;
    uint32_t ts_payload_len = tsp->payload_len;

    uint8_t pointer_field = ts_payload[0];
    //printf("pointer_field:[%d]\n", pointer_field);
    const uint8_t * p = ts_payload + pointer_field + 1;
    pid_buf->type = PXX_BUF_TYPE_PSI;
    pid_buf->table_id = p[0];
    pid_buf->reserved0 = p[1] & 0xf0;
    pid_buf->expect_len = ( (p[1] & 0x0f) << 8) | p[2];

    if (tsp->has_pcr != 0) {
        pid_buf->has_pcr = tsp->has_pcr;
        pid_buf->pcr = tsp->pcr;
    }

    return cgts_pid_buffer_append(
            pid_buf
            , p + 3 /* skip table_id and section_length */
            , ts_payload_len - 1 /* pointer_field length */ - pointer_field - 3);
}

bool cgts_get188_from_file(FILE * fp, uint8_t * buf) {
    int read_bytes = (int) fread(buf, 1, CGTS_TS_PACKET_SIZE, fp);
    if (read_bytes == CGTS_TS_PACKET_SIZE) {
        return true;
    }
    return false;
}

bool cgts_get188_from_memory(struct cgts_demux_context * ct, uint8_t * buf) {
    if (ct->input_offset >= ct->input_len || (ct->input_len - ct->input_offset) < CGTS_TS_PACKET_SIZE) {
        return false;
    }
    memcpy(buf, ct->input_ptr + ct->input_offset, CGTS_TS_PACKET_SIZE);
    ct->input_offset += CGTS_TS_PACKET_SIZE;
    return true;
}

bool cgts_get188(struct cgts_demux_context * ct, uint8_t * buf) {
    if (ct->input_type == CGTS_INPUT_TYPE_FILE) {
        return cgts_get188_from_file(ct->input_fp, buf);
    } else if (ct->input_type == CGTS_INPUT_TYPE_MEMORY) {
        return cgts_get188_from_memory(ct, buf);
    } else {
        return false;
    }
}
