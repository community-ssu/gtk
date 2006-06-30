// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: dpkgpm.cc,v 1.28 2004/01/27 02:25:01 mdz Exp $
/* ######################################################################

   DPKG Package Manager - Provide an interface to dpkg
   
   ##################################################################### */
									/*}}}*/
// Includes								/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/dpkgpm.h"
#endif
#include <apt-pkg/dpkgpm.h>
#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/strutl.h>

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <sstream>
#include <map>

#include <config.h>
#include <apti18n.h>
									/*}}}*/

using namespace std;

// DPkgPM::pkgDPkgPM - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgDPkgPM::pkgDPkgPM(pkgDepCache *Cache) : pkgPackageManager(Cache)
{
}
									/*}}}*/
// DPkgPM::pkgDPkgPM - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgDPkgPM::~pkgDPkgPM()
{
}
									/*}}}*/
// DPkgPM::Install - Install a package					/*{{{*/
// ---------------------------------------------------------------------
/* Add an install operation to the sequence list */
bool pkgDPkgPM::Install(PkgIterator Pkg,string File)
{
   if (File.empty() == true || Pkg.end() == true)
      return _error->Error("Internal Error, No file name for %s",Pkg.Name());

   List.push_back(Item(Item::Install,Pkg,File));
   return true;
}
									/*}}}*/
// DPkgPM::Configure - Configure a package				/*{{{*/
// ---------------------------------------------------------------------
/* Add a configure operation to the sequence list */
bool pkgDPkgPM::Configure(PkgIterator Pkg)
{
   if (Pkg.end() == true)
      return false;
   
   List.push_back(Item(Item::Configure,Pkg));
   return true;
}
									/*}}}*/
// DPkgPM::Remove - Remove a package					/*{{{*/
// ---------------------------------------------------------------------
/* Add a remove operation to the sequence list */
bool pkgDPkgPM::Remove(PkgIterator Pkg,bool Purge)
{
   if (Pkg.end() == true)
      return false;
   
   if (Purge == true)
      List.push_back(Item(Item::Purge,Pkg));
   else
      List.push_back(Item(Item::Remove,Pkg));
   return true;
}
									/*}}}*/
// DPkgPM::RunScripts - Run a set of scripts				/*{{{*/
// ---------------------------------------------------------------------
/* This looks for a list of script sto run from the configuration file,
   each one is run with system from a forked child. */
bool pkgDPkgPM::RunScripts(const char *Cnf)
{
   Configuration::Item const *Opts = _config->Tree(Cnf);
   if (Opts == 0 || Opts->Child == 0)
      return true;
   Opts = Opts->Child;

   // Fork for running the system calls
   pid_t Child = ExecFork();
   
   // This is the child
   if (Child == 0)
   {
      if (chdir("/tmp/") != 0)
	 _exit(100);
	 
      unsigned int Count = 1;
      for (; Opts != 0; Opts = Opts->Next, Count++)
      {
	 if (Opts->Value.empty() == true)
	    continue;
	 
	 if (system(Opts->Value.c_str()) != 0)
	    _exit(100+Count);
      }
      _exit(0);
   }      

   // Wait for the child
   int Status = 0;
   while (waitpid(Child,&Status,0) != Child)
   {
      if (errno == EINTR)
	 continue;
      return _error->Errno("waitpid","Couldn't wait for subprocess");
   }

   // Restore sig int/quit
   signal(SIGQUIT,SIG_DFL);
   signal(SIGINT,SIG_DFL);   

   // Check for an error code.
   if (WIFEXITED(Status) == 0 || WEXITSTATUS(Status) != 0)
   {
      unsigned int Count = WEXITSTATUS(Status);
      if (Count > 100)
      {
	 Count -= 100;
	 for (; Opts != 0 && Count != 1; Opts = Opts->Next, Count--);
	 _error->Error("Problem executing scripts %s '%s'",Cnf,Opts->Value.c_str());
      }
      
      return _error->Error("Sub-process returned an error code");
   }
   
   return true;
}
                                                                        /*}}}*/
