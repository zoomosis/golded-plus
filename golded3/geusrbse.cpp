
//  ------------------------------------------------------------------
//  GoldED+
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
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
//  MA 02111-1307 USA
//  ------------------------------------------------------------------
//  $Id$
//  ------------------------------------------------------------------
//  Userbase (Addressbook) functions.
//  ------------------------------------------------------------------

#include <cerrno>
#include <fcntl.h>
#include <golded.h>
#include <geusrbse.h>
#include <gftnnl.h>


//  ------------------------------------------------------------------

guserbase::guserbase() {

  long tries = 0;

  strcpy(fname, AddPath(CFG->goldpath, CFG->golduser));

  do {
    usrbase.open(fname, O_RDWR|O_CREAT|O_BINARY, SH_DENYNO, S_STDRW);
    if(not usrbase) {
      if((errno != EACCES) or (not PopupLocked(++tries, false, fname))) {
        WideLog->ErrOpen();
        WideLog->printf("! GoldED's Addressbook cannot be opened.");
        WideLog->printf(": %s", fname);
        WideLog->ErrOSInfo();
        OpenErrorExit();
      }
    }
  } while(not usrbase);

  if(tries)
    PopupLocked(0, 0, NULL);

  if((uint) usrbase.filelength() < sizeof(gusrbaseheader) + sizeof(gusrbaseentry)) {

    header.version = 0;

    strcpy(entry.macro, "_asa_");
    strcpy(entry.name, "Alexander S. Aganichev");
    entry.fidoaddr.reset();
    entry.fidoaddr.zone = 2;
    entry.fidoaddr.net  = 5049;
    entry.fidoaddr.node = 49;
    entry.fidoaddr.point = 50;
    strcpy(entry.iaddr, "asa@eed.miee.ru");
    entry.prefer_internet = YES;
    entry.is_deleted = NO;
    strcpy(entry.pseudo, "As\'ka");
    strcpy(entry.organisation, "[unemployed]");
    strcpy(entry.snail1, "Zelenograd");
    strcpy(entry.snail2, "Moscow");
    strcpy(entry.snail3, "Russia");
    entry.dataphone[0] = NUL;
    entry.voicephone[0] = NUL;
    entry.faxphone[0] = NUL;
    entry.firstdate = entry.lastdate = entry.times = 0;
    strcpy(entry.homepage, "http://asa.i-connect.ru");
    entry.group = 0;
    strcpy(entry.comment1, "GoldED+ Maintainer");
    entry.comment2[0] = NUL;
    entry.comment3[0] = NUL;

    usrbase.lseek(0, SEEK_SET);
    usrbase.write(&header.version, sizeof(header.version));
    write_entry(0);
  }

  index = 0;
  read_time = 0;  // Indicate that we're should reread timestamp
  refresh_maximum_index();
  need_update = false;
}


//  ------------------------------------------------------------------

guserbase::~guserbase() {

  usrbase.close();
}


//  ------------------------------------------------------------------

void guserbase::refresh_maximum_index() {

  // Are we doing it for the first time?
  if(not read_time) {
    usrbase.getftime(&read_time);
    need_update = true;
  }
  else {
    dword tmp;
    usrbase.getftime(&tmp);
    if(read_time != tmp) {
      read_time = tmp;
      need_update = true;
    }
  }
  if(need_update)
    maximum_index = (usrbase.filelength()-sizeof(gusrbaseheader)) / sizeof(gusrbaseentry) - 1;
  if(index > maximum_index)
    index = maximum_index;
}


//  ------------------------------------------------------------------

void guserbase::lock() {

  if(WideCanLock) {

    long tries = 0;

    do {
      usrbase.lock(0, 1);
      if(not usrbase.okay()) {
        if(not PopupLocked(++tries, false, fname)) {
          WideLog->ErrLock();
          WideLog->printf("! GoldED's Addressbook could not be locked.");
          WideLog->printf(": %s", fname);
          WideLog->ErrOSInfo();
          LockErrorExit();
        }
      }
    } while(not usrbase.okay());
    if(tries)
      PopupLocked(0, 0, NULL);
  }
}


//  ------------------------------------------------------------------

void guserbase::unlock() {

  if(WideCanLock)
    usrbase.unlock(0, 1);
}


