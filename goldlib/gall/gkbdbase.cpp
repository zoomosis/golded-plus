//  This may look like C code, but it is really -*- C++ -*-

//  ------------------------------------------------------------------
//  The Goldware Library
//  Copyright (C) 1990-1999 Odinn Sorensen
//  Copyright (C) 1999-2000 Alexander S. Aganichev
//  Copyright (C) 2000 Jacobo Tarrio
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
//  Keyboard functions.
//  ------------------------------------------------------------------

#include <gctype.h>
#include <gmemdbg.h>
#include <gkbdcode.h>
#include <gkbdbase.h>
#include <gmemall.h>

#if defined(__OS2__)
#define INCL_BASE
#include <os2.h>
#endif

#ifdef __WIN32__
#include <windows.h>
#endif

#if defined(__UNIX__) and not defined(__USE_NCURSES__)
#include <gkbdunix.h>
#endif

#if defined(__DJGPP__)
#include <sys/farptr.h>
#endif

#if defined(__USE_NCURSES__)
#include <gcurses.h>
#endif


//  ------------------------------------------------------------------

#if defined(__USE_NCURSES__)
int curses_initialized = 0;
#endif


//  ------------------------------------------------------------------

#if defined(__WIN32__)
#define KBD_TEXTMODE (ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT)
#endif


//  ------------------------------------------------------------------
//  Global keyboard data

#if defined(__WIN32__) and not defined(__USE_NCURSES__)
HANDLE gkbd_hin;
DWORD  gkbd_kbdmode;
int    gkbd_nt;
#endif

GKbd gkbd;

int blanked = false;

bool right_alt_same_as_left = false;

//  ------------------------------------------------------------------
//  Keyboard Class Initializer

void GKbd::Init() {

  #if defined(__USE_NCURSES__)

  if(not curses_initialized++)
    initscr();
  raw();
  noecho();
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);

  // WARNING: this might break with another version of ncurses, or
  // with another implementation of curses. I'm putting it here because
  // it is quote useful most of the time :-) For other implementations of
  // curses, you might have to compile curses yourself to achieve this.  -jt
  #if defined(NCURSES_VERSION)
  ESCDELAY = 50; // ms, slow for a 300bps terminal, fast for humans :-)
  #endif
  // For more ncurses-dependent code, look at the gkbd_curstable array
  // and at the kbxget_raw() function  -jt

  #elif defined(__OS2__)

  KBDINFO kbstInfo;
  kbstInfo.cb = sizeof(kbstInfo);
  KbdGetStatus(&kbstInfo, 0);
  kbstInfo.fsMask = (USHORT)((kbstInfo.fsMask & 0xFFF7) | 0x0004);
  KbdSetStatus(&kbstInfo, 0);

  #elif defined(__WIN32__)

  OSVERSIONINFO osversion;
  osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
  GetVersionEx(&osversion);
  gkbd_nt = (osversion.dwPlatformId & VER_PLATFORM_WIN32_NT) ? true : false;
  gkbd_hin = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, 0, NULL);
  GetConsoleMode(gkbd_hin, &gkbd_kbdmode);
  if(gkbd_kbdmode & KBD_TEXTMODE)
    SetConsoleMode(gkbd_hin, gkbd_kbdmode & ~KBD_TEXTMODE);

  #elif defined(__UNIX__)

  gkbd_tty_init();

  #endif
}


//  ------------------------------------------------------------------
//  Keyboard Class constructor

GKbd::GKbd() {

  kbuf = NULL;
  onkey  = NULL;
  curronkey = NULL;
  inmenu = 0;
  source = 0;
  polling = 0;
  tickinterval = 0;
  tickvalue = 0;
  tickfunc = NULL;
  tickpress = 0;
  inidle = 0;
  quitall = NO;

  // Detect enhanced keyboard by checking bit 4 at 0x00000496
  #if defined(__USE_NCURSES__)
  extkbd = true;
  #elif defined(__DJGPP__)
  extkbd = _farpeekb (_dos_ds, 0x0496) & (1 << 4);
  #elif defined(__MSDOS__)
  extkbd = *((byte*)0x0496) & (1 << 4);
  #elif defined(__OS2__) or defined(__WIN32__)
  extkbd = true;
  #endif

  Init();

  #if defined(__UNIX__) and not defined(__USE_NCURSES__)

  gkbd_keymap_init();

  char escseq[2];
  escseq[1] = NUL;
  for(int n=0; n<256; n++) {
    escseq[0] = (char)n;
    if(n == 0x7F or n == 0x08)
      gkbd_define_keysym(escseq, Key_BS);
    else if(n == 0x09)
      gkbd_define_keysym(escseq, Key_Tab);
    else if(n == 0x0D)
      gkbd_define_keysym(escseq, Key_Ent);
    else
      gkbd_define_keysym(escseq, (n < 128) ? (scancode_table[n]|n) : n);
  }

  gkbd_define_keysym("^@", 0);

  gkbd_define_keysym("\033[A", Key_Up);
  gkbd_define_keysym("\033[B", Key_Dwn);
  gkbd_define_keysym("\033[C", Key_Rgt);
  gkbd_define_keysym("\033[D", Key_Lft);

  gkbd_define_keysym("\033[[W", Key_C_Up);
  gkbd_define_keysym("\033[[Z", Key_C_Dwn);
  gkbd_define_keysym("\033[[Y", Key_C_Rgt);
  gkbd_define_keysym("\033[[X", Key_C_Lft);

  gkbd_define_keysym("\033[1~", Key_Home);
  gkbd_define_keysym("\033[7~", Key_Home);
  gkbd_define_keysym("\033[H",  Key_Home);
  gkbd_define_keysym("\033[2~", Key_Ins);
  gkbd_define_keysym("\033[3~", Key_Del);
  gkbd_define_keysym("\033[4~", Key_End);
  gkbd_define_keysym("\033[8~", Key_End);
  gkbd_define_keysym("\033[F",  Key_End);
  gkbd_define_keysym("\033[5~", Key_PgUp);
  gkbd_define_keysym("\033[6~", Key_PgDn);

  gkbd_define_keysym("\033[[A",  Key_F1);
  gkbd_define_keysym("\033[[B",  Key_F2);
  gkbd_define_keysym("\033[[C",  Key_F3);
  gkbd_define_keysym("\033[[D",  Key_F4);
  gkbd_define_keysym("\033[[E",  Key_F5);
  gkbd_define_keysym("\033[17~", Key_F6);
  gkbd_define_keysym("\033[18~", Key_F7);
  gkbd_define_keysym("\033[19~", Key_F8);
  gkbd_define_keysym("\033[20~", Key_F9);
  gkbd_define_keysym("\033[21~", Key_F10);

  gkbd_define_keysym("\033[23~", Key_S_F1);
  gkbd_define_keysym("\033[24~", Key_S_F2);
  gkbd_define_keysym("\033[25~", Key_S_F3);
  gkbd_define_keysym("\033[26~", Key_S_F4);
  gkbd_define_keysym("\033[28~", Key_S_F5);
  gkbd_define_keysym("\033[29~", Key_S_F6);
  gkbd_define_keysym("\033[31~", Key_S_F7);
  gkbd_define_keysym("\033[32~", Key_S_F8);
  gkbd_define_keysym("\033[33~", Key_S_F9);
  gkbd_define_keysym("\033[34~", Key_S_F10);

  gkbd_define_keysym("\033""0", Key_A_0);
  gkbd_define_keysym("\033""1", Key_A_1);
  gkbd_define_keysym("\033""2", Key_A_2);
  gkbd_define_keysym("\033""3", Key_A_3);
  gkbd_define_keysym("\033""4", Key_A_4);
  gkbd_define_keysym("\033""5", Key_A_5);
  gkbd_define_keysym("\033""6", Key_A_6);
  gkbd_define_keysym("\033""7", Key_A_7);
  gkbd_define_keysym("\033""8", Key_A_8);
  gkbd_define_keysym("\033""9", Key_A_9);

  gkbd_define_keysym("\033a", Key_A_A);
  gkbd_define_keysym("\033b", Key_A_B);
  gkbd_define_keysym("\033c", Key_A_C);
  gkbd_define_keysym("\033d", Key_A_D);
  gkbd_define_keysym("\033e", Key_A_E);
  gkbd_define_keysym("\033f", Key_A_F);
  gkbd_define_keysym("\033g", Key_A_G);
  gkbd_define_keysym("\033h", Key_A_H);
  gkbd_define_keysym("\033i", Key_A_I);
  gkbd_define_keysym("\033j", Key_A_J);
  gkbd_define_keysym("\033k", Key_A_K);
  gkbd_define_keysym("\033l", Key_A_L);
  gkbd_define_keysym("\033m", Key_A_M);
  gkbd_define_keysym("\033n", Key_A_N);
  gkbd_define_keysym("\033o", Key_A_O);
  gkbd_define_keysym("\033p", Key_A_P);
  gkbd_define_keysym("\033q", Key_A_Q);
  gkbd_define_keysym("\033r", Key_A_R);
  gkbd_define_keysym("\033s", Key_A_S);
  gkbd_define_keysym("\033t", Key_A_T);
  gkbd_define_keysym("\033u", Key_A_U);
  gkbd_define_keysym("\033v", Key_A_V);
  gkbd_define_keysym("\033w", Key_A_W);
  gkbd_define_keysym("\033x", Key_A_X);
  gkbd_define_keysym("\033y", Key_A_Y);
  gkbd_define_keysym("\033z", Key_A_Z);

  gkbd_define_keysym("^?", Key_BS);
  gkbd_define_keysym("\033\x7F", Key_A_BS);
  gkbd_define_keysym("\033\x0D", Key_A_Ent);
  gkbd_define_keysym("\033\x09", Key_A_Tab);

  #endif
}


