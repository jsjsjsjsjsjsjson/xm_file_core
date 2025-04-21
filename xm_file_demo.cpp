#include <stdio.h>
#include "xm_file.h"

XMFile xmfile;

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <input .XM> <output .XM>\n", argv[0]);
        return -1;
    }
    xmfile.open_xm(argv[1]);
    xmfile.read_all();
    xmfile.save_as(argv[2]);
    return 0;
}