//  ------------------------------------------------------------------

void guserbase::open() {

  window.openxy(ypos, xpos, ylen+2, xlen+2,  btype, battr, 7);
  cwidth = (xlen-28) / 2;

  window.message(LNG->UserHeaderName, TP_BORD, 3, tattr);
  window.message(LNG->UserHeaderOrg,  TP_BORD, 4+cwidth, tattr);
  window.message(LNG->UserHeaderAka,  TP_BORD, 5+(cwidth*2)/3 + cwidth, tattr);

  center(CFG->displistcursor);
}


//  ------------------------------------------------------------------

void guserbase::close() {

  window.close();
}


//  ------------------------------------------------------------------

void guserbase::do_delayed() {

  wscrollbar(W_VERT, maximum_index+1, maximum_index, index);
  update_statuslinef(LNG->UserStatusline, index+1, maximum_index+1, maximum_index-index);
}


//  ------------------------------------------------------------------

void guserbase::print_line(uint idx, uint pos, bool isbar) {

  char buf[200];
  char buf2[100];

  read_entry(idx);

  *buf2 = NUL;

  if(AA->isinternet() or not entry.fidoaddr.valid()) {
    if(*entry.iaddr) {
      strcat(buf2, "<");
      strcat(buf2, entry.iaddr);
      strcat(buf2, ">");
    }
  }
  else {
    if(entry.fidoaddr.valid()) {
      *buf2 = '(';
      entry.fidoaddr.make_string(buf2+1);
      strcat(buf2, ")");
    }
  }

  sprintf(buf, "%c %-*.*s %-*.*s %s ",
    entry.is_deleted ? 'D' : ' ',
    cwidth, (int)cwidth, entry.name,
    (cwidth*2)/3, (int)(cwidth*2)/3, entry.organisation,
    buf2);

  strsetsz(buf, xlen);
  window.prints(pos, 0, isbar ? sattr : wattr, buf);
}


//  ------------------------------------------------------------------

addressbook_form::~addressbook_form() { }
addressbook_form::addressbook_form(gwindow& w) : gwinput2(w) { };
void addressbook_form::after() { gwinput2::after(); };
void addressbook_form::before() { gwinput2::before(); };


//  ------------------------------------------------------------------

bool addressbook_form::validate() {

  if(current->id == id_name) {
    if(g->find_entry(current->buf)) {
      LoadForm();
      reload_all();
      go_next_field();
    }
  }
  return true;
}


//  ------------------------------------------------------------------

void addressbook_form::LoadForm() {

  gusrbaseentry& entry = g->entry;

  entry.fidoaddr.make_string(fidoaddr);
  name      = entry.name;
  macro     = entry.macro;
  pseudo    = entry.pseudo;
  iaddr     = entry.iaddr;
  organisation = entry.organisation;
  voicephone= entry.voicephone;
  faxphone  = entry.faxphone;
  dataphone = entry.dataphone;
  snail1    = entry.snail1;
  snail2    = entry.snail2;
  snail3    = entry.snail3;
  comment1  = entry.comment1;
  comment2  = entry.comment2;
  comment3  = entry.comment3;
  homepage  = entry.homepage;
}


//  ------------------------------------------------------------------

void addressbook_form::SaveForm() {

  gusrbaseentry& entry = g->entry;
  entry.fidoaddr.reset();
  entry.fidoaddr.set(fidoaddr);
  strcpy(entry.name, name.c_str());
  strcpy(entry.macro, macro.c_str());
  strcpy(entry.pseudo, pseudo.c_str());
  strcpy(entry.iaddr, iaddr.c_str());
  strcpy(entry.organisation, organisation.c_str());
  strcpy(entry.voicephone, voicephone.c_str());
  strcpy(entry.faxphone, faxphone.c_str());
  strcpy(entry.dataphone, dataphone.c_str());
  strcpy(entry.snail1, snail1.c_str());
  strcpy(entry.snail2, snail2.c_str());
  strcpy(entry.snail3, snail3.c_str());
  strcpy(entry.comment1, comment1.c_str());
  strcpy(entry.comment2, comment2.c_str());
  strcpy(entry.comment3, comment3.c_str());
  strcpy(entry.homepage, homepage.c_str());
}


