//  This may look like C code, but it is really -*- C++ -*-

//  ------------------------------------------------------------------
//  The Goldware Library
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
//  String manipulation routines.
//  ------------------------------------------------------------------

#ifndef __gstrarr_h
#define __gstrarr_h


//  ------------------------------------------------------------------

#include <cstring>
#include <vector>
#include <string>
#include <gdefs.h>
#include <gmemdbg.h>


//  ------------------------------------------------------------------

typedef vector<string> gstrarray;


//  ------------------------------------------------------------------

inline void tokenize(gstrarray& array, const char* str, const char* delim = NULL) {
	if(delim == NULL)
		delim = ", \t";
	char* tmp = throw_xstrdup(str);
	char* token = strtok(tmp, delim);
	while(token) {
		array.push_back(token);
		token = strtok(NULL, delim);
	}
	throw_xfree(tmp);
}


//  ------------------------------------------------------------------

#endif // __gstrarr_h

//  ------------------------------------------------------------------