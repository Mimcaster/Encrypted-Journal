#include "fidat.h"

/*
	fidat.c is a library that allows other programs to interact with the FIDAT file system.
	Data structures associated with the file system such as the root directory and file ID allocation table are loaded into memory with the initializeFileSystem() function.
	Function are created in order to read, write, access, create and manage the file system.
	readFile() and writeFile() are the functions used to read and write to files in the file system.
	openFile() and closeFile() are functions that allow the user to open and close streams to the file system.
	createFile() and deleteFile() are functions that allow the user to create and delete files in the file system.
*/

/*
	When the library is first used, this is the first method to be called. It makes sure that the file system is set properly, that the start sector is filled with the needed data, and reates teh data structure representing the FIDAT. It also opens the stream to the file system.
	Takes the arguement systemname. Systemname is the name of the disk in the same directory as the program accessing the file system.
	Returns nothing.

*/
void initializeFileSystem(char *systemname)
{
	//Allocates memory to the variable that holds the name of the file holding the filesystem.
	filesystemname = (char*)malloc(sizeof(char) * strlen(systemname));
	//Copys in the name of the file system.
	strcat(filesystemname, systemname);
	//Opening the stream to the file system.
	if(fopen(filesystemname, "r+") == NULL)	//checking to see the file exists, creates it if it does not
	{
		FILE * createfile;
		createfile = fopen(filesystemname, "w");
		int onemeg = 1024 * 1024;
		fseek(createfile, (long)onemeg, SEEK_SET);
		char nullterm[1];
		nullterm[0] = '\0';
		fwrite(nullterm, sizeof(char), 1, createfile);
		fclose(createfile);
	}			

	filesystem = fopen(filesystemname, "r+");


	//Loading information from the start sector into the data structure representing it.
	ssector = getStartSector(filesystem);


	//Making sure the file system partition has the correct header. If it does not, then that mean the start sector does not have the proper information inside it
	if(strcmp(ssector.checkcode, CHECK_CODE) != 0)
	{
		initializeStartingSector();
		ssector = getStartSector(filesystem);
		printf("%s\n", ssector.checkcode);
	}
	//Allocating memory to the root directory information being stored
	rootdirectory = (struct RootDirEntry *) malloc(sizeof(struct RootDirEntry) * ssector.numoffiles);
	//Loading in data structures that will be used by other methods in the file system.
	loadRootDirectory();
	loadFidat();
	//Prints out information stored in the start sector.
	printf("Number of sectors: %d\n Start sector of root directory: %d\n Number of sectors in the root directory: %d\n Start sector of FIDAT: %d\n Number of sectors in the FIDAT: %d\n Total Number of Sectors Allocated: %d\n Start sector of the data area: %d\n Total Number of FIles Allocated: %d\n", ssector.numberofsectors, ssector.rootdirstart, ssector.rootdirsize, ssector.alloctablestart, ssector.alloctablesize, ssector.totalsectorsallocated, ssector.dataareastart, ssector.numoffiles);
}