//  ------------------------------------------------------------------

bool guserbase::edit_entry(uint idx) {

  gwindow window;
  char tbuf[50];

  const int width = 75;
  const int height = 16;

  window.openxy((MAXROW-height)/2, (MAXCOL-width)/2, height, width, btype, battr, wattr);
  window.shadow(C_SHADOW);

  sprintf(tbuf, " Record <%d> ", idx+1);
  window.title(tbuf, tattr);

  window.prints( 0, 1, wattr, "Full Name :");
  window.prints( 1, 1, wattr, "Macro Name:");
  window.prints( 1,34, wattr, "Nick Name :");
  window.prints( 2, 1, wattr, "Fidonet   :");
  window.prints( 2,34, wattr, "Internet  :");
  window.prints( 3, 1, wattr, "Organisat.:");
  window.horizontal_line(4, 0, width-2, btype, battr);

  window.prints( 5, 1, wattr, "Voice Num.:");
  window.prints( 6, 1, wattr, "Fax Number:");
  window.prints( 7, 1, wattr, "Data Num. :");
  window.prints( 5,34, wattr, "Address(1):");
  window.prints( 6,34, wattr, "Address(2):");
  window.prints( 7,34, wattr, "Address(3):");
  window.prints( 8, 1, wattr, "Group     :");
  window.prints( 8,34, wattr, "Homepage  :");
  window.prints( 9, 1, wattr, "Comment(1):");
  window.prints(10, 1, wattr, "Comment(2):");
  window.prints(11, 1, wattr, "Comment(3):");
  window.horizontal_line(12, 0, width-2, btype, battr);

  window.prints(13, 1, wattr, "First Used:");
  window.prints(13, 27, wattr, "Last Used:");
  window.prints(13, 53, wattr, "Times Used:");

  char dbuf[16];
  time_t dt = entry.firstdate;
  if(dt)
    window.prints(13, 13, wattr, strftimei(dbuf, 16, "%d %b %y", gmtime(&dt)));
  dt = entry.lastdate;
  if(dt)
    window.prints(13, 38, wattr, strftimei(dbuf, 16, "%d %b %y", gmtime(&dt)));

  sprintf(dbuf, "%8ld", entry.times);
  window.prints(13, width-11, wattr, dbuf);

  addressbook_form form(window);
  form.g = this;
  form.setup(C_HEADW, C_HEADW, C_HEADE, _box_table(W_BHEAD, 13), true);

  read_entry(idx);
  form.LoadForm();

  form.add_field(addressbook_form::id_name,         0, 13, 59, form.name, sizeof(entry.name));
  form.add_field(addressbook_form::id_macro,        1, 13, 20, form.macro, sizeof(entry.macro));
  form.add_field(addressbook_form::id_pseudo,       1, 46, 26, form.pseudo, sizeof(entry.pseudo));
  form.add_field(addressbook_form::id_fidoaddr,     2, 13, 20, form.fidoaddr, 24);
  form.add_field(addressbook_form::id_iaddr,        2, 46, 26, form.iaddr, sizeof(entry.iaddr));
  form.add_field(addressbook_form::id_organisation, 3, 13, 59, form.organisation, sizeof(entry.organisation));
  form.add_field(addressbook_form::id_voicephone,   5, 13, 20, form.voicephone, sizeof(entry.voicephone));
  form.add_field(addressbook_form::id_faxphone,     6, 13, 20, form.faxphone, sizeof(entry.faxphone));
  form.add_field(addressbook_form::id_dataphone,    7, 13, 20, form.dataphone, sizeof(entry.dataphone));
  form.add_field(addressbook_form::id_snail1,       5, 46, 26, form.snail1, sizeof(entry.snail1));
  form.add_field(addressbook_form::id_snail2,       6, 46, 26, form.snail2, sizeof(entry.snail2));  
  form.add_field(addressbook_form::id_snail3,       7, 46, 26, form.snail3, sizeof(entry.snail3));
  form.add_field(addressbook_form::id_homepage ,    8, 46, 26, form.homepage, sizeof(entry.homepage));
  form.add_field(addressbook_form::id_comment1,     9, 13, 59, form.comment1, sizeof(entry.comment1));
  form.add_field(addressbook_form::id_comment2,    10, 13, 59, form.comment2, sizeof(entry.comment2));
  form.add_field(addressbook_form::id_comment3,    11, 13, 59, form.comment3, sizeof(entry.comment3));

  form.run(H_EditAdrEntry);
  window.close();

  if(not form.dropped)
    form.SaveForm();

  return not form.dropped;
}


