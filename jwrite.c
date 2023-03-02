/* jwrite.c  version 1v3
 *
 * A *really* simple JSON writer in C
 *
 * see: jwrite.h for info
 *
 * from jWrite.c:
 * https://github.com/jonaskgandersson/jWrite
 * TonyWilk, Mar 2015
 * Redo for kernel use by Fred Weigel, 2023
 */

#include "jwrite.h"

static void jwPutch(jwc_t *jwc, char c);
static void jwPutstr(jwc_t *jwc, char *str);
static void jwPutraw(jwc_t *jwc, char *str);
static void jwPretty(jwc_t *jwc);
static enum jwNodeType jwPop(jwc_t *jwc);
static void jwPush(jwc_t *jwc, enum jwNodeType nodeType);
static int _jwObj(jwc_t *jwc, char *key);
static int _jwArr(jwc_t *jwc);


/* jwOpen
 * - open writing of JSON starting with rootType = JW_OBJECT or JW_ARRAY
 * - initialise with user string buffer of length buflen
 * - isPretty = true, adds \n and spaces to prettify output
 */
void jwOpen(jwc_t *jwc, char *buffer, size_t buflen, 
		        enum jwNodeType rootType, boolean_t isPretty) {
    jwc->buffer = buffer;
    jwc->buflen = buflen;
    jwc->bufp = buffer;
    jwc->nodeStack[0].nodeType = rootType;
    jwc->nodeStack[0].elementNo = 0;
    jwc->stackpos = 0;
    jwc->error = JWRITE_OK;
    jwc->callNo = 1;
    jwc->isPretty = isPretty;
    jwPutch(jwc, (rootType == JW_OBJECT) ? '{' : '[');
}


/* jwClose
 * - closes the root JSON object started by jwOpen()
 * - returns error code
 */
int jwClose(jwc_t *jwc) {
    if (jwc->error == JWRITE_OK) {
        if (jwc->stackpos == 0) {
            enum jwNodeType node = jwc->nodeStack[0].nodeType;
            if (jwc->isPretty) {
	        jwPutch(jwc, '\n');
                jwPutch(jwc, (node == JW_OBJECT) ? '}' : ']');
	        jwPutch(jwc, '\n');
            }
	} else
	    jwc->error = JWRITE_NEST_ERROR;
    }
    return jwc->error;
}


/* End the current array/object
 */
int jwEnd(jwc_t *jwc) {
    if (jwc->error == JWRITE_OK) {
	enum jwNodeType node;
	int lastElemNo= jwc->nodeStack[jwc->stackpos].elementNo;
	node = jwPop(jwc);
	if (lastElemNo > 0)
	    jwPretty(jwc);
	jwPutch(jwc, (node == JW_OBJECT) ? '}' : ']');
    }
    return jwc->error;
}


/* jwErrorPos
 * - Returns position of error: the nth call to a jWrite function
 */
int jwErrorPos(jwc_t *jwc) {
    return jwc->callNo;
}


/* Object insert functions
 */


/* put raw string to object (i.e. contents of rawtext without quotes)
 */
void jwObj_raw(jwc_t *jwc, char *key, char *rawtext) {
    if (_jwObj(jwc, key) == JWRITE_OK)
        jwPutraw(jwc, rawtext);
}


/* put "quoted" string to object
 */
void jwObj_string(jwc_t *jwc, char *key, char *value) {
    if (_jwObj(jwc, key) == JWRITE_OK)
        jwPutstr(jwc, value);
}


void jwObj_int(jwc_t *jwc, char *key, int64_t value) {
    int n;
    n = snprintf(jwc->tmpbuf, sizeof (jwc->tmpbuf), "%ld", value);
    if ((n >= sizeof (jwc->tmpbuf)) || (n < 0))
        jwObj_raw(jwc, key, "####");
    else
        jwObj_raw(jwc, key, jwc->tmpbuf);
}


void jwObj_double(jwc_t *jwc, char *key, double value) {
    int n;
    n = snprintf(jwc->tmpbuf, sizeof (jwc->tmpbuf), "%g", value);
    jwObj_raw(jwc, key, jwc->tmpbuf);
    if ((n >= sizeof (jwc->tmpbuf)) || (n < 0))
	jwObj_raw(jwc, key, "####");
    else
	jwObj_raw(jwc, key, jwc->tmpbuf);
}


