//  This may look like C code, but it is really -*- C++ -*-

//  ------------------------------------------------------------------
//  The Goldware Library
//  Copyright (C) 1990-1999 Odinn Sorensen
//  Copyright (C) 1999-2000 Alexander S. Aganichev
//  ------------------------------------------------------------------
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this program; if not, write to the Free
//  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
//  MA 02111-1307, USA
//  ------------------------------------------------------------------
//  $Id$
//  ------------------------------------------------------------------
//  Basic definitions and types.
//  ------------------------------------------------------------------

#ifndef __goldall_h
#define __goldall_h


//  ------------------------------------------------------------------

#include <cstddef>
#include <gcmpall.h>


//  ------------------------------------------------------------------
//  Define portability and shorthand notation

#ifndef and
#define not      !
#define and      &&
#define or       ||
#endif

#ifndef true
#define true  1
#define false 0
#endif

#define NO     0
#define YES    1
#define ALWAYS 2
#define ASK    2
#define GAUTO  3
#define MAYBE  4

#define NUL '\x00'    // Common ASCII control codes
#define BEL '\x07'
#define BS  '\x08'
#define HT  '\x09'
#define LF  '\x0A'
#define FF  '\x0C'
#define CR  '\x0D'
#define ESC '\x1B'

#ifdef __UNIX__
#define NL "\r\n"
#else
#define NL "\n"
#endif


//  ------------------------------------------------------------------
//  Special character constants

#define CTRL_A '\x01'   // FidoNet kludge line char
#define SOFTCR '\x8D'   // "Soft" carriage-return


//  ------------------------------------------------------------------
//  Supplements for the built-in types

typedef   signed char   schar;
typedef unsigned char   uchar;
typedef unsigned short ushort;
typedef unsigned int     uint;
typedef unsigned long   ulong;
typedef unsigned char    byte;
typedef   signed char   sbyte;
typedef unsigned short   word;
typedef   signed short  sword;
typedef unsigned long   dword;
typedef   signed long  sdword;


//  ------------------------------------------------------------------
//  Common function-pointer types

typedef void (*VfvCP)();
typedef int (*IfvCP)();
typedef int (*IfcpCP)(char*);


//  ------------------------------------------------------------------
//  Function pointer for stdlib qsort(), bsearch() compare functions

typedef int (*StdCmpCP)(const void*, const void*);


//  ------------------------------------------------------------------
//  Utility templates

template <class T> inline bool in_range(T a, T b, T c)   { return (a >= b) and (a <= c); }
template <class T> inline    T absolute(T a)             { return a < 0 ? -a : a; }
template <class T> inline  int compare_two(T a, T b)     { return a < b ? -1 : a > b ? 1 : 0; }
template <class T> inline    T minimum_of_two(T a, T b)  { return __extension__ (a <? b); }
template <class T> inline    T maximum_of_two(T a, T b)  { return __extension__ (a >? b); }
template <class T> inline  int zero_or_one(T e)          { return e ? 1 : 0; }
template <class T> inline bool make_bool(T a)            { return a ? true : false; }


//  ------------------------------------------------------------------
//  Handy macro for safe casting.           Public domain by Bob Stout
//  ------------------------------------------------------------------
//
//  Example of CAST macro at work
//
//  union {
//    char  ch[4];
//    int   i[2];
//  } my_union;
//
//  long longvar;
//
//  longvar = (long)my_union;         // Illegal cast
//  longvar = CAST(long, my_union);   // Legal cast
//
//  ------------------------------------------------------------------

#define CAST(new_type,old_object) (*((new_type *)&(old_object)))


//  ------------------------------------------------------------------
//  Get size of structure member

#define sizeofmember(__struct, __member)  sizeof(((__struct*)0)->__member)


//  ------------------------------------------------------------------
//  Legacy defines

#ifndef AND
#define NOT      !
#define AND      &&
#define OR       ||
#endif

#define RngV in_range
#define AbsV absolute
#define CmpV compare_two
#define MinV minimum_of_two
#define MaxV maximum_of_two


//  ------------------------------------------------------------------

#endif

//  ------------------------------------------------------------------