//  ------------------------------------------------------------------

bool guserbase::find_entry(char* name, bool lookup) {

  gusrbaseentry ent;

  if(not strblank(name)) {
    refresh_maximum_index();
    usrbase.lseek(sizeof(gusrbaseheader), SEEK_SET);
    for(uint i=0; i<=maximum_index; i++) {
      read_entry(i, &ent);

      if(strieql(name, ent.name) or (lookup and strieql(name, ent.macro))) {
        strcpy(entry.macro, ent.macro);
        strcpy(entry.name, ent.name);
        entry.fidoaddr = ent.fidoaddr;
        strcpy(entry.iaddr, ent.iaddr);
        entry.prefer_internet = ent.prefer_internet;
        entry.is_deleted = ent.is_deleted;
        strcpy(entry.pseudo, ent.pseudo);
        strcpy(entry.organisation, ent.organisation);
        strcpy(entry.snail1, ent.snail1);
        strcpy(entry.snail2, ent.snail2);
        strcpy(entry.snail3, ent.snail3);
        strcpy(entry.dataphone, ent.dataphone);
        strcpy(entry.voicephone, ent.voicephone);
        strcpy(entry.faxphone, ent.faxphone);
        entry.firstdate = ent.firstdate;
        entry.lastdate = ent.lastdate;
        entry.times = ent.times;
        strcpy(entry.homepage, ent.homepage);
        entry.group = ent.group;
        strcpy(entry.comment1, ent.comment1);
        strcpy(entry.comment2, ent.comment1);
        strcpy(entry.comment3, ent.comment1);
        index = i;
        return true;
      }
    }
  }

  return false;
}


//  ------------------------------------------------------------------

void guserbase::write_entry(uint idx, bool updateit) {

  if(updateit and not entry.is_deleted) {
    time_t a = time(NULL);
    time_t b = mktime(gmtime(&a));
    entry.lastdate = a + a - b;
    if(not entry.firstdate)
      entry.firstdate = entry.lastdate;

    entry.times++;
  }

  usrbase.lseek(sizeof(gusrbaseheader) + sizeof(gusrbaseentry)*(idx+1)-1, SEEK_SET);
  char z = 0;
  usrbase.write(&z, 1); // adjust entry size first...
  usrbase.lseek(sizeof(gusrbaseheader) + sizeof(gusrbaseentry)*idx, SEEK_SET);
  usrbase.write(entry.macro, sizeof(entry.macro));
  usrbase.write(entry.name, sizeof(entry.name));
  usrbase.write(&entry.fidoaddr.zone, sizeof(entry.fidoaddr.zone));
  usrbase.write(&entry.fidoaddr.net, sizeof(entry.fidoaddr.net));
  usrbase.write(&entry.fidoaddr.node, sizeof(entry.fidoaddr.node));
  usrbase.write(&entry.fidoaddr.point, sizeof(entry.fidoaddr.point));
  usrbase.write(entry.iaddr, sizeof(entry.iaddr));
  usrbase.write(&entry.prefer_internet, sizeof(entry.prefer_internet));
  usrbase.write(&entry.is_deleted, sizeof(entry.is_deleted));
  usrbase.write(entry.pseudo, sizeof(entry.pseudo));
  usrbase.write(entry.organisation, sizeof(entry.organisation));
  usrbase.write(entry.snail1, sizeof(entry.snail1));
  usrbase.write(entry.snail2, sizeof(entry.snail2));
  usrbase.write(entry.snail3, sizeof(entry.snail3));
  usrbase.write(entry.dataphone, sizeof(entry.dataphone));
  usrbase.write(entry.voicephone, sizeof(entry.voicephone));
  usrbase.write(entry.faxphone, sizeof(entry.faxphone));
  usrbase.write(&entry.firstdate, sizeof(entry.firstdate));
  usrbase.write(&entry.lastdate, sizeof(entry.lastdate));
  usrbase.write(&entry.times, sizeof(entry.times));
  usrbase.write(entry.homepage, sizeof(entry.homepage));
  usrbase.write(&entry.group, sizeof(entry.group));
  usrbase.write(entry.comment1, sizeof(entry.comment1));
  usrbase.write(entry.comment2, sizeof(entry.comment1));
  usrbase.write(entry.comment3, sizeof(entry.comment1));
}

