#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#include "cgts_demux.h"

int main(int argc, char *argv[]) {

    // Init
    char * input_filename = argv[1];
    fprintf(stderr, "input filename: \t%s\n", input_filename);
    struct cgts_demux_context * demux_ct = cgts_demux_context_alloc_with_file(input_filename);

    // Work
    struct cgts_pid_buffer * packet = NULL;
    while(cgts_read_pxx_packet(demux_ct, &packet) == true) {
        cgts_pid_buffer_debug(packet);
        cgts_pid_buffer_reset(packet);
    }
    cgts_demux_context_debug(demux_ct);

    // Finalize
    cgts_demux_context_free(demux_ct);

    return 0;
}