//  ------------------------------------------------------------------
//  Keyboard Class destructor

GKbd::~GKbd() {

  #if defined(__USE_NCURSES__)
  
  if(not --curses_initialized)
    endwin();
  
  #elif defined(__WIN32__)
  
  if(gkbd_kbdmode & KBD_TEXTMODE)
    SetConsoleMode(gkbd_hin, gkbd_kbdmode);
  
  #elif defined(__UNIX__)
  
  gkbd_keymap_reset();
  gkbd_tty_reset();

  #endif
}


//  ------------------------------------------------------------------
//  Local table for scancode()

gkey scancode_table[] = {

  Key_C_2   & 0xFF00u,     //  0x0300          C <2 @>     [NUL]
  Key_C_A   & 0xFF00u,     //  0x1E01          C <A>       [SOH]
  Key_C_B   & 0xFF00u,     //  0x3002          C <B>       [STX]
  Key_C_C   & 0xFF00u,     //  0x2E03          C <C>       [ETX]
  Key_C_D   & 0xFF00u,     //  0x2004          C <D>       [EOT]
  Key_C_E   & 0xFF00u,     //  0x1205          C <E>       [ENQ]
  Key_C_F   & 0xFF00u,     //  0x2106          C <F>       [ACK]
  Key_C_G   & 0xFF00u,     //  0x2207          C <G>       [BEL]
  Key_C_H   & 0xFF00u,     //  0x2308          C <H>       [BS]
  Key_C_I   & 0xFF00u,     //  0x1709          C <I>       [HT]
  Key_C_J   & 0xFF00u,     //  0x240A          C <J>       [LF]
  Key_C_K   & 0xFF00u,     //  0x250B          C <K>       [VT]
  Key_C_L   & 0xFF00u,     //  0x260C          C <L>       [FF]
  Key_C_M   & 0xFF00u,     //  0x320D          C <M>       [CR]
  Key_C_N   & 0xFF00u,     //  0x310E          C <N>       [SO]
  Key_C_O   & 0xFF00u,     //  0x180F          C <O>       [SI]
  Key_C_P   & 0xFF00u,     //  0x1910          C <P>       [DLE]
  Key_C_Q   & 0xFF00u,     //  0x1011          C <Q>       [DC1]
  Key_C_R   & 0xFF00u,     //  0x1312          C <R>       [DC2]
  Key_C_S   & 0xFF00u,     //  0x1F13          C <S>       [DC3]
  Key_C_T   & 0xFF00u,     //  0x1414          C <T>       [DC4]
  Key_C_U   & 0xFF00u,     //  0x1615          C <U>       [NAK]
  Key_C_V   & 0xFF00u,     //  0x2F16          C <V>       [SYN]
  Key_C_W   & 0xFF00u,     //  0x1117          C <W>       [ETB]
  Key_C_X   & 0xFF00u,     //  0x2D18          C <X>       [CAN]
  Key_C_Y   & 0xFF00u,     //  0x1519          C <Y>       [EM]
  Key_C_Z   & 0xFF00u,     //  0x2C1A          C <Z>       [SUB]
  Key_Esc   & 0xFF00u,     //  0x011B          C <[ {>     [ESC] (was: 0x1A1B)
  Key_C_Bsl & 0xFF00u,     //  0x2B1C          C <\ |>     [FS]
  Key_C_Rbr & 0xFF00u,     //  0x1B1D          C <] }>     [GS]
  Key_C_6   & 0xFF00u,     //  0x071E          C <7 &>     [RS]
  Key_C_Min & 0xFF00u,     //  0x0C1F          C <- _>
  Key_Space & 0xFF00u,     //  0x3920            <Space>
  Key_S_1   & 0xFF00u,     //  0x0221          <1 !>
  Key_S_Quo & 0xFF00u,     //  0x2822          <' ">
  Key_S_3   & 0xFF00u,     //  0x0423          <3 #>
  Key_S_4   & 0xFF00u,     //  0x0524          <4 $>
  Key_S_5   & 0xFF00u,     //  0x0625          <5 %>
  Key_S_7   & 0xFF00u,     //  0x0826          <7 &>
  Key_Quo   & 0xFF00u,     //  0x2827          <'>
  Key_S_9   & 0xFF00u,     //  0x0A28          <9 (>
  Key_S_0   & 0xFF00u,     //  0x0B29          <0 )>
  Key_S_8   & 0xFF00u,     //  0x092A          <8 *>
  Key_S_Equ & 0xFF00u,     //  0x0D2B          <= +>
  Key_Com   & 0xFF00u,     //  0x332C            <,>
  Key_Min   & 0xFF00u,     //  0x0C2D          <->
  Key_Dot   & 0xFF00u,     //  0x342E            <.>
  Key_Sls   & 0xFF00u,     //  0x352F          </>
  Key_0     & 0xFF00u,     //  0x0B30          <0>
  Key_1     & 0xFF00u,     //  0x0231          <1>
  Key_2     & 0xFF00u,     //  0x0332          <2>
  Key_3     & 0xFF00u,     //  0x0433          <3>
  Key_4     & 0xFF00u,     //  0x0534          <4>
  Key_5     & 0xFF00u,     //  0x0635          <5>
  Key_6     & 0xFF00u,     //  0x0736          <6>
  Key_7     & 0xFF00u,     //  0x0837          <7>
  Key_8     & 0xFF00u,     //  0x0938          <8>
  Key_9     & 0xFF00u,     //  0x0A39          <9>
  Key_S_Smi & 0xFF00u,     //  0x273A          <; :>
  Key_Smi   & 0xFF00u,     //  0x273B          <;>
  Key_S_Com & 0xFF00u,     //  0x333C          <, >>
  Key_Equ   & 0xFF00u,     //  0x0D3D          <=>
  Key_S_Dot & 0xFF00u,     //  0x343E          <. <>
  Key_S_Sls & 0xFF00u,     //  0x353F          </ ?>
  Key_S_2   & 0xFF00u,     //  0x0340          <2 @>
  Key_S_A   & 0xFF00u,     //  0x1E41          <A>
  Key_S_B   & 0xFF00u,     //  0x3042          <B>
  Key_S_C   & 0xFF00u,     //  0x2E43          <C>
  Key_S_D   & 0xFF00u,     //  0x2044          <D>
  Key_S_E   & 0xFF00u,     //  0x1245          <E>
  Key_S_F   & 0xFF00u,     //  0x2146          <F>
  Key_S_G   & 0xFF00u,     //  0x2247          <G>
  Key_S_H   & 0xFF00u,     //  0x2348          <H>
  Key_S_I   & 0xFF00u,     //  0x1749          <I>
  Key_S_J   & 0xFF00u,     //  0x244A          <J>
  Key_S_K   & 0xFF00u,     //  0x254B          <K>
  Key_S_L   & 0xFF00u,     //  0x264C          <L>
  Key_S_M   & 0xFF00u,     //  0x324D          <M>
  Key_S_N   & 0xFF00u,     //  0x314E          <N>
  Key_S_O   & 0xFF00u,     //  0x184F          <O>
  Key_S_P   & 0xFF00u,     //  0x1950          <P>
  Key_S_Q   & 0xFF00u,     //  0x1051          <Q>
  Key_S_R   & 0xFF00u,     //  0x1352          <R>
  Key_S_S   & 0xFF00u,     //  0x1F53          <S>
  Key_S_T   & 0xFF00u,     //  0x1454          <T>
  Key_S_U   & 0xFF00u,     //  0x1655          <U>
  Key_S_V   & 0xFF00u,     //  0x2F56          <V>
  Key_S_W   & 0xFF00u,     //  0x1157          <W>
  Key_S_X   & 0xFF00u,     //  0x2D58          <X>
  Key_S_Y   & 0xFF00u,     //  0x1559          <Y>
  Key_S_Z   & 0xFF00u,     //  0x2C5A          <Z>
  Key_Lbr   & 0xFF00u,     //  0x1A5B          <[>
  Key_Bsl   & 0xFF00u,     //  0x2B5C          <\>
  Key_Rbr   & 0xFF00u,     //  0x1B5D          <]>
  Key_S_6   & 0xFF00u,     //  0x075E          <6 ^>
  Key_S_Min & 0xFF00u,     //  0x0C5F          <- _>
  Key_Grv   & 0xFF00u,     //  0x2960          <`>
  Key_A     & 0xFF00u,     //  0x1E61          <a>
  Key_B     & 0xFF00u,     //  0x3062          <b>
  Key_C     & 0xFF00u,     //  0x2E63          <c>
  Key_D     & 0xFF00u,     //  0x2064          <d>
  Key_E     & 0xFF00u,     //  0x1265          <e>
  Key_F     & 0xFF00u,     //  0x2166          <f>
  Key_G     & 0xFF00u,     //  0x2267          <g>
  Key_H     & 0xFF00u,     //  0x2368          <h>
  Key_I     & 0xFF00u,     //  0x1769          <i>
  Key_J     & 0xFF00u,     //  0x246A          <j>
  Key_K     & 0xFF00u,     //  0x256B          <k>
  Key_L     & 0xFF00u,     //  0x266C          <l>
  Key_M     & 0xFF00u,     //  0x326D          <m>
  Key_N     & 0xFF00u,     //  0x316E          <n>
  Key_O     & 0xFF00u,     //  0x186F          <o>
  Key_P     & 0xFF00u,     //  0x1970          <p>
  Key_Q     & 0xFF00u,     //  0x1071          <q>
  Key_R     & 0xFF00u,     //  0x1372          <r>
  Key_S     & 0xFF00u,     //  0x1F73          <s>
  Key_T     & 0xFF00u,     //  0x1474          <t>
  Key_U     & 0xFF00u,     //  0x1675          <u>
  Key_V     & 0xFF00u,     //  0x2F76          <v>
  Key_W     & 0xFF00u,     //  0x1177          <w>
  Key_X     & 0xFF00u,     //  0x2D78          <x>
  Key_Y     & 0xFF00u,     //  0x1579          <y>
  Key_Z     & 0xFF00u,     //  0x2C7A          <z>
  Key_S_Lbr & 0xFF00u,     //  0x1A7B          <[ {>
  Key_S_Bsl & 0xFF00u,     //  0x2B7C          <\ |>
  Key_S_Rbr & 0xFF00u,     //  0x1B7D          <] }>
  Key_S_Grv & 0xFF00u,     //  0x297E          <` ~>
  Key_C_BS  & 0xFF00u      //  0x0E7F          C <BS>      [RUB]
};