//  ------------------------------------------------------------------

void guserbase::clear_entry(gusrbaseentry *ent) {

  ent->macro[0] = NUL;
  ent->name[0] = NUL;
  ent->fidoaddr.reset();
  ent->iaddr[0] = NUL;
  ent->prefer_internet = NO;
  ent->is_deleted = NO;
  ent->pseudo[0] = NUL;
  ent->organisation[0] = NUL;
  ent->snail1[0] = NUL;
  ent->snail2[0] = NUL;
  ent->snail3[0] = NUL;
  ent->dataphone[0] = NUL;
  ent->voicephone[0] = NUL;
  ent->faxphone[0] = NUL;
  ent->firstdate = ent->lastdate = ent->times = 0;
  ent->homepage[0] = NUL;
  ent->group = 0;
  ent->comment1[0] = NUL;
  ent->comment2[0] = NUL;
  ent->comment3[0] = NUL;
}

//  ------------------------------------------------------------------

bool guserbase::read_entry(uint idx, gusrbaseentry *ent) {

  if(ent == NULL)
    ent = &entry;
  refresh_maximum_index();
  if(idx > maximum_index) {
    clear_entry(ent);
    return false;
  }
  else {                  
    usrbase.lseek(idx*sizeof(gusrbaseentry)+sizeof(gusrbaseheader), SEEK_SET);
    usrbase.read(ent->macro, sizeof(ent->macro));
    usrbase.read(ent->name, sizeof(ent->name));
    usrbase.read(&ent->fidoaddr.zone, sizeof(ent->fidoaddr.zone));
    usrbase.read(&ent->fidoaddr.net, sizeof(ent->fidoaddr.net));
    usrbase.read(&ent->fidoaddr.node, sizeof(ent->fidoaddr.node));
    usrbase.read(&ent->fidoaddr.point, sizeof(ent->fidoaddr.point));
    usrbase.read(ent->iaddr, sizeof(ent->iaddr));
    usrbase.read(&ent->prefer_internet, sizeof(ent->prefer_internet));
    usrbase.read(&ent->is_deleted, sizeof(ent->is_deleted));
    usrbase.read(ent->pseudo, sizeof(ent->pseudo));
    usrbase.read(ent->organisation, sizeof(ent->organisation));
    usrbase.read(ent->snail1, sizeof(ent->snail1));
    usrbase.read(ent->snail2, sizeof(ent->snail2));
    usrbase.read(ent->snail3, sizeof(ent->snail3));
    usrbase.read(ent->dataphone, sizeof(ent->dataphone));
    usrbase.read(ent->voicephone, sizeof(ent->voicephone));
    usrbase.read(ent->faxphone, sizeof(ent->faxphone));
    usrbase.read(&ent->firstdate, sizeof(ent->firstdate));
    usrbase.read(&ent->lastdate, sizeof(ent->lastdate));
    usrbase.read(&ent->times, sizeof(ent->times));
    usrbase.read(ent->homepage, sizeof(ent->homepage));
    usrbase.read(&ent->group, sizeof(ent->group));
    usrbase.read(ent->comment1, sizeof(ent->comment1));
    usrbase.read(ent->comment2, sizeof(ent->comment1));
    usrbase.read(ent->comment3, sizeof(ent->comment1));
    return true;
  }
}


//  ------------------------------------------------------------------