void jwObj_bool(jwc_t *jwc, char *key, boolean_t value) {
    jwObj_raw(jwc, key, value ? "true" : "false");
}


void jwObj_null(jwc_t *jwc, char *key) {
    jwObj_raw(jwc, key, "null");
}


/* put Object in Object
 */
void jwObj_object(jwc_t *jwc, char *key) {
    if (_jwObj(jwc, key ) == JWRITE_OK)	{
        jwPutch(jwc, '{');
	jwPush(jwc, JW_OBJECT);
    }
}


/* put Array in Object
 */
void jwObj_array(jwc_t *jwc, char *key) {
    if (_jwObj(jwc, key) == JWRITE_OK) {
    	jwPutch(jwc, '[');
    	jwPush(jwc, JW_ARRAY);
    }
}

  
/* Array insert functions
 */

/* put raw string to array (i.e. contents of rawtext without quotes)
 */
void jwArr_raw(jwc_t *jwc, char *rawtext) {
    if (_jwArr(jwc) == JWRITE_OK)
        jwPutraw(jwc, rawtext);
}


/* put "quoted" string to array
 */
void jwArr_string(jwc_t *jwc, char *value) {
    if (_jwArr(jwc) == JWRITE_OK)
        jwPutstr(jwc, value);
}


void jwArr_int( jwc_t *jwc, int64_t value )
{
    int n;
    n = snprintf(jwc->tmpbuf, sizeof (jwc->tmpbuf), "%ld", value);
    if ((n >= sizeof (jwc->tmpbuf)) || (n < 0))
        jwArr_raw(jwc, "####");
    else
        jwArr_raw(jwc, jwc->tmpbuf);
}


void jwArr_double(jwc_t *jwc, double value) {
    int n;
    n = snprintf(jwc->tmpbuf, sizeof (jwc->tmpbuf), "%g", value);
    if ((n >= sizeof (jwc->tmpbuf)) || (n < 0))
	jwArr_raw(jwc, "####");
    else
	jwArr_raw(jwc, jwc->tmpbuf);
}


void jwArr_bool(jwc_t *jwc, boolean_t value) {
    jwArr_raw(jwc, value ? "true" : "false");
}


void jwArr_null(jwc_t *jwc) {
    jwArr_raw(jwc, "null");
}


void jwArr_object(jwc_t *jwc) {
    if (_jwArr(jwc) == JWRITE_OK) {
        jwPutch(jwc, '{');
	jwPush(jwc, JW_OBJECT);
    }
}


void jwArr_array(jwc_t *jwc) {
    if (_jwArr(jwc) == JWRITE_OK) {
        jwPutch(jwc, '[');
	jwPush(jwc, JW_ARRAY);
    }
}


/* jwError
 * - return the error, if any, to date
 */
int jwError(jwc_t *jwc) {
    return jwc->error;
}


/* jwErrorToString
 * - returns string describing error code
 */
char *jwErrorToString(int err) {
    switch (err) {
    case JWRITE_OK:          return "OK"; 
    case JWRITE_BUF_FULL:    return "output buffer full";
    case JWRITE_NOT_ARRAY:   return "tried to write Array value into Object";
    case JWRITE_NOT_OBJECT:
        return "tried to write Object key/value into Array";
    case JWRITE_STACK_FULL:  return "array/object nesting > JWRITE_STACK_DEPTH";
    case JWRITE_STACK_EMPTY: return "stack underflow error (too many 'end's)";
    case JWRITE_NEST_ERROR:
        return "nesting error, not all objects closed when jwClose() called";
    }
    return "Unknown error";
}


/* Internal functions
 */

static void jwPretty(jwc_t *jwc) {
    int i;
    if (jwc->isPretty) {
        jwPutch(jwc, '\n');
        for (i = 0; i < jwc->stackpos + 1; ++i)
            jwPutraw(jwc, "  ");
    }
}


/* Push / Pop node stack
 */
static void jwPush(jwc_t *jwc, enum jwNodeType nodeType) {
    if ((jwc->stackpos + 1) >= JWRITE_STACK_DEPTH)
        jwc->error = JWRITE_STACK_FULL;
    else {
        jwc->nodeStack[++jwc->stackpos].nodeType = nodeType;
	jwc->nodeStack[jwc->stackpos].elementNo = 0;
    }
}


