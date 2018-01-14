#include <stdlib.h>
#include "fidat.h"

#define defaultdrive "Journal"

void findCommand(char **parsedcommand);
char **commandParser(char *unparsedcommand);

struct FilePointer *currentfile;

int numofwords;
int streamisopen = 0;

/*
	The main method. It makes sure that there is a file to be read from, initializes the file system, prints out messages that tell the user how to use the different commands, obtains input from the user, and sends out that input to the other functions in the program.
	Parameters: The name of file that the file system is located in. The program will terminate if this is not available.
*/
int main(int argc, char *argv[])
{
	numofwords = 0;
	if(argc < 2)		//Program ends if there is no file system to be opened.
		initializeFileSystem(defaultdrive);	
	else
		initializeFileSystem(argv[1]);	//Initializes the file system


	char *input = (char *) malloc(sizeof(char) * 100);	//Allocates memory for user input
	char **parsedinput;
	
	printf("Welcome to the FIDAT file system!\n");	//A list of messages to help the user use fidattest
	printf("Before you begin, you must open a file\n");
	printf("The commands are:\n");
	printf("\nIn order to create a file\n");
	printf("create <string representing file name> <integer representing if its a file or a folder. 0 means folder, any non zero number means file>\n");
	printf("\nIn order to read from the file\n");
	printf("read <integer representing start of the location in the file being read from> <integer representing how many bytes are to be read>\n");
	printf("\nIn order to delete the file\n");
	printf("delete\n");
	printf("\nIn order to write data into the file\n");
	printf("write <integer representing the location in the file being written to> <the word you are reading into the file>");
	printf("\nIn order to view a list of all previously created files\n");	
	printf("show\n");
	printf("\nIn order to open a stream to a file. When a new file stream is opened, the older one is overwritten.\n");
	printf("open <name of the file being opened>\n");
	printf("\nIn order to close the stream to the file currently being accessed\n");
	printf("close\n");
	printf("\nYou currently have no open files\n");

	do		//Stays on a loop until the user types in the command quit
	{
		fgets(input, sizeof(char) * 100, stdin);	//Getting input from the terminal
		input[strlen(input) - 1] = '\0';

		parsedinput = commandParser(input);		//Turns a sentence into a group of words
		if(numofwords > 0)				//If the user did something other than just press enter
			findCommand(parsedinput);			//Finds the proper command that the user put in.
	}while(strcmp(input, "quit") != 0);


	for(int i = 0; i < ssector.numoffiles; i++)
	{
		printf("File ID: %d\nFile Name : %s\nFile Time: %ld\nFile Size: %lu \nNumber of Sectors: %d\n", rootdirectory[i].fileid, rootdirectory[i].filename, (long)rootdirectory[i].creationtime, (unsigned long)rootdirectory[i].filesize, rootdirectory[i].numberofsectors);
	}

	return 1;
}

