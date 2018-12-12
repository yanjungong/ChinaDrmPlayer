#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cgts_demux.h"
#include "cgts_mux.h"
#include "cgts_nal_adts_parse.h"

int main(int argc, char *argv[]) {

    // Init
    char * input_filename = argv[1];
    fprintf(stderr, "input filename: \t%s\n", input_filename);
    struct cgts_demux_context * demux_ct = cgts_demux_context_alloc_with_file(input_filename);

    // --- remux mode ---
    struct cgts_pid_buffer * packet = NULL;
    while(cgts_read_pxx_packet(demux_ct, &packet) == true) {
        //cgts_pid_buffer_debug(packet);

        if (packet->type == PXX_BUF_TYPE_PES
                && packet->stream_id == CGTS_STREAM_ID_VIDEO_MPEG1_MPEG2_MPEG4_AVC_HEVC ) 
        {
            uint32_t nalu_start_pos = 0;
            uint32_t nalu_end_pos = 0;
            uint32_t payload_start_pos = packet->payload_offset;

            while(cgts_find_nal_unit(packet->buf, packet->buf_pos, payload_start_pos, &nalu_start_pos, &nalu_end_pos) == true) {
                int32_t stream_type = cgts_demux_context_pid_stream_type(demux_ct, packet->pid);
                printf("NAL found, stream type: %02x, start-end: %d-%d, ", stream_type, nalu_start_pos, nalu_end_pos);

                if (stream_type == CGTS_STREAM_TYPE_VIDEO_AVC) 
                {
                    //uint8_t nalu_type = packet->buf[(nalu_start_pos + 4)] & 0x1f;
                    uint8_t nalu_type = cgts_find_nal_type_avc(packet->buf + nalu_start_pos);
                    printf("NAL type: AVC 0x%02x", nalu_type);

                    if (nalu_type == CGTS_NAL_TYPE_AVC_IDR_SLICE ) {
                        printf(", keyframe ");
                    }
                } 
                else if (stream_type == CGTS_STREAM_TYPE_VIDEO_HEVC) 
                {
                    //uint8_t nalu_type = (packet->buf[(nalu_start_pos + 4)] >> 1) & 0x3f;
                    uint8_t nalu_type = cgts_find_nal_type_hevc(packet->buf + nalu_start_pos);
                    printf("NAL type: HEVC 0x%02x(%d)", nalu_type, nalu_type);

                    if (nalu_type == CGTS_NAL_TYPE_HEVC_CRA_NUT
                            || nalu_type == CGTS_NAL_TYPE_HEVC_IDR_N_LP 
                            || nalu_type == CGTS_NAL_TYPE_HEVC_CRA_NUT ) {
                        printf(", keyframe ");
                    }
                }
                printf("\n");
                payload_start_pos = nalu_end_pos;
            }
        }
    }
    cgts_demux_context_debug(demux_ct);

    // Finalize
    cgts_demux_context_free(demux_ct);

    return 0;
}