//  ------------------------------------------------------------------
//  Returns the scan code of an ASCII character

byte scancode(gkey ch) {

  if(KCodAsc(ch) <= 127)
    return (byte)(scancode_table[KCodAsc(ch)] >> 8);
  return 0;
}


//  ------------------------------------------------------------------
//  Translate scancode for ASCII keys

gkey keyscanxlat(gkey k) {

  // Only translate ASCII keys
  if(KCodAsc(k)) {

    // if scancode zero and ascii-code non-zero, it's a "explicit" character,
    // entered by ALT + <Numpad 0-9>, so don't change it
    //if(KCodScn(k) == 0x00)
    //  return k;

    // Check for certain ctrl-keys
    switch(KCodAsc(k)) {

      case 0x08:  // CtrlH or BackSpace                     23/0E
        if(KCodScn(k) == 0x0E)
          return k;
        else
          break;

      case 0x09:  // CtrlI or Tab                           17/0F
        if(KCodScn(k) == 0x0F)
          return k;
        else
          break;

      case 0x0A:  // CtrlJ or CtrlEnter or GreyCtrlEnter    24/1C/E0
      case 0x0D:  // CtrlM or Enter or GreyEnter            32/1C/E0
        if(KCodScn(k) == 0x1C)
          return k;
        else if(KCodScn(k) == 0xE0) {
          KCodScn(k) = 0x1C;  // Translate Numpad-Enter to main Enter
          return k;
        }
        else
          break;

      case 0x1B:  // Ctrl[ or Esc                           1A/01
        if(KCodScn(k) == 0x01)
          return k;
        else
          break;

      case 0x15: // CtrlU or Shift3 (on german keyboards)   16/04
        if(KCodScn(k) == 0x04)
          return k;
        break;
      case 0xE0: // Check for extended key and fix it if necessary
        if(KCodScn(k)) {
          KCodAsc(k) = 0x00;
          return k;
        }
        break;
    }

    // Translate scancode of ASCII key to a known value
    if (KCodAsc(k) <= 127)
      return (gkey)(scancode_table[KCodAsc(k)] | KCodAsc(k));
    else
      return (gkey)(KCodAsc(k));
  }
  else {

    // Check if its the center key and translate it to '5'
    if(k == 0x4c00)
      k = (gkey)(scancode_table['5'] | '5');

  }

  return k;
}


