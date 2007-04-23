// -*- mode: cpp; mode: fold -*-
// Description								/*{{{*/
// $Id: acquire-item.cc,v 1.46.2.9 2004/01/16 18:51:11 mdz Exp $
/* ######################################################################

   Acquire Item - Item to acquire

   Each item can download to exactly one file at a time. This means you
   cannot create an item that fetches two uri's to two files at the same 
   time. The pkgAcqIndex class creates a second class upon instantiation
   to fetch the other index files because of this.

   ##################################################################### */
									/*}}}*/
// Include Files							/*{{{*/
#ifdef __GNUG__
#pragma implementation "apt-pkg/acquire-item.h"
#endif
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/vendorlist.h>
#include <apt-pkg/error.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/md5.h>
#include <apt-pkg/sha1.h>
#include <apt-pkg/tagfile.h>

#include <apti18n.h>
    
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <sstream>
#include <stdio.h>
									/*}}}*/

using namespace std;

// Acquire::Item::Item - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgAcquire::Item::Item(pkgAcquire *Owner) : Owner(Owner), FileSize(0),
                       PartialSize(0), Mode(0), ID(0), Complete(false), 
                       Local(false), QueueCounter(0)
{
   Owner->Add(this);
   Status = StatIdle;
}
									/*}}}*/
// Acquire::Item::~Item - Destructor					/*{{{*/
// ---------------------------------------------------------------------
/* */
pkgAcquire::Item::~Item()
{
   Owner->Remove(this);
}
									/*}}}*/
// Acquire::Item::Failed - Item failed to download			/*{{{*/
// ---------------------------------------------------------------------
/* We return to an idle state if there are still other queues that could
   fetch this object */
void pkgAcquire::Item::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   Status = StatIdle;
   ErrorText = LookupTag(Message,"Message");
   if (QueueCounter <= 1)
   {
      /* This indicates that the file is not available right now but might
         be sometime later. If we do a retry cycle then this should be
	 retried [CDROMs] */
      if (Cnf->LocalOnly == true &&
	  StringToBool(LookupTag(Message,"Transient-Failure"),false) == true)
      {
	 Status = StatIdle;
	 Dequeue();
	 return;
      }
      
      Status = StatError;
      Dequeue();
   }   
}
									/*}}}*/
// Acquire::Item::Start - Item has begun to download			/*{{{*/
// ---------------------------------------------------------------------
/* Stash status and the file size. Note that setting Complete means 
   sub-phases of the acquire process such as decompresion are operating */
void pkgAcquire::Item::Start(string /*Message*/,unsigned long Size)
{
   Status = StatFetching;
   if (FileSize == 0 && Complete == false)
      FileSize = Size;
}
									/*}}}*/
// Acquire::Item::Done - Item downloaded OK				/*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgAcquire::Item::Done(string Message,unsigned long Size,string,
			    pkgAcquire::MethodConfig *Cnf)
{
   // We just downloaded something..
   string FileName = LookupTag(Message,"Filename");
   // we only inform the Log class if it was actually not a local thing
   if (Complete == false && !Local && FileName == DestFile)
   {
      if (Owner->Log != 0)
	 Owner->Log->Fetched(Size,atoi(LookupTag(Message,"Resume-Point","0").c_str()));
   }

   if (FileSize == 0)
      FileSize= Size;
   
   Status = StatDone;
   ErrorText = string();
   Owner->Dequeue(this);
}
									/*}}}*/
// Acquire::Item::Rename - Rename a file				/*{{{*/
// ---------------------------------------------------------------------
/* This helper function is used by alot of item methods as thier final
   step */
void pkgAcquire::Item::Rename(string From,string To)
{
   if (rename(From.c_str(),To.c_str()) != 0)
   {
      char S[300];
      snprintf(S,sizeof(S),_("rename failed, %s (%s -> %s)."),strerror(errno),
	      From.c_str(),To.c_str());
      Status = StatError;
      ErrorText = S;
   }   
}
									/*}}}*/



// AcqDiffIndex::AcqDiffIndex - Constructor			
// ---------------------------------------------------------------------
/* Get the DiffIndex file first and see if there are patches availabe 
 * If so, create a pkgAcqIndexDiffs fetcher that will get and apply the
 * patches. If anything goes wrong in that process, it will fall back to
 * the original packages file
 */
pkgAcqDiffIndex::pkgAcqDiffIndex(pkgAcquire *Owner,
				 string URI,string URIDesc,string ShortDesc,
				 string ExpectedMD5)
   : Item(Owner), RealURI(URI), ExpectedMD5(ExpectedMD5), Description(URIDesc)
{
   
   Debug = _config->FindB("Debug::pkgAcquire::Diffs",false);

   Desc.Description = URIDesc + "/DiffIndex";
   Desc.Owner = this;
   Desc.ShortDesc = ShortDesc;
   Desc.URI = URI + ".diff/Index";

   DestFile = _config->FindDir("Dir::State::lists") + "partial/";
   DestFile += URItoFileName(URI) + string(".DiffIndex");

   if(Debug)
      std::clog << "pkgAcqDiffIndex: " << Desc.URI << std::endl;

   // look for the current package file
   CurrentPackagesFile = _config->FindDir("Dir::State::lists");
   CurrentPackagesFile += URItoFileName(RealURI);

   // FIXME: this file:/ check is a hack to prevent fetching
   //        from local sources. this is really silly, and
   //        should be fixed cleanly as soon as possible
   if(!FileExists(CurrentPackagesFile) || 
      Desc.URI.substr(0,strlen("file:/")) == "file:/")
   {
      // we don't have a pkg file or we don't want to queue
      if(Debug)
	 std::clog << "No index file, local or canceld by user" << std::endl;
      Failed("", NULL);
      return;
   }

   if(Debug) 
      std::clog << "pkgAcqIndexDiffs::pkgAcqIndexDiffs(): " 
		<< CurrentPackagesFile << std::endl;
   
   QueueURI(Desc);

}

// AcqIndex::Custom600Headers - Insert custom request headers		/*{{{*/
// ---------------------------------------------------------------------
/* The only header we use is the last-modified header. */
string pkgAcqDiffIndex::Custom600Headers()
{
   string Final = _config->FindDir("Dir::State::lists");
   Final += URItoFileName(RealURI) + string(".IndexDiff");
   
   if(Debug)
      std::clog << "Custom600Header-IMS: " << Final << std::endl;

   struct stat Buf;
   if (stat(Final.c_str(),&Buf) != 0)
      return "\nIndex-File: true";
   
   return "\nIndex-File: true\nLast-Modified: " + TimeRFC1123(Buf.st_mtime);
}


