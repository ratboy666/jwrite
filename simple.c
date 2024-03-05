/* simple.c
 */

#include "jprint.h"

int main(int argc, char **argv) {
    char buf[2048];
    int err;
    jprint_t jp;

    jp_open(&jp, buf, sizeof buf);
    jp_printf(&jp, "{");                // open root node as object 
    jp_printf(&jp, "key: %s", "value"); // writes "key":"value"
    jp_printf(&jp, "int: %d", 1);       // writes "int":1
    jp_printf(&jp, "anArray: [");       // start "anArray": [...] 
    jp_printf(&jp, "%d", 0);            // add a few integers to the array
    jp_printf(&jp, "%d", 1);
    jp_printf(&jp, "%d", 2);
    jp_printf(&jp, "]");                // end the array
    jp_printf(&jp, "}");                // close root node
    err = jp_close(&jp);
    puts(buf);
    return 0;
}