//  ------------------------------------------------------------------
//  The following tables map curses keyboard codes to BIOS keyboard
//  values.

#if defined(__USE_NCURSES__)

// This might not work with something other than ncurses... :-(
// If you ever port it to other curses implementation, remember
// that it might have to be changed to another data structure, or
// the array might have to be filled in another manner...

int gkbd_curstable[] = {
  Key_C_Brk, //  KEY_BREAK
  Key_Dwn,   //  KEY_DOWN
  Key_Up,    //  KEY_UP
  Key_Lft,   //  KEY_LEFT
  Key_Rgt,   //  KEY_RIGHT
  Key_Home,  //  KEY_HOME
  Key_BS,    //  KEY_BACKSPACE
  -1,        //  KEY_F0
  Key_F1,    //  KEY_F(1)
  Key_F2,    //  KEY_F(2)
  Key_F3,    //  KEY_F(3)
  Key_F4,    //  KEY_F(4)
  Key_F5,    //  KEY_F(5)
  Key_F6,    //  KEY_F(6)
  Key_F7,    //  KEY_F(7)
  Key_F8,    //  KEY_F(8)
  Key_F9,    //  KEY_F(9)
  Key_F10,   //  KEY_F(10)
  Key_F11,   //  KEY_F(11)
  Key_F12,   //  KEY_F(12)
  Key_S_F3,  //  KEY_F(13)
  Key_S_F4,  //  KEY_F(14)
  Key_S_F5,  //  KEY_F(15)
  Key_S_F6,  //  KEY_F(16)
  Key_S_F7,  //  KEY_F(17)
  Key_S_F8,  //  KEY_F(18)
  Key_S_F9,  //  KEY_F(19)
  Key_S_F10, //  KEY_F(20)
  Key_S_F11, //  KEY_F(21)
  Key_S_F12, //  KEY_F(22)
  -1,        //  KEY_F(23)
  -1,        //  KEY_F(24)
  -1,        //  KEY_F(25)
  -1,        //  KEY_F(26)
  -1,        //  KEY_F(27)
  -1,        //  KEY_F(28)
  -1,        //  KEY_F(29)
  -1,        //  KEY_F(30)
  -1,        //  KEY_F(31)
  -1,        //  KEY_F(32)
  -1,        //  KEY_F(33)
  -1,        //  KEY_F(34)
  -1,        //  KEY_F(35)
  -1,        //  KEY_F(36)
  -1,        //  KEY_F(37)
  -1,        //  KEY_F(38)
  -1,        //  KEY_F(39)
  -1,        //  KEY_F(40)
  -1,        //  KEY_F(41)
  -1,        //  KEY_F(42)
  -1,        //  KEY_F(43)
  -1,        //  KEY_F(44)
  -1,        //  KEY_F(45)
  -1,        //  KEY_F(46)
  -1,        //  KEY_F(47)
  -1,        //  KEY_F(48)
  -1,        //  KEY_F(49)
  -1,        //  KEY_F(50)
  -1,        //  KEY_F(51)
  -1,        //  KEY_F(52)
  -1,        //  KEY_F(53)
  -1,        //  KEY_F(54)
  -1,        //  KEY_F(55)
  -1,        //  KEY_F(56)
  -1,        //  KEY_F(57)
  -1,        //  KEY_F(58)
  -1,        //  KEY_F(59)
  -1,        //  KEY_F(60)
  -1,        //  KEY_F(61)
  -1,        //  KEY_F(62)
  -1,        //  KEY_F(63)
  -1,        //  KEY_DL
  -1,        //  KEY_IL
  Key_Del,   //  KEY_DC
  Key_Ins,   //  KEY_IC
  Key_Ins,   //  KEY_EIC
  -1,        //  KEY_CLEAR
  -1,        //  KEY_EOS
  -1,        //  KEY_EOL
  -1,        //  KEY_SF
  -1,        //  KEY_SR
  Key_PgDn,  //  KEY_NPAGE
  Key_PgUp,  //  KEY_PPAGE
  -1,        //  KEY_STAB
  -1,        //  KEY_CTAB
  -1,        //  KEY_CATAB
  Key_Ent,   //  KEY_ENTER
  -1,        //  KEY_SRESET
  -1,        //  KEY_RESET
  -1,        //  KEY_PRINT
  Key_End,   //  KEY_LL
  Key_Home,  //  KEY_A1
  Key_PgUp,  //  KEY_A3
  Key_Cent,  //  KEY_B2
  Key_End,   //  KEY_C1
  Key_PgDn,  //  KEY_C3
  Key_S_Tab, //  KEY_BTAB
  Key_Home,  //  KEY_BEG
  -1,        //  KEY_CANCEL
  -1,        //  KEY_CLOSE
  -1,        //  KEY_COMMAND
  -1,        //  KEY_COPY
  -1,        //  KEY_CREATE
  Key_End,   //  KEY_END
  -1,        //  KEY_EXIT
  -1,        //  KEY_FIND
  -1,        //  KEY_HELP
  -1,        //  KEY_MARK
  -1,        //  KEY_MESSAGE
  -1,        //  KEY_MOVE
  -1,        //  KEY_NEXT
  -1,        //  KEY_OPEN
  -1,        //  KEY_OPTIONS
  -1,        //  KEY_PREVIOUS
  -1,        //  KEY_REDO
  -1,        //  KEY_REFERENCE
  -1,        //  KEY_REFRESH
  -1,        //  KEY_REPLACE
  -1,        //  KEY_RESTART
  -1,        //  KEY_RESUME
  -1,        //  KEY_SAVE
  Key_S_Home,//  KEY_SBEG
  -1,        //  KEY_SCANCEL
  -1,        //  KEY_SCOMMAND
  -1,        //  KEY_SCOPY
  -1,        //  KEY_SCREATE
  Key_S_Del, //  KEY_SDC
  -1,        //  KEY_SDL
  -1,        //  KEY_SELECT
  Key_S_End, //  KEY_SEND
  -1,        //  KEY_SEOL
  -1,        //  KEY_SEXIT
  -1,        //  KEY_SFIND
  -1,        //  KEY_SHELP
  Key_S_Home,//  KEY_SHOME
  Key_S_Ins, //  KEY_SIC
  Key_S_Lft, //  KEY_SLEFT
  -1,        //  KEY_SMESSAGE
  -1,        //  KEY_SMOVE
  -1,        //  KEY_SNEXT
  -1,        //  KEY_SOPTIONS
  -1,        //  KEY_SPREVIOUS
  -1,        //  KEY_SPRINT
  -1,        //  KEY_SREDO
  -1,        //  KEY_SREPLACE
  Key_S_Rgt, //  KEY_SRIGHT
  -1,        //  KEY_SRSUME
  -1,        //  KEY_SSAVE
  -1,        //  KEY_SSUSPEND
  -1,        //  KEY_SUNDO
  -1,        //  KEY_SUSPEND
  -1,        //  KEY_UNDO
  -1,        //  KEY_MOUSE
  -1         //  KEY_RESIZE
};