bool pkgAcqDiffIndex::ParseDiffIndex(string IndexDiffFile)
{
   if(Debug)
      std::clog << "pkgAcqIndexDiffs::ParseIndexDiff() " << IndexDiffFile 
		<< std::endl;

   pkgTagSection Tags;
   string ServerSha1;
   vector<DiffInfo> available_patches;
   
   FileFd Fd(IndexDiffFile,FileFd::ReadOnly);
   pkgTagFile TF(&Fd);
   if (_error->PendingError() == true)
      return false;

   if(TF.Step(Tags) == true)
   {
      string local_sha1;
      bool found = false;
      DiffInfo d;
      string size;

      string tmp = Tags.FindS("SHA1-Current");
      std::stringstream ss(tmp);
      ss >> ServerSha1;

      FileFd fd(CurrentPackagesFile, FileFd::ReadOnly);
      SHA1Summation SHA1;
      SHA1.AddFD(fd.Fd(), fd.Size());
      local_sha1 = string(SHA1.Result());

      if(local_sha1 == ServerSha1) 
      {
	 // we have the same sha1 as the server
	 if(Debug)
	    std::clog << "Package file is up-to-date" << std::endl;
	 // set found to true, this will queue a pkgAcqIndexDiffs with
	 // a empty availabe_patches
	 found = true;
      } 
      else 
      {
	 if(Debug)
	    std::clog << "SHA1-Current: " << ServerSha1 << std::endl;

	 // check the historie and see what patches we need
	 string history = Tags.FindS("SHA1-History");     
	 std::stringstream hist(history);
	 while(hist >> d.sha1 >> size >> d.file) 
	 {
	    d.size = atoi(size.c_str());
	    // read until the first match is found
	    if(d.sha1 == local_sha1) 
	       found=true;
	    // from that point on, we probably need all diffs
	    if(found) 
	    {
	       if(Debug)
		  std::clog << "Need to get diff: " << d.file << std::endl;
	       available_patches.push_back(d);
	    }
	 }
      }

      // we have something, queue the next diff
      if(found) 
      {
	 // queue the diffs
	int last_space = Description.rfind(" ");
	if(last_space != string::npos)
	  Description.erase(last_space, Description.size()-last_space);
	 new pkgAcqIndexDiffs(Owner, RealURI, Description, Desc.ShortDesc,
			      ExpectedMD5, available_patches);
	 Complete = false;
	 Status = StatDone;
	 Dequeue();
	 return true;
      }
   }

   // Nothing found, report and return false
   // Failing here is ok, if we return false later, the full
   // IndexFile is queued
   if(Debug)
      std::clog << "Can't find a patch in the index file" << std::endl;
   return false;
}

void pkgAcqDiffIndex::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   if(Debug)
      std::clog << "pkgAcqDiffIndex failed: " << Desc.URI << std::endl
		<< "Falling back to normal index file aquire" << std::endl;

   new pkgAcqIndex(Owner, RealURI, Description, Desc.ShortDesc, 
		   ExpectedMD5);

   Complete = false;
   Status = StatDone;
   Dequeue();
}

void pkgAcqDiffIndex::Done(string Message,unsigned long Size,string Md5Hash,
			   pkgAcquire::MethodConfig *Cnf)
{
   if(Debug)
      std::clog << "pkgAcqDiffIndex::Done(): " << Desc.URI << std::endl;

   Item::Done(Message,Size,Md5Hash,Cnf);

   string FinalFile;
   FinalFile = _config->FindDir("Dir::State::lists")+URItoFileName(RealURI);

   // sucess in downloading the index
   // rename the index
   FinalFile += string(".IndexDiff");
   if(Debug)
      std::clog << "Renaming: " << DestFile << " -> " << FinalFile 
		<< std::endl;
   Rename(DestFile,FinalFile);
   chmod(FinalFile.c_str(),0644);
   DestFile = FinalFile;

   if(!ParseDiffIndex(DestFile))
      return Failed("", NULL);

   Complete = true;
   Status = StatDone;
   Dequeue();
   return;
}



// AcqIndexDiffs::AcqIndexDiffs - Constructor			
// ---------------------------------------------------------------------
/* The package diff is added to the queue. one object is constructed
 * for each diff and the index
 */
pkgAcqIndexDiffs::pkgAcqIndexDiffs(pkgAcquire *Owner,
				   string URI,string URIDesc,string ShortDesc,
				   string ExpectedMD5, vector<DiffInfo> diffs)
   : Item(Owner), RealURI(URI), ExpectedMD5(ExpectedMD5), 
     available_patches(diffs)
{
   
   DestFile = _config->FindDir("Dir::State::lists") + "partial/";
   DestFile += URItoFileName(URI);

   Debug = _config->FindB("Debug::pkgAcquire::Diffs",false);

   Description = URIDesc;
   Desc.Owner = this;
   Desc.ShortDesc = ShortDesc;

   if(available_patches.size() == 0) 
   {
      // we are done (yeah!)
      Finish(true);
   }
   else
   {
      // get the next diff
      State = StateFetchDiff;
      QueueNextDiff();
   }
}


void pkgAcqIndexDiffs::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   if(Debug)
      std::clog << "pkgAcqIndexDiffs failed: " << Desc.URI << std::endl
		<< "Falling back to normal index file aquire" << std::endl;
   new pkgAcqIndex(Owner, RealURI, Description,Desc.ShortDesc, 
		   ExpectedMD5);
   Finish();
}


// helper that cleans the item out of the fetcher queue
void pkgAcqIndexDiffs::Finish(bool allDone)
{
   // we restore the original name, this is required, otherwise
   // the file will be cleaned
   if(allDone) 
   {
      DestFile = _config->FindDir("Dir::State::lists");
      DestFile += URItoFileName(RealURI);

      // do the final md5sum checking
      MD5Summation sum;
      FileFd Fd(DestFile, FileFd::ReadOnly);
      sum.AddFD(Fd.Fd(), Fd.Size());
      Fd.Close();
      string MD5 = (string)sum.Result();

      if (!ExpectedMD5.empty() && MD5 != ExpectedMD5)
      {
	 Status = StatAuthError;
	 ErrorText = _("MD5Sum mismatch");
	 Rename(DestFile,DestFile + ".FAILED");
	 Dequeue();
	 return;
      }

      // this is for the "real" finish
      Complete = true;
      Status = StatDone;
      Dequeue();
      if(Debug)
	 std::clog << "\n\nallDone: " << DestFile << "\n" << std::endl;
      return;
   }

   if(Debug)
      std::clog << "Finishing: " << Desc.URI << std::endl;
   Complete = false;
   Status = StatDone;
   Dequeue();
   return;
}



