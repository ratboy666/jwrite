/* jwrite.h
 *
 * A *really* simple JSON writer in C
 *  - a collection of functions to generate JSON semi-automatically
 *
 * The idea is to simplify writing native C values into a JSON string and
 * to provide some error trapping to ensure that the result is valid JSON.
 *
 * https://www.rfc-editor.org/rfc/rfc8259.html
 *
 * Example:
 *
 *  jwc_t jwc;
 *  jwOpen(&jwc, buffer, buflen, JW_OBJECT, true);
 *  jwObj_string(&jwc, "key", "value");
 *  jwObj_int(&jwc, "int", 1);
 *  jwObj_array(&jwc, "anArray");
 *    jwArr_int(&jwc, 0);
 *    jwArr_int(&jwc, 1);
 *    jwArr_int(&jwc, 2);
 *  jwEnd(&jwc);
 *  err = jwClose(&jwc);
 *
 * results:
 *
 *  {
 *    "key": "value",
 *    "int": 1,
 *    "anArray": [
 *      0,
 *      1,
 *      2
 *    ]
 *  }
 *
 * Note that jWrite handles string quoting and getting commas in the right
 * place.
 * If the sequence of calls is incorrect
 * e.g.
 *   jwOpen(&jwc, buffer, buflen, JW_OBJECT, 1);
 *   jwObj_string(&jwc, "key", "value");
 *   jwArr_int(&jwc, 0);
 *   ...
 *
 * then the error code returned from jwClose() would indicate that you
 * attempted to put an array element into an object (instead of a key:value
 * pair). To locate the error, the supplied buffer has the JSON created up to
 * the error point and a call to jwErrorPos() would return the function call
 * at which the error occurred - in this case 3, the 3rd function call
 * "jwArr_int(0)" is not correct at this point.
 *
 * The root JSON type can be JW_OBJECT or JW_ARRAY.
 *
 * For more information on each function, see the prototypes below.
 *
 * TonyWilk, Mar 2015
 * Modified for Kernel, Fred Weigel 2023
 */

#include <stdbool.h>
#define boolean_t bool
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* int64_t used because we may want to output pointers as integer.
 * Careful when reading the values... json itself does not define
 * numeric ranges.
 */

#define JWRITE_STACK_DEPTH 32 /* max nesting depth of objects/arrays */

enum jwNodeType {
    JW_OBJECT = 1,
    JW_ARRAY
};

struct jwNodeStack {
    enum jwNodeType nodeType;
    int elementNo;
};

typedef struct jWriteControl {
    char *buffer;       /* pointer to application's buffer */
    size_t buflen;      /* length of buffer */
    char *bufp;         /* current write position in buffer */
    char tmpbuf[32];    /* local buffer for conversions */
    int error;          /* error code */
    int callNo;         /* API call on which error occurred */
    struct jwNodeStack  /* stack of array/object nodes */
        nodeStack[JWRITE_STACK_DEPTH];
    int stackpos;
    boolean_t isPretty; /* pretty output (inserts \n and spaces) */
} jwc_t;


/* Error Codes
 * -----------
 */
#define JWRITE_OK          0
#define JWRITE_BUF_FULL    1 /* output buffer full */
#define JWRITE_NOT_ARRAY   2 /* tried to write Array value into Object */
#define JWRITE_NOT_OBJECT  3 /* tried to write Object key/value into Array */
#define JWRITE_STACK_FULL  4 /* array/object nesting > JWRITE_STACK_DEPTH */
#define JWRITE_STACK_EMPTY 5 /* stack underflow error (too many 'end's) */
#define JWRITE_NEST_ERROR  6 /* not all objects closed on jwClose() */


/* API functions
 * -------------
 */

/* Returns '\0'-terminated string describing the error (as returned by
 * jwClose())
 */
char *jwErrorToString(int err);

/* Return error "up to now" in the building of the json. This is the
 * return that jwEnd() and jwClose() return... but gives the error
 * sooner. JWRITE_BUF_FULL may be of interest to expand the buffer.
 */
int jwError(jwc_t *jwc);

/* jwOpen
 * - initialises jWrite with the application supplied 'buffer' of length
 *   'buflen'
 *   in operation, the buffer will always contain a valid '\0'-terminated
 *   string
 * - jWrite will not overrun the buffer (it returns an "output buffer full"
 *   error)
 * - rootType is the base JSON type: JW_OBJECT or JW_ARRAY
 * - isPretty controls 'prettifying' the output: JW_PRETTY or JW_COMPACT
 */
void jwOpen(jwc_t *jwc, char *buffer, size_t buflen,
            enum jwNodeType rootType, boolean_t isPretty);

/* jwClose
 * - closes the element opened by jwOpen()
 * - returns error code (0 = JWRITE_OK)
 * - after an error, all following jWrite calls are skipped internally
 *   so the error code is for the first error detected
 */
int jwClose(jwc_t *jwc);

/* jwErrorPos
 * - if jwClose returned an error, this function returns the sequence number
 *   of the jWrite function call which caused that error.
 */
int jwErrorPos(jwc_t *jwc);

/* Object insertion functions
 * - used to insert "key":"value" pairs into an object
 */
void jwObj_string(jwc_t *jwc, char *key, char *value);
void jwObj_int(jwc_t *jwc, char *key, int64_t value);
void jwObj_double(jwc_t *jwc, char *key, double value);
void jwObj_bool(jwc_t *jwc, char *key, boolean_t value);
void jwObj_null(jwc_t *jwc, char *key);
void jwObj_object(jwc_t *jwc, char *key);
void jwObj_array(jwc_t *jwc, char *key);

/* Array insertion functions
 * - used to insert "value" elements into an array
 */
void jwArr_string(jwc_t *jwc, char *value);
void jwArr_int(jwc_t *jwc, int64_t value);
void jwArr_double(jwc_t  *jwc, double value);
void jwArr_bool(jwc_t *jwc, boolean_t value);
void jwArr_null(jwc_t *jwc);
void jwArr_object(jwc_t *jwc);
void jwArr_array(jwc_t *jwc);

/* jwEnd
 * - defines the end of an Object or Array definition
 */
int jwEnd(jwc_t *jwc);

/* these 'raw' routines write the JSON value as the contents of rawtext
 * i.e. enclosing quotes are not added
 * - use if your app. supplies its own value->string functions
 */
void jwObj_raw(jwc_t *jwc, char *key, char *rawtext);
void jwArr_raw(jwc_t *jwc, char *rawtext);

/* end of jwrite.h */
