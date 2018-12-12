#include "cgts_util.h"

void print_hex(uint8_t * buf, int len) {
  int i;
  for(i=0;i<len;i++) {
    fprintf(stdout, " %02x", buf[i]);
    if (i % 8 == 7) {
      fprintf(stdout, " ");
    }
    if (i % 16 == 15) {
      fprintf(stdout, "\n");
    }
  }
  fprintf(stdout, "\n");
}