/*
	Writes all the meta data associated with the file system into the first sector.
	Takes no arguements, returns nothing.

*/
void initializeStartingSector()
{
	long int currentpos;	//the position of where the file system is being written to	
	
	//Writing in the file system header
	rewind(filesystem);
	fwrite(CHECK_CODE, sizeof(char), 3, filesystem);
	
	currentpos = ftell(filesystem);

	//Getting and the size of the file system, and writing it in
	fseek(filesystem, 0, SEEK_END);


	long int *partitionsize;
	partitionsize = (long int *)malloc (sizeof(long int));
	*partitionsize = ftell(filesystem);
	printf("%ld\n", *partitionsize);
	if(fseek(filesystem, currentpos, SEEK_SET) == -1)
		perror("seek error");

	
	//Getting the number of sectors in the file system and writing it in
	int numberofsectors = *partitionsize/SECTOR_SIZE;
	fwrite(&numberofsectors, sizeof(int), 1, filesystem);

	//Writing in the root directory size and starting sector
	int *rootdirstart = (int*)malloc(sizeof(int));
	*rootdirstart = 1;
	fwrite(rootdirstart, sizeof(int), 1, filesystem);
	int rootdirsize = 10;
	fwrite(&rootdirsize, sizeof(int), 1, filesystem);

	//Writing in the fid allocation table size and start
	int alloctablestart = *rootdirstart + rootdirsize;
	fwrite(&alloctablestart, sizeof(int), 1, filesystem);
	int entriesperfidatsector = SECTOR_SIZE/sizeof(struct FidatEntry);
	int alloctablesize = ((numberofsectors - (rootdirsize + 1))/ entriesperfidatsector) + 1;
	fwrite(&alloctablesize, sizeof(int), 1, filesystem);

	//Writing in the the number of sectors that have been allocated so far. It's at zero, because if this function is being called, the file system has never been used before, meaning its not possible for anything to have been allocated. 
	int totalsectorsallocated = 0;
	fwrite(&totalsectorsallocated, sizeof(int), 1, filesystem);
	
	//Writing in where the data area starts, which is after the FIDAT table.
	int dataareastart = alloctablestart + alloctablesize;
	fwrite(&dataareastart, sizeof(int), 1, filesystem);
}

/*
	Updates the file system when changes are made to the start sector data structure in memory
	Parameters: None
	Returns: Nothing
*/
void updateStartSector()
{
	//Going to the start of the start sector
	rewind(filesystem);

	//Writing in informaton to the start sector.
	fwrite(ssector.checkcode, sizeof(char),3, filesystem);
	fwrite(&ssector.numberofsectors,sizeof(int),1,filesystem);
	fwrite(&ssector.rootdirstart,sizeof(int),1,filesystem);
	fwrite(&ssector.rootdirsize,sizeof(int),1,filesystem);
	fwrite(&ssector.alloctablestart,sizeof(int),1,filesystem);
	fwrite(&ssector.alloctablesize,sizeof(int),1,filesystem);
	fwrite(&ssector.totalsectorsallocated,sizeof(int),1,filesystem);
	fwrite(&ssector.dataareastart,sizeof(int),1,filesystem);
	fwrite(&ssector.numoffiles,sizeof(int),1,filesystem);
}

/*
	Goes into the file system and writes in any changes that have been made to the FIDAT.
	Parameters: None
	Returns: Nothing
*/
void updateFidat()
{
	//Going to the part of the file system holding the Fidat
	fseek(filesystem, (long)(SECTOR_SIZE * ssector.alloctablestart), SEEK_SET);

	//Overwriting what is there
	for(int i = 0; i < ssector.totalsectorsallocated;i++)
	{
		fwrite(&fidalloctable[i], sizeof(struct FidatEntry), 1, filesystem);
	}
	
}


/*
	Goes into the file system and loads the FIDAT into memory.
	Parameters: None
	Returns: Nothing
*/
void loadFidat()
{		//Allocates enough memory to hold an entry for every FIDAT entry in the file system.
	fidalloctable = (struct FidatEntry *)malloc(sizeof(struct FidatEntry) * (ssector.totalsectorsallocated + 1));
	
	fseek(filesystem,(long)(SECTOR_SIZE * ssector.alloctablestart),SEEK_SET);

	//For every FIDAT entry that has been used so far
	for(int i = 0; i < ssector.totalsectorsallocated + 1; i++)
	{
		fread(&fidalloctable[i], sizeof(struct FidatEntry), 1, filesystem);	//Fidat entry gets read into memory
	}
}

/*
	Goes to the start of the filesystem and reads in the information stored.
	Returns: data structure representing the start sector
	filesystem: file descriptor representing the partition
	
*/
struct StartSector getStartSector(FILE *filesystem)
{
	struct StartSector retrievedsector;
	
	//Going back to the start of the file system.
	rewind(filesystem);

	//Allocating memory to store the start sector.
	char *unparsedsector;
	unparsedsector = (char *)malloc(sizeof(char) * 3);

	//Reading the start sector header and storing it in the buffer.
	fread(unparsedsector, sizeof(char), 3, filesystem);
	
