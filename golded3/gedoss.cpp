
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
//  DOS Shell, cleanup, errorhandling etc.
//  ------------------------------------------------------------------


#include <golded.h>
#include <gutlos.h>
#include <gmoprot.h>
#ifdef __UNIX__
#include <gkbdunix.h>
#endif


//  ------------------------------------------------------------------

extern GPickArealist* PickArealist;
extern bool in_arealist;

//  ------------------------------------------------------------------
//  Clean up the screen, memory and files before exiting to DOS

void Cleanup(void) {

  if(CFG) {

    if(CFG->switches.get(areakeeplast) and startupscan_success)
      AL.WriteGoldLast();

    #ifndef GMB_NOXBBS
    if(AL.msgbases & MT_ADEPTXBBS)
      XbbsExit();
    #endif
    #ifndef GMB_NOEZY
    if(AL.msgbases & MT_EZYCOM)
      EzycomExit();
    #endif
    if(AL.msgbases & MT_FIDO)
      FidoExit();
    #ifndef GMB_NOGOLD
    if(AL.msgbases & MT_GOLDBASE)
      GoldExit();
    #endif
    #ifndef GMB_NOHUDS
    if(AL.msgbases & MT_HUDSON)
      HudsExit();
    #endif
    #ifndef GMB_NOJAM
    if(AL.msgbases & MT_JAM)
      JamExit();
    #endif
    #ifndef GMB_NOPCB
    if(AL.msgbases & MT_PCBOARD)
      PcbExit();
    #endif
    #ifndef GMB_NOSQSH
    if(AL.msgbases & MT_SQUISH)
      SquishExit();
    #endif
    #ifndef GMB_NOWCAT
    if(AL.msgbases & MT_WILDCAT)
      WCatExit();
    #endif
    #ifndef GMB_NOSMB
    if(AL.msgbases & MT_SMB)
      SMBExit();
    #endif

    SearchExit();

    // Free area data
    AL.Reset();

    // Free various lists
    CFG->addressmacro.clear();
    CFG->aka.clear();
    CFG->akamatch.clear();
    CFG->event.clear();
    CFG->externutil.clear();
    CFG->filealias.clear();
    CFG->frqext.clear();
    CFG->frqnodemap.clear();
    CFG->kludge.clear();
    CFG->mailinglist.clear();
    CFG->mappath.clear();
    CFG->origin.clear();
    CFG->robotname.clear();
    CFG->tagline.clear();
    CFG->tpl.clear();
    CFG->twitname.clear();
    CFG->twitsubj.clear();
    CFG->username.clear();
    CFG->xlatcharset.clear();
    CFG->xlatescset.clear();
    CFG->cmdkey.clear();
    CFG->macro.clear();

    // Free misc data
    throw_xrelease(CharTable);
    throw_xrelease(CompTable);
    throw_xrelease(MNETable);
    throw_xrelease(I51Table);

    throw_delete(QWK);

    if(errorlevel != EXIT_CFGERR) {
      if(netpost)
        TouchFile(AddPath(CFG->areapath, CFG->semaphore.netscan));
      if(echopost)
        TouchFile(AddPath(CFG->areapath, CFG->semaphore.echoscan));
    }

    // Reset border color
    if(C_BACKB != 0)
      gvid->setoverscan(gvid->orig.color.overscan);

    wcloseall();                      // Close all windows
    if(in_arealist)
      PickArealist->close_all();      // Unlink hidden area windows
    whelpundef();                     // Disengage the help system
    kbclear();                        // Clear CXL keyboard buffer
    freonkey();                       // Free all onkeys (macros)
    FreePastebuf();                   // Free the internal editor cut'n'paste buffer

    #if not defined(__UNIX__) and not defined(__USE_NCURSES__)
    if(CFG->screenpalette[16])
      gvid->setpalette(gvid->orig.color.palette);

    if(gvid->curr.screen.mode != gvid->orig.screen.mode)
      gvid->setmode(gvid->orig.screen.mode);
    if(gvid->curr.screen.rows != gvid->orig.screen.rows)
      gvid->setrows(gvid->orig.screen.rows);

    if(oldscreen) {
      vrestore(oldscreen);
      throw_xrelease(oldscreen);
    }
    if(CFG->intensecolors)
      gvid->setintensity(gvid->orig.color.intensity);
    vcurset(gvid->orig.cursor.start, gvid->orig.cursor.end);
    #endif

    #ifndef __WIN32__
    vposset(gvid->orig.cursor.row, 0);
    vputx(gvid->orig.cursor.row, 0, gvid->orig.color.textattr, ' ', gvid->orig.screen.columns);
    #endif

    vposset(gvid->orig.cursor.row-1, 0);
    vcurshow();
  }
  throw_xdelete(BodyView);
  throw_xdelete(HeaderView);
  throw_xdelete(gvid);

  int smax = MinV((int)GLOG_STORELINES, LOG.storelines);
  for(int s=0; s<smax; s++)
    cout << LOG.storeline[s] << endl;
  if(LOG.storelines > GLOG_STORELINES)
    cout << "(See also " << CFG->logfile << ")" << endl;
  if(errorlevel > EXIT_NONAME)
    MakeNoise(SND_S_O_S);

  #if defined(GUTLOS_FUNCS)
  g_deinit_os();
  #endif

  // Back to default Ctrl-Break handler
  signal(SIGINT, SIG_DFL);
}