int gkbd_cursgetch(int mode) {

  int key;
  nodelay(stdscr, mode);
  key = getch();
  nodelay(stdscr, FALSE);

  return key;
}


//  ------------------------------------------------------------------
//  The following table maps NT virtual keycodes to PC BIOS keyboard
//  values.  For each virtual keycode there are four possible BIOS
//  values: normal, shift, Ctrl, and ALT.  Key combinations that have
//  no BIOS equivalent have a value of -1, and are ignored.  Extended
//  (non-ASCII) key values have bit 8 set to 1 using the EXT macro.

#elif defined(__WIN32__)

#define EXT(key)    ((key)|0x10000)
#define ISEXT(val)  ((val)&0x10000)
#define EXTVAL(val) ((val)&0xFF)

struct kbd {
  int keycode;            // virtual keycode
  int normal;             // BIOS keycode - normal
  int shift;              // BIOS keycode - Shift-
  int ctrl;               // BIOS keycode - Ctrl-
  int alt;                // BIOS keycode - Alt-
} __gkbd_nt2b_table [] =
{

//  ------------------------------------------------------------------
//  Virtual key   Normal      Shift       Control     Alt

  { VK_BACK,      0x0E08,     0x0E08,     0x0E7F,     EXT(14)  },
  { VK_TAB,       0x0F09,     EXT(15),    EXT(148),   EXT(165) },
  { VK_RETURN,    0x1C0D,     0x1C0D,     0x1C0A,     EXT(166) },
  { VK_ESCAPE,    0x011B,     0x011B,     0x011B,     EXT(1)   },
  { VK_SPACE,     0x20,       0x20,       0x20,       0x20,    },

  { '0',          '0',        ')',        -1,         EXT(129) },
  { '1',          '1',        '!',        -1,         EXT(120) },
  { '2',          '2',        '@',        EXT(3),     EXT(121) },
  { '3',          '3',        '#',        -1,         EXT(122) },
  { '4',          '4',        '$',        -1,         EXT(123) },
  { '5',          '5',        '%',        -1,         EXT(124) },
  { '6',          '6',        '^',        0x1E,       EXT(125) },
  { '7',          '7',        '&',        -1,         EXT(126) },
  { '8',          '8',        '*',        -1,         EXT(127) },
  { '9',          '9',        '(',        -1,         EXT(128) },

  { 'A',          'a',        'A',        0x01,       EXT(30)  },
  { 'B',          'b',        'B',        0x02,       EXT(48)  },
  { 'C',          'c',        'C',        0x03,       EXT(46)  },
  { 'D',          'd',        'D',        0x04,       EXT(32)  },
  { 'E',          'e',        'E',        0x05,       EXT(18)  },
  { 'F',          'f',        'F',        0x06,       EXT(33)  },
  { 'G',          'g',        'G',        0x07,       EXT(34)  },
  { 'H',          'h',        'H',        0x08,       EXT(35)  },
  { 'I',          'i',        'I',        0x09,       EXT(23)  },
  { 'J',          'j',        'J',        0x0A,       EXT(36)  },
  { 'K',          'k',        'K',        0x0B,       EXT(37)  },
  { 'L',          'l',        'L',        0x0C,       EXT(38)  },
  { 'M',          'm',        'M',        0x0D,       EXT(50)  },
  { 'N',          'n',        'N',        0x0E,       EXT(49)  },
  { 'O',          'o',        'O',        0x0F,       EXT(24)  },
  { 'P',          'p',        'P',        0x10,       EXT(25)  },
  { 'Q',          'q',        'Q',        0x11,       EXT(16)  },
  { 'R',          'r',        'R',        0x12,       EXT(19)  },
  { 'S',          's',        'S',        0x13,       EXT(31)  },
  { 'T',          't',        'T',        0x14,       EXT(20)  },
  { 'U',          'u',        'U',        0x15,       EXT(22)  },
  { 'V',          'v',        'V',        0x16,       EXT(47)  },
  { 'W',          'w',        'W',        0x17,       EXT(17)  },
  { 'X',          'x',        'X',        0x18,       EXT(45)  },
  { 'Y',          'y',        'Y',        0x19,       EXT(21)  },
  { 'Z',          'z',        'Z',        0x1A,       EXT(44)  },

  { VK_PRIOR,     EXT(73),    EXT(73|0x80),    EXT(132),   EXT(153) },
  { VK_NEXT,      EXT(81),    EXT(81|0x80),    EXT(118),   EXT(161) },
  { VK_END,       EXT(79),    EXT(79|0x80),    EXT(117),   EXT(159) },
  { VK_HOME,      EXT(71),    EXT(71|0x80),    EXT(119),   EXT(151) },
  { VK_LEFT,      EXT(75),    EXT(75|0x80),    EXT(115),   EXT(155) },
  { VK_UP,        EXT(72),    EXT(72|0x80),    EXT(141),   EXT(152) },
  { VK_RIGHT,     EXT(77),    EXT(77|0x80),    EXT(116),   EXT(157) },
  { VK_DOWN,      EXT(80),    EXT(80|0x80),    EXT(145),   EXT(160) },
  { VK_INSERT,    EXT(82),    EXT(82|0x80),    EXT(146),   EXT(162) },
  { VK_DELETE,    EXT(83),    EXT(83|0x80),    EXT(147),   EXT(163) },
  { VK_NUMPAD0,   '0',        EXT(82|0x80),    EXT(146),   -1       },
  { VK_NUMPAD1,   '1',        EXT(79|0x80),    EXT(117),   -1       },
  { VK_NUMPAD2,   '2',        EXT(80|0x80),    EXT(145),   -1       },
  { VK_NUMPAD3,   '3',        EXT(81|0x80),    EXT(118),   -1       },
  { VK_NUMPAD4,   '4',        EXT(75|0x80),    EXT(115),   -1       },
  { VK_NUMPAD5,   '5',        EXT(76),         EXT(143),   -1       },
  { VK_NUMPAD6,   '6',        EXT(77|0x80),    EXT(116),   -1       },
  { VK_NUMPAD7,   '7',        EXT(71|0x80),    EXT(119),   -1       },
  { VK_NUMPAD8,   '8',        EXT(72|0x80),    EXT(141),   -1       },
  { VK_NUMPAD9,   '9',        EXT(73|0x80),    EXT(132),   -1       },
  { VK_MULTIPLY,  '*',        '*',        EXT(150),   EXT(55)  },
  { VK_ADD,       '+',        '+',        EXT(144),   EXT(78)  },
  { VK_SUBTRACT,  '-',        '-',        EXT(142),   EXT(74)  },
  { VK_DECIMAL,   '.',        '.',        EXT(83),    EXT(147) },
  { VK_DIVIDE,    '/',        '/',        EXT(149),   EXT(164) },
  { VK_F1,        EXT(59),    EXT(84),    EXT(94),    EXT(104) },
  { VK_F2,        EXT(60),    EXT(85),    EXT(95),    EXT(105) },
  { VK_F3,        EXT(61),    EXT(86),    EXT(96),    EXT(106) },
  { VK_F4,        EXT(62),    EXT(87),    EXT(97),    EXT(107) },
  { VK_F5,        EXT(63),    EXT(88),    EXT(98),    EXT(108) },
  { VK_F6,        EXT(64),    EXT(89),    EXT(99),    EXT(109) },
  { VK_F7,        EXT(65),    EXT(90),    EXT(100),   EXT(110) },
  { VK_F8,        EXT(66),    EXT(91),    EXT(101),   EXT(111) },
  { VK_F9,        EXT(67),    EXT(92),    EXT(102),   EXT(112) },
  { VK_F10,       EXT(68),    EXT(93),    EXT(103),   EXT(113) },
  { VK_F11,       EXT(133),   EXT(135),   EXT(137),   EXT(139) },
  { VK_F12,       EXT(134),   EXT(136),   EXT(138),   EXT(140) },

  { -1,           -1,         -1,         -1,         -1       }  // THE END
};