	//Reading in the file system header, which contains 3 characters.
	retrievedsector.checkcode[0] = unparsedsector[0];
	retrievedsector.checkcode[1] = unparsedsector[1];
	retrievedsector.checkcode[2] = unparsedsector[2];
	retrievedsector.checkcode[3] = '\0';

	//Reading in the number of sectors in the file system
	fread(&retrievedsector.numberofsectors, sizeof(int), 1, filesystem);


	//Reading in the starting sector number of the root directory
	int rootdirstrt;
	fread(&rootdirstrt, sizeof(int), 1, filesystem);
	retrievedsector.rootdirstart = rootdirstrt;

	//Reading in the size of the root directory
	int rootsize;
	fread(&rootsize, sizeof(int), 1, filesystem);
	retrievedsector.rootdirsize = rootsize;

	//Reading in the starting sector of the FIDAT
	int allocstart;
	fread(&allocstart, sizeof(int), 1, filesystem);
	retrievedsector.alloctablestart = allocstart;

	//Reading in the size of the FIDAT
	int fidatsize;
	fread(&fidatsize, sizeof(int), 1, filesystem);
	retrievedsector.alloctablesize = fidatsize;	

	//Reading in the number of sectors that have been allocated so far. I realized it would be easier to just read it into the data structure directly around here.
	fread(&retrievedsector.totalsectorsallocated, sizeof(int), 1, filesystem);

	//Reading in the starting sector of the data area
	fread(&retrievedsector.dataareastart, sizeof(int), 1, filesystem);

	//Reading in the number of files and folders currently in the file system.
	fread(&retrievedsector.numoffiles, sizeof(int), 1, filesystem);
	return retrievedsector;
}


/*
	Gets all the root directory entries currently stored in the file system, and loads them into memory.
	Parameters: None
	Returns: Nothing
*/
void loadRootDirectory()
{
	//Going to the start of the root directory.
	fseek(filesystem, (long)(SECTOR_SIZE * ssector.rootdirstart), SEEK_SET);
	
	//char *unparseddir;
	//unparseddir = (char*)malloc(sizeof(struct RootDirEntry));


	//struct RootDirEntry loadedentry;

	//For every file stored in the root directory.
	for(int i = 0; i < ssector.numoffiles; i++)
	{	//Read it into the data structure representing the root directory
		fread(&(rootdirectory[i]), sizeof(struct RootDirEntry), 1, filesystem);
		printf("%s\n",rootdirectory[i].filename);
	}

}