bool pkgAcqIndexDiffs::QueueNextDiff()
{

   // calc sha1 of the just patched file
   string FinalFile = _config->FindDir("Dir::State::lists");
   FinalFile += URItoFileName(RealURI);

   FileFd fd(FinalFile, FileFd::ReadOnly);
   SHA1Summation SHA1;
   SHA1.AddFD(fd.Fd(), fd.Size());
   string local_sha1 = string(SHA1.Result());
   if(Debug)
      std::clog << "QueueNextDiff: " 
		<< FinalFile << " (" << local_sha1 << ")"<<std::endl;

   // remove all patches until the next matching patch is found
   // this requires the Index file to be ordered
   for(vector<DiffInfo>::iterator I=available_patches.begin();
       available_patches.size() > 0 && 
	  I != available_patches.end() &&
	  (*I).sha1 != local_sha1; 
       I++) 
   {
      available_patches.erase(I);
   }

   // error checking and falling back if no patch was found
   if(available_patches.size() == 0) 
   { 
      Failed("", NULL);
      return false;
   }

   // queue the right diff
   Desc.URI = string(RealURI) + ".diff/" + available_patches[0].file + ".gz";
   Desc.Description = Description + " " + available_patches[0].file + string(".pdiff");

   DestFile = _config->FindDir("Dir::State::lists") + "partial/";
   DestFile += URItoFileName(RealURI + ".diff/" + available_patches[0].file);

   if(Debug)
      std::clog << "pkgAcqIndexDiffs::QueueNextDiff(): " << Desc.URI << std::endl;
   
   QueueURI(Desc);

   return true;
}



void pkgAcqIndexDiffs::Done(string Message,unsigned long Size,string Md5Hash,
			    pkgAcquire::MethodConfig *Cnf)
{
   if(Debug)
      std::clog << "pkgAcqIndexDiffs::Done(): " << Desc.URI << std::endl;

   Item::Done(Message,Size,Md5Hash,Cnf);

   string FinalFile;
   FinalFile = _config->FindDir("Dir::State::lists")+URItoFileName(RealURI);

   // sucess in downloading a diff, enter ApplyDiff state
   if(State == StateFetchDiff) 
   {

      if(Debug)
	 std::clog << "Sending to gzip method: " << FinalFile << std::endl;

      string FileName = LookupTag(Message,"Filename");
      State = StateUnzipDiff;
      Local = true;
      Desc.URI = "gzip:" + FileName;
      DestFile += ".decomp";
      QueueURI(Desc);
      Mode = "gzip";
      return;
   } 

   // sucess in downloading a diff, enter ApplyDiff state
   if(State == StateUnzipDiff) 
   {

      // rred excepts the patch as $FinalFile.ed
      Rename(DestFile,FinalFile+".ed");

      if(Debug)
	 std::clog << "Sending to rred method: " << FinalFile << std::endl;

      State = StateApplyDiff;
      Local = true;
      Desc.URI = "rred:" + FinalFile;
      QueueURI(Desc);
      Mode = "rred";
      return;
   } 


   // success in download/apply a diff, queue next (if needed)
   if(State == StateApplyDiff)
   {
      // remove the just applied patch
      available_patches.erase(available_patches.begin());

      // move into place
      if(Debug) 
      {
	 std::clog << "Moving patched file in place: " << std::endl
		   << DestFile << " -> " << FinalFile << std::endl;
      }
      Rename(DestFile,FinalFile);
      chmod(FinalFile.c_str(),0644);

      // see if there is more to download
      if(available_patches.size() > 0) {
	 new pkgAcqIndexDiffs(Owner, RealURI, Description, Desc.ShortDesc,
			      ExpectedMD5, available_patches);
	 return Finish();
      } else 
	 return Finish(true);
   }
}


// AcqIndex::AcqIndex - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* The package file is added to the queue and a second class is 
   instantiated to fetch the revision file */   
pkgAcqIndex::pkgAcqIndex(pkgAcquire *Owner,
			 string URI,string URIDesc,string ShortDesc,
			 string ExpectedMD5, string comprExt)
   : Item(Owner), RealURI(URI), ExpectedMD5(ExpectedMD5)
{
   Decompression = false;
   Erase = false;

   DestFile = _config->FindDir("Dir::State::lists") + "partial/";
   DestFile += URItoFileName(URI);

   if(comprExt.empty()) 
   {
      // autoselect the compression method
      if(FileExists("/bin/bzip2")) 
	 CompressionExtension = ".bz2";
      else 
	 CompressionExtension = ".gz";
   } else {
      CompressionExtension = comprExt;
   }
   Desc.URI = URI + CompressionExtension; 

   Desc.Description = URIDesc;
   Desc.Owner = this;
   Desc.ShortDesc = ShortDesc;
      
   QueueURI(Desc);
}
									/*}}}*/
// AcqIndex::Custom600Headers - Insert custom request headers		/*{{{*/
// ---------------------------------------------------------------------
/* The only header we use is the last-modified header. */
string pkgAcqIndex::Custom600Headers()
{
   string Final = _config->FindDir("Dir::State::lists");
   Final += URItoFileName(RealURI);
   
   struct stat Buf;
   if (stat(Final.c_str(),&Buf) != 0)
      return "\nIndex-File: true";
   
   return "\nIndex-File: true\nLast-Modified: " + TimeRFC1123(Buf.st_mtime);
}
									/*}}}*/

void pkgAcqIndex::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   // no .bz2 found, retry with .gz
   if(Desc.URI.substr(Desc.URI.size()-3) == "bz2") {
      Desc.URI = Desc.URI.substr(0,Desc.URI.size()-3) + "gz"; 

      // retry with a gzip one 
      new pkgAcqIndex(Owner, RealURI, Desc.Description,Desc.ShortDesc, 
		      ExpectedMD5, string(".gz"));
      Status = StatDone;
      Complete = false;
      Dequeue();
      return;
   }

   
   Item::Failed(Message,Cnf);
}


// AcqIndex::Done - Finished a fetch					/*{{{*/
// ---------------------------------------------------------------------
/* This goes through a number of states.. On the initial fetch the
   method could possibly return an alternate filename which points
   to the uncompressed version of the file. If this is so the file
   is copied into the partial directory. In all other cases the file
   is decompressed with a gzip uri. */