// DPkgPM::SendV2Pkgs - Send version 2 package info			/*{{{*/
// ---------------------------------------------------------------------
/* This is part of the helper script communication interface, it sends
   very complete information down to the other end of the pipe.*/
bool pkgDPkgPM::SendV2Pkgs(FILE *F)
{
   fprintf(F,"VERSION 2\n");
   
   /* Write out all of the configuration directives by walking the 
      configuration tree */
   const Configuration::Item *Top = _config->Tree(0);
   for (; Top != 0;)
   {
      if (Top->Value.empty() == false)
      {
	 fprintf(F,"%s=%s\n",
		 QuoteString(Top->FullTag(),"=\"\n").c_str(),
		 QuoteString(Top->Value,"\n").c_str());
      }

      if (Top->Child != 0)
      {
	 Top = Top->Child;
	 continue;
      }
      
      while (Top != 0 && Top->Next == 0)
	 Top = Top->Parent;
      if (Top != 0)
	 Top = Top->Next;
   }   
   fprintf(F,"\n");
 
   // Write out the package actions in order.
   for (vector<Item>::iterator I = List.begin(); I != List.end(); I++)
   {
      pkgDepCache::StateCache &S = Cache[I->Pkg];
      
      fprintf(F,"%s ",I->Pkg.Name());
      // Current version
      if (I->Pkg->CurrentVer == 0)
	 fprintf(F,"- ");
      else
	 fprintf(F,"%s ",I->Pkg.CurrentVer().VerStr());
      
      // Show the compare operator
      // Target version
      if (S.InstallVer != 0)
      {
	 int Comp = 2;
	 if (I->Pkg->CurrentVer != 0)
	    Comp = S.InstVerIter(Cache).CompareVer(I->Pkg.CurrentVer());
	 if (Comp < 0)
	    fprintf(F,"> ");
	 if (Comp == 0)
	    fprintf(F,"= ");
	 if (Comp > 0)
	    fprintf(F,"< ");
	 fprintf(F,"%s ",S.InstVerIter(Cache).VerStr());
      }
      else
	 fprintf(F,"> - ");
      
      // Show the filename/operation
      if (I->Op == Item::Install)
      {
	 // No errors here..
	 if (I->File[0] != '/')
	    fprintf(F,"**ERROR**\n");
	 else
	    fprintf(F,"%s\n",I->File.c_str());
      }      
      if (I->Op == Item::Configure)
	 fprintf(F,"**CONFIGURE**\n");
      if (I->Op == Item::Remove ||
	  I->Op == Item::Purge)
	 fprintf(F,"**REMOVE**\n");
      
      if (ferror(F) != 0)
	 return false;
   }
   return true;
}
									/*}}}*/
// DPkgPM::RunScriptsWithPkgs - Run scripts with package names on stdin /*{{{*/
// ---------------------------------------------------------------------
/* This looks for a list of scripts to run from the configuration file
   each one is run and is fed on standard input a list of all .deb files
   that are due to be installed. */