/*
	Deletes the file.
	Goes to the root directory and replaces the filename with XXXXXXXX and the FID with FF to represent its deletion.
	Goes to through the FIDAT and any entry matching the FID of the deleted file gets zeroed out.
	FilePointer *filename: the pointer of the file being deleted.
	Returns 1 on success and 0 on failure.
*/
int deleteFile(struct FilePointer filedeleted)
{
	//Blanks out the filename.
	for(int i = 0; i < 32; i ++)
		rootdirectory[filedeleted.fid - 1].filename[i] = 'X';

	rootdirectory[filedeleted.fid - 1].filename[32] = '\0';	

	//Updates the directory with the now blanked out file name
	fseek(filedeleted.filestream, (long)(SECTOR_SIZE * ssector.rootdirstart), SEEK_SET);
	fseek(filedeleted.filestream, (long)(sizeof(struct RootDirEntry) * (filedeleted.fid - 1)), SEEK_CUR);
	fwrite(&rootdirectory[filedeleted.fid - 1], sizeof(struct RootDirEntry), 1, filedeleted.filestream);

	//Go through the FIDAT, and every entry containing the file id of the file getting deleted gets replaced with (ssector.rootdirsize * SECTOR_SIZE / sizeof(struct RootDirEntry) +1)

	for(int i = 0; i < ssector.totalsectorsallocated; i ++)
	{
		if(fidalloctable[i].occupyingfid == filedeleted.fid)
			fidalloctable[i].occupyingfid = (ssector.rootdirsize * SECTOR_SIZE / sizeof(struct RootDirEntry)) + 1;
	}
	//Writes to the FIDAT to update the fact that sectors have been unallocated.
	updateFidat();

	return 1;
}
/*
	Used to create a file or folder.
	If it is meant to be a file, isitfile is set to a nonzero integer.
	If it is meant to be a folder, isitfile is set to 0.
	Char filename: Contains the name of the file and the path leading to it.
	Returns 1 on success and 0 on failure. May fail if a file with the same name already exists, or if the file path does not exist
*/
int createFile(char *filename)
{
	char newfilename[8];		//Variables that hold information related to the file
	//char fileext[4];
	size_t filesize = 0;
	
	//Setting the File ID to be the number of files that have ever been tracked by the file system
	ssector.numoffiles++;		
	int fileid = ssector.numoffiles;
	
	updateStartSector();	//Updating the start sector to reflect that another file has been created

	//The number of sectors currently allocated to the file. Gets changed to 1 if it is a directory instead of a file.
	int numberofsectors = 0;

	rewind(filesystem);

	//char parentfolder[8];
	int filenameplace =0;	//Current location in the filename string

	//The first 32 characters of the file that the user puts in become the filename of the new file.
	for(int i = 0; i < 32; i++)
	{
		if(filename[filenameplace] != '\0')
		{
			newfilename[i] = filename[filenameplace];
			newfilename[i+1] = '\0';
			filenameplace ++;
		}
		else
		{
			newfilename[i] = '\0';
			break;
		}
	}


	time_t creationtime = time(NULL);

	//going to the closest empty entry in the root directory
	fseek(filesystem, (long)(ssector.rootdirstart * SECTOR_SIZE), SEEK_SET);
	fseek(filesystem, (long)((ssector.numoffiles -1) * sizeof(struct RootDirEntry)), SEEK_CUR);	//-1 because this is the first file being read

	struct RootDirEntry newdirentry;
	newdirentry.fileid = fileid;

	//Writing in the file meta data to a RootDirEntry structure

	strcat(newdirentry.filename, newfilename);
	//newdirentry.isfile = isitfile;
	newdirentry.creationtime = creationtime;
	newdirentry.modifiedtime = creationtime;
	newdirentry.filesize = filesize;
	newdirentry.numberofsectors = numberofsectors;
	//Allocating memory to the rootdirectory structure to make room for the new file	
	rootdirectory = (struct RootDirEntry*)realloc(rootdirectory, sizeof(struct RootDirEntry) * (ssector.numoffiles + 1));

	rootdirectory[ssector.numoffiles-1] = newdirentry;
	//Updates the filesystem with the new metadata.
	fwrite(&newdirentry, sizeof(struct RootDirEntry),1,filesystem);	

	return 1;
}