void pkgAcqIndex::Done(string Message,unsigned long Size,string MD5,
		       pkgAcquire::MethodConfig *Cfg)
{
   Item::Done(Message,Size,MD5,Cfg);

   if (Decompression == true)
   {
      if (_config->FindB("Debug::pkgAcquire::Auth", false))
      {
         std::cerr << std::endl << RealURI << ": Computed MD5: " << MD5;
         std::cerr << "  Expected MD5: " << ExpectedMD5 << std::endl;
      }

      if (MD5.empty())
      {
         MD5Summation sum;
         FileFd Fd(DestFile, FileFd::ReadOnly);
         sum.AddFD(Fd.Fd(), Fd.Size());
         Fd.Close();
         MD5 = (string)sum.Result();
      }

      if (!ExpectedMD5.empty() && MD5 != ExpectedMD5)
      {
         Status = StatAuthError;
         ErrorText = _("MD5Sum mismatch");
         Rename(DestFile,DestFile + ".FAILED");
         return;
      }
      // Done, move it into position
      string FinalFile = _config->FindDir("Dir::State::lists");
      FinalFile += URItoFileName(RealURI);
      Rename(DestFile,FinalFile);
      chmod(FinalFile.c_str(),0644);
      
      /* We restore the original name to DestFile so that the clean operation
         will work OK */
      DestFile = _config->FindDir("Dir::State::lists") + "partial/";
      DestFile += URItoFileName(RealURI);
      
      // Remove the compressed version.
      if (Erase == true)
	 unlink(DestFile.c_str());
      return;
   }

   Erase = false;
   Complete = true;
   
   // Handle the unzipd case
   string FileName = LookupTag(Message,"Alt-Filename");
   if (FileName.empty() == false)
   {
      // The files timestamp matches
      if (StringToBool(LookupTag(Message,"Alt-IMS-Hit"),false) == true)
	 return;

      Decompression = true;
      Local = true;
      DestFile += ".decomp";
      Desc.URI = "copy:" + FileName;
      QueueURI(Desc);
      Mode = "copy";
      return;
   }

   FileName = LookupTag(Message,"Filename");
   if (FileName.empty() == true)
   {
      Status = StatError;
      ErrorText = "Method gave a blank filename";
   }
   
   // The files timestamp matches
   if (StringToBool(LookupTag(Message,"IMS-Hit"),false) == true)
      return;

   if (FileName == DestFile)
      Erase = true;
   else
      Local = true;
   
   string compExt = Desc.URI.substr(Desc.URI.size()-3);
   char *decompProg;
   if(compExt == "bz2") 
      decompProg = "bzip2";
   else if(compExt == ".gz") 
      decompProg = "gzip";
   else {
      _error->Error("Unsupported extension: %s", compExt.c_str());
      return;
   }

   Decompression = true;
   DestFile += ".decomp";
   Desc.URI = string(decompProg) + ":" + FileName;
   QueueURI(Desc);
   Mode = decompProg;
}

pkgAcqMetaSig::pkgAcqMetaSig(pkgAcquire *Owner,
			     string URI,string URIDesc,string ShortDesc,
			     string MetaIndexURI, string MetaIndexURIDesc,
			     string MetaIndexShortDesc,
			     const vector<IndexTarget*>* IndexTargets,
			     indexRecords* MetaIndexParser) :
   Item(Owner), RealURI(URI), MetaIndexURI(MetaIndexURI),
   MetaIndexURIDesc(MetaIndexURIDesc), MetaIndexShortDesc(MetaIndexShortDesc),
   MetaIndexParser(MetaIndexParser), IndexTargets(IndexTargets)
{
   DestFile = _config->FindDir("Dir::State::lists") + "partial/";
   DestFile += URItoFileName(URI);

   // remove any partial downloaded sig-file. it may confuse proxies
   // and is too small to warrant a partial download anyway
   unlink(DestFile.c_str());

   // Create the item
   Desc.Description = URIDesc;
   Desc.Owner = this;
   Desc.ShortDesc = ShortDesc;
   Desc.URI = URI;
   
   QueueURI(Desc);
}
									/*}}}*/
// pkgAcqMetaSig::Custom600Headers - Insert custom request headers	/*{{{*/
// ---------------------------------------------------------------------
/* The only header we use is the last-modified header. */
string pkgAcqMetaSig::Custom600Headers()
{
   string Final = _config->FindDir("Dir::State::lists");
   Final += URItoFileName(RealURI);
   struct stat Buf;
   if (stat(Final.c_str(),&Buf) != 0)
      return "\nIndex-File: true";

   return "\nIndex-File: true\nLast-Modified: " + TimeRFC1123(Buf.st_mtime);
}

void pkgAcqMetaSig::Done(string Message,unsigned long Size,string MD5,
			 pkgAcquire::MethodConfig *Cfg)
{
   Item::Done(Message,Size,MD5,Cfg);

   string FileName = LookupTag(Message,"Filename");
   if (FileName.empty() == true)
   {
      Status = StatError;
      ErrorText = "Method gave a blank filename";
      return;
   }

   if (FileName != DestFile)
   {
      // We have to copy it into place
      Local = true;
      Desc.URI = "copy:" + FileName;
      QueueURI(Desc);
      return;
   }

   Complete = true;

   string Final = _config->FindDir("Dir::State::lists");
   Final += URItoFileName(RealURI);
   if (StringToBool(LookupTag(Message,"IMS-Hit"),false))
     {
       // Move it into position
       Rename (Final, DestFile);
     }
   else
     {
       // Delete the old version in lists/.  The new version will be moved
       // there from partial/ when the signature verification succeeds
       //
       unlink (Final.c_str ());
     }

   // queue a pkgAcqMetaIndex to be verified against the sig we just retrieved
   new pkgAcqMetaIndex(Owner, MetaIndexURI, MetaIndexURIDesc, MetaIndexShortDesc,
		       DestFile, IndexTargets, MetaIndexParser);

}
									/*}}}*/
