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
//  Read areas from Ezycom
//  ------------------------------------------------------------------

#include <cstdlib>
#include <gstrall.h>
#include <gmemdbg.h>
#include <gedacfg.h>
#include <gs_ez110.h>


//  ------------------------------------------------------------------

void gareafile::ReadEzycom110(FILE* fp, char* path, char* file, char* options) {

  int n;
  AreaCfg aa;
  char abuf[40];

  CONFIGRECORD* config = new CONFIGRECORD; throw_new(config);
  CONSTANTRECORD* constant = new CONSTANTRECORD; throw_new(constant);
  MESSAGERECORD* messages = new MESSAGERECORD; throw_new(messages);

  fread(config, sizeof(CONFIGRECORD), 1, fp);
  fclose(fp);

  MakePathname(file, path, "constant.ezy");
  fp = fsopen(file, "rb", sharemode);
  if(fp) {

    if(not quiet)
      cout << "* Reading " << file << endl;

    fread(constant, sizeof(CONSTANTRECORD), 1, fp);
    fclose(fp);

    STRNP2C(config->defaultorigin);
    STRNP2C(config->userbasepath);
    STRNP2C(config->msgpath);
    STRNP2C(config->netmailpath);
    CfgOrigin(config->defaultorigin);

    STRNP2C(constant->sysopname);
    STRNP2C(constant->sysopalias);

    if(*ezycom_msgbasepath == NUL)
      AddBackslash(strcpy(ezycom_msgbasepath, MapPath(config->msgpath)));
    if(*ezycom_userbasepath == NUL)
      AddBackslash(strcpy(ezycom_userbasepath, MapPath(config->userbasepath)));

    // Fido netmail directory
    if(not strblank(config->netmailpath)) {
      aa.reset();
      aa.msgbase = fidomsgtype;
      aa.type = GMB_NET;
      aa.attr = attribsnet;
      aa.aka.zone  = constant->netaddress[0].zone;
      aa.aka.net   = constant->netaddress[0].net;
      aa.aka.node  = constant->netaddress[0].node;
      aa.aka.point = constant->netaddress[0].point;
      aa.setpath(config->netmailpath);
      aa.setdesc("Ezycom Netmail");
      aa.setautoid("NETMAIL");
      AddNewArea(aa);
    }

    // Ezycom aka netmail boards
    for(n=0; n<MAXAKA; n++) {
      if(constant->netaddress[n].net) {
        if(constant->netmailboard[n]) {
          aa.reset();
          aa.msgbase = GMB_EZYCOM;
          aa.type = GMB_NET;
          aa.attr = attribsnet;
          aa.aka.zone  = constant->netaddress[n].zone;
          aa.aka.net   = constant->netaddress[n].net;
          aa.aka.node  = constant->netaddress[n].node;
          aa.aka.point = constant->netaddress[n].point;
          aa.board = constant->netmailboard[n];
          Desc desc;
          sprintf(desc, "Ezycom Netmail Board, %s", constant->netaddress[n].make_string(abuf));
          aa.setdesc(desc);
          sprintf(desc, "NET_AKA%u", n);
          aa.setautoid(desc);
          AddNewArea(aa);
        }
      }
    }

    // Ezycom watchdog board
    if(constant->watchmess) {
      aa.reset();
      aa.msgbase = GMB_EZYCOM;
      aa.type = GMB_LOCAL;
      aa.attr = attribslocal;
      aa.aka.zone  = constant->netaddress[0].zone;
      aa.aka.net   = constant->netaddress[0].net;
      aa.aka.node  = constant->netaddress[0].node;
      aa.aka.point = constant->netaddress[0].point;
      aa.board = constant->watchmess;
      aa.setdesc("Ezycom Watchdog Board");
      aa.setautoid("LOCAL_WATCHDOG");
      AddNewArea(aa);
    }

    // Ezycom paging board
    if(constant->pagemessboard) {
      aa.reset();
      aa.msgbase = GMB_EZYCOM;
      aa.type = GMB_LOCAL;
      aa.attr = attribslocal;
      aa.aka.zone  = constant->netaddress[0].zone;
      aa.aka.net   = constant->netaddress[0].net;
      aa.aka.node  = constant->netaddress[0].node;
      aa.aka.point = constant->netaddress[0].point;
      aa.board = constant->pagemessboard;
      aa.setdesc("Ezycom Paging Board");
      aa.setautoid("LOCAL_PAGING");
      AddNewArea(aa);
    }

    // Ezycom bad logon board
    if(constant->badpwdmsgboard) {
      aa.reset();
      aa.msgbase = GMB_EZYCOM;
      aa.type = GMB_LOCAL;
      aa.attr = attribslocal;
      aa.aka.zone  = constant->netaddress[0].zone;
      aa.aka.net   = constant->netaddress[0].net;
      aa.aka.node  = constant->netaddress[0].node;
      aa.aka.point = constant->netaddress[0].point;
      aa.board = constant->badpwdmsgboard;
      aa.setdesc("Ezycom Bad Logon Board");
      aa.setautoid("LOCAL_BADLOGON");
      AddNewArea(aa);
    }

    // Ezycom bad qwk board
    if(constant->qwkmsgboard) {
      aa.reset();
      aa.msgbase = GMB_EZYCOM;
      aa.type = GMB_ECHO;
      aa.attr = attribsecho;
      aa.aka.zone  = constant->netaddress[0].zone;
      aa.aka.net   = constant->netaddress[0].net;
      aa.aka.node  = constant->netaddress[0].node;
      aa.aka.point = constant->netaddress[0].point;
      aa.board = constant->qwkmsgboard;
      aa.setdesc("Ezycom Bad QWK Board");
      aa.setautoid("ECHO_BADQWK");
      AddNewArea(aa);
    }

    // Ezycom bad echomail board
    if(constant->badmsgboard) {
      aa.reset();
      aa.msgbase = GMB_EZYCOM;
      aa.type = GMB_ECHO;
      aa.attr = attribsecho;
      aa.aka.zone  = constant->netaddress[0].zone;
      aa.aka.net   = constant->netaddress[0].net;
      aa.aka.node  = constant->netaddress[0].node;
      aa.aka.point = constant->netaddress[0].point;
      aa.board = constant->badmsgboard;
      aa.setdesc("Ezycom Bad Echomail Board");
      aa.setautoid("ECHO_BAD");
      AddNewArea(aa);
    }

    MakePathname(file, path, "MESSAGES.EZY");
    fp = fsopen(file, "rb", sharemode);
    if(fp) {

      if(not quiet)
        cout << "* Reading " << file << endl;

      int record = 1;

      while(fread(messages, sizeof(MESSAGERECORD), 1, fp) == 1) {

        if(record <= constant->maxmess) {

          if(*messages->name) {

            switch(messages->typ) {

              case 0:   // localmail
              case 1:   // netmail
              case 2:   // echomail
              case 5:   // allmail

                aa.reset();

                STRNP2C(messages->name);
                STRNP2C(messages->areatag);
                STRNP2C(messages->originline);

                aa.board = record;
                aa.msgbase = GMB_EZYCOM;
                aa.groupid = messages->areagroup;
                aa.setorigin(*messages->originline ? messages->originline : config->defaultorigin);

                aa.setdesc(messages->name);
                aa.setechoid(messages->areatag);

                switch(messages->typ) {
                  case 0:
                    aa.type = GMB_LOCAL;
                    aa.attr = attribslocal;
                    break;
                  case 1:
                    aa.type = GMB_NET;
                    aa.attr = attribsnet;
                    break;
                  default:
                    aa.type = GMB_ECHO;
                    aa.attr = attribsecho;
                }

                switch(messages->msgkinds) {
                  case 1:
                    aa.attr.pvt1();
                    break;
                  case 0:
                  case 2:
                    aa.attr.pvt0();
                    break;
                }

                aa.aka.zone  = constant->netaddress[messages->originaddress-1].zone;
                aa.aka.net   = constant->netaddress[messages->originaddress-1].net;
                aa.aka.node  = constant->netaddress[messages->originaddress-1].node;
                aa.aka.point = constant->netaddress[messages->originaddress-1].point;

                AddNewArea(aa);

                break;
            }
          }
        }

        record++;
      }
      fclose(fp);
    }
  }

  throw_delete(messages);
  throw_delete(constant);
  throw_delete(config);
}