//  ------------------------------------------------------------------

int gkbd_nt2bios(INPUT_RECORD& inp) {

  int keycode = inp.Event.KeyEvent.wVirtualKeyCode;
  int state   = inp.Event.KeyEvent.dwControlKeyState;
  int ascii   = inp.Event.KeyEvent.uChar.AsciiChar;

  // Look up the virtual keycode in the table. Ignore unrecognized keys.

  kbd* k = &__gkbd_nt2b_table[0];
  while((keycode != k->keycode) and (k->keycode != -1))
    k++;
  if(k->keycode == -1) {  // value not in table
    int c = ascii;
    return c ? c : -1;
  }

  // Check the state of the shift keys. ALT has highest
  // priority, followed by Control, followed by Shift.
  // Select the appropriate table entry based on shift state.

  int c;
  if(state & (RIGHT_ALT_PRESSED | LEFT_ALT_PRESSED))
    c = k->alt;
  else if(state & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED))
    c = k->ctrl;
  else if(state & SHIFT_PRESSED)
    c = k->shift;
  else {
    // If it is a letter key, use the ASCII value supplied
    // by NT to take into account the CapsLock state.
    if(isupper(keycode))
      c = ascii;
    else
      c = k->normal;
  }

  if(c != -1)
    if(ascii and not (right_alt_same_as_left ? (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) : (state & LEFT_ALT_PRESSED)))
      if(isalnum(keycode))
        return ascii;
  if(ISEXT(c))
    return EXTVAL(c) << 8;

  return c;
}

//  ------------------------------------------------------------------

bool is_numpad_key(const INPUT_RECORD& inp) {

  if(not (inp.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY)) {
    switch(inp.Event.KeyEvent.wVirtualKeyCode) {
      case 0x0C:
      case VK_PRIOR:
      case VK_NEXT:
      case VK_END:
      case VK_HOME:
      case VK_LEFT:
      case VK_UP:
      case VK_RIGHT:
      case VK_DOWN:
      case VK_INSERT:
      case VK_DELETE:
      case VK_NUMPAD0:
      case VK_NUMPAD1:
      case VK_NUMPAD2:
      case VK_NUMPAD3:
      case VK_NUMPAD4:
      case VK_NUMPAD5:
      case VK_NUMPAD6:
      case VK_NUMPAD7:
      case VK_NUMPAD8:
      case VK_NUMPAD9:
        return true;
    }
  }
  return false;
}


//  ------------------------------------------------------------------
//  Numpad translation table

#elif defined(__MSDOS__) or defined(__OS2__)

const word numpad_keys[] = {
  0x4737, 0x4838, 0x4939, 0x0000,
  0x4B34, 0x0000, 0x4D36, 0x0000,
  0x4F31, 0x5032, 0x5133,
  0x5230, 0x532e
};

#endif

//  ------------------------------------------------------------------
//  Get key stroke