void pkgAcqMetaSig::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   string Final =
     _config->FindDir("Dir::State::lists") + URItoFileName(RealURI);

   // If this is a transient failure, we use the old version, if we
   // have one.  "Using it" means moving it into the partial/
   // directory for further consumption by gpgv.
   //
   if (StringToBool(LookupTag(Message,"Transient-Failure"),false))
     {
       struct stat Buf;
       if (stat(Final.c_str(),&Buf) == 0)
	 {
	   Rename (Final, DestFile); 
	   new pkgAcqMetaIndex(Owner, MetaIndexURI, MetaIndexURIDesc,
			       MetaIndexShortDesc,
			       DestFile, IndexTargets, MetaIndexParser);
	 }
       else
	 {
	   // queue a pkgAcqMetaIndex with no sigfile
	   new pkgAcqMetaIndex(Owner, MetaIndexURI, MetaIndexURIDesc,
			       MetaIndexShortDesc,
			       "", IndexTargets, MetaIndexParser);
	 }
     }
   else
     {
       // Delete any existing sigfile, so that this source isn't
       // mistakenly trusted
       unlink(Final.c_str());

       // if we get a network error we fail gracefully
       if(LookupTag(Message,"FailReason") == "Timeout" || 
	  LookupTag(Message,"FailReason") == "TmpResolveFailure" ||
	  LookupTag(Message,"FailReason") == "ConnectionRefused") {
	 Item::Failed(Message,Cnf);
	 return;
       }

       // queue a pkgAcqMetaIndex with no sigfile
       new pkgAcqMetaIndex(Owner, MetaIndexURI, MetaIndexURIDesc, MetaIndexShortDesc,
			   "", IndexTargets, MetaIndexParser);
     }

   if (Cnf->LocalOnly == true || 
       StringToBool(LookupTag(Message,"Transient-Failure"),false) == false)
   {      
      // Ignore this
      Status = StatDone;
      Complete = false;
      Dequeue();
      return;
   }
   
   Item::Failed(Message,Cnf);
}

pkgAcqMetaIndex::pkgAcqMetaIndex(pkgAcquire *Owner,
				 string URI,string URIDesc,string ShortDesc,
				 string SigFile,
				 const vector<struct IndexTarget*>* IndexTargets,
				 indexRecords* MetaIndexParser) :
   Item(Owner), RealURI(URI), SigFile(SigFile), AuthPass(false),
   MetaIndexParser(MetaIndexParser), IndexTargets(IndexTargets), IMSHit(false)
{
   DestFile = _config->FindDir("Dir::State::lists") + "partial/";
   DestFile += URItoFileName(URI);

   // Create the item
   Desc.Description = URIDesc;
   Desc.Owner = this;
   Desc.ShortDesc = ShortDesc;
   Desc.URI = URI;

   QueueURI(Desc);
}

									/*}}}*/
// pkgAcqMetaIndex::Custom600Headers - Insert custom request headers	/*{{{*/
// ---------------------------------------------------------------------
/* The only header we use is the last-modified header. */
string pkgAcqMetaIndex::Custom600Headers()
{
   string Final = _config->FindDir("Dir::State::lists");
   Final += URItoFileName(RealURI);
   
   struct stat Buf;
   if (stat(Final.c_str(),&Buf) != 0)
      return "\nIndex-File: true";
   
   return "\nIndex-File: true\nLast-Modified: " + TimeRFC1123(Buf.st_mtime);
}

void pkgAcqMetaIndex::Done(string Message,unsigned long Size,string MD5,
			   pkgAcquire::MethodConfig *Cfg)
{
   Item::Done(Message,Size,MD5,Cfg);

   // MetaIndexes are done in two passes: one to download the
   // metaindex with an appropriate method, and a second to verify it
   // with the gpgv method

   if (AuthPass == true)
   {
      AuthDone(Message);
   }
   else
   {
      RetrievalDone(Message);
      if (!Complete)
         // Still more retrieving to do
         return;

      if (SigFile == "")
      {
         // There was no signature file, so we are finished.  Download
         // the indexes without verification.
         QueueIndexes(false);
      }
      else
      {
         // There was a signature file, so pass it to gpgv for
         // verification

         if (_config->FindB("Debug::pkgAcquire::Auth", false))
            std::cerr << "Metaindex acquired, queueing gpg verification ("
                      << SigFile << "," << DestFile << ")\n";
         AuthPass = true;
         Desc.URI = "gpgv:" + SigFile;
         QueueURI(Desc);
         Mode = "gpgv";
      }
   }
}

void pkgAcqMetaIndex::RetrievalDone(string Message)
{
   // We have just finished downloading a Release file (it is not
   // verified yet)

   string FileName = LookupTag(Message,"Filename");
   if (FileName.empty() == true)
   {
      Status = StatError;
      ErrorText = "Method gave a blank filename";
      return;
   }

   if (FileName != DestFile)
   {
      Local = true;
      Desc.URI = "copy:" + FileName;
      QueueURI(Desc);
      return;
   }

   // see if the download was a IMSHit
   IMSHit = StringToBool(LookupTag(Message,"IMS-Hit"),false);

   Complete = true;

   string FinalFile = _config->FindDir("Dir::State::lists");
   FinalFile += URItoFileName(RealURI);

   // The files timestamp matches
   if (StringToBool(LookupTag(Message,"IMS-Hit"),false) == false)
   {
      // Move it into position
      Rename(DestFile,FinalFile);
   }
   chmod(FinalFile.c_str(),0644);
   DestFile = FinalFile;
}

void pkgAcqMetaIndex::AuthDone(string Message)
{
   // At this point, the gpgv method has succeeded, so there is a
   // valid signature from a key in the trusted keyring.  We
   // perform additional verification of its contents, and use them
   // to verify the indexes we are about to download

   if (!MetaIndexParser->Load(DestFile))
   {
      Status = StatAuthError;
      ErrorText = MetaIndexParser->ErrorText;
      return;
   }

   if (!VerifyVendor(Message))
   {
      return;
   }

   if (_config->FindB("Debug::pkgAcquire::Auth", false))
      std::cerr << "Signature verification succeeded: "
                << DestFile << std::endl;

   // Download further indexes with verification
   QueueIndexes(true);

   // Done, move signature file into position

   string VerifiedSigFile = _config->FindDir("Dir::State::lists") +
      URItoFileName(RealURI) + ".gpg";
   Rename(SigFile,VerifiedSigFile);
   chmod(VerifiedSigFile.c_str(),0644);

   // Record what gpgv had to say about it

   string SigInfoFile = _config->FindDir("Dir::State::lists") +
      URItoFileName(RealURI) + ".gpg.info";
   string GPGVOutput = LookupTag (Message,"GPGVOutput");
   FILE *f = fopen (SigInfoFile.c_str(), "w");
   if (f)
     {
       fputs (GPGVOutput.c_str(), f);
       fputc ('\n', f);
       fclose (f);
     }
   else
     std::cerr << "Can't write info file: "
	       << SigInfoFile << ": "
	       << strerror (errno) << "\n";
}

bool pkgAcqMetaIndex::IsMyFile (string file)
{
  return (Item::IsYourFile (file)
	  || file == flNotDir (DestFile) + ".gpg.info");
}