static enum jwNodeType jwPop(jwc_t *jwc) {
    enum jwNodeType retval = jwc->nodeStack[jwc->stackpos].nodeType;
    if (jwc->stackpos == 0)
        jwc->error = JWRITE_STACK_EMPTY;
    else
        --jwc->stackpos;
    return retval;
}


static void jwPutch(jwc_t *jwc, char c) {
    if ((jwc->bufp - jwc->buffer + 1) >= jwc->buflen)
        jwc->error = JWRITE_BUF_FULL;
    else {
        *jwc->bufp++ = c;
	*jwc->bufp = '\0';
    }
}


/* Put string enclosed in quotes. This handles escaping the
 * contents. jwPutraw() can be used if that is not desired.
 * Note that this supports ONLY valid utf-8 strings. It
 * is the callers responsibility to produce such strings.
 */
static void jwPutstr(jwc_t *jwc, char *str) {
    static char *hex = "0123456789ABCDEF";
    jwPutch(jwc, '\"');
    while (*str != '\0') {
        int c = *str++;
	/* formfeed, newline, return, tab, backspace
	 */
	if (c == 12)
	    jwPutraw(jwc, "\\f");
	else if (c == 10)
	    jwPutraw(jwc, "\\n");
	else if (c == 13)
	    jwPutraw(jwc, "\\r");
	else if (c == 9)
	    jwPutraw(jwc, "\\t");
	else if (c == 8)
	    jwPutraw(jwc, "\\b");
	/* all characters from 0x00 to 0x1f, and 0x7f are
	 * escaped as: \u00xx
	 */
	else if (((0 <= c) && (c <= 0x1f)) || (c == 0x7f)) {
	    jwPutraw(jwc, "\\u00");
	    jwPutch(jwc, hex[(c >> 4) & 0x0f]);
	    jwPutch(jwc, hex[c & 0x0f]);
	/* " \ /
	 */
	} else if (c == '"')
	    jwPutraw(jwc, "\\\"");
	else if (c == '\\')
	    jwPutraw(jwc, "\\\\");
	else if (c == '/')
	    jwPutraw(jwc, "\\/");
	/* all other printable characters ' ' to '~', and
	 * any utf-8 sequences (high bit set):
	 * 1xxxxxxx 10xxxxxx ...
	 * is a utf-8 sequence (10xxxxxx may occur 1 to 3 times).
	 * Note that this is simply distinguished here as high
	 * bit set.
	 */
	else
            jwPutch(jwc, c);
    }
    jwPutch(jwc, '\"');
}


/* put raw string
 */
static void jwPutraw(jwc_t *jwc, char *str) {
    while (*str != '\0')
        jwPutch(jwc, *str++);
}


/* *common Object function*
 * - checks error
 * - checks current node is OBJECT
 * - adds comma if reqd
 * - adds "key" :
 */
static int _jwObj(jwc_t *jwc, char *key) {
    if (jwc->error == JWRITE_OK) {
        ++jwc->callNo;
    if (jwc->nodeStack[jwc->stackpos].nodeType != JW_OBJECT)
        jwc->error = JWRITE_NOT_OBJECT;
    else if (jwc->nodeStack[jwc->stackpos].elementNo++ > 0)
        jwPutch(jwc, ',');
    jwPretty(jwc);
    jwPutstr(jwc, key);
    jwPutch(jwc, ':');
    if(jwc->isPretty)
        jwPutch(jwc, ' ');
    }
    return jwc->error;
}


/* *common Array function*
 * - checks error
 * - checks current node is ARRAY
 * - adds comma if reqd
 */
static int _jwArr(jwc_t *jwc) {
    if (jwc->error == JWRITE_OK) {
        ++jwc->callNo;
	if (jwc->nodeStack[jwc->stackpos].nodeType != JW_ARRAY)
	    jwc->error = JWRITE_NOT_ARRAY;
        else if (jwc->nodeStack[jwc->stackpos].elementNo++ > 0)
	    jwPutch(jwc, ',');
	jwPretty(jwc);
    }
    return jwc->error;
}