gkey kbxget_raw(int mode) {
//  mode - =0 - wait for key is pressed (returns code)
//         =1 - test if keystroke is available (returns code if YES,
//              otherwise returns 0)
//         =2 - return Shifts key status
  gkey k;

  #if defined(__USE_NCURSES__)

  int key;
  if(mode == 2) {
    // We can't do much but we can at least this :-)
    k = kbxget_raw(1);
    key = 0;
    switch(k) {
      case Key_C_Brk:
        key = GCTRL;
        break;
      case Key_S_Tab:
      case Key_S_Home:
      case Key_S_Del:
      case Key_S_Ins:
      case Key_S_Lft:
      case Key_S_Rgt:
      case Key_S_End:
        key = LSHIFT;
        break;
    }
    return key;
  }
  
  // Get keystroke
  key = gkbd_cursgetch(mode);
  if(key == ERR)
    return 0;

  // Prefix for Meta-key or Alt-key sequences
  if(key == 27) {
    int key2 = gkbd_cursgetch(TRUE);
    // If no key follows, it is no Meta- or Alt- seq, but a single Esc
    if(key2 == ERR)
      k = Key_Esc;
    // Compute the right keycode for the alt sequence
    else if((key2 >= '1') and (key2 <= '9'))
      k = 0x7800 + ((key2 - '1') << 8);
    else if(key2 == '0')
      k = 0x8100;
    else if(isalpha(key2))
      k = (scancode_table[key2]);
    else {
      // No correct Alt-sequence; ungetch last key and return Esc
      if (mode != 1)
        ungetch(key2);
      k = Key_Esc;
    }

    if((key2 != ERR) and (mode == 1))
      ungetch(key2);    
  }
  // Curses sequence; lookup in nice table above
  else if(key > KEY_CODE_YES)
    k = (gkbd_curstable[key - KEY_MIN]);
  else if(key == '\015')
    k = Key_Ent;
  else if(key == '\011')
    k = Key_Tab;
  else
    k = key;

  if(mode == 1)
    ungetch(key);

  #elif defined(__MSDOS__)

  if(gkbd.extkbd)
    mode |= 0x10;

  i86 cpu;
  cpu.ah((byte)mode);
  cpu.genint(0x16);
  if(mode & 0x01)
    if(cpu.flags() & 0x40)   // if ZF is set, no key is available
      return 0;
  k = (gkey)cpu.ax();

  if((mode & ~0x10) == 0) {
    if((KCodAsc(k) == 0xE0) and (KCodScn(k) != 0)) {
      if(kbxget_raw(2) & (LSHIFT | RSHIFT)) {
        KCodAsc(k) = 0;
        KCodScn(k) |= 0x80;
      }
    }
    else
      switch(KCodScn(k)) {
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4B:
        case 0x4D:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
          {
            int shifts = kbxget_raw(2);
            if(shifts & (LSHIFT | RSHIFT)) {
              if(shifts & NUMLOCK)
                KCodAsc(k) = 0;
              else {
                KCodAsc(k) = 0;
                KCodScn(k) |= 0x80;
              }
            }
          }
          break;
        default:
          break;
      }
  }

  // If you test shift/alt/ctrl status with bios calls (e.g., using
  // bioskey (2) or bioskey (0x12)) then you should also use bios calls
  // for testing for keys.  This can be done with by bioskey (1) or
  // bioskey (0x11).  Failing to do so can cause trouble in multitasking
  // environments like DESQview/X. (Taken from DJGPP documentation)
  if((mode & 0x02) == 1)
    kbxget_raw(1);

  #elif defined(__OS2__)

  KBDKEYINFO kb;
  mode &= 0xF;
  if(mode == 0)
    KbdCharIn(&kb, IO_WAIT, 0);
  else if(mode == 2) {
    KbdPeek(&kb, 0);
    if(kb.fbStatus)
      return (gkey)(kb.fsState & (RSHIFT|LSHIFT|GCTRL|ALT));
    else
      return 0;
  }
  else {
    KbdPeek(&kb, 0);
    if(!(kb.fbStatus & 0x40))
      return 0;
  }
  KCodScn(k) = kb.chScan;
  KCodAsc(k) = kb.chChar;
  if(0x000 == KCodKey(k))
    return KEY_BRK;
  if(0xE0 == KCodScn(k))
    KCodScn(k) = 0x1C;
  else {
    if(0xE0 == KCodAsc(k)) {
      // If key on the alphanumeric part then don't touch it.
      // This need to enter for example, russian 'p' char (code 0xe0)
      if(KCodScn(k) >= 0x38) {
        KCodAsc(k) = 0x00;
        if(kb.fsState & (LSHIFT | RSHIFT))
          KCodScn(k) |= 0x80;
      }
      else
        KCodScn(k) = 0x00;
    }
    else
      switch(KCodScn(k)) {
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4B:
        case 0x4D:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
          if(kb.fsState & (LSHIFT | RSHIFT)) {
            if(kb.fsState & NUMLOCK)
              KCodAsc(k) = 0;
            else {
              KCodAsc(k) = 0;
              KCodScn(k) |= 0x80;
            }
          }
          break;
        default:
          break;
      }
  }

  #elif defined(__WIN32__)

  INPUT_RECORD inp;
  DWORD nread;

  if(mode == 2) {
    return 0;
  }
  else if(mode & 0x01) {

    // Peek at next key
    k = 0;
    PeekConsoleInput(gkbd_hin, &inp, 1, &nread);
    if(nread) {
      if((inp.EventType & KEY_EVENT) and inp.Event.KeyEvent.bKeyDown) {
        int kc = gkbd_nt2bios(inp);
        if((kc != -1) or (inp.Event.KeyEvent.wVirtualKeyCode == 0xBA)) {
          k = (gkey)kc;
          return k;
        }
      }
      // Discard other events
      ReadConsoleInput(gkbd_hin, &inp, 1, &nread);
    }
  }
  else {

    // Get next key
    inp.Event.KeyEvent.bKeyDown = false;
    while(1) {

      PeekConsoleInput(gkbd_hin, &inp, 1, &nread);
      if(not nread) {
        WaitForSingleObject(gkbd_hin, 1000);
        continue;
      }

      if(inp.EventType == KEY_EVENT and inp.Event.KeyEvent.bKeyDown) {
        bool alt_pressed = (inp.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED)) ? true : false;
        // bool right_alt_pressed = (inp.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED) ? true : false;
        bool enhanced_key = (inp.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ? true : false;
        bool numpad_key = is_numpad_key(inp);
        int vk = inp.Event.KeyEvent.wVirtualKeyCode;
        char raw_ch = inp.Event.KeyEvent.uChar.AsciiChar;
        int kc;
        int test;
        char ch;

        if(enhanced_key and (raw_ch == 0xE0))
          inp.Event.KeyEvent.uChar.AsciiChar = raw_ch = 0;
        if(gkbd_nt)
          test = (inp.Event.KeyEvent.uChar.AsciiChar and not alt_pressed and vk != -1) or (alt_pressed and vk == -1) or (alt_pressed and numpad_key);
        else
          test = (inp.Event.KeyEvent.uChar.AsciiChar and not alt_pressed and vk != -1) or (alt_pressed and vk == -1) or (alt_pressed and numpad_key) or (vk == 0xBA);
        if(test) {
          // Ascii char
          if(gkbd_nt and not (alt_pressed and numpad_key)) {
            ch = raw_ch;
            ReadConsoleInput(gkbd_hin, &inp, 1, &nread);
          }
          else {
            ReadConsole(gkbd_hin, &ch, 1, &nread, NULL);
          }
          if(alt_pressed) {
            k = (gkey)ch;
            break;
          }
          if(gkbd_nt) {
            if(ch == '\x5E' or ch == '\x7E' or ch == '\x60' or ch == '\xF9' or ch == '\xEF')
              inp.Event.KeyEvent.wVirtualKeyCode = (word)(ch << 8);
          }
          else {
            if(ch == '\x5E' or ch == '\x7E' or ch == '\x60' or ch == '\x27' or ch == '\x2E')
              inp.Event.KeyEvent.wVirtualKeyCode = (word)(ch << 8);
          }
          inp.Event.KeyEvent.uChar.AsciiChar = ch;
        }
        else {
          // Control keycode
          ReadConsoleInput(gkbd_hin, &inp, 1, &nread);
        }
        kc = gkbd_nt2bios(inp);
        if(kc != -1) {
          k = (gkey)kc;
          break;
        }
      }
      else {
        // Discard other events
        ReadConsoleInput(gkbd_hin, &inp, 1, &nread);
      }
    }
  }

  #elif defined(__UNIX__)

  if(mode == 2) {
    return 0;
  }
  else if(mode & 0x01) {

    // Peek at next key
    return gkbd_input_pending() ? 0xFFFF : 0;
  }
  else {

    k = gkbd_getmappedkey();
  }

  #endif

  return k;
}