/*
	Writes to the file. If the file does not have enough space allocated to it already, then more space is created for it. Once more sectors are allocated to it, the contents of the sectors are replaced. The amount of contents which are replaced are specified by the variable buffersize.
	FilePointer fp: The stream being used to access the file
	int filelocation: The location in the file being written to
	int buffersize: The amount of data being written to the file
	char *buffer: The buffer being written into the file.
	Returns nothing
*/
void writeFile(struct FilePointer fp, int filelocation, int buffersize, char *buffer)
{
	//Getting the meta data associated with the file.
	struct RootDirEntry filedata;

	filedata = rootdirectory[fp.fid-1];		 //The File ID can also be used to figure out where in the root directory the file meta data can be located.

	//Updating the directory with the new last modified time
	rootdirectory[fp.fid-1].modifiedtime = time(NULL);
	fseek(fp.filestream, (long)(ssector.rootdirstart * SECTOR_SIZE), SEEK_SET);
	fseek(fp.filestream, (long)(sizeof(struct RootDirEntry) * (filedata.fileid - 1)), SEEK_CUR);
	fwrite(&rootdirectory[fp.fid-1], sizeof(struct RootDirEntry), 1, filesystem);

	int sizechanged = 1;
	//Changing the file size in the meta data to the new file size
	if(filelocation + buffersize > filedata.filesize)
	{
		sizechanged = 0;
		filedata.filesize = filelocation + buffersize;
	}


	//Finding the number of new sectors that need to be allocated to the file
	while((filelocation + buffersize) > (filedata.numberofsectors * SECTOR_SIZE))
	{
		sizechanged = 0;
		filedata.filesize += SECTOR_SIZE;
		filedata.numberofsectors++;
		
		int currentfidat = 0;
		
		//Searches through fidat entries until it finds a sector that is not being used.
		//1 + ssector.rootdirsize * SECTOR_SIZE / Root Directory Entry Size is the greater than the greatest file ID size possible, because the root directory can't store information for that many files.
		while(fidalloctable[currentfidat].occupyingfid != 0 && fidalloctable[currentfidat].occupyingfid != ((ssector.rootdirsize * SECTOR_SIZE / sizeof(struct RootDirEntry) +1)))	
		{
			currentfidat ++;
		}
		
		//Updating the start sector to reflect the fact that there is a new entry in the FIDAT table. Making more space in the FIDAT data structure for more file entries.
		if(fidalloctable[currentfidat].occupyingfid == 0)
		{
			ssector.totalsectorsallocated++;
			updateStartSector();
			fidalloctable = realloc(fidalloctable, sizeof(struct FidatEntry) * (ssector.totalsectorsallocated + 1));
		}

		fseek(filesystem, (long)((ssector.rootdirstart * SECTOR_SIZE) + (sizeof(int) * 2 * currentfidat)), SEEK_SET);	//Goes to the entry in the fidat being overwritten
		
		fidalloctable[currentfidat].sectorspace = filedata.numberofsectors;
		fidalloctable[currentfidat].occupyingfid = fp.fid;
		
		
	}

	//If more sectors were allocated to the file, it updates the FIDAT and the root directory entry for the file. 
	if(sizechanged == 0)
	{
		updateFidat();
		filedata.filesize = filelocation + buffersize;
		fseek(filesystem, (long)(ssector.rootdirstart * SECTOR_SIZE), SEEK_SET);
		fseek(filesystem, (long)(sizeof(struct RootDirEntry) * (filedata.fileid - 1)), SEEK_CUR);
		fwrite(&filedata, sizeof(struct RootDirEntry), 1, filesystem);

		rootdirectory[filedata.fileid-1] = filedata;
	}

	//Now that the sectors have been allocated, the file must be actually written to.
	//First sector written to is "int filelocation / SECTOR_SIZE". This is found by looping through the Fidat until an entries is found with the File ID of the file being written to, and the first sector.
	//That sector gets written to until either there is no more space or there is nothing more that needs to be written in.
	while(buffersize > 0)	//While there is still data left in the buffer
	{
		for(int i = 0; i < ssector.totalsectorsallocated; i++)
		{		//For every entry in the fidat
			if(fidalloctable[i].occupyingfid == fp.fid && fidalloctable[i].sectorspace == ((filelocation/SECTOR_SIZE) + 1))	//If its in the right sector to be written to
			{		//Go to the start of the sector
				
				fseek(fp.filestream, (long)(SECTOR_SIZE * (ssector.dataareastart + i)), SEEK_SET);
				fseek(fp.filestream, (long)(filelocation % SECTOR_SIZE), SEEK_CUR);
				if(buffersize > SECTOR_SIZE - (filelocation % SECTOR_SIZE))	//if this is not the first sector in the file being written to
				{
					fwrite(buffer, sizeof(char), SECTOR_SIZE - (filelocation % SECTOR_SIZE), fp.filestream);
					buffersize -= SECTOR_SIZE - (filelocation % SECTOR_SIZE);
					filelocation += SECTOR_SIZE - (filelocation % SECTOR_SIZE);	
				}
				else	//if it is
				{
					fwrite(buffer, sizeof(char), buffersize, fp.filestream);
					buffersize = 0;				
				}
	
				break;
			}	 
		}	
	}
	

		
}