/*
	The function that allows the user to interact with the filesystem.
	Looks at the first word that the user typed in, and tries to figure out which IOCS function to call based on that.
	If the command does not exist, then nothing happens.
	Parameters: char **parsedcommand- The command and possible arguements for the command.
	Returns: Nothing. Prints out messages to the terminal based on the type of command that was entered in.
*/
void findCommand(char**parsedcommand)
{
	if(strcmp(parsedcommand[0], "read") == 0)	//If the user entered in the read command.
	{
		if(numofwords >= 3 && streamisopen != 0)
		{
			int filelocation = atoi(parsedcommand[1]);	//Converts the string representation of a number into an integer
			int buffersize = atoi(parsedcommand[2]);
			char *message = readFile(*currentfile,filelocation, buffersize);	//Retrieves the requested data from the file system
			printf("%s\n", message);	//Prints out the message.
		}
		else
			printf("You are missing an arguement.\n");
	}
	else if(strcmp(parsedcommand[0], "write") == 0)	//The write command
	{
		if(numofwords >= 2 && streamisopen != 0)			//If they put in the name of a text file
		{

			FILE *writefile;		//Opening the file being used as a write command
			if(fopen(parsedcommand[1], "r") != NULL)		//Check to see if the file exists
				writefile = fopen(parsedcommand[1], "r");
			else
			{
				printf("You must enter in the name of a valid text file,\n");
				return;
			}

			int filelength;			//Getting the length of the file
			fseek(writefile, 0L, SEEK_END);
			filelength = ftell(writefile);	

			fseek(writefile, 0L, SEEK_SET);	//Getting the contents of the file
			char *writefilebuffer;
			writefilebuffer = (char *)malloc(sizeof(char) * filelength);
			fread(writefilebuffer, sizeof(char), filelength, writefile);

			writeFile(*currentfile, sizeof(char), filelength, parsedcommand[2]);		//Writing the message that the user entered in into the file
			printf("You have just written %s.\n", parsedcommand[2]);	//Lets the user know what they just wrote into the file and where
		}
		else
			printf("You are missing an arguement.\n");
	}
	else if(strcmp(parsedcommand[0], "open") == 0)	//The open command
	{
		if(numofwords >= 2)
		{
			currentfile = openFile(parsedcommand[1]);	//Opens a stream to a file based on the name of the file that the user entered in
			//printf("The currently open file is %s.\n", parsedcommand[1]);
			if(currentfile->fid != 0)
				streamisopen = 1;
		}
		else
			printf("You must specify the name of a file\n");	
	}
	else if(strcmp(parsedcommand[0], "close") == 0)	//The close command
	{
		if(streamisopen != 0)
		{
			closeFile(currentfile);		//Closes the currently opened file stream.
			printf("The file has been closed, there is currently no file open at this time\n");
			currentfile->fid = 0;
			streamisopen = 0;
		}
		else
			printf("No file to close.\n");
	}
	else if(strcmp(parsedcommand[0], "create") == 0)	//The create command
	{
		if(numofwords >= 2)
		{
			createFile(parsedcommand[1]);	//Creating the file
			printf("You have created the file.\n");	
		}
		else
			printf("You are missing the name of the file being created\n");
	}
	else if(strcmp(parsedcommand[0], "show") == 0)	//The create command
	{
		for(int i = 0; i < ssector.numoffiles; i++)
		{
			const time_t consttime = rootdirectory[i].creationtime;
			struct tm *filetime = localtime(&consttime);
			printf("File Name : %s\nFile Creation Date: %d/%d/%d\nFile Creation Time: %d:%d:%d\n", rootdirectory[i].filename, 1900 + filetime->tm_year, filetime->tm_mon, filetime->tm_mday, filetime->tm_hour, filetime->tm_min, filetime->tm_sec);
		}		
	}
	else if(strcmp(parsedcommand[0], "delete") == 0)	//The delete command
	{
		if(streamisopen != 0)
		{
			streamisopen = 0;
			deleteFile(*currentfile);		//Deletes the file from the filesystem
			closeFile(currentfile);		//Closes the file stream
			currentfile->fid = 0;
			printf("The file has been deleted.\n");
		}
		else
			printf("There is no file to delete.\n");	
	}
}

/*
	Function that takes in a sentence and turns that sentence into a series of words. All erroneous white spaces are eliminated.
	Parameters: char *inputstring- The string that will be converted into an array of words, the start and end of each word determined by the presence of white space around it.
	Returns: A two dimensional array based around pointers. The two dimensional array consists of an array of words.
*/
char **commandParser(char *inputstring)
{
	char **parsed = (char**)malloc(sizeof(char*));

	int stringlength = strlen(inputstring);
	int wordnum = 0;	//number of words in the parser
	int wordsize = 0;

	for(int i = 0; i < stringlength; i ++)		//loops through every character in the string
	{
		if(inputstring[i] != ' ' && inputstring[i] != '\t')	//if the character is not a white space
		{
			if(i == 0)	//if the first character is the start of a word, make room for the word
			{
				wordnum++;	//increases the number of known words in the command
				parsed = realloc(parsed, sizeof(char*) * wordnum);	//provides more space for the new word
				wordsize = 0;	//goes to the start of the word
				parsed[wordnum - 1] = (char*)malloc(sizeof(char));	//initializes the memory for the new word
			}
			else if(inputstring[i - 1] == ' ' || inputstring[i - 1] == '\t')
			{		//if the character is the start of a word	
				wordnum++;
				parsed = realloc(parsed, sizeof(char*) * wordnum);	//provides more space for the new word
				wordsize = 0;
				parsed[wordnum - 1] = (char*)malloc(sizeof(char));
			}


			wordsize++;	//makes room for the next letter of the word
			parsed[wordnum -1] = realloc(parsed[wordnum-1], sizeof(char) * wordsize);
			parsed[wordnum - 1][wordsize-1] = inputstring[i];	//stores the letter of the word

			if(i == stringlength - 1 )//if its the end of the word (if theres a whitespace next, or if its the last characer in string input), add a null terminator
			{
				parsed[wordnum -1] = realloc(parsed[wordnum-1], sizeof(char) * wordsize + 1);
				parsed[wordnum - 1][wordsize] = '\0';	//adds a null terminator to the end of the word

			}
			else if(inputstring[i + 1] == ' ' || inputstring[i + 1] == '\t')//if its the end of the word (if theres a whitespace next, or if its the last characer in string input), add a null terminator, corrupted characters appear at the end of the string sometimes otherwise
			{
				parsed[wordnum -1] = realloc(parsed[wordnum-1], sizeof(char) * wordsize + 1);
				parsed[wordnum - 1][wordsize] = '\0';	//adds a null terminator to the end of the word

			}
		}

	}
	parsed = realloc(parsed, sizeof(char*) * wordnum + 1);	//provides more space for the new word
	parsed[wordnum] = NULL;	//ends the array
	numofwords = wordnum;

	return parsed;

}