//  ------------------------------------------------------------------
//  Multipurpose DOS shell function

int ShellToDos(char* command, char* message, int cls, int cursor, int swap, int pause) {

  NW(swap);

  int error = 0;

  #if defined(GUTLOS_FUNCS)
  char ge_temptitle[GMAXTITLE];
  #endif

  #ifndef __UNIX__
  if(strlen(command) > 125) {
    w_info(" Warning: Command line longer than 125 characters! ");
    waitkeyt(10000);
    w_info(NULL);
  }
  #endif

  // Put up a wait window
  if(shellvid)
    w_info(LNG->Wait);

  #if defined(GUTLOS_FUNCS)
  g_get_ostitle_name(ge_temptitle);
  g_set_ostitle_name("OS Shell",0);
  #endif

  // Close msgbase files
  int _wasopen = AA->isopen();
  if(_wasopen)
    AA->Suspend();

  HandleGEvent(EVTT_DOSSHELL);

  // Change the prompt
  #ifndef __UNIX__
  static char prompt[256];
  static char oldprompt[256];

  if(CFG->switches.get(dosprompt)) {
    #ifdef __DJGPP__
    const char* p = getenv("PROMPT");
    if(p) {
      strcpy(oldprompt, p);
      strcpy(stpcpy(prompt, LNG->Prompt), p);
      setenv("PROMPT", prompt, true);
    }
    #else
    int envn = 0;
    while(environ[envn] and *environ[envn]) {
      if(strnieql(environ[envn], "PROMPT=", 7)) {
        strcpy(oldprompt, environ[envn]);
        sprintf(prompt, "PROMPT=%s%s", LNG->Prompt, *oldprompt ? oldprompt+7 : "");
        environ[envn] = prompt;
        break;
      }
      envn++;
    }
    #endif
  } 
  #endif

  // Store the screen
  vatch* scrnbuf = vsave();

  // Store current drive/dir
  Path orgdir;
  getcwd(orgdir, sizeof(Path));

  // Set cursor position
  if(gvid->curr.screen.rows != gvid->orig.screen.rows)
    gvid->setrows(gvid->orig.screen.rows);
  if(gvid->curr.screen.mode != gvid->orig.screen.mode)
    gvid->setmode(gvid->orig.screen.mode);

  // Clear screen
  if(cls)
    vclrscr(cls);

  // Reset border color
  if(C_BACKB != 0)
    gvid->setoverscan(gvid->orig.color.overscan);

  // Turn on the blinking attributes
  gvid->setintensity(gvid->orig.color.intensity);

  // Restore original palette during the shell
  if(CFG->screenpalette[16])
    gvid->setpalette(gvid->orig.color.palette);

  #if defined(__UNIX__) and not defined(__USE_NCURSES__)
  gkbd_tty_reset();
  #endif

  #ifdef __USE_NCURSES__
  def_prog_mode();
  reset_shell_mode();
  #endif

  // Write message on screen
  if(*message)
    puts(message);

  // Turn on cursor
  int yy, xx;
  if(cursor) {
    vposget(&yy, &xx);
    vcurset(gvid->orig.cursor.start, gvid->orig.cursor.end);
    vcurshow();
  }
  
  HandleGEvent(EVTT_BREAKLOOP);

  // Shell return value
  int status = -1;

  // Shell using the regular RTL function
  #ifndef __CYGWIN__
  status = system(command);
  #else
  // Get executable and parameters
  char* _arg_v[3];

  char* _pars = "";
  char _xfn[256] = ""; // Call command interpreter
  if(strnieql(command, "/c", 2))
    _pars = strskip_wht(command+2);
  else {
    _pars = strpbrk(command, " \t");
    if(_pars) {
      ++_pars++;
      strxcpy(_xfn, command, _pars-command);
      _pars = strskip_wht(_pars);
    }
    else
      _xfn = command;
  }
  _arg_v[0] = _xfn;
  _arg_v[1] = _pars;
  _arg_v[2] = NULL;
  status = spawnvpe(P_WAIT, _xfn, _arg_v, environ);
  #endif

  if(status == -1)
    error = errno;

  if(status != -1)
    status = 0;

  // Restore console settings
  #ifdef __USE_NCURSES__
  reset_prog_mode();
  clearok(stdscr, TRUE);
  #else
  gkbd.Init();
  #endif

  // Pause if needed
  if(pause) {
    if((pause > 0) or (status != 0))
      kbxget();
  }

  // Restore current directory
  gchdir(orgdir);

  // Restore video mode and rows
  if(CFG->screensize > 0xFF)
    gvid->setmode(CFG->screensize >> 8);
  else if(CFG->screensize)
    gvid->setrows(CFG->screensize);

  // Restore cursor position and form
  if(cursor) {
    vposset(yy+1, xx);
    vcurhide();
  }

  // Restore screen
  if(scrnbuf) {
    vrestore(scrnbuf);
    throw_xrelease(scrnbuf);
  } else
    vclrscr();

  // Restore screen intensity
  gvid->setintensity(CFG->intensecolors);

  // Restore border color
  if(C_BACKB != 0)
    gvid->setoverscan(C_BACKB);

  // Set palette if changes were specified
  if(CFG->screenpalette[16])
    gvid->setpalette(CFG->screenpalette);
  
  // Restore prompt
  #ifndef __UNIX__
  if(CFG->switches.get(dosprompt)) {
    #ifdef __DJGPP__
    setenv("PROMPT", oldprompt, true);
    #else
    int envn = 0;
    while(environ[envn] and *environ[envn]) {
      if(strnieql(environ[envn], "PROMPT=", 7)) {
        environ[envn] = oldprompt;
        break;
      }
      envn++;
    }
    #endif
  }
  #endif

  // Re-open msgbase
  if(_wasopen)
    AA->Resume();

  // Remove the wait window
  if(shellvid)
    w_info(NULL);

  // Popup error message
  if(error) {
    switch(errno) {
      case E2BIG:   w_info("Argument list too long!");             break;
      case EACCES:  w_info("Permission denied!");                  break;
      case EAGAIN:  w_info("Ressource temporarily unavailable!");  break;
      case EBADF:   w_info("Bad file descriptor!");                break;
      case EBUSY:   w_info("Resource busy!");                      break;
      case ECHILD:  w_info("No child processes!");                 break;
      case EEXIST:  w_info("File exists!");                        break;
      case EFAULT:  w_info("Bad address!");                        break;
      case EFBIG:   w_info("File too large!");                     break;
      case EINTR:   w_info("Interrupted system call!");            break;
      case EINVAL:  w_info("Invalid argument!");                   break;
      case EISDIR:  w_info("Is a directory!");                     break;
      case EMFILE:  w_info("Too many open files!");                break;
      case ENFILE:  w_info("Too many open files in system!");      break;
      case ENOENT:  w_info("No such file or directory!");          break;
      case ENOEXEC: w_info("Unable to execute file!");             break;
      case ENOMEM:  w_info("Not enough memory!");                  break;
      default:
        w_info("error during shelling");
    }
    waitkeyt(10000);
    w_info(NULL);
  }

  // Reset tick values to avoid triggering screenblanker or timeout
  gkbdtickpressreset();
  gkbdtickvaluereset();

  #if defined(GUTLOS_FUNCS)
  g_set_ostitle_name(ge_temptitle, 1);
  g_set_osicon();
  #endif

  return status;
}


//  ------------------------------------------------------------------
//  Error exit function

void ErrorExit(int type) {

  static int in_error_exit = false;

  if(not in_error_exit++) {

    error_exit = type;

    if(type) {

      HandleGEvent(EVTT_ERRORFATAL);

      if(_in_editor) {
        LOG.printf("+ TIP: If you were writing a msg and want to recover it,");
        LOG.printf("+ try looking in the %s file.", AddPath(CFG->goldpath, EDIT->File()));
      }

      // Dump the function tracker log
      #if defined(GFTRK_ENABLE)
      __gftrk_log();
      #endif

      if(type != 9)
        errorlevel = EXIT_ERRORS;
      else if(type == 5)
        errorlevel = 100;
      else
        errorlevel = EXIT_CFGERR;

      exit(errorlevel);
    }

    exit(EXIT_OK);
  }
}


//  ------------------------------------------------------------------

