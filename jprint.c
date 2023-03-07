/* jprint.c
 */

/* If in ZFS */
//#include <sys/zfs_context.h>
//#include <sys/jprint.h>

/* If standalone */
#include "jprint.h"

/* Here is one for the ages:
 *
 * Compile on Linux (Fedora 37):
 * jprint.c:226:43: error: SSE register argument with SSE disabled
 * 226 |                                         x = va_arg(ap, double);
 * cc1: all warnings being treated as errors
 *
 * So, we replace this with an argument fetch of long. BUILD_BUG_ON
 * is used to validate that sizeof (double) == sizeof (long)
 *
 * If the sizes are the same, the alignment should be aok, and we
 * type-pun through a pointer.
 */

/* BUILD_BUG_ON(sizeof(someThing) != PAGE_SIZE);
 */
#ifdef BUILD_BUG_ON
#undef BUILD_BUG_ON
#endif
#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))


/* literal key length maximum
 */
#define KEYLEN 255


/* return error position (call number of jp_printf)
 */
int jp_errorpos(jprint_t *jp) {
	return (jp->ncall);
}


/* return string for error code
 */
const char *jp_errorstring(int err) {
	switch (err) {
	case JPRINT_OK:          return "jprint ok";
	case JPRINT_BUF_FULL:    return "jprint buffer full";
	case JPRINT_NEST_ERROR:  return "jprint nest error";
	case JPRINT_STACK_FULL:  return "jprint stack full";
	case JPRINT_STACK_EMPTY: return "jprint stack empty";
	case JPRINT_OPEN:        return "jprint open";
	case JPRINT_FMT:         return "jprint format";
	}
	return "jprint: unknown error";
}


/* return error from jprint_t
 */
int jp_error(jprint_t *jp) {
	return (jp->error);
}


/* open json using buffer of length buflen
 */
void jp_open(jprint_t *jp, char *buffer, size_t buflen) {
	jp->buffer = jp->bufp = buffer;
	jp->buflen = buflen;
	jp->error = JPRINT_OK;
	jp->ncall = 0;
	jp->stackp = -1;
}


/* close json
 */
int jp_close(jprint_t *jp) {
	if (jp->stackp != -1)
		jp->error = JPRINT_OPEN;
	return (jp->error);
}


/* put character to json
 */
static void jp_putc(jprint_t *jp, char c) {
	if (jp->error == JPRINT_OK) {
		if ((jp->bufp - jp->buffer + 1) >= jp->buflen)
			jp->error = JPRINT_BUF_FULL;
		else {
			*jp->bufp++ = c;
			*jp->bufp = '\0';
		}
	}
}


/* put string to json
 */
static void jp_puts(jprint_t *jp, char *s) {
	while (*s)
		jp_putc(jp, *s++);
}


/* put quoted string to json
 */
static void jp_putsq(jprint_t *jp, char *s) {
	static const char *hex = "0123456789ABCDEF";
	int c;

	if (s == NULL) {
		jp_puts(jp, (char *)"null");
		return;
	}
	jp_putc(jp, '\"');
	while (*s != '\0') {
                c = (int)*s++;
                /* formfeed, newline, return, tab, backspace
                 */
                if (c == 12)
                        jp_puts(jp, (char *)"\\f");
                else if (c == 10)
                        jp_puts(jp, (char *)"\\n");
                else if (c == 13)
                        jp_puts(jp, (char *)"\\r");
                else if (c == 9)
                        jp_puts(jp, (char *)"\\t");
                else if (c == 8)
                        jp_puts(jp, (char *)"\\b");
               /* all characters from 0x00 to 0x1f, and 0x7f are
                * escaped as: \u00xx
                */
                else if (((0 <= c) && (c <= 0x1f)) || (c == 0x7f)) {
                        jp_puts(jp, (char *)"\\u00");
                        jp_putc(jp, hex[(c >> 4) & 0x0f]);
                        jp_putc(jp, hex[c & 0x0f]);
                /* " \ /
                 */
                } else if (c == '"')
                        jp_puts(jp, (char *)"\\\"");
                else if (c == '\\')
                        jp_puts(jp, (char *)"\\\\");
                else if (c == '/')
                        jp_puts(jp, (char *)"\\/");
                /* all other printable characters ' ' to '~', and
                 * any utf-8 sequences (high bit set):
                 * 1xxxxxxx 10xxxxxx ...
                 * is a utf-8 sequence (10xxxxxx may occur 1 to 3 times).
                 * Note that this is simply distinguished here as high
                 * bit set.
                 */
                else
			jp_putc(jp, (char)c);
        }
	jp_putc(jp, '\"');
}


/* put out key if object open. error if nothing open
 */
static int jp_key(jprint_t *jp, char *key) {
	if (jp->error != JPRINT_OK)
		goto err;
	/* at top level, no frame exists yet, no error
	 */
	if (jp->stackp == -1)
		goto err;
	/* stackp has been "popped" too many times
	 */
	if (jp->stackp < -1) {
		jp->error = JPRINT_STACK_EMPTY;
		goto err;
	}
	/* put comma separator in (both object and array)
	 */
	if (++jp->stack[jp->stackp].nelem > 1)
		jp_putc(jp, ',');
	/* if its in an object, put out the key and separator
	 */
	if (jp->stack[jp->stackp].type == JP_OBJECT) {
		jp_putsq(jp, key);
		jp_putc(jp, ':');
	}
err:
	return (jp->error);
}