void guserbase::pack_addressbook() {

  long tries = 0;

  lock();
  refresh_maximum_index();
  uint nidx = 0;
  uint nindex = index;
  for(uint idx = 0; idx <= maximum_index; idx++) {
    read_entry(idx);
    if(not entry.is_deleted) {
      if(nidx != idx)
        write_entry(nidx);
      ++nidx;
    }
    else if(idx < index)
      --nindex;
  }
  index = nindex;
  // zap
  maximum_index = nidx;
  // At least one record should present
  if(maximum_index)
    --maximum_index;
  usrbase.chsize((maximum_index + 1) * sizeof(gusrbaseentry) + sizeof(gusrbaseheader));
  usrbase.close();

  do {
    usrbase.open(fname, O_RDWR|O_CREAT|O_BINARY, SH_DENYNO, S_STDRW);
    if(not usrbase) {
      if((errno != EACCES) or (not PopupLocked(++tries, false, fname))) {
        WideLog->ErrOpen();
        WideLog->printf("! GoldED's Addressbook cannot be opened.");
        WideLog->printf(": %s", fname);
        WideLog->ErrOSInfo();
        OpenErrorExit();
      }
    }
  } while(not usrbase);

  if(tries)
    PopupLocked(0, 0, NULL);

  unlock();
}


//  ------------------------------------------------------------------

void guserbase::update_screen(bool force) {

  refresh_maximum_index();
  if(force or need_update) {
    if(position > index)
      center(CFG->displistcursor);
    update();
    do_delayed();
    need_update = false;
  }
}
                                   

//  ------------------------------------------------------------------

bool guserbase::handle_key() {

  switch(key) {
    case Key_Esc:
      aborted = true;
      return false;
    case Key_Ins:           // Add new entry
      {
        clear_entry(&entry);
        lock();
        refresh_maximum_index();
        uint nidx = maximum_index + 1;
        if(edit_entry(nidx)) {
          if(nidx == maximum_index + 1) {
            maximum_index++;
            if(maximum_position < ylen-1)
              maximum_position = maximum_index;
          }
          write_entry(nidx);
          index = nidx;
        }
        unlock();  
        center(CFG->displistcursor);
      }
      break;
    case Key_Ent:          // Select/Edit entry
      if(not select_list) {
        if(not entry.is_deleted) {
          lock();
          if(edit_entry(index)) {
            write_entry(index);
            display_bar();
          }
          unlock();
        }
        break;
      }
      else {
        aborted = false;
        return false;
      }
    case Key_Del:          // Soft-Delete Entry       
      lock();
      refresh_maximum_index();
      read_entry(index);
      entry.is_deleted = not entry.is_deleted;
      write_entry(index);
      unlock();
      update_screen();
      break;
    case Key_A_P:          // Pack Addressbook
      pack_addressbook();
      index = position = 0;
      update_screen();
      break;

    case Key_Tick:
      CheckTick(0);    // KK_UserQuitNow ???
      break;
    default:
      SayBibi();
  }
  return true;
}


//  ------------------------------------------------------------------

bool guserbase::run(GMsg* msg, bool selectonly) {

  select_list = selectonly;

  ypos    = selectonly ? 6 : 1;
  xpos    = 0;
  ylen    = MAXROW-3-ypos;
  xlen    = MAXCOL-2;
  btype   = W_BMENU;
  battr   = C_MENUB;
  wattr   = C_MENUW;
  tattr   = C_MENUT;
  sattr   = C_MENUS;
  hattr   = C_MENUQ;
  sbattr  = C_MENUPB;
  helpcat = H_Userbase;
  listwrap  = CFG->switches.get(displistwrap);
  maximum_position = MinV(maximum_index, ylen - 1);

  if(not select_list)
    find_entry(msg->By(), false);

  run_picker();

  if(not aborted) {
    read_entry(index);

    // other stuff not yet implemented
  }

  return not aborted;
}


//  ------------------------------------------------------------------

