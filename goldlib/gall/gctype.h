//  This may look like C code, but it is really -*- C++ -*-

//  ------------------------------------------------------------------
//  The Goldware Library
//  Copyright (C) 1999 Alexander S. Aganichev
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
//  Portable NLS functions for ctype.
//  ------------------------------------------------------------------

#ifndef __gctype_h
#define __gctype_h


//  ------------------------------------------------------------------

#include <gdefs.h>
#include <gutlos.h>
#ifdef __BORLANDC__
#define __USELOCALES__
#elif defined(__EMX__)
#define _CTYPE_FUN
#endif
#include <ctype.h>
#if defined(__EMX__)
#include <sys/nls.h>
#define tolower(a) _nls_tolower((unsigned char)(a))
#define toupper(a) _nls_toupper((unsigned char)(a))
#elif defined(__WIN32__)
extern char tl[256], tu[256];
inline char tolower(char c) { return tl[c]; }
inline char toupper(char c) { return tu[c]; }
#endif


//  ------------------------------------------------------------------

inline int iswhite(char c)  { return c < '!' and c; }
inline int isxalnum(char c) { return isalnum(c) or (c >= 128); }


//  ------------------------------------------------------------------

#endif

//  ------------------------------------------------------------------