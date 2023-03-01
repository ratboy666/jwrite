/* main.c
 *
 * Test harness command-line for jWrite 
 *
 * TonyWilk
 * 17mar2015
 */

#include <stdio.h>
#include "jwrite.h"
#include <math.h>

void jWriteTest();

int main(int argc, char * argv[]) {
    jWriteTest();
    return 0;
}


void jWriteTest(void) {
    char buffer[1024];
    size_t buflen = sizeof buffer;
    int err;
    jwc_t jwc;

    printf("jwrite - JSON writer test case:\n\n" );

    printf("A JSON object example:\n\n" );
    jwOpen(&jwc, buffer, buflen, JW_OBJECT, true);

    jwObj_string(&jwc, "key", "value");
    jwObj_int(&jwc, "int", 1);
    jwObj_double(&jwc, "double", 1.234);
    jwObj_null(&jwc, "nullThing");
    jwObj_bool(&jwc, "bool", true);
    jwObj_array(&jwc, "EmptyArray"); /* empty array */
    jwEnd(&jwc);
    jwObj_array(&jwc, "anArray");
	jwArr_string(&jwc, "array one");
	jwArr_int(&jwc, 2);
	jwArr_double(&jwc, 1234.567);
	jwArr_null(&jwc);
	jwArr_bool(&jwc, 0);
	jwArr_object(&jwc);
	    jwObj_string(&jwc, "obj3_one", "one");
	    jwObj_string(&jwc, "obj3_two", "two");
	jwEnd(&jwc);
	jwArr_array(&jwc);
	    jwArr_int(&jwc, 0);
	    jwArr_int(&jwc, 1);
	    jwArr_int(&jwc, 2);
	jwEnd(&jwc);
    jwEnd(&jwc);

    jwObj_object(&jwc, "EmptyObject");
    jwEnd(&jwc);

    jwObj_object(&jwc, "anObject");
	jwObj_string(&jwc, "msg","object in object");
	jwObj_string(&jwc, "msg2","object in object 2nd entry");
    jwEnd(&jwc);
    jwObj_string(&jwc, "ObjEntry", "This is the last one");

    err = jwClose(&jwc);

    printf(buffer);
    if (err != JWRITE_OK)
	printf( "\nError: %s at function call %d\n", jwErrorToString(err),
	                                             jwErrorPos(&jwc));

    printf("\n\nA JSON array example:\n\n" );

    jwOpen(&jwc, buffer, buflen, JW_ARRAY, true);
    jwArr_string(&jwc, "String value");
    jwArr_int(&jwc, 1234);
    jwArr_double(&jwc, 567.89012);
    jwArr_bool(&jwc, true);
    jwArr_null(&jwc);
    jwArr_object(&jwc); /* empty object */
    jwEnd(&jwc);
    jwArr_object(&jwc);
	jwObj_string(&jwc, "key", "value");
	jwObj_string(&jwc, "key2", "value2");
    jwEnd(&jwc);
    jwArr_array(&jwc); /* array in array */
	jwArr_string(&jwc, "Array in array");
	jwArr_string(&jwc, "the end");
    jwEnd(&jwc);
    /* Now, we convert some numbers... NONE of these are
     * in the standard... we just want to know what happens...
     * (we should not crash, or fail)
     */
    jwArr_double(&jwc, nan(""));   /* nan */
    jwArr_double(&jwc, sqrt(-1));  /* -nan */
    jwArr_double(&jwc, INFINITY);  /* +infinity */
    jwArr_double(&jwc, 1.0 / 0.0); /* +infinity */
    jwArr_double(&jwc, log(0.0));  /* -infinity */

    /* We are not testing denormal numbers... just the "normal range"
     * for ieee double precision floating point. These numbers should
     * be converted to exactly the input.
     */
    jwArr_double(&jwc, 2.22507e-308);
    jwArr_double(&jwc, -2.22507e-308);
    jwArr_double(&jwc, 1.79769e308);
    jwArr_double(&jwc, -1.79769e308);

    err = jwClose(&jwc);

    printf(buffer);
    if (err != JWRITE_OK)
	printf("\nError: %s at function call %d\n", jwErrorToString(err),
	                                            jwErrorPos(&jwc));

    printf("\n\nExample error:\n\n");
    /* Annotated with call number */
    jwOpen(&jwc, buffer, buflen, JW_ARRAY, true);			// 1
    jwArr_string(&jwc, "String value");					// 2
    jwArr_int(&jwc, 1234);						// 3
    jwArr_double(&jwc, 567.89012);					// 4
    jwArr_bool(&jwc, true);						// 5
    jwArr_null(&jwc);							// 6
    jwArr_object(&jwc);	/* empty object */				// 7
    //jwEnd(&jwc);
    printf("Before error: jwError() -> %d\n", jwError(&jwc));
    jwArr_object(&jwc);	/* <-- this is where the error is */		// 8
    printf("After error: jwError() -> %d\n", jwError(&jwc));
	jwObj_string(&jwc, "key", "value");		    		// 9
	jwObj_string(&jwc, "key2", "value2");				// 10
    jwEnd(&jwc);							// 11 
    jwArr_array(&jwc);	/* array in array */				// 12
	jwArr_string(&jwc, "Array in array");				// 13
	jwArr_string(&jwc, "the end");					// 14
    jwEnd(&jwc);							// 15
    err= jwClose(&jwc);							// 16

    printf(buffer);
    if (err != JWRITE_OK)
	printf( "\nError: %s at function call %d\n", jwErrorToString(err),
	                                             jwErrorPos(&jwc));

    return;
}

