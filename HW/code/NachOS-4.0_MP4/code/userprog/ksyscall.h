/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt() { kernel->interrupt->Halt(); }

int SysCreate(char *filename, int initialSize) {
  return kernel->fileSystem->Create(filename, initialSize);
}

OpenFileId SysOpen(char *name) {
  OpenFileId openFileId = (OpenFileId)kernel->fileSystem->Open(name);
  if (openFileId > 0)
    return openFileId;
  else
    return -1;
}

int SysWrite(char *buffer, int size, OpenFileId id) {
  return kernel->fileSystem->Write(buffer, size, id);
}

int SysRead(char *buf, int size, OpenFileId id) {
  return kernel->fileSystem->Read(buf, size, id);
}

int SysClose(OpenFileId id) { return kernel->fileSystem->Close(id); }

int SysAdd(int op1, int op2) { return op1 + op2; }

#ifdef FILESYS_STUB
int SysCreate(char *filename) {
  // return value
  // 1: success
  // 0: failed
  return kernel->interrupt->CreateFile(filename);
}
#endif

#endif /* ! __USERPROG_KSYSCALL_H__ */