//  ------------------------------------------------------------------
//  Get key stroke

gkey kbxget(int mode) {

  return keyscanxlat(kbxget_raw(mode));
}


//  ------------------------------------------------------------------
//  Returns keycode of waiting key or zero if none

gkey kbxhit() {

  return kbxget(0x01);
}


//  ------------------------------------------------------------------
//  Clears internal keyboard buffer

void kbclear() {

  while(gkbd.kbuf != NULL) {

    KBuf *kbuf = gkbd.kbuf->next;
    throw_free(gkbd.kbuf);
    gkbd.kbuf = kbuf;
  }
}


//  ------------------------------------------------------------------
//  Clear keyboard buffer

void clearkeys() {

  while(kbxhit())
    kbxget(0x00);
}


//  ------------------------------------------------------------------
//  Puts a keystroke into the CXL keyboard "buffer"

int kbput(gkey xch) {

  KBuf* kbuf;
  KBuf* temp;

  // allocate space for another keypress record
  kbuf=(KBuf*)throw_malloc(sizeof(KBuf));

  // find last record in linked list
  if((temp=gkbd.kbuf)!=NULL)
    for(;temp->next!=NULL;temp=temp->next);

  // add new record to end of linked list
  kbuf->next=NULL;
  kbuf->prev=temp;
  if(temp != NULL)
    temp->next=kbuf;

  // add keypress info to new record
  kbuf->xch=xch;

  // if kbuf pointer was NULL, point it to new record
  if(gkbd.kbuf == NULL)
    gkbd.kbuf=kbuf;

  // return normally
  return 0;
}


//  ------------------------------------------------------------------
//  Put keys into the real keyboard buffer

gkey kbput_(gkey xch) {

  #if defined(__MSDOS__)

  #if defined(__DJGPP__)
  if(gkbd.extkbd) {
    i86 cpu;

    cpu.ah(0x05);
    cpu.cx((word)xch);
    cpu.genint(0x16);
  }
  else {
  #endif

  #define BufStart (word)peek(0x40,0x80)
  #define BufEnd   (word)peek(0x40,0x82)
  #define BufHead  (word)peek(0x40,0x1A)
  #define BufTail  (word)peek(0x40,0x1C)
  #define BufTail_(a) poke(0x40,0x1C,(word)(a))

  word OldBufTail;

  OldBufTail = BufTail;
  if(BufTail == BufEnd-2)
    BufTail_(BufStart);
  else
    BufTail_(BufTail+2);

  if(BufTail == BufHead)
    BufTail_(OldBufTail);
  else {
    poke(0x40, OldBufTail, xch);
  }

  #if defined(__DJGPP__)
  }
  #endif

  #endif

  return xch;
}


//  ------------------------------------------------------------------
//  Put keys into the real keyboard buffer

void kbputs_(char* str) {

  char* p;

  for(p=str; *p ;p++)
    kbput_(gkey((scancode(*p)<<8)|*p));
}


//  ------------------------------------------------------------------
//  Change defined "on-key" list pointer

KBnd* chgonkey(KBnd* list) {

  KBnd* temp;

  temp = gkbd.onkey;
  gkbd.onkey = list;

  return temp;
}


//  ------------------------------------------------------------------
//  Frees all active onkey definitions from memory

void freonkey() {

  KBnd* temp;

  // free all onkey records in linked list
  while(gkbd.onkey!=NULL) {
    temp = gkbd.onkey->prev;
    throw_free(gkbd.onkey);
    gkbd.onkey = temp;
  }
}


//  ------------------------------------------------------------------
//  Attaches/detaches a key to a function

int setonkey(gkey keycode, VfvCP func, gkey pass) {

  // search for a keycode that is already defined
  KBnd* onkey = gkbd.onkey;
  while(onkey) {
    if(onkey->keycode == keycode)
      break;
    onkey = onkey->prev;
  }

  // check to see if a key detachment is being requested
  if(func == NULL) {

    // if no defined onkey was found, then error
    if(onkey == NULL)
      return 2;

    // delete record from linked list
    KBnd* prev = onkey->prev;
    KBnd* next = onkey->next;
    if(prev)
      prev->next = next;
    if(next)
      next->prev = prev;
    if(onkey == gkbd.onkey)
      gkbd.onkey = prev;

    // free memory allocated for deleted record
    throw_free(onkey);
  }
  else {

    // if key was found, change func pointer
    // otherwise create a new onkey record
    if(onkey)
      onkey->func = func;
    else {

      // allocate memory for new record
      onkey = (KBnd*)throw_malloc(sizeof(KBnd));
      if(onkey == NULL)
        return 1;

      // add new record to linked list
      if(gkbd.onkey)
        gkbd.onkey->next = onkey;
      onkey->prev = gkbd.onkey;
      onkey->next = NULL;
      gkbd.onkey = onkey;

      // save info in onkey record
      gkbd.onkey->keycode = keycode;
      gkbd.onkey->func = func;
      gkbd.onkey->pass = pass;
    }
  }

  // return normally
  return 0;
}


//  ------------------------------------------------------------------

gkey key_tolower(gkey __keycode) {

  if(isupper(KCodAsc(__keycode)))
    return (gkey)(__keycode + 'a' - 'A');
  return __keycode;
}


//  ------------------------------------------------------------------