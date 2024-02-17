// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "debug.h"
#include "filehdr.h"
#include "main.h"
#include "synchdisk.h"

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or
// FetchFrom. 	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader() {
  numBytes = -1;
  numSectors = -1;
  memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader() {
  // nothing to do now
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize) {
  numBytes = fileSize;
  numSectors = divRoundUp(fileSize, SectorSize);
  if (freeMap->NumClear() < numSectors)
    return FALSE; // not enough space

  if (fileSize > numOfBytesLevel4) {
    int i = 0;
    while (fileSize > 0) {
      dataSectors[i] = freeMap->FindAndSet();
      FileHeader *sub_hdr = new FileHeader();
      if (fileSize > numOfBytesLevel4) {
        sub_hdr->Allocate(freeMap, numOfBytesLevel4);
      } else {
        sub_hdr->Allocate(freeMap, fileSize);
      }
      fileSize -= numOfBytesLevel4;
      sub_hdr->WriteBack(dataSectors[i]);
      i++;
    }
  } else if (fileSize > numOfBytesLevel3) {
    int i = 0;
    while (fileSize > 0) {
      dataSectors[i] = freeMap->FindAndSet();
      FileHeader *sub_hdr = new FileHeader();
      if (fileSize > numOfBytesLevel3) {
        sub_hdr->Allocate(freeMap, numOfBytesLevel3);
      } else {
        sub_hdr->Allocate(freeMap, fileSize);
      }
      fileSize -= numOfBytesLevel3;
      sub_hdr->WriteBack(dataSectors[i]);
      i++;
    }
  } else if (fileSize > numOfBytesLevel2) {
    int i = 0;
    while (fileSize > 0) {
      dataSectors[i] = freeMap->FindAndSet();
      FileHeader *sub_hdr = new FileHeader();
      if (fileSize > numOfBytesLevel2) {
        sub_hdr->Allocate(freeMap, numOfBytesLevel2);
      } else {
        sub_hdr->Allocate(freeMap, fileSize);
      }
      fileSize -= numOfBytesLevel2;
      sub_hdr->WriteBack(dataSectors[i]);
      i++;
    }
  } else if (fileSize > numOfBytesLevel1) {
    int i = 0;
    while (fileSize > 0) {
      dataSectors[i] = freeMap->FindAndSet();
      FileHeader *sub_hdr = new FileHeader();
      if (fileSize > numOfBytesLevel1) {
        sub_hdr->Allocate(freeMap, numOfBytesLevel1);
      } else {
        sub_hdr->Allocate(freeMap, fileSize);
      }
      fileSize -= numOfBytesLevel1;
      sub_hdr->WriteBack(dataSectors[i]);
      i++;
    }
  } else {
    for (int i = 0; i < numSectors; i++) {
      dataSectors[i] = freeMap->FindAndSet();

      ASSERT(dataSectors[i] >= 0);

      char *clean_data = new char[SectorSize]();
      kernel->synchDisk->WriteSector(dataSectors[i], clean_data);
      delete clean_data;
    }
  }

  return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(PersistentBitmap *freeMap) {
  if (numBytes > numOfBytesLevel1) {
    for (int i = 0; i < divRoundUp(numSectors, NumDirect); i++) {
      FileHeader *sub_hdr = new FileHeader();
      sub_hdr->FetchFrom(dataSectors[i]);
      sub_hdr->Deallocate(freeMap);
    }
  } else {
    for (int i = 0; i < numSectors; i++) {
      ASSERT(freeMap->Test((int)dataSectors[i]));
      freeMap->Clear((int)dataSectors[i]);
    }
  }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector) {
  kernel->synchDisk->ReadSector(sector, (char *)this);

  /*
          MP4 Hint:
          After you add some in-core informations, you will need to rebuild the
     header's structure
  */
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector) {
  kernel->synchDisk->WriteSector(sector, (char *)this);

  /*
          MP4 Hint:
          After you add some in-core informations, you may not want to write all
     fields into disk. Use this instead: char buf[SectorSize]; memcpy(buf +
     offset, &dataToBeWritten, sizeof(dataToBeWritten));
          ...
  */
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset) {
  if (numBytes > numOfBytesLevel4) {
    FileHeader *sub_hdr = new FileHeader;
    int entry_number = divRoundDown(offset, numOfBytesLevel4);
    sub_hdr->FetchFrom(dataSectors[entry_number]);
    sub_hdr->ByteToSector(offset - numOfBytesLevel4 * entry_number);
  } else if (numBytes > numOfBytesLevel3) {
    FileHeader *sub_hdr = new FileHeader;
    int entry_number = divRoundDown(offset, numOfBytesLevel3);
    sub_hdr->FetchFrom(dataSectors[entry_number]);
    sub_hdr->ByteToSector(offset - numOfBytesLevel3 * entry_number);
  } else if (numBytes > numOfBytesLevel2) {
    FileHeader *sub_hdr = new FileHeader;
    int entry_number = divRoundDown(offset, numOfBytesLevel2);
    sub_hdr->FetchFrom(dataSectors[entry_number]);
    sub_hdr->ByteToSector(offset - numOfBytesLevel2 * entry_number);
  } else if (numBytes > numOfBytesLevel1) {
    FileHeader *sub_hdr = new FileHeader;
    int entry_number = divRoundDown(offset, numOfBytesLevel1);
    sub_hdr->FetchFrom(dataSectors[entry_number]);
    sub_hdr->ByteToSector(offset - numOfBytesLevel1 * entry_number);
  } else {
    return (dataSectors[offset / SectorSize]);
  }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength() { return numBytes; }

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print() {
  int i, j, k;
  char *data = new char[SectorSize];

  printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
  for (i = 0; i < numSectors; i++)
    printf("%d ", dataSectors[i]);
  printf("\nFile contents:\n");

  if (numBytes > numOfBytesLevel1) {
    for (int i = 0; i < numSectors / NumDirect; i++) {
      OpenFile *openFile = new OpenFile(dataSectors[i]);
      FileHeader *sub_hdr = openFile->GetHdr();
      sub_hdr->Print();
    }
  } else {
    for (i = k = 0; i < numSectors; i++) {
      kernel->synchDisk->ReadSector(dataSectors[i], data);
      for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
        if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
          printf("%c", data[j]);
        else
          printf("\\%x", (unsigned char)data[j]);
      }
      printf("\n");
    }
  }
  delete[] data;
}