void pkgAcqMetaIndex::QueueIndexes(bool verify)
{
   for (vector <struct IndexTarget*>::const_iterator Target = IndexTargets->begin();
        Target != IndexTargets->end();
        Target++)
   {
      string ExpectedIndexMD5;
      if (verify)
      {
         const indexRecords::checkSum *Record = MetaIndexParser->Lookup((*Target)->MetaKey);
         if (!Record)
         {
            Status = StatAuthError;
            ErrorText = "Unable to find expected entry  "
               + (*Target)->MetaKey + " in Meta-index file (malformed Release file?)";
            return;
         }
         ExpectedIndexMD5 = Record->MD5Hash;
         if (_config->FindB("Debug::pkgAcquire::Auth", false))
         {
            std::cerr << "Queueing: " << (*Target)->URI << std::endl;
            std::cerr << "Expected MD5: " << ExpectedIndexMD5 << std::endl;
         }
         if (ExpectedIndexMD5.empty())
         {
            Status = StatAuthError;
            ErrorText = "Unable to find MD5 sum for "
               + (*Target)->MetaKey + " in Meta-index file";
            return;
         }
      }
      
      // Queue Packages file (either diff or full packages files, depending
      // on the users option)
      if(_config->FindB("Acquire::PDiffs",true) == true) 
	 new pkgAcqDiffIndex(Owner, (*Target)->URI, (*Target)->Description,
			     (*Target)->ShortDesc, ExpectedIndexMD5);
      else 
	 new pkgAcqIndex(Owner, (*Target)->URI, (*Target)->Description,
			    (*Target)->ShortDesc, ExpectedIndexMD5);
   }
}

bool pkgAcqMetaIndex::VerifyVendor(string Message)
{
//    // Maybe this should be made available from above so we don't have
//    // to read and parse it every time?
//    pkgVendorList List;
//    List.ReadMainList();

//    const Vendor* Vndr = NULL;
//    for (std::vector<string>::const_iterator I = GPGVOutput.begin(); I != GPGVOutput.end(); I++)
//    {
//       string::size_type pos = (*I).find("VALIDSIG ");
//       if (_config->FindB("Debug::Vendor", false))
//          std::cerr << "Looking for VALIDSIG in \"" << (*I) << "\": pos " << pos 
//                    << std::endl;
//       if (pos != std::string::npos)
//       {
//          string Fingerprint = (*I).substr(pos+sizeof("VALIDSIG"));
//          if (_config->FindB("Debug::Vendor", false))
//             std::cerr << "Looking for \"" << Fingerprint << "\" in vendor..." <<
//                std::endl;
//          Vndr = List.FindVendor(Fingerprint) != "";
//          if (Vndr != NULL);
//          break;
//       }
//    }
   string::size_type pos;

   // check for missing sigs (that where not fatal because otherwise we had
   // bombed earlier)
   string missingkeys;
   string msg = _("There is no public key available for the "
		  "following key IDs:\n");
   pos = Message.find("NO_PUBKEY ");
   if (pos != std::string::npos)
   {
      string::size_type start = pos+strlen("NO_PUBKEY ");
      string Fingerprint = Message.substr(start, Message.find("\n")-start);
      missingkeys += (Fingerprint);
   }
   if(!missingkeys.empty())
      _error->Warning("%s", string(msg+missingkeys).c_str());

   string Transformed = MetaIndexParser->GetExpectedDist();

   if (Transformed == "../project/experimental")
   {
      Transformed = "experimental";
   }

   pos = Transformed.rfind('/');
   if (pos != string::npos)
   {
      Transformed = Transformed.substr(0, pos);
   }

   if (Transformed == ".")
   {
      Transformed = "";
   }

   if (_config->FindB("Debug::pkgAcquire::Auth", false)) 
   {
      std::cerr << "Got Codename: " << MetaIndexParser->GetDist() << std::endl;
      std::cerr << "Expecting Dist: " << MetaIndexParser->GetExpectedDist() << std::endl;
      std::cerr << "Transformed Dist: " << Transformed << std::endl;
   }

   if (MetaIndexParser->CheckDist(Transformed) == false)
   {
      // This might become fatal one day
//       Status = StatAuthError;
//       ErrorText = "Conflicting distribution; expected "
//          + MetaIndexParser->GetExpectedDist() + " but got "
//          + MetaIndexParser->GetDist();
//       return false;
      if (!Transformed.empty())
      {
         _error->Warning("Conflicting distribution: %s (expected %s but got %s)",
                         Desc.Description.c_str(),
                         Transformed.c_str(),
                         MetaIndexParser->GetDist().c_str());
      }
   }

   return true;
}
       								/*}}}*/
// pkgAcqMetaIndex::Failed - no Release file present or no signature
//      file present	                                        /*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgAcqMetaIndex::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   if (AuthPass == true)
   {
      // if we fail the authentication but got the file via a IMS-Hit 
      // this means that the file wasn't downloaded and that it might be
      // just stale (server problem, proxy etc). we delete what we have
      // queue it again without i-m-s 
      // alternatively we could just unlink the file and let the user try again
      if (IMSHit)
      {
	 Complete = false;
	 Local = false;
	 AuthPass = false;
	 unlink(DestFile.c_str());

	 DestFile = _config->FindDir("Dir::State::lists") + "partial/";
	 DestFile += URItoFileName(RealURI);
	 Desc.URI = RealURI;
	 QueueURI(Desc);
	 return;
      }

      // gpgv method failed 
      _error->Warning("GPG error: %s: %s",
                      Desc.Description.c_str(),
                      LookupTag(Message,"Message").c_str());

   }
   else
     {
       // If this is a transient failure, we use the old version, if we
       // have one.
       //
       if (StringToBool(LookupTag(Message,"Transient-Failure"),false))
	 {
	   string Final = _config->FindDir("Dir::State::lists");
	   Final += URItoFileName(RealURI);
	   struct stat Buf;
	   if (stat(Final.c_str(),&Buf) == 0)
	     {
	       DestFile = Final;

	       if (SigFile != "")
		 {
		   // There was a signature file, so pass it to gpgv
		   // for verification

		   if (_config->FindB("Debug::pkgAcquire::Auth", false))
		     std::cerr << "Metaindex acquired, queueing gpg verification ("
			       << SigFile << "," << DestFile << ")\n";
		   AuthPass = true;
		   Desc.URI = "gpgv:" + SigFile;
		   QueueURI(Desc);
		   Mode = "gpgv";
		   return;
		 }
	     }
	 }
     }
       
   // No Release file was present, or verification failed, so fall
   // back to queueing Packages files without verification
   QueueIndexes(false);
}

									/*}}}*/

