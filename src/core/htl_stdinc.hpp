#ifndef _htl_stdinc_hpp
#define _htl_stdinc_hpp

/*
#include <htl_stdinc.hpp>
*/

// for 32bit os, 2G big file limit for unistd io, 
// ie. read/write/lseek to use 64bits size for huge file.
#ifndef _FILE_OFFSET_BITS
    #define _FILE_OFFSET_BITS 64
#endif

// for int64_t print using PRId64 format.
#ifndef __STDC_FORMAT_MACROS
    #define __STDC_FORMAT_MACROS
#endif

#endif