void guserbase::update_addressbook(GMsg* msg, bool reverse, bool force) {

  Addr fidoaddr; 
  IAdr iaddr;
  INam name;

  strcpy(name, (reverse ? msg->By() : msg->To()));
  strcpy(iaddr, (reverse ? msg->iorig : msg->idest));
  fidoaddr = (reverse ? msg->orig : msg->dest);

  if(not strblank(name)) {
    if(find_entry(name)) {
      if(not force)
        if((CFG->addressbookadd == NO) or ((CFG->addressbookadd == YES) and not (AA->isnet() or AA->isemail())))
          return;
    }
    else {

      if(not force) {

        // Don't automatically add the user if...
        // 1. It's not allowed
        if((CFG->addressbookadd == NO) or ((CFG->addressbookadd == YES) and not (AA->isnet() or AA->isemail())))
          return;

        // 2. It's a robotname
        for(gstrarray::iterator n = CFG->robotname.begin(); n != CFG->robotname.end(); n++)
          if(striinc(n->c_str(), name))
            return;

        // 3. It's a username
        if(is_user(name))
          return;

        // 4. It's a WHOTO name
        if(strieql(AA->Whoto(), name))
          return;

        // 5. It's the mailinglist's sender address
        if(AA->isemail()) {
          vector<MailList>::iterator z;
          for(z = CFG->mailinglist.begin(); z != CFG->mailinglist.end(); z++)
            if(strieql(z->sender, name) or strieql(z->sender, iaddr))
              return;
        }

        // 6. If it is already an email address
        if(AA->isemail() and strchr(name, '@'))
          return;


        // 7. User listed in nodelist
        const char *nlname = lookup_nodelist(&fidoaddr);
        if(nlname and strieql(nlname, name))
          return;

      }

      // Ok, let's add it...
      clear_entry(&entry);

      strxcpy(entry.name, name, sizeof(entry.name));

      refresh_maximum_index();
      index = ++maximum_index;
    }

    if((AA->isinternet() and strblank(iaddr)) or (not AA->isinternet() and not fidoaddr.valid()))
      return;

    // Update address
    if(AA->isinternet())
      strxcpy(entry.iaddr, iaddr, sizeof(entry.iaddr));
    else
      entry.fidoaddr = fidoaddr;

    lock();
    write_entry(index, not force);
    unlock();
  }
}


//  ------------------------------------------------------------------

bool guserbase::lookup_addressbook(GMsg* msg, char* name, char* aka, bool browser) {

  bool found = false;

  if(not strblank(name) or browser) {
    w_info(LNG->UserWait);

    if((find_entry(name, true) and not entry.is_deleted) or browser) {

      if(browser) {
        if(not run(msg, true)) {
          w_info(NULL);
          return false;
        }
      }

      found = true;

      strcpy(name, entry.name);
      if(not strblank(entry.pseudo))
        strcpy(msg->pseudoto, entry.pseudo);

      if(aka) {
        *aka = NUL;
        if(AA->isinternet())
          strcpy(aka, entry.iaddr);
        else {
          entry.fidoaddr.make_string(aka);
          if(strblank(aka) and not strblank(entry.iaddr)) {
            // do UUCP addressing
            strcpy(msg->realto, entry.name);
            strcpy(name, entry.iaddr);
            strcpy(msg->iaddr, entry.iaddr);
          }
        }
      }
    }
    w_info(NULL);
    return found ? (aka ? not strblank(aka) : true) : false;
  }
  return false;
}


//  ------------------------------------------------------------------

void guserbase::build_pseudo(GMsg* msg, char* name, char* aka, bool direction) {

  strcpy(direction ? msg->pseudoto : msg->pseudofrom, strlword(name));

  if(find_entry(name, true) and not entry.is_deleted) {

    if(not strblank(entry.pseudo)) {

      if(AA->isinternet()) {
        if(strcmp(aka, entry.iaddr))
          return;
      }
      else {
        Addr AKA = aka;
        if(entry.fidoaddr != AKA)
          return;
      }

      strcpy(direction ? msg->pseudoto : msg->pseudofrom, entry.pseudo);

    }
  }
}


//  ------------------------------------------------------------------

void update_addressbook(GMsg* msg, bool reverse, bool force) {

  guserbase p;

  p.update_addressbook(msg, reverse, force);
}


//  ------------------------------------------------------------------

bool lookup_addressbook(GMsg* msg, char* name, char* aka, bool browser) {

  guserbase p;

  bool result = p.lookup_addressbook(msg, name, aka, browser);

  return result;
}

 
//  ------------------------------------------------------------------

void build_pseudo(GMsg* msg, bool direction) {

  guserbase p;
  char buf[128];

  if(direction)
    p.build_pseudo(msg, strbtrim(msg->To()), msg->dest.make_string(buf));
  else
    p.build_pseudo(msg, strbtrim(msg->By()), msg->orig.make_string(buf), false);
}                                  


//  ------------------------------------------------------------------

void edit_addressbook(GMsg* msg) {

  guserbase p;

  p.run(msg, false);

}


//  ------------------------------------------------------------------
