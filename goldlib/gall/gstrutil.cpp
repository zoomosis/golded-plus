
//  ------------------------------------------------------------------
//  The Goldware Library
//  Copyright (C) 1990-1999 Odinn Sorensen
//  Copyright (C) 1999-2000 Alexander S. Aganichev
//  ------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2 of the
//  License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//  ------------------------------------------------------------------
//  $Id$
//  ------------------------------------------------------------------
//  Based on source from the CXL library by Mike Smedley.
//  String manipulation.
//  ------------------------------------------------------------------

#include <gctype.h>
#include <cstdio>
#include <stdarg.h>
#include <gstrall.h>


//  ------------------------------------------------------------------
//  Determines if a string is blank

bool strblank(const char* str) {

  const char* p;

  for(p = str; *p; p++)
    if(not isspace(*p))
      return false;

  return true;
}


//  ------------------------------------------------------------------
//  Changes all occurrences of one character to another

int strchg(char* str, char oldch, char newch) {

  int count = 0;

  for(char* p=str; *p; p++) {
    if(oldch == *p) {
      *p = newch;
      count++;
    }
  }

  return count;
}


//  ------------------------------------------------------------------
//  Deletes a substring from within a string

static char* strdel(const char* substr, char* str) {

  char* dest = strstr(str, substr);
  if(!dest)
    return NULL;
  char* src = dest + strlen(substr);
  strcpy(dest, src);

  return str;
}


//  ------------------------------------------------------------------
//  Deletes a substring from within a string, ignores case

static char* stridel(const char* substr, char* str) {

  char* dest = (char*)striinc(substr, str);
  if(!dest)
    return NULL;
  char* src = dest + strlen(substr);
  strcpy(dest, src);

  return str;
}


//  ------------------------------------------------------------------
//  Deletes all occurrences of one string inside another

char* stridela(const char* substr, char* str) {

  int count = 0;
  char* p = str;

  while((p = (char*)striinc(substr, p)) != NULL) {
    stridel(substr, p);
    count++;
  }

  if(count)
    return str;
  return NULL;
}


//  ------------------------------------------------------------------
//  Compare two strings, allowing wildcards

int strnicmpw(const char* str1, const char* str2, int len) {

  int cmp = 0;

  for(int n=0; n<len; n++) {
    // Single char match?
    if(str1[n] == '?')
      continue;
    // Matches rest of string?
    else if(str1[n] == '*')
      return 0;
    // Compare chars
    else if((cmp = compare_two(toupper(str1[n]), toupper(str2[n]))) != 0)
      return cmp;
  }

  return cmp;
}


//  ------------------------------------------------------------------
//  Determines if string1 is included in string2

const char* striinc(const char* str1, const char* str2) {

  int max = strlen(str1);

  for(const char* p=str2; *p; p++)
    if(!strnicmp(str1,p,max))
      return p;

  return NULL;                         // string1 not found in string2
}


//  ------------------------------------------------------------------
//  Inserts one string into another

char* strins(const char* instr, char* str, int st_pos) {

  int i, leninstr;

  // get length of string to insert
  leninstr = strlen(instr);

  // shift affected portion of str text to the right
  for(i=strlen(str); i>=st_pos; i--)
    *(str+leninstr+i) = *(str+i);

  // insert instr text
  for(i=0; i<leninstr; i++)
    *(str+st_pos+i) = *(instr+i);

  // return address of modified string
  return str;
}


//  ------------------------------------------------------------------
//  String search and replace, case insensitive

static char* strisrep(char* str, const char* search, const char* replace) {

  char* p;

  if((p = (char*)striinc(search,str)) != NULL) {
    stridel(search, str);
    strins(replace, str, (int)(p-str));
    p = str;
  }

  return p;
}


//  ------------------------------------------------------------------
//  Changes all occurrences of one string to another

char* strischg(char* str, const char* find, const char* replace) {

  int count = 0;
  char* p = str;

  int len = strlen(replace);
  while((p = (char*)striinc(find, p)) != NULL) {
    strisrep(p, find, replace);
    p += len;
    count++;
  }

  if(count)
    return str;
  return NULL;
}


//  ------------------------------------------------------------------
//  Takes a long number and makes a string with the form x.xxx.xxx.xxx

char* longdotstr(long num) {

  // 1234567890123
  // 4.294.967.296

  static char buf[15];
  return longdotstr(buf, num);
}


//  ------------------------------------------------------------------
//  Takes a long number and makes a string with the form x.xxx.xxx.xxx

char* longdotstr(char* str, long num) {

  char tmp[20], pos=0;

  char* out = str;
  char* ptr = tmp;

  do {
    if(pos == 3 or pos == 6 or pos == 9)
      *ptr++ = '.';
    pos++;
    *ptr++ = (char)(num % 10 + '0');
  } while((num = num/10) > 0);

  while(ptr-- > tmp)
    *str++ = *ptr;
  *str = NUL;

  return out;
}


//  ------------------------------------------------------------------

char* strp2c(char* str) {

  int len = *str;

  memmove(str, str+1, len);       // Copy data part
  str[len] = NUL;                 // Set length
  return str;
}


//  ------------------------------------------------------------------

char* strnp2c(char* str, int n) {

  int len = (n < *str) ? n : *str;

  memmove(str, str+1, len);     // Copy data part
  str[len] = NUL;               // Set length
  return str;
}


//  ------------------------------------------------------------------

char* strnp2cc(char* dest, const char* str, int n) {

  int len = (n < *str) ? n : *str;

  memcpy(dest, str+1, len);           // Copy data part
  dest[len] = NUL;                    // Set length
  return dest;
}