//  ------------------------------------------------------------------

void gareafile::ReadEzycom(char* tag) {

  FILE* fp;
  char* ptr;
  Path path, file;
  char options[80], abuf[40];

  *file = NUL;
  *path = NUL;
  strcpy(options, tag);
  ptr = strtok(tag, " \t");
  while(ptr) {
    if(*ptr != '-') {
      AddBackslash(strcpy(path, ptr));
      break;
    }
    ptr = strtok(NULL, " \t");
  }
  if(*path == NUL) {
    ptr = getenv("EZY");
    if(ptr)
      AddBackslash(strcpy(path, ptr));
  }
  if(*path == NUL)
    strcpy(path, areapath);

  ptr = getenv("TASK");
  if(ptr and *ptr) {
    sprintf(abuf, "CONFIG.%u", atoi(ptr));
    MakePathname(file, path, abuf);
  }
  if(not fexist(file))
    MakePathname(file, path, "config.ezy");
  fp = fsopen(file, "rb", sharemode);
  if(fp) {

    if(not quiet)
      cout << "* Reading " << file << endl;

    char _verstr[9];
    fread(_verstr, 9, 1, fp);
    rewind(fp);

    strp2c(_verstr);

    if(strnicmp(_verstr, "1.02", 4) < 0) {
      cout << "* Error: Ezycom v" << _verstr << " is not supported - Skipping." << endl;
      return;
    }
    else if(strnicmp(_verstr, "1.10", 4) >= 0)
      ReadEzycom110(fp, path, file, options);
    else
      ReadEzycom102(fp, path, file, options);
  }
}


//  ------------------------------------------------------------------