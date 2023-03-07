/* main.c
 */

#include <math.h>

#include "jprint.h"

int main(int argc, char **argv) {
	char buf[2048];
	int err;
	jprint_t jp;

	jp_open(&jp, buf, sizeof buf);
	jp_printf(&jp, "{}");
	err = jp_close(&jp);
	printf("%d %s\n", err, jp_errorstring(err));
	puts(buf);

	jp_open(&jp, buf, sizeof buf);
	jp_printf(&jp, " [ : , \t ] ");
	err = jp_close(&jp);
	printf("%d %s\n", err, jp_errorstring(err));
	puts(buf);

	jp_open(&jp, buf, sizeof buf);
	jp_printf(&jp, "{");
	jp_printf(&jp, "item1: %d, item2: %d", 10, -55);
	jp_printf(&jp, "%k %s", "key", "value");
	jp_printf(&jp, "true: %b, false: %b", B_TRUE, B_FALSE);
	jp_printf(&jp, "}");
	err = jp_close(&jp);
	printf("%d %s\n", err, jp_errorstring(err));
	puts(buf);

	jp_open(&jp, buf, sizeof buf);
	jp_printf(&jp, "[");
	jp_printf(&jp, "{}");
	jp_printf(&jp, "{ nest\\ key: %s }", NULL);
	jp_printf(&jp, "[]");
	jp_printf(&jp, "%b", B_TRUE);
	jp_printf(&jp, "this_key_ignored %g", 1.5);

        /* Now, we convert some numbers... NONE of these are
         * in the standard... we just want to know what happens...
         * (we should not crash, or fail)
         */
        jp_printf(&jp, "%g", nan(""));   /* nan */
        jp_printf(&jp, "%g", sqrt(-1));  /* -nan */
        jp_printf(&jp, "%g", INFINITY);  /* +infinity */
        jp_printf(&jp, "%g", 1.0 / 0.0); /* +infinity */
        jp_printf(&jp, "%g", log(0.0));  /* -infinity */

        /* We are not testing denormal numbers... just the "normal range"
         * for ieee double precision floating point. These numbers should
         * be converted to exactly the input.
         */
        jp_printf(&jp, "%g",  2.22507e-308);
        jp_printf(&jp, "%g", -2.22507e-308);
        jp_printf(&jp, "%g",  1.79769e308);
        jp_printf(&jp, "%g", -1.79769e308);

        /* We will do 2, 3, 4 byte sequence
	 */
        jp_printf(&jp, "%s", "some utf-8: µΔ㾯𖭜 :");

	/* Do all 255 characters (1..255), \0x00 is end of string, so
	 * we won't bother with that.
	 */
        char s[256];
        for (int i = 1; i < 256; ++i)
            s[i - 1] =  i;
        s[255] = '\0';
	jp_printf(&jp, "%s", s);

	jp_printf(&jp, "]");
	err = jp_close(&jp);
	printf("%d %s\n", err, jp_errorstring(err));
	puts(buf);
	return 0;
}