//  ------------------------------------------------------------------

char* strc2p(char* str) {

  char len = (char)strlen(str);

  memmove(str+1, str, len);       // Copy data part
  *str = len;                     // Set length
  return str;
}


//  ------------------------------------------------------------------
//  Strip the quotes off a quoted string ("" or '')

char* StripQuotes(char* str) {

  int len;

  switch(*str) {
    case '\'':
    case '\"':
      len = strlen(str);
      switch(*(str+len-1)) {
        case '\'':
        case '\"':
          memmove(str, str+1, len);
          str[len-2] = NUL;
      }
  }
  return str;
}


//  ------------------------------------------------------------------
//  Right justifies a string

char* strrjust(char* str) {

  char* p;
  char* q;

  for(p=str; *p; p++)
    ;   // find end of string
  p--;
  for(q=p; isspace(*q) and q>=str; q--)
    ;   // find last non-space character
  if(p != q) {
    while(q >= str) {
      *p-- = *q;
      *q-- = ' ';
    }
  }
  return str;
}


//  ------------------------------------------------------------------
//  Changes all occurrences of one string to another

char* strschg(char* str, const char* find, const char* replace) {

  int count = 0;
  char* p = str;

  int len = strlen(replace);
  while((p = strstr(p, find)) != NULL) {
    strsrep(p, find, replace);
    p += len;
    count++;
  }

  if(count)
    return str;
  return NULL;
}


//  ------------------------------------------------------------------
//  Adjusts the size of a string

char* strsetsz(char* str, int newsize) {

  int i;

  int len = strlen(str);
  if(newsize < len)
    *(str+newsize) = NUL;
  else {
    for(i=len; i<newsize; i++)
      *(str+i) = ' ';
    *(str+i) = NUL;
  }

  return str;
}


//  ------------------------------------------------------------------
//  Shifts a string left

char* strshl(char* str, int count) {

  int i, j;

  if(*str) {
    for(j=0; j<count; j++) {
      for(i=0; *(str+i); i++)
        *(str+i) = *(str+i+1);
      *(str+i-1) = ' ';
    }
  }

  return str;
}


//  ------------------------------------------------------------------
//  Shifts a string right

char* strshr(char* str, int count) {

  int i, j, len;

  if(*str) {
    len = strlen(str)-1;
    for(j=0; j<count; j++) {
      for(i=len; i>0; i--)
        *(str+i) = *(str+i-1);
      *(str) = ' ';
    }
  }

  return str;
}


//  ------------------------------------------------------------------
//  String search and replace, case sensitive

char* strsrep(char* str, const char* search, const char* replace) {

  char* p;

  if((p = strstr(str, search)) != NULL) {
    strdel(search, str);
    strins(replace, str, (int)(p-str));
    p = str;
  }

  return p;
}


//  ------------------------------------------------------------------
//  Trims trailing spaces off of a string

char* strtrim(char* p) {

  int i;
  for(i=strlen(p)-1; (i >= 0) and ('!' > p[i]); i--) {}
  p[i+1] = NUL;
  return p;
}


//  ------------------------------------------------------------------
//  Trims leading spaces off of a string

char* strltrim(char* str) {

  char* p;
  char* q;

  p = q = str;
  while(*p and isspace(*p))
    p++;

  if(p != q) {
    while(*p)
      *q++ = *p++;
    *q = NUL;
  }

  return str;
}


//  ------------------------------------------------------------------

const char* strlword(const char* str) {

  char buf[256];
  static char left[40];

  *left = NUL;
  if(*str) {
    strcpy(buf, str);
    if(strtok(buf, " ") != NULL) {
      strcpy(left, buf);
    }
  }
  return left;
}


//  ------------------------------------------------------------------

const char* strrword(const char* str) {

  char* ptr;
  char* ptr2;
  char buf[256];
  static char right[40];

  *right = NUL;
  if(*str) {
    strcpy(buf, str);
    ptr = strtok(buf, " ");
    ptr2 = ptr;
    while(ptr != NULL) {
      ptr2 = ptr;
      ptr = strtok(NULL, " ");
    }
    if(ptr2) {
      strcpy(right, ptr2);
    }
  }
  return right;
}


//  ------------------------------------------------------------------

char* strxcpy(char* d, const char* s, int n) {

  if(n) {
    strncpy(d, s, n-1);
    d[n-1] = NUL;
  }
  else
    *d = NUL;
  return d;
}


//  ------------------------------------------------------------------

char *strxcat(char *dest, const char *src, size_t max)
{
  while (*dest and (max > 0)) {
    --max;
    dest++;
  }
  while (*src and (max > 0)) {
    --max;
    *dest++ = *src++;
  }
  *dest = NUL;
  return dest;
}


//  ------------------------------------------------------------------

char *strxmerge(char *dest, size_t max, ...)
{
  va_list a;
  va_start(a, max);
  for(; max > 0;) {
    const char *src = va_arg(a, const char *);
    if (src == NULL)
      break;
    while (*src and (max > 0)) {
      --max;
      *dest++ = *src++;
    }
  }
  va_end(a);
  *dest = NUL;
  return dest;
}


//  ------------------------------------------------------------------

GTok::GTok(char* sep) {

  separator = sep ? sep : ", \t";
}


//  ------------------------------------------------------------------

#if defined(__GNUC__) and not defined(__EMX__)

char* strupr(char* s) {

  char* p = s;
  while(*p) {
    *p = toupper(*p);
    p++;
  }
  return s;
}

char* strlwr(char* s) {

  char* p = s;
  while(*p) {
    *p = tolower(*p);
    p++;
  }
  return s;
}

#endif


//  ------------------------------------------------------------------