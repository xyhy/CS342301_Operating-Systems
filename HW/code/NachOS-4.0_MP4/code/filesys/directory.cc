// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "directory.h"
#include "copyright.h"
#include "filehdr.h"
#include "filesys.h"
#include "kernel.h"
#include "main.h"
#include "utility.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size) {
  table = new DirectoryEntry[size];

  // MP4 mod tag
  memset(table, 0, sizeof(DirectoryEntry) * size);
  // dummy operation to keep valgrind happy

  tableSize = size;
  for (int i = 0; i < tableSize; i++) {
    table[i].inUse = FALSE;
    table[i].isDir = FALSE;
  }
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory() { delete[] table; }

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file) {
  (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file) {
  (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name) {
  for (int i = 0; i < tableSize; i++)
    if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
      return i;
  return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name) {
  int i = FindIndex(name);

  if (i != -1)
    return table[i].sector;
  return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector) {
  if (FindIndex(name) != -1)
    return FALSE;

  for (int i = 0; i < tableSize; i++)
    if (!table[i].inUse) {
      table[i].inUse = TRUE;
      strncpy(table[i].name, name, FileNameMaxLen);
      table[i].sector = newSector;
      return TRUE;
    }
  return FALSE; // no space.  Fix when we have extensible files.
}

// MP4
bool Directory::Add(char *name, int newSector, bool isDir) {
  if (FindIndex(name) != -1)
    return FALSE;

  for (int i = 0; i < tableSize; i++)
    if (!table[i].inUse) {
      table[i].inUse = TRUE;
      strncpy(table[i].name, name, FileNameMaxLen);
      table[i].sector = newSector;
      table[i].isDir = isDir;
      return TRUE;
    }
  return FALSE; // no space.  Fix when we have extensible files.
}
//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name) {
  int i = FindIndex(name);

  if (i == -1)
    return FALSE; // name not in directory
  table[i].inUse = FALSE;
  return TRUE;
}

void Directory::RecursiveRemove(char *name) {
  bool empty = TRUE;

  cout << "Current directory: " << name << endl;
  for (int i = 0; i < tableSize; i++) {
    if (table[i].inUse) {
      empty = FALSE;
      if (table[i].isDir) {
        cout << "entry " << i << " is Dir. name " << table[i].name
             << ", sector " << table[i].sector << ""
             << "\n";
        Directory *next_dir = new Directory(NumDirEntries);
        OpenFile *next_dir_file = new OpenFile(table[i].sector);
        next_dir->FetchFrom(next_dir_file);

        PersistentBitmap *freeMap = new PersistentBitmap(
            kernel->fileSystem->getFreeMapFile(), NumSectors);
        FileHeader *next_dirfile_tobeRemove = new FileHeader;
        next_dirfile_tobeRemove->FetchFrom(table[i].sector);
        next_dirfile_tobeRemove->Deallocate(freeMap);
        freeMap->Clear(table[i].sector);
        next_dir->RecursiveRemove(table[i].name);
        next_dir->WriteBack(next_dir_file);
        this->Remove(table[i].name);
        freeMap->WriteBack(kernel->fileSystem->getFreeMapFile());

        delete next_dir_file;
        delete next_dir;
        delete next_dirfile_tobeRemove;
        delete freeMap;

      } else {
        cout << "entry " << i << " is a File. name " << table[i].name
             << ", sector " << table[i].sector << ""
             << "\n";

        PersistentBitmap *freeMap = new PersistentBitmap(
            kernel->fileSystem->getFreeMapFile(), NumSectors);
        FileHeader *fileHdr_of_file_tobeRemove = new FileHeader;
        fileHdr_of_file_tobeRemove->FetchFrom(table[i].sector);
        fileHdr_of_file_tobeRemove->Deallocate(freeMap);
        freeMap->Clear(table[i].sector);

        if (!Remove(table[i].name))
          cout << "Error in removing file: " << name << "\n";

        freeMap->WriteBack(kernel->fileSystem->getFreeMapFile());
      }
    }
  }

  if (empty)
    cout << "This directory is empty.\n";
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void Directory::List() {
  bool empty = TRUE;
  for (int i = 0; i < tableSize; i++) {
    if (table[i].inUse) {
      empty = FALSE;
      if (table[i].isDir)
        cout << "[D] " << table[i].name << endl;
      else
        cout << "[F] " << table[i].name << endl;
    }
  }
  if (empty)
    cout << "The directory is empty." << endl;
}

void Directory::RecursiveList(int depth) {
  bool empty = TRUE;
  Directory *subdir = new Directory(NumDirEntries);
  OpenFile *subdir_openfile;

  DEBUG(dbgFile, "Directory::RecursiveList in depth: " << depth);

  for (int i = 0; i < tableSize; i++) {
    if (table[i].inUse) {
      cout << "\n";
      empty = FALSE;
      for (int k = 0; k < depth; k++)
        cout << "  ";

      if (table[i].isDir) {
        printf("[D] %s", table[i].name);
        subdir_openfile = new OpenFile(table[i].sector);
        subdir->FetchFrom(subdir_openfile);
        subdir->RecursiveList(depth + 1);
      } else {
        printf("[F] %s", table[i].name);
      }
    }
  }

  if (empty) {
    for (int k = 0; k < depth; k++)
      cout << " ";
    cout << "(Empty directory)";
  }
  depth--;
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print() {
  FileHeader *hdr = new FileHeader;

  printf("Directory contents:\n");
  for (int i = 0; i < tableSize; i++)
    if (table[i].inUse) {
      printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
      hdr->FetchFrom(table[i].sector);
      hdr->Print();
    }
  printf("\n");
  delete hdr;
}
