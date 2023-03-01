# jwrite
This is a library to allow simple generation of JSON. Originally
written by TonyWilk; later published on Github: 

https://github.com/jonaskgandersson/jWrite


Original article on codeproject: 
    http://www.codeproject.com/Articles/887604/jWrite-a-really-simple-JSON-writer-in-C

TonyWilk
17mar2015

I have reviewed the code, and made changes necessary to use as
a kernel level component (for Linux and FreeBSD).

snprintf() is used to convert integer (int64_t) and double to
string. This has been reviewed on Linux, but not on FreeBSD
yet.

Note that there is really no special handling for nan, or
infinity. These cases are produced in the test, but handling
may vary.

Style has been altered (slightly) to match my preferred style.

gcc main.c jwrite.c

to generate a.out, which illustrates the tektonics.