// AcqArchive::AcqArchive - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* This just sets up the initial fetch environment and queues the first
   possibilitiy */
pkgAcqArchive::pkgAcqArchive(pkgAcquire *Owner,pkgSourceList *Sources,
			     pkgRecords *Recs,pkgCache::VerIterator const &Version,
			     string &StoreFilename) :
               Item(Owner), Version(Version), Sources(Sources), Recs(Recs), 
               StoreFilename(StoreFilename), Vf(Version.FileList()), 
	       Trusted(false)
{
   Retries = _config->FindI("Acquire::Retries",0);

   if (Version.Arch() == 0)
   {
      _error->Error(_("I wasn't able to locate a file for the %s package. "
		      "This might mean you need to manually fix this package. "
		      "(due to missing arch)"),
		    Version.ParentPkg().Name());
      return;
   }
   
   /* We need to find a filename to determine the extension. We make the
      assumption here that all the available sources for this version share
      the same extension.. */
   // Skip not source sources, they do not have file fields.
   for (; Vf.end() == false; Vf++)
   {
      if ((Vf.File()->Flags & pkgCache::Flag::NotSource) != 0)
	 continue;
      break;
   }
   
   // Does not really matter here.. we are going to fail out below
   if (Vf.end() != true)
   {     
      // If this fails to get a file name we will bomb out below.
      pkgRecords::Parser &Parse = Recs->Lookup(Vf);
      if (_error->PendingError() == true)
	 return;
            
      // Generate the final file name as: package_version_arch.foo
      StoreFilename = QuoteString(Version.ParentPkg().Name(),"_:") + '_' +
	              QuoteString(Version.VerStr(),"_:") + '_' +
     	              QuoteString(Version.Arch(),"_:.") + 
	              "." + flExtension(Parse.FileName());
   }

   // check if we have one trusted source for the package. if so, switch
   // to "TrustedOnly" mode
   for (pkgCache::VerFileIterator i = Version.FileList(); i.end() == false; i++)
   {
      pkgIndexFile *Index;
      if (Sources->FindIndex(i.File(),Index) == false)
         continue;
      if (_config->FindB("Debug::pkgAcquire::Auth", false))
      {
         std::cerr << "Checking index: " << Index->Describe()
                   << "(Trusted=" << Index->IsTrusted() << ")\n";
      }
      if (Index->IsTrusted()) {
         Trusted = true;
	 break;
      }
   }

   // "allow-unauthenticated" restores apts old fetching behaviour
   // that means that e.g. unauthenticated file:// uris are higher
   // priority than authenticated http:// uris
   if (_config->FindB("APT::Get::AllowUnauthenticated",false) == true)
      Trusted = false;

   // Select a source
   if (QueueNext() == false && _error->PendingError() == false)
      _error->Error(_("I wasn't able to locate file for the %s package. "
		    "This might mean you need to manually fix this package."),
		    Version.ParentPkg().Name());
}
									/*}}}*/
// AcqArchive::QueueNext - Queue the next file source			/*{{{*/
// ---------------------------------------------------------------------
/* This queues the next available file version for download. It checks if
   the archive is already available in the cache and stashs the MD5 for
   checking later. */
bool pkgAcqArchive::QueueNext()
{   
   for (; Vf.end() == false; Vf++)
   {
      // Ignore not source sources
      if ((Vf.File()->Flags & pkgCache::Flag::NotSource) != 0)
	 continue;

      // Try to cross match against the source list
      pkgIndexFile *Index;
      if (Sources->FindIndex(Vf.File(),Index) == false)
	    continue;
      
      // only try to get a trusted package from another source if that source
      // is also trusted
      if(Trusted && !Index->IsTrusted()) 
	 continue;

      // Grab the text package record
      pkgRecords::Parser &Parse = Recs->Lookup(Vf);
      if (_error->PendingError() == true)
	 return false;
      
      string PkgFile = Parse.FileName();
      MD5 = Parse.MD5Hash();
      if (PkgFile.empty() == true)
	 return _error->Error(_("The package index files are corrupted. No Filename: "
			      "field for package %s."),
			      Version.ParentPkg().Name());

      Desc.URI = Index->ArchiveURI(PkgFile);
      Desc.Description = Index->ArchiveInfo(Version);
      Desc.Owner = this;
      Desc.ShortDesc = Version.ParentPkg().Name();

      // See if we already have the file. (Legacy filenames)
      FileSize = Version->Size;
      string FinalFile = _config->FindDir("Dir::Cache::Archives") + flNotDir(PkgFile);
      struct stat Buf;
      if (stat(FinalFile.c_str(),&Buf) == 0)
      {
	 // Make sure the size matches
	 if ((unsigned)Buf.st_size == Version->Size)
	 {
	    Complete = true;
	    Local = true;
	    Status = StatDone;
	    StoreFilename = DestFile = FinalFile;
	    return true;
	 }
	 
	 /* Hmm, we have a file and its size does not match, this means it is
	    an old style mismatched arch */
	 unlink(FinalFile.c_str());
      }

      // Check it again using the new style output filenames
      FinalFile = _config->FindDir("Dir::Cache::Archives") + flNotDir(StoreFilename);
      if (stat(FinalFile.c_str(),&Buf) == 0)
      {
	 // Make sure the size matches
	 if ((unsigned)Buf.st_size == Version->Size)
	 {
	    Complete = true;
	    Local = true;
	    Status = StatDone;
	    StoreFilename = DestFile = FinalFile;
	    return true;
	 }
	 
	 /* Hmm, we have a file and its size does not match, this shouldnt
	    happen.. */
	 unlink(FinalFile.c_str());
      }

      DestFile = _config->FindDir("Dir::Cache::Archives") + "partial/" + flNotDir(StoreFilename);
      
      // Check the destination file
      if (stat(DestFile.c_str(),&Buf) == 0)
      {
	 // Hmm, the partial file is too big, erase it
	 if ((unsigned)Buf.st_size > Version->Size)
	    unlink(DestFile.c_str());
	 else
	    PartialSize = Buf.st_size;
      }
      
      // Create the item
      Local = false;
      Desc.URI = Index->ArchiveURI(PkgFile);
      Desc.Description = Index->ArchiveInfo(Version);
      Desc.Owner = this;
      Desc.ShortDesc = Version.ParentPkg().Name();
      QueueURI(Desc);

      Vf++;
      return true;
   }
   return false;
}   
									/*}}}*/
