#ifndef __CGTS_MUX_H__
#define __CGTS_MUX_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cgts_structs.h"
#include "cgts_util.h"

bool cgts_write_pxx_packet(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf);
bool cgts_write_psi_packet_header(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf, uint32_t * wrote_bytes);
bool cgts_write_pes_packet_header(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf, uint32_t * wrote_bytes);
bool cgts_write_pxx_packet_payload(struct cgts_mux_context * ct, struct cgts_pid_buffer * pid_buf, uint32_t pid_buf_offset);
bool cgts_write_ts_packet(struct cgts_mux_context * ct, bool is_pes_start, struct cgts_pid_buffer * pid_buf, uint8_t * payload, uint32_t payload_len, uint32_t * wrote_bytes);
bool cgts_write_bytes(struct cgts_mux_context * ct, uint8_t * buf, uint32_t buf_len);

#endif