bool pkgDPkgPM::RunScriptsWithPkgs(const char *Cnf)
{
   Configuration::Item const *Opts = _config->Tree(Cnf);
   if (Opts == 0 || Opts->Child == 0)
      return true;
   Opts = Opts->Child;
   
   unsigned int Count = 1;
   for (; Opts != 0; Opts = Opts->Next, Count++)
   {
      if (Opts->Value.empty() == true)
         continue;

      // Determine the protocol version
      string OptSec = Opts->Value;
      string::size_type Pos;
      if ((Pos = OptSec.find(' ')) == string::npos || Pos == 0)
	 Pos = OptSec.length();
      OptSec = "DPkg::Tools::Options::" + string(Opts->Value.c_str(),Pos);
      
      unsigned int Version = _config->FindI(OptSec+"::Version",1);
      
      // Create the pipes
      int Pipes[2];
      if (pipe(Pipes) != 0)
	 return _error->Errno("pipe","Failed to create IPC pipe to subprocess");
      SetCloseExec(Pipes[0],true);
      SetCloseExec(Pipes[1],true);
      
      // Purified Fork for running the script
      pid_t Process = ExecFork();      
      if (Process == 0)
      {
	 // Setup the FDs
	 dup2(Pipes[0],STDIN_FILENO);
	 SetCloseExec(STDOUT_FILENO,false);
	 SetCloseExec(STDIN_FILENO,false);      
	 SetCloseExec(STDERR_FILENO,false);

	 const char *Args[4];
	 Args[0] = "/bin/sh";
	 Args[1] = "-c";
	 Args[2] = Opts->Value.c_str();
	 Args[3] = 0;
	 execv(Args[0],(char **)Args);
	 _exit(100);
      }
      close(Pipes[0]);
      FILE *F = fdopen(Pipes[1],"w");
      if (F == 0)
	 return _error->Errno("fdopen","Faild to open new FD");
      
      // Feed it the filenames.
      bool Die = false;
      if (Version <= 1)
      {
	 for (vector<Item>::iterator I = List.begin(); I != List.end(); I++)
	 {
	    // Only deal with packages to be installed from .deb
	    if (I->Op != Item::Install)
	       continue;

	    // No errors here..
	    if (I->File[0] != '/')
	       continue;
	    
	    /* Feed the filename of each package that is pending install
	       into the pipe. */
	    fprintf(F,"%s\n",I->File.c_str());
	    if (ferror(F) != 0)
	    {
	       Die = true;
	       break;
	    }
	 }
      }
      else
	 Die = !SendV2Pkgs(F);

      fclose(F);
      
      // Clean up the sub process
      if (ExecWait(Process,Opts->Value.c_str()) == false)
	 return _error->Error("Failure running script %s",Opts->Value.c_str());
   }

   return true;
}
									/*}}}*/