/* printf to json
 */
int jp_printf(jprint_t *jp, const char *fmt, ...) {
	char key[KEYLEN + 1];
	int k, i;
	va_list ap;
	int64_t n;
	double x;
	boolean_t b;
	char *s;
	char *start = jp->bufp;

	++jp->ncall;
	va_start(ap, fmt);
	key[k = 0] = '\0';
	while (*fmt && (jp->error == JPRINT_OK)) {
		switch (*fmt) {
		case '%':
			++fmt;
			switch (*fmt) {
			case 'k': /* next parameter is key */
				s = va_arg(ap, char *);
				if (strlen(s) <= KEYLEN)
					strcpy(key, s);
				else
					jp->error = JPRINT_FMT;
				break;
			case 'd': /* next parameter is int */
				n = va_arg(ap, int);
				goto int_cmn;
			case 'l': /* next parameter is int64_t */
				n = va_arg(ap, int64_t);
int_cmn:			if (jp_key(jp, key) == JPRINT_OK) {
					i = snprintf(
					    jp->tmpbuf, sizeof (jp->tmpbuf),
				            "%lld", (long long int)n);
					if ((i >= sizeof (jp->tmpbuf)) ||
					    (i < 0))
						jp_puts(jp, (char *)"####");
					else
						jp_puts(jp, jp->tmpbuf);
				}
				key[k = 0] = '\0';
				break;
			case 's': /* next parameter is string */
				if (jp_key(jp, key) == JPRINT_OK) {
					s = va_arg(ap, char *);
					jp_putsq(jp, s);
				}
				key[k = 0] = '\0';
				break;
			case 'g': /* next parameter is double */
				if (jp_key(jp, key) == JPRINT_OK) {
BUILD_BUG_ON(sizeof(double) != sizeof(long));
					long t;
					volatile double *p = (double *)&t;
					t = va_arg(ap, long);
					x = *p;
					i = snprintf(
					    jp->tmpbuf, sizeof (jp->tmpbuf),
				            "%g", x);
					if ((i >= sizeof (jp->tmpbuf)) ||
					    (i < 0))
						jp_puts(jp, (char *)"####");
					else
						jp_puts(jp, jp->tmpbuf);
				}
				key[k = 0] = '\0';
				break;
			case 'b': /* next parameter is boolean */
				if (jp_key(jp, key) == JPRINT_OK) {
					b = (boolean_t)va_arg(ap, int);
					s = b ? (char *)"true" :
					        (char *)"false";
					jp_puts(jp, s);
				}
				key[k = 0] = '\0';
				break;
			case '%': /* literal % */
				if (k < KEYLEN) {
					key[k++] = '%';
					key[k] = '\0';
				} else
					jp->error = JPRINT_FMT;
				break;
			default:
				jp->error = JPRINT_FMT;
			}
			break;
		case '{': /* open object */
			if (jp->stackp >= (JP_MAX_STACK - 1))
				jp->error = JPRINT_STACK_FULL;
			else {
				(void) jp_key(jp, key);
				++jp->stackp;
				jp->stack[jp->stackp].type = JP_OBJECT;
				jp->stack[jp->stackp].nelem = 0;
				jp_putc(jp, '{');
			}
			break;
		case '}': /* close object */
			if (jp->stackp < 0)
				jp->error = JPRINT_STACK_EMPTY;
			else if (jp->stack[jp->stackp].type != JP_OBJECT)
				jp->error = JPRINT_NEST_ERROR;
			else {
				--jp->stackp;
				jp_putc(jp, '}');
			}
			break;
		case '[': /* open array */
			if (jp->stackp >= (JP_MAX_STACK - 1))
				jp->error = JPRINT_STACK_FULL;
			else {
				(void) jp_key(jp, key);
				++jp->stackp;
				jp->stack[jp->stackp].type = JP_ARRAY;
				jp->stack[jp->stackp].nelem = 0;
				jp_putc(jp, '[');
			}
			break;
		case ']': /* close array */
			if (jp->stackp < 0)
				jp->error = JPRINT_STACK_EMPTY;
			else if (jp->stack[jp->stackp].type != JP_ARRAY)
				jp->error = JPRINT_NEST_ERROR;
			else {
				--jp->stackp;
				jp_putc(jp, ']');
			}
			break;

		case ',': /* ,: space tab are ignored */
		case ':':
		case ' ':
		case '\t':
			break;
		case '\\':
			/* allow inclusion of ,: space tab to key */
			if (fmt[1] == '\0')
				jp->error = JPRINT_FMT;
			++fmt;
			/* and drop through */
			goto deflt;
		default:
deflt:
			if (k < KEYLEN) {
				key[k++] = *fmt;
				key[k] = '\0';
			} else
				jp->error = JPRINT_FMT;
			break;
		}
		++fmt;
	}
	va_end(ap);
	if (jp->error != JPRINT_OK)
		return (-1);
	
	return (int)(jp->bufp - start);
}