// AcqArchive::Done - Finished fetching					/*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgAcqArchive::Done(string Message,unsigned long Size,string Md5Hash,
			 pkgAcquire::MethodConfig *Cfg)
{
   Item::Done(Message,Size,Md5Hash,Cfg);
   
   // Check the size
   if (Size != Version->Size)
   {
      Status = StatError;
      ErrorText = _("Size mismatch");
      return;
   }
   
   // Check the md5
   if (Md5Hash.empty() == false && MD5.empty() == false)
   {
      if (Md5Hash != MD5)
      {
	 Status = StatError;
	 ErrorText = _("MD5Sum mismatch");
	 if(FileExists(DestFile))
	    Rename(DestFile,DestFile + ".FAILED");
	 return;
      }
   }

   // Grab the output filename
   string FileName = LookupTag(Message,"Filename");
   if (FileName.empty() == true)
   {
      Status = StatError;
      ErrorText = "Method gave a blank filename";
      return;
   }

   Complete = true;

   // Reference filename
   if (FileName != DestFile)
   {
      StoreFilename = DestFile = FileName;
      Local = true;
      return;
   }
   
   // Done, move it into position
   string FinalFile = _config->FindDir("Dir::Cache::Archives");
   FinalFile += flNotDir(StoreFilename);
   Rename(DestFile,FinalFile);
   
   StoreFilename = DestFile = FinalFile;
   Complete = true;
}
									/*}}}*/
// AcqArchive::Failed - Failure handler					/*{{{*/
// ---------------------------------------------------------------------
/* Here we try other sources */
void pkgAcqArchive::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   ErrorText = LookupTag(Message,"Message");
   
   /* We don't really want to retry on failed media swaps, this prevents 
      that. An interesting observation is that permanent failures are not
      recorded. */
   if (Cnf->Removable == true && 
       StringToBool(LookupTag(Message,"Transient-Failure"),false) == true)
   {
      // Vf = Version.FileList();
      while (Vf.end() == false) Vf++;
      StoreFilename = string();
      Item::Failed(Message,Cnf);
      return;
   }
   
   if (QueueNext() == false)
   {
      // This is the retry counter
      if (Retries != 0 &&
	  Cnf->LocalOnly == false &&
	  StringToBool(LookupTag(Message,"Transient-Failure"),false) == true)
      {
	 Retries--;
	 Vf = Version.FileList();
	 if (QueueNext() == true)
	    return;
      }
      
      StoreFilename = string();
      Item::Failed(Message,Cnf);
   }
}
									/*}}}*/
// AcqArchive::IsTrusted - Determine whether this archive comes from a
// trusted source							/*{{{*/
// ---------------------------------------------------------------------
bool pkgAcqArchive::IsTrusted()
{
   return Trusted;
}

// AcqArchive::Finished - Fetching has finished, tidy up		/*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgAcqArchive::Finished()
{
   if (Status == pkgAcquire::Item::StatDone &&
       Complete == true)
      return;
   StoreFilename = string();
}
									/*}}}*/

// AcqFile::pkgAcqFile - Constructor					/*{{{*/
// ---------------------------------------------------------------------
/* The file is added to the queue */
pkgAcqFile::pkgAcqFile(pkgAcquire *Owner,string URI,string MD5,
		       unsigned long Size,string Dsc,string ShortDesc,
		       const string &DestDir, const string &DestFilename) :
                       Item(Owner), Md5Hash(MD5)
{
   Retries = _config->FindI("Acquire::Retries",0);
   
   if(!DestFilename.empty())
      DestFile = DestFilename;
   else if(!DestDir.empty())
      DestFile = DestDir + "/" + flNotDir(URI);
   else
      DestFile = flNotDir(URI);

   // Create the item
   Desc.URI = URI;
   Desc.Description = Dsc;
   Desc.Owner = this;

   // Set the short description to the archive component
   Desc.ShortDesc = ShortDesc;
      
   // Get the transfer sizes
   FileSize = Size;
   struct stat Buf;
   if (stat(DestFile.c_str(),&Buf) == 0)
   {
      // Hmm, the partial file is too big, erase it
      if ((unsigned)Buf.st_size > Size)
	 unlink(DestFile.c_str());
      else
	 PartialSize = Buf.st_size;
   }

   QueueURI(Desc);
}
									/*}}}*/
// AcqFile::Done - Item downloaded OK					/*{{{*/
// ---------------------------------------------------------------------
/* */
void pkgAcqFile::Done(string Message,unsigned long Size,string MD5,
		      pkgAcquire::MethodConfig *Cnf)
{
   // Check the md5
   if (Md5Hash.empty() == false && MD5.empty() == false)
   {
      if (Md5Hash != MD5)
      {
	 Status = StatError;
	 ErrorText = "MD5Sum mismatch";
	 Rename(DestFile,DestFile + ".FAILED");
	 return;
      }
   }
   
   Item::Done(Message,Size,MD5,Cnf);

   string FileName = LookupTag(Message,"Filename");
   if (FileName.empty() == true)
   {
      Status = StatError;
      ErrorText = "Method gave a blank filename";
      return;
   }

   Complete = true;
   
   // The files timestamp matches
   if (StringToBool(LookupTag(Message,"IMS-Hit"),false) == true)
      return;
   
   // We have to copy it into place
   if (FileName != DestFile)
   {
      Local = true;
      if (_config->FindB("Acquire::Source-Symlinks",true) == false ||
	  Cnf->Removable == true)
      {
	 Desc.URI = "copy:" + FileName;
	 QueueURI(Desc);
	 return;
      }
      
      // Erase the file if it is a symlink so we can overwrite it
      struct stat St;
      if (lstat(DestFile.c_str(),&St) == 0)
      {
	 if (S_ISLNK(St.st_mode) != 0)
	    unlink(DestFile.c_str());
      }
      
      // Symlink the file
      if (symlink(FileName.c_str(),DestFile.c_str()) != 0)
      {
	 ErrorText = "Link to " + DestFile + " failure ";
	 Status = StatError;
	 Complete = false;
      }      
   }
}
									/*}}}*/
// AcqFile::Failed - Failure handler					/*{{{*/
// ---------------------------------------------------------------------
/* Here we try other sources */
void pkgAcqFile::Failed(string Message,pkgAcquire::MethodConfig *Cnf)
{
   ErrorText = LookupTag(Message,"Message");
   
   // This is the retry counter
   if (Retries != 0 &&
       Cnf->LocalOnly == false &&
       StringToBool(LookupTag(Message,"Transient-Failure"),false) == true)
   {
      Retries--;
      QueueURI(Desc);
      return;
   }
   
   Item::Failed(Message,Cnf);
}
									/*}}}*/
