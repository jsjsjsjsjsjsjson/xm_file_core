#include <stdio.h>
#include "xm_file.h"

XMFile xmfile;

int main() {
    xmfile.open_xm("./woodz_n_moodz.xm");
    int ret = xmfile.read_all();
    // xmfile.print_pattern(21, 0, 7, 0, 64);
    xmfile.save_as("./test.xm");
    return ret;
}