// DPkgPM::Go - Run the sequence					/*{{{*/
// ---------------------------------------------------------------------
/* This globs the operations and calls dpkg 
 *   
 * If it is called with "OutStatusFd" set to a valid file descriptor
 * apt will report the install progress over this fd. It maps the
 * dpkg states a package goes through to human readable (and i10n-able)
 * names and calculates a percentage for each step.
*/
bool pkgDPkgPM::Go(int OutStatusFd)
{
   unsigned int MaxArgs = _config->FindI("Dpkg::MaxArgs",8*1024);   
   unsigned int MaxArgBytes = _config->FindI("Dpkg::MaxArgBytes",32*1024);

   if (RunScripts("DPkg::Pre-Invoke") == false)
      return false;

   if (RunScriptsWithPkgs("DPkg::Pre-Install-Pkgs") == false)
      return false;
   
   // prepare the progress reporting 
   int Done = 0;
   int Total = 0;
   // map the dpkg states to the operations that are performed
   // (this is sorted in the same way as Item::Ops)
   static const struct DpkgState DpkgStatesOpMap[][5] = {
      // Install operation
      { 
	 {"half-installed", _("Preparing %s")}, 
	 {"unpacked", _("Unpacking %s") }, 
	 {NULL, NULL}
      },
      // Configure operation
      { 
	 {"unpacked",_("Preparing to configure %s") },
	 {"half-configured", _("Configuring %s") },
	 { "installed", _("Installed %s")},
	 {NULL, NULL}
      },
      // Remove operation
      { 
	 {"half-configured", _("Preparing for removal of %s")},
	 {"half-installed", _("Removing %s")},
	 {"config-files",  _("Removed %s")},
	 {NULL, NULL}
      },
      // Purge operation
      { 
	 {"config-files", _("Preparing for remove with config %s")},
	 {"not-installed", _("Removed with config %s")},
	 {NULL, NULL}
      },
   };

   // the dpkg states that the pkg will run through, the string is 
   // the package, the vector contains the dpkg states that the package
   // will go through
   map<string,vector<struct DpkgState> > PackageOps;
   // the dpkg states that are already done; the string is the package
   // the int is the state that is already done (e.g. a package that is
   // going to be install is already in state "half-installed")
   map<string,int> PackageOpsDone;

   // init the PackageOps map, go over the list of packages that
   // that will be [installed|configured|removed|purged] and add
   // them to the PackageOps map (the dpkg states it goes through)
   // and the PackageOpsTranslations (human readable strings)
   for (vector<Item>::iterator I = List.begin(); I != List.end();I++)
   {
      string name = (*I).Pkg.Name();
      PackageOpsDone[name] = 0;
      for(int i=0; (DpkgStatesOpMap[(*I).Op][i]).state != NULL;  i++) 
      {
	 PackageOps[name].push_back(DpkgStatesOpMap[(*I).Op][i]);
	 Total++;
      }
   }   

   // this loop is runs once per operation
   for (vector<Item>::iterator I = List.begin(); I != List.end();)
   {
      vector<Item>::iterator J = I;
      for (; J != List.end() && J->Op == I->Op; J++);

      // Generate the argument list
      const char *Args[MaxArgs + 50];
      if (J - I > (signed)MaxArgs)
	 J = I + MaxArgs;
      
      unsigned int n = 0;
      unsigned long Size = 0;
      string Tmp = _config->Find("Dir::Bin::dpkg","dpkg");
      Args[n++] = Tmp.c_str();
      Size += strlen(Args[n-1]);
      
      // Stick in any custom dpkg options
      Configuration::Item const *Opts = _config->Tree("DPkg::Options");
      if (Opts != 0)
      {
	 Opts = Opts->Child;
	 for (; Opts != 0; Opts = Opts->Next)
	 {
	    if (Opts->Value.empty() == true)
	       continue;
	    Args[n++] = Opts->Value.c_str();
	    Size += Opts->Value.length();
	 }	 
      }
      
      char status_fd_buf[20];
      int fd[2];
      pipe(fd);
      
      Args[n++] = "--status-fd";
      Size += strlen(Args[n-1]);
      snprintf(status_fd_buf,sizeof(status_fd_buf),"%i", fd[1]);
      Args[n++] = status_fd_buf;
      Size += strlen(Args[n-1]);

      switch (I->Op)
      {
	 case Item::Remove:
	 Args[n++] = "--force-depends";
	 Size += strlen(Args[n-1]);
	 Args[n++] = "--force-remove-essential";
	 Size += strlen(Args[n-1]);
	 Args[n++] = "--remove";
	 Size += strlen(Args[n-1]);
	 break;
	 
	 case Item::Purge:
	 Args[n++] = "--force-depends";
	 Size += strlen(Args[n-1]);
	 Args[n++] = "--force-remove-essential";
	 Size += strlen(Args[n-1]);
	 Args[n++] = "--purge";
	 Size += strlen(Args[n-1]);
	 break;
	 
	 case Item::Configure:
	 Args[n++] = "--configure";
	 Size += strlen(Args[n-1]);
	 break;
	 
	 case Item::Install:
	 Args[n++] = "--unpack";
	 Size += strlen(Args[n-1]);
	 break;
      }
      
      // Write in the file or package names
      if (I->Op == Item::Install)
      {
	 for (;I != J && Size < MaxArgBytes; I++)
	 {
	    if (I->File[0] != '/')
	       return _error->Error("Internal Error, Pathname to install is not absolute '%s'",I->File.c_str());
	    Args[n++] = I->File.c_str();
	    Size += strlen(Args[n-1]);
	 }
      }      
      else
      {
	 for (;I != J && Size < MaxArgBytes; I++)
	 {
	    Args[n++] = I->Pkg.Name();
	    Size += strlen(Args[n-1]);
	 }	 
      }      
      Args[n] = 0;
      J = I;
      
      if (_config->FindB("Debug::pkgDPkgPM",false) == true)
      {
	 for (unsigned int k = 0; k != n; k++)
	    clog << Args[k] << ' ';
	 clog << endl;
	 continue;
      }
      
      cout << flush;
      clog << flush;
      cerr << flush;

      /* Mask off sig int/quit. We do this because dpkg also does when 
         it forks scripts. What happens is that when you hit ctrl-c it sends
	 it to all processes in the group. Since dpkg ignores the signal 
	 it doesn't die but we do! So we must also ignore it */
      sighandler_t old_SIGQUIT = signal(SIGQUIT,SIG_IGN);
      sighandler_t old_SIGINT = signal(SIGINT,SIG_IGN);
	
       // Fork dpkg
      pid_t Child;
      _config->Set("APT::Keep-Fds::",fd[1]);
      Child = ExecFork();
            
      // This is the child
      if (Child == 0)
      {
	 close(fd[0]); // close the read end of the pipe
	 
	 if (chdir(_config->FindDir("DPkg::Run-Directory","/").c_str()) != 0)
	    _exit(100);
	 
	 if (_config->FindB("DPkg::FlushSTDIN",true) == true && isatty(STDIN_FILENO))
	 {
	    int Flags,dummy;
	    if ((Flags = fcntl(STDIN_FILENO,F_GETFL,dummy)) < 0)
	       _exit(100);
	    
	    // Discard everything in stdin before forking dpkg
	    if (fcntl(STDIN_FILENO,F_SETFL,Flags | O_NONBLOCK) < 0)
	       _exit(100);
	    
	    while (read(STDIN_FILENO,&dummy,1) == 1);
	    
	    if (fcntl(STDIN_FILENO,F_SETFL,Flags & (~(long)O_NONBLOCK)) < 0)
	       _exit(100);
	 }
	 
	 /* No Job Control Stop Env is a magic dpkg var that prevents it
	    from using sigstop */
	 putenv("DPKG_NO_TSTP=yes");
	 execvp(Args[0],(char **)Args);
	 cerr << "Could not exec dpkg!" << endl;
	 _exit(100);
      }      

      // clear the Keep-Fd again
      _config->Clear("APT::Keep-Fds",fd[1]);

      // Wait for dpkg
      int Status = 0;

      // we read from dpkg here
      int _dpkgin = fd[0];
      fcntl(_dpkgin, F_SETFL, O_NONBLOCK);
      close(fd[1]);                        // close the write end of the pipe

      // the read buffers for the communication with dpkg
      char line[1024] = {0,};
      char buf[2] = {0,0};
      
      // the result of the waitpid call
      int res;

      while ((res=waitpid(Child,&Status, WNOHANG)) != Child) {
	 if(res < 0) {
	    // FIXME: move this to a function or something, looks ugly here
	    // error handling, waitpid returned -1
	    if (errno == EINTR)
	       continue;
	    RunScripts("DPkg::Post-Invoke");

	    // Restore sig int/quit
	    signal(SIGQUIT,old_SIGQUIT);
	    signal(SIGINT,old_SIGINT);
	    return _error->Errno("waitpid","Couldn't wait for subprocess");
	 }
	 
	 // read a single char, make sure that the read can't block 
	 // (otherwise we may leave zombies)
         int len = read(_dpkgin, buf, 1);

	 // nothing to read, wait a bit for more
	 if(len <= 0)
	 {
	    usleep(1000);
	    continue;
	 }
	 
	 // sanity check (should never happen)
	 if(strlen(line) >= sizeof(line)-10)
	 {
	    _error->Error("got a overlong line from dpkg: '%s'",line);
	    line[0]=0;
	 }
	 // append to line, check if we got a complete line
	 strcat(line, buf);
	 if(buf[0] != '\n')
	    continue;

	 if (_config->FindB("Debug::pkgDPkgProgressReporting",false) == true)
	    std::clog << "got from dpkg '" << line << "'" << std::endl;

	 // the status we output
	 ostringstream status;

	 /* dpkg sends strings like this:
	    'status:   <pkg>:  <pkg  qstate>'
	    errors look like this:
	    'status: /var/cache/apt/archives/krecipes_0.8.1-0ubuntu1_i386.deb : error : trying to overwrite `/usr/share/doc/kde/HTML/en/krecipes/krectip.png', which is also in package krecipes-data 
	    and conffile-prompt like this
	    'status: conffile-prompt: conffile : 'current-conffile' 'new-conffile' useredited distedited
	    
	 */
	 char* list[4];
	 TokSplitString(':', line, list, 5);
	 char *pkg = list[1];
	 char *action = _strstrip(list[2]);

	 if(strncmp(action,"error",strlen("error")) == 0)
	 {
	    status << "pmerror:" << list[1]
		   << ":"  << (Done/float(Total)*100.0) 
		   << ":" << list[3]
		   << endl;
	    if(OutStatusFd > 0)
	       write(OutStatusFd, status.str().c_str(), status.str().size());
	    line[0]=0;
	    if (_config->FindB("Debug::pkgDPkgProgressReporting",false) == true)
	       std::clog << "send: '" << status.str() << "'" << endl;
	    continue;
	 }
	 if(strncmp(action,"conffile",strlen("conffile")) == 0)
	 {
	    status << "pmconffile:" << list[1]
		   << ":"  << (Done/float(Total)*100.0) 
		   << ":" << list[3]
		   << endl;
	    if(OutStatusFd > 0)
	       write(OutStatusFd, status.str().c_str(), status.str().size());
	    line[0]=0;
	    if (_config->FindB("Debug::pkgDPkgProgressReporting",false) == true)
	       std::clog << "send: '" << status.str() << "'" << endl;
	    continue;
	 }

	 vector<struct DpkgState> &states = PackageOps[pkg];
	 const char *next_action = NULL;
	 if(PackageOpsDone[pkg] < states.size())
	    next_action = states[PackageOpsDone[pkg]].state;
	 // check if the package moved to the next dpkg state
	 if(next_action && (strcmp(action, next_action) == 0)) 
	 {
	    // only read the translation if there is actually a next
	    // action
	    const char *translation = states[PackageOpsDone[pkg]].str;
	    char s[200];
	    snprintf(s, sizeof(s), translation, pkg);

	    // we moved from one dpkg state to a new one, report that
	    PackageOpsDone[pkg]++;
	    Done++;
	    // build the status str
	    status << "pmstatus:" << pkg 
		   << ":"  << (Done/float(Total)*100.0) 
		   << ":" << s
		   << endl;
	    if(OutStatusFd > 0)
	       write(OutStatusFd, status.str().c_str(), status.str().size());
	    if (_config->FindB("Debug::pkgDPkgProgressReporting",false) == true)
	       std::clog << "send: '" << status.str() << "'" << endl;

	 }
	 if (_config->FindB("Debug::pkgDPkgProgressReporting",false) == true) 
	    std::clog << "(parsed from dpkg) pkg: " << pkg 
		      << " action: " << action << endl;

	 // reset the line buffer
	 line[0]=0;
      }
      close(_dpkgin);

      // Restore sig int/quit
      signal(SIGQUIT,old_SIGQUIT);
      signal(SIGINT,old_SIGINT);
       
      // Check for an error code.
      if (WIFEXITED(Status) == 0 || WEXITSTATUS(Status) != 0)
      {
	 RunScripts("DPkg::Post-Invoke");
	 if (WIFSIGNALED(Status) != 0 && WTERMSIG(Status) == SIGSEGV)
	    return _error->Error("Sub-process %s received a segmentation fault.",Args[0]);

	 if (WIFEXITED(Status) != 0)
	    return _error->Error("Sub-process %s returned an error code (%u)",Args[0],WEXITSTATUS(Status));
	 
	 return _error->Error("Sub-process %s exited unexpectedly",Args[0]);
      }      
   }

   if (RunScripts("DPkg::Post-Invoke") == false)
      return false;
   return true;
}
									/*}}}*/
// pkgDpkgPM::Reset - Dump the contents of the command list		/*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgDPkgPM::Reset() 
{
   List.erase(List.begin(),List.end());
}
									/*}}}*/