/*
	Reads from the file, and returns its contents
	FilePointer fp: The stream to the file
	int filelocation: The location in the file thats being read from
	int buffersize: The size of the buffer being read from
	Returns: The contents of the file being read from
*/
char *readFile(struct FilePointer fp, int filelocation, int buffersize)
{
	int thesize = buffersize;
	char *returnedcontents;
	returnedcontents = (char*)malloc(sizeof(char) * buffersize + 1);

	int bufferindex = 0;

	//Now that the sectors have been allocated, the may be read from.
	//First sector read from is "int filelocation / SECTOR_SIZE". This is found by looping through the Fidat until an entries is found with the File ID of the file being written to, and the first sector.
	//That sector gets written to until either there is no more space or there is nothing more that needs to be written in.
	while(buffersize > 0)	//While there is still data left in the buffer
	{
		for(int i = 0; i < ssector.totalsectorsallocated; i++)
		{		//For every entry in the fidat
			if(fidalloctable[i].occupyingfid == fp.fid && fidalloctable[i].sectorspace == ((filelocation/SECTOR_SIZE) + 1))	//If its in the right sector to be written to
			{		//Go to the start of the sector
				
				fseek(fp.filestream, (long)(SECTOR_SIZE * (ssector.dataareastart + i)), SEEK_SET);
				fseek(fp.filestream, (long)(filelocation % SECTOR_SIZE), SEEK_CUR);

				char *sectorread;
				
				if(buffersize > SECTOR_SIZE - (filelocation % SECTOR_SIZE))	//If the sector is not being read from halfway through
				{	//Allocate memory for the contents to be retrieved
					sectorread = (char *)malloc(sizeof(char) * SECTOR_SIZE - (filelocation % SECTOR_SIZE));		//Read them into memory
					fread(sectorread, sizeof(char), SECTOR_SIZE - (filelocation % SECTOR_SIZE), fp.filestream);
					buffersize -= SECTOR_SIZE - (filelocation % SECTOR_SIZE);
					filelocation += SECTOR_SIZE - (filelocation % SECTOR_SIZE);	
					bufferindex += SECTOR_SIZE - (filelocation % SECTOR_SIZE);
				}
				else	//if it is
				{
					sectorread = (char *)malloc(sizeof(char) * buffersize);
					fread(sectorread, sizeof(char), buffersize, fp.filestream);
					buffersize = 0;				
				}

				strcat(returnedcontents,sectorread);	
				break;
			}	 
		}	
	}

	returnedcontents[thesize] = '\0';
	return returnedcontents;
}


/*
	User enters the name of the file, and and a data structure containing its FID and a stream to the file system is returned. The file ID is used to locate the file for read and write operations in other methods.
	Parameters: char *filename: The name of the file being opened
	Returns: A FilePointer that holds the file id of the file being represented, and a stream to the file system.
*/
struct FilePointer *openFile(char *filename)
{
	struct FilePointer *fileopened;

	//Allocating memory for the pointer to the filesystem
	fileopened = (struct FilePointer *)malloc(sizeof(struct FilePointer));

	//Getting the first 8 characters of the name of the file that the user is searching for, as the filesystem can only hold 8 characters for a file name
	char filesearch[33];
	for(int j = 0; j < 32; j++)
	{
		if(filename[j] == '\0')
		{
			filesearch[j] = filename[j];
			break;
		}
		else
			filesearch[j] = filename[j];
	}
	filesearch[32] = '\0';

	int foundfile = 0;
	
	//Searches through every entry in the root directory until it finds a matching file name
	for(int i = 0; i < ssector.numoffiles; i++)
	{
		if(strcmp(rootdirectory[i].filename, filesearch) == 0)
		{
			foundfile++;	//Changes variable to indicate that the file has been found
			fileopened->fid = rootdirectory[i].fileid;	//Sets the file id
			break;		//Exits the loop
		} 
	}

	//Informs the user if the file could be found.
	if(foundfile == 0)
	{
		printf("file could not be found\n");
	}
	else
		printf("file was opened\n");

	//Opens a read/write stream to the file system
	fileopened->filestream = fopen(filesystemname, "r+");

	return fileopened;
}

/*
	Closes the stream to the data structure stored in fileclosing.
	FilePointer fileclosing: The data structure whose stream is being closed
	Returns nothing.
*/
void closeFile(struct FilePointer *pointer)
{
	fclose(pointer->filestream);
}

