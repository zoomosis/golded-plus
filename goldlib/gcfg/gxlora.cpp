//  This may look like C code, but it is really -*- C++ -*-

//  ------------------------------------------------------------------
//  The Goldware Library
//  Copyright (C) 1990-1999 Odinn Sorensen
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
//  Read areas from LoraBBS (2.33 and 2.40).
//  ------------------------------------------------------------------

#include <cstdlib>
#include <gmemdbg.h>
#include <gfile.h>
#include <gstrall.h>
#include <gedacfg.h>
#include <gs_lo240.h>


//  ------------------------------------------------------------------

void gareafile::ReadLoraBBS(char* tag) {

  Path _path;
  *_path = NUL;
  char options[80];
  strcpy(options, tag);
  char* ptr = strtok(tag, " \t");
  while(ptr) {
    if(*ptr != '-') {
      AddBackslash(strcpy(_path, ptr));
      break;
    }
    ptr = strtok(NULL, " \t");
  }
  if(*_path == NUL) {
    ptr = getenv("LORA");
    if(ptr)
      AddBackslash(strcpy(_path, ptr));
  }
  if(*_path == NUL) {
    ptr = getenv("LORABBS");
    if(ptr)
      AddBackslash(strcpy(_path, ptr));
  }
  if(*_path == NUL)
    strcpy(_path, areapath);

  gfile fp;
  const char* _file = AddPath(_path, "config.dat");
  fp.fopen(_file, "rb");
  if(fp.isopen()) {

    if(not quiet)
      cout << "* Reading " << _file << endl;

    _configuration* cfg = (_configuration*)throw_calloc(1, sizeof(_configuration));
    fp.fread(cfg, sizeof(_configuration));
    fp.fclose();

    //CfgUsername(cfg->sysop);

    if(*hudsonpath == NUL)
      PathCopy(hudsonpath, MapPath(cfg->quick_msgpath));
    if(*goldbasepath == NUL)
      PathCopy(goldbasepath, MapPath(cfg->quick_msgpath));

    AreaCfg aa;

    // Netmail *.MSG
    if(not strblank(cfg->netmail_dir)) {
      aa.reset();
      aa.msgbase = GMB_OPUS;
      aa.type = GMB_NET;
      aa.aka = CAST(ftn_addr, cfg->alias[0]);
      aa.setpath(cfg->netmail_dir);
      aa.setdesc("LoraBBS Netmail");
      aa.setautoid("NETMAIL");
      AddNewArea(aa);
    }

    // Bad *.MSG
    if(not strblank(cfg->bad_msgs)) {
      aa.reset();
      aa.msgbase = GMB_OPUS;
      aa.type = GMB_ECHO;
      aa.aka = CAST(ftn_addr, cfg->alias[0]);
      aa.setpath(cfg->bad_msgs);
      aa.setdesc("LoraBBS Bad Echo");
      aa.setautoid("ECHO_BAD");
      AddNewArea(aa);
    }

    // Dupes *.MSG
    if(not strblank(cfg->dupes)) {
      aa.reset();
      aa.msgbase = GMB_OPUS;
      aa.type = GMB_ECHO;
      aa.aka = CAST(ftn_addr, cfg->alias[0]);
      aa.setpath(cfg->dupes);
      aa.setdesc("LoraBBS Duplicate Msgs");
      aa.setautoid("ECHO_DUPES");
      AddNewArea(aa);
    }

    // Personal mail *.MSG
    if(cfg->save_my_mail and not strblank(cfg->my_mail)) {
      aa.reset();
      aa.msgbase = GMB_OPUS;
      aa.type = GMB_ECHO;
      aa.aka = CAST(ftn_addr, cfg->alias[0]);
      aa.setpath(cfg->my_mail);
      aa.setdesc("LoraBBS Personal Mail");
      aa.setautoid("ECHO_PERSONAL");
      AddNewArea(aa);
    }

    _file = AddPath(_path, "sysmsg.dat");
    fp.fopen(_file, "rb");
    if(fp.isopen()) {
      fp.setvbuf(NULL, _IOFBF, 8192);

      if(not quiet)
        cout << "* Reading " << _file << endl;

      _sysmsg* sysmsg = (_sysmsg*)throw_calloc(1, sizeof(_sysmsg));

      while(fp.fread(sysmsg, sizeof(_sysmsg)) == 1) {

        if(sysmsg->passthrough)
          continue;

        aa.reset();

        if(sysmsg->gold_board) {
          aa.msgbase = GMB_GOLDBASE;
          aa.board = sysmsg->gold_board;
        }
        else if(sysmsg->quick_board) {
          aa.msgbase = GMB_HUDSON;
          aa.board = sysmsg->quick_board;
        }
        else if(sysmsg->pip_board) {
          // Not supported (yet)
          continue;
        }
        else if(sysmsg->squish) {
          aa.msgbase = GMB_SQUISH;
          aa.setpath(sysmsg->msg_path);
        }
        else {
          aa.msgbase = GMB_OPUS;
          aa.setpath(sysmsg->msg_path);
        }

        if(sysmsg->netmail) {
          aa.type = GMB_NET;
          aa.attr = attribsnet;
        }
        else if(sysmsg->echomail) {
          aa.type = GMB_ECHO;
          aa.attr = attribsecho;
        }
        else {
          aa.type = GMB_LOCAL;
          aa.attr = attribslocal;
        }

        aa.attr.pvt(sysmsg->doprivate);

        aa.aka = CAST(ftn_addr, cfg->alias[sysmsg->use_alias]);

        aa.setdesc(sysmsg->msg_name);
        aa.setechoid(sysmsg->echotag);
        aa.setorigin(sysmsg->origin);

        AddNewArea(aa);
      }
      throw_free(sysmsg);
      fp.fclose();
    }
    throw_free(cfg);
  }
}


//  ------------------------------------------------------------------