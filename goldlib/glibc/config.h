/* This is handmade generic config.h */

#if !defined WINDOWS32 && defined _WIN32
#define WINDOWS32     1
#endif

#define HAVE_DIRENT_H 1
#define STDC_HEADERS  1
#define HAVE_UNISTD_H 1
#define HAVE_STRING_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRCOLL  1

#define __P(protos)   protos