#include "cgts_mux.h"

bool cgts_write_pxx_packet(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf) {
    uint32_t wrote_bytes = 0;
    bool ret = false;
    if (pid_buf->type == PXX_BUF_TYPE_PSI) {
        ret = cgts_write_psi_packet_header(ct, pid_buf, & wrote_bytes);
    } else if (pid_buf->type == PXX_BUF_TYPE_PES) {
        ret = cgts_write_pes_packet_header(ct, pid_buf, & wrote_bytes);
    }

    if (ret == false) {
        return false;
    } else if (wrote_bytes > pid_buf->buf_pos){
        return false;
    } else if (wrote_bytes == pid_buf->buf_pos){
        return true;
    }

    ret = cgts_write_pxx_packet_payload(ct, pid_buf, wrote_bytes);
    return ret;
}


bool cgts_write_psi_packet_header(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf, uint32_t * wrote_bytes) {
    if (pid_buf->type != PXX_BUF_TYPE_PSI) {
        return false;
    }

    uint32_t header_buf_len = 0;
    if (pid_buf->buf_pos <= CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE - CGTS_PSI_PACKET_HEADER_SIZE) {
        // in this case: A single ts packet can delive the whole psi packet.
        header_buf_len = pid_buf->buf_pos + CGTS_PSI_PACKET_HEADER_SIZE;
    } else {
        header_buf_len = CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE;
    }

    uint8_t * header_buf = (uint8_t *)calloc(1, header_buf_len);

    // first byte: pointer_field
    header_buf[0] = 0x00;

    // second byte: table_id
    header_buf[1] = pid_buf->table_id;

    // 3th and 4th bytes: expect_len
    header_buf[3] = pid_buf->expect_len % 256;
    header_buf[2] = (pid_buf->expect_len - (pid_buf->expect_len % 256) ) / 256;
    header_buf[2] = header_buf[2] | pid_buf->reserved0;

    // CGTS_PSI_PACKET_HEADER_SIZE equal 4
    memcpy(header_buf + CGTS_PSI_PACKET_HEADER_SIZE, pid_buf->buf, header_buf_len - CGTS_PSI_PACKET_HEADER_SIZE);

    cgts_write_ts_packet(ct, true, pid_buf, header_buf, header_buf_len, wrote_bytes);
    (*wrote_bytes) = (*wrote_bytes) - CGTS_PSI_PACKET_HEADER_SIZE;

    free(header_buf);

    return true;
}

bool cgts_write_pes_packet_header(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf, uint32_t * wrote_bytes) {
    if (pid_buf->type != PXX_BUF_TYPE_PES) {
        return false;
    }

    uint32_t header_buf_len = 0;
    if (pid_buf->buf_pos < CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE - CGTS_PES_PACKET_HEADER_SIZE) {
        header_buf_len = pid_buf->buf_pos + CGTS_PES_PACKET_HEADER_SIZE;
    } else {
        header_buf_len = CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE;
    }

    uint8_t * header_buf = (uint8_t *)calloc(1, header_buf_len);

    // first three bytes: pes start bytes
    header_buf[0] = 0x00;
    header_buf[1] = 0x00;
    header_buf[2] = 0x01;

    // 4th byte: stream_id
    header_buf[3] = pid_buf->stream_id;

    // 5th and 6th bytes: expect_len
    header_buf[5] = pid_buf->expect_len % 256;
    header_buf[4] = (pid_buf->expect_len - (pid_buf->expect_len % 256) ) / 256;

    // CGTS_PES_PACKET_HEADER_SIZE equal 6
    memcpy(header_buf + CGTS_PES_PACKET_HEADER_SIZE, pid_buf->buf, header_buf_len - CGTS_PES_PACKET_HEADER_SIZE);

    cgts_write_ts_packet(ct, true, pid_buf, header_buf, header_buf_len, wrote_bytes);
    (*wrote_bytes) = (*wrote_bytes) - CGTS_PES_PACKET_HEADER_SIZE;

    free(header_buf);
    return true;
}

bool cgts_write_pxx_packet_payload(struct cgts_mux_context *ct, struct cgts_pid_buffer * pid_buf, uint32_t pid_buf_offset) {
    uint32_t wrote_bytes = 0;
    while(true) {
        cgts_write_ts_packet(ct, false, pid_buf, pid_buf->buf + pid_buf_offset, pid_buf->buf_pos - pid_buf_offset, &wrote_bytes);
        pid_buf_offset = pid_buf_offset + wrote_bytes;
        if (pid_buf_offset == pid_buf->buf_pos) {
            break;
        } else if (pid_buf_offset > pid_buf->buf_pos) {
            return false;
        }
    }
    return true;
}

bool cgts_write_ts_packet(struct cgts_mux_context * ct, bool is_pes_start, struct cgts_pid_buffer * pid_buf, uint8_t * payload, uint32_t payload_len, uint32_t * wrote_bytes) {
    uint16_t pid = pid_buf->pid;
    uint8_t * tsp_buf = (uint8_t *)calloc(1, CGTS_TS_PACKET_SIZE);

    // first byte: sync byte
    tsp_buf[0] = CGTS_SYNC_BYTE;

    // 2th and 3th bytes: start indicator AND pid
    tsp_buf[2] = pid % 256;
    tsp_buf[1] = (pid - pid % 256) / 256;
    tsp_buf[1] = tsp_buf[1] & 0x1f;
    if (is_pes_start == true) {
        tsp_buf[1] = tsp_buf[1] | 0x40;
    } else {
        tsp_buf[1] = tsp_buf[1] | 0x00;
    }

    // 4th byte: continuity counter AND adaotation field control AND scrambling control
    tsp_buf[3] = pid_buf->ccounter;
    tsp_buf[3] = tsp_buf[3] & 0x0f;
    tsp_buf[3] = tsp_buf[3] | 0x00; // scrambling control: no scrambling

    uint16_t tsp_buf_len = 0;
    if (payload_len < CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE) {
        /********************************************************************/
        /*
         *                       --- CASE A ---
         *                 --- THE LAST TS PACKET ---
         *
         *      This is the LAST ts PACKET of a single pes packet,
         *      we MUST use adaptaion field padding the ts packet.
         *
         *                                                                  */
        /********************************************************************/
        (*wrote_bytes) = payload_len;
        tsp_buf_len = CGTS_TS_PACKET_HEADER_SIZE + payload_len;

        tsp_buf[3] = tsp_buf[3] | 0x30; // adaptation field control : adaptation_field followed by payload 

        /*** write adaptation_field start ***/
        uint8_t adaptation_len = CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE - payload_len - 1;
        uint8_t adaptation_flags = 0x00;
        tsp_buf[4] = adaptation_len;
        tsp_buf[5] = adaptation_flags;
        for(int i=6;i<CGTS_TS_PACKET_SIZE-payload_len;i++) {
            tsp_buf[i] = 0xff;
        }
        /*** write adaptation_field end ***/

        /*** write payload start ***/
        for(int j=CGTS_TS_PACKET_SIZE-payload_len;j<CGTS_TS_PACKET_SIZE;j++) {
            tsp_buf[j] = payload[j-(CGTS_TS_PACKET_SIZE-payload_len)];
        }
        /*** write payload end ***/
    } else if (is_pes_start == true && pid_buf->has_pcr != 0) {
        /********************************************************************/
        /*
         *                       --- CASE B ---
         *                 --- TS PACKET WITH PCR---
         *
         *      This is the ts PACKET taken a PCR value,
         *      we MUST indicate PCR in the adaptaion field.
         *
         *                                                                  */
        /********************************************************************/
        uint8_t adaptation_len = 7; //PCR need 6btye + 1byte flags
        (*wrote_bytes) = CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE - adaptation_len - 1; // 1 byte for indicate adaptation length
        tsp_buf[3] = tsp_buf[3] | 0x30; // adaptation field control : adaptation_field followed by payload

        /*** write adaptation_field start ***/
        uint8_t adaptation_flags = 0x10;
        tsp_buf[4] = adaptation_len;
        tsp_buf[5] = adaptation_flags;

        // start write PCR
        int64_t pcr_low  = pid_buf->pcr % 300;
        int64_t pcr_high = pid_buf->pcr / 300;

        tsp_buf[6]  = pcr_high >> 25;
        tsp_buf[7]  = pcr_high >> 17;
        tsp_buf[8]  = pcr_high >>  9;
        tsp_buf[9]  = pcr_high >>  1;
        tsp_buf[10] = pcr_high <<  7 | pcr_low >> 8 | 0x7e;
        tsp_buf[11] = pcr_low;
        /*** write adaptation_field end ***/

        /*** write payload start ***/
        for(int i=CGTS_TS_PACKET_HEADER_SIZE+adaptation_len+1;i<CGTS_TS_PACKET_SIZE;i++) {
            tsp_buf[i] = payload[i-CGTS_TS_PACKET_HEADER_SIZE-adaptation_len-1];
        }
        /*** write payload end ***/

    } else {
        /********************************************************************/
        /*
         *                       --- CASE C ---
         *                --- NOT THE LAST TS PACKET ---
         *
         *      This is NOT THE LAST ts packet of a single pes packet,
         *      we fill it with only real and useful payload only,
         *      no adaptation here.
         *
         *                                                                  */
        /********************************************************************/
        (*wrote_bytes) = CGTS_TS_PACKET_SIZE - CGTS_TS_PACKET_HEADER_SIZE;
        tsp_buf_len = CGTS_TS_PACKET_SIZE;
        tsp_buf[3] = tsp_buf[3] | 0x10; // adaptation field control : payload only
        /*** write payload start ***/
        for(int i=CGTS_TS_PACKET_HEADER_SIZE;i<CGTS_TS_PACKET_SIZE;i++) {
            tsp_buf[i] = payload[i-CGTS_TS_PACKET_HEADER_SIZE];
        }
        /*** write payload end ***/
    }

    cgts_write_bytes(ct, tsp_buf, CGTS_TS_PACKET_SIZE);
    ct->tsp_counter = ct->tsp_counter + 1;
    if (pid_buf->ccounter == 15) {
        pid_buf->ccounter = 0;
    } else {
        pid_buf->ccounter = pid_buf->ccounter + 1;
    }

    free(tsp_buf);

    return true;
}

bool cgts_write_bytes(struct cgts_mux_context * ct, uint8_t * buf, uint32_t buf_len) {
    if (ct->output_type == CGTS_CONTEXT_OUTPUT_TYPE_FILE) {
        fwrite(buf, buf_len, 1, ct->output_fp);
    } else if (ct->output_type == CGTS_CONTEXT_OUTPUT_TYPE_MEMORY) {
        if (ct->output_offset >= ct->output_buffer_len || (ct->output_buffer_len - ct->output_offset) < buf_len) {
            return false;
        }
        memcpy(ct->output_ptr + ct->output_offset, buf, buf_len);
        ct->output_offset += buf_len;
    } else {
        return false;
    }
    return true;
}
