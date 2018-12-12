#ifndef __CGTS_DEMUX_H__
#define __CGTS_DEMUX_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cgts_structs.h"
#include "cgts_util.h"

bool cgts_read_pxx_packet(struct cgts_demux_context * ct, struct cgts_pid_buffer ** pxx_packet);
bool cgts_pxx_packet_append(struct cgts_demux_context * ct, uint16_t pid, bool is_start, const uint8_t * ts_payload, uint32_t ts_payload_len);

int64_t cgts_pes_parse_pts_dts(uint8_t * buf);
bool cgts_pxx_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf);
bool cgts_pes_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf);
bool cgts_pmt_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf);
bool cgts_pat_parse(struct cgts_demux_context * ct, struct cgts_pid_buffer * pid_buf);

bool cgts_pid_buffer_append_psi_header(struct cgts_pid_buffer * pid_buf, struct cgts_ts_packet * tsp);
bool cgts_pid_buffer_append_pes_header(struct cgts_pid_buffer * pid_buf, struct cgts_ts_packet * tsp);
bool cgts_ts_packet_parse(struct cgts_demux_context * ct, struct cgts_ts_packet * tsp, uint8_t * buf);

bool cgts_get188_from_file(FILE * fp, uint8_t * buf);
bool cgts_get188(struct cgts_demux_context * context, uint8_t * buf);

#endif
