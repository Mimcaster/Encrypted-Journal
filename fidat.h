#ifndef FIDAT_H
#define FIDAT_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>	//for time_t
#include <string.h>

#define CHECK_CODE "AZD"
#define SECTOR_SIZE 512

#define IS_FOLDER 0
#define IS_FILE 1

//data structure representing an open file descriptor for a file in the parition
struct FilePointer
{
	FILE *filestream;
	int fid;
};

//data structure representing the start secotr
struct StartSector
{
	char checkcode[3];
	int numberofsectors;
	
	int rootdirstart;
	int rootdirsize;

	int alloctablestart;
	int alloctablesize;
	int totalsectorsallocated;

	int dataareastart;
	int numoffiles;
};

//data structure representing an entry in the root directory
struct RootDirEntry
{
	int fileid;
	char filename[33];	//an extra character is stored at the end of the name and the extension for the null terminator
	//char fileextension[4];
	//int isfile;	//0 means it is a file, 1 means its a folder
	time_t creationtime;
	time_t modifiedtime;
	size_t filesize;
	int numberofsectors;

};

//data structure representing an entry in the FIDAT
struct FidatEntry
{
	int sectorspace;
	int occupyingfid;
};

void initializeFileSystem();
void initializeStartingSector();

struct StartSector getStartSector(FILE *filesystem);

void updateStartSector();
void loadRootDirectory();

void loadFidat();
void updateFidat();

int deleteFile(struct FilePointer filedeleted);
int createFile(char *filename);
void writeFile(struct FilePointer fp, int filelocation, int buffersize, char *buffer);
char *readFile(struct FilePointer fp, int filelocation, int buffersize);
struct FilePointer *openFile(char *pathname);
void closeFile(struct FilePointer *pointer);

FILE *filesystem;
struct StartSector ssector;
struct RootDirEntry *rootdirectory;
struct FidatEntry *fidalloctable;
char *filesystemname;
//#define filesystemname "Drive2MB" //Drive2MB Drive3MB Drive5MB Drive10MB


#endif
