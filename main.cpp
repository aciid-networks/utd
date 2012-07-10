// --------------------------------------------------------------
//  Program: synchpath
//  Author: Jose Luis Blanco Claraco <jlblanco@ctima.uma.es>
//  License: GPL v3
// ---------------------------------------------------------------

#define PROG_VERSION "0.4"

#define _CRT_SECURE_NO_DEPRECATE 1

#include <stdio.h>
#include <string>
#include <queue>
#include <iostream>
#include <cstdio>
#include <time.h>
#include <fstream>

#include "fileUtils.h"

using namespace std;

// ---------------------------------------------
//   Definitions
// ---------------------------------------------

// The *main* function. Returns 0 on OK, -1 on errors.
int processTask( const string& originPath, const string& targetPath);


// ---------------------------------------------
//   Variables
// ---------------------------------------------

// Stats:
long   comparedFiles=0, copiedFiles=0, erasedFiles=0, newFiles=0;

struct TOptions
{
	bool    deleteFiles;
	bool    verbose;
	int   	timeWindow;
	bool	waitKey;
	bool    skipSym;
} options;

// For logging
ofstream logFile;


void showUsage()
{
	cerr << "synchpath " << PROG_VERSION << " (Build " << __DATE__ << ")"<< endl;
	cerr << "Usage: syncpath [-d] [-l logfile] [-L logfile] [-p] [-v] [-s] [-w number] SOURCE_PATH TARGET_PATH" << endl;
	cerr << " For bug reports: <jlblanco@ctima.uma.es>" << endl << endl;
	cerr << "Parameters:" << endl;
	cerr << "  -d            Allow deletion of files in target not found in the source" << endl;
	cerr << "  -l/L logfile  Write/Append log information to a given file." << endl;
	cerr << "  -p            Pause at the end until key pressed" << endl;
	cerr << "  -v            Verbose output" << endl;
	cerr << "  -s            Skip symbolic links" << endl;
	cerr << "  -w number     Time window in seconds for updating a file (eg. 5) see README" << endl;
	cerr << "  SOURCE_PATH   Existing path to copy files from" << endl;
	cerr << "  TARGET_PATH   Existing path to copy files to" << endl;
}


void exitProgram(int errCode, bool waitKey)
{
	if (options.waitKey) waitForKey();
	exit(errCode);
}

// ---------------------------------------------
//   MAIN: Parse arguments & invoke main code
// ---------------------------------------------

int main(int argc, char **argv)
{
    // Parse arguments:
    // -----------------------------
    options.deleteFiles = false;
    options.verbose     = false;
    options.timeWindow  = 0;
    options.waitKey     = false;
    options.skipSym     = false;


    if (argc<3)
    {
		showUsage();
        exitProgram(-1,false);
    }

	int argIdx=1;
	while (argIdx<argc && argv[argIdx][0]=='-')
	{
		switch (argv[argIdx][1])
		{
		case 'v':
			options.verbose=true;
			break;

		case 'p':
			options.waitKey=true;
			break;

		case 's':
			options.skipSym=true;
			break;

		case 'w':
			{
				int i=-1;
				sscanf(argv[++argIdx],"%i", &i);
				if (i<=0)
				{
					cerr << "Invalid -w parameter. Run syncpath without params to see usage." << endl;
					exitProgram(-1);
				}
				options.timeWindow=i;
			}
			break;

		case 'l':
		case 'L':
			{
				bool  isAppend = argv[argIdx][1]=='L';
				if (argIdx>=(argc-1))
				{
					cerr << "You must provide a valid log file name. Run syncpath without params to see usage." << endl;
					exitProgram(-1);
				}

				// Try to open the file:
				char *fileName = argv[++argIdx];
				logFile.open( fileName,  isAppend ? ios::out|ios::app : ios::out );
				if (!logFile.is_open())
				{
					cerr << "[ERROR] Couldn't open file " << fileName << " for logging..." << endl;
					exitProgram(-1);
				}
			}
			break;

		case 'd':
			options.deleteFiles=true;
			break;

		default:
			cerr << "Unknown command-line option: " << argv[argIdx] << endl;
			showUsage();
			exitProgram(-1);
		};
		argIdx++;
	}

	string originPath=string(argv[argIdx++]);
	string targetPath=string(argv[argIdx++]);
	time_t startTime = time(NULL);

	if (logFile.is_open())
	{
		logFile << endl << "====================================================================" << endl;
		logFile << " Logfile for: synchpath " << PROG_VERSION << " (Build " << __DATE__ << ")"<< endl;
	}

	SHOW_AND_LOG_MSG2("SOURCE PATH: ",originPath);
	SHOW_AND_LOG_MSG2("TARGET PATH: ",targetPath);
	SHOW_AND_LOG_MSG2("Deletion allowed: ", options.deleteFiles ? "Yes":"No" );
	SHOW_AND_LOG_MSG2("Follow symbolic links: ", !options.skipSym ? "Yes":"No" );
	SHOW_AND_LOG_MSG2("Time window for update: ", options.timeWindow );
	SHOW_AND_LOG_MSG2("Starting synchronization at ",ctime(&startTime) );

	// Check existence of paths:
	if (!pathExists(originPath))
	{
		SHOW_AND_LOG_ERROR("CRITICAL ERROR: Cannot access to source path");
		exitProgram(-1);
	}

	if (!pathExists(targetPath))
	{
		SHOW_AND_LOG_ERROR("CRITICAL ERROR: Cannot access to target path");
		exitProgram(-1);
	}

	// Create the initial task: the rest of paths will be examined recursively:
	if (processTask( originPath, targetPath ))
	{
		cerr << endl;
		exitProgram(-1);      // An error occurred
	}

	// Final report:
	time_t endTime = time(NULL);
	double ellapsedSecs = difftime( endTime, startTime);

	SHOW_AND_LOG_MSG(endl);
	SHOW_AND_LOG_MSG2("Synchronization finish at ",ctime(&endTime));
	SHOW_AND_LOG_MSG2("Ellapsed time : ", formatTime((long)ellapsedSecs) );
	SHOW_AND_LOG_MSG2("Compared files: ", comparedFiles );
	SHOW_AND_LOG_MSG2("Updated files : ", copiedFiles );
	SHOW_AND_LOG_MSG2("New files     : ", newFiles );
	SHOW_AND_LOG_MSG2("Removed files : ", erasedFiles );

	SHOW_AND_LOG_MSG("END!");
	if (logFile.is_open())
		logFile << "====================================================================" << endl;

	exitProgram(0);   // OK
}

#define  SHOW_CUR_DIRECTORY_ONCE \
    if (!dirTitlePrint) \
    { \
		SHOW_AND_LOG_MSG2("= SYNCHRONIZING: ",originPath); \
		SHOW_AND_LOG_MSG2("============ TO: ",targetPath); \
        dirTitlePrint = true; \
    }

/* ---------------------------------------------------------------

		 The *main* function. Returns 0 on OK, -1 on errors.

   --------------------------------------------------------------- */
int processTask( const string& originPath, const string& targetPath)
{
	bool dirTitlePrint = false;

	if (options.verbose)
	{
		SHOW_AND_LOG_MSG2("=== SYNCHING: ",originPath);
		SHOW_AND_LOG_MSG2("========= TO: ",targetPath);
		dirTitlePrint = true;
	}

	TDirListing orgList,trgList;    // Lists of the files:

	if (!dirExplorer(originPath,orgList)) return -1;
	if (!dirExplorer(targetPath,trgList)) return -1;

	// Make a list for the state of each file in origin:
	// The numeric codes are:
	//    0: INIT   Only used at start up, at the end no file should remain like this.
	//    1: DONE   The file has been copied, etc...
	// ------------------------------------------------------------------------------------------
	vector<int>  orgState(orgList.size(),0);

	// --------------------------------------------------------------------
	// STEP 1: Compare for file name matches:
	//  For each file match:
	//    - Same timestamps? --> state=DONE, delete target from list
	//    - Diff timestamps? --> state=DONE, copy file, update timestamps
	//  For each directory match:
	//    - Add to task list!
	// --------------------------------------------------------------------
	vector<int>::iterator   itState;
	TDirListing::iterator   itSrc,itTrg;

	comparedFiles+=orgList.size();

	for (itSrc=orgList.begin(),itState=orgState.begin();itSrc!=orgList.end();itSrc++,itState++)
	{
		// Look for a match in target list:
		for (itTrg=trgList.begin();itTrg!=trgList.end();itTrg++)
			if ( !strcmp( itSrc->name.c_str(),itTrg->name.c_str() ) )
				break;

		// A match has been found if iterator is not end:
		if (itTrg!=trgList.end())
		{
			// Are directories?
			if (itSrc->isDir && itTrg->isDir )
			{
				// Both are dirs: Explore recursively:
				if (!( (itSrc->isSymLink || itTrg->isSymLink) && options.skipSym ))  // Skip symbolic links?
				{
					if (0!=processTask( itSrc->wholePath,itTrg->wholePath) )
						return -1;
				}
			} // Else, keep on...
			else
			if (itSrc->isDir && !itTrg->isDir )
			{
				if (!( (itSrc->isSymLink || itTrg->isSymLink) && options.skipSym ))  // Skip symbolic links?
				{
					SHOW_CUR_DIRECTORY_ONCE;
					SHOW_AND_LOG_MSG5_NOENDL("Copy: DIR ", itSrc->name," --> FILE ",itTrg->name,". Copying...");

					// Copy dir to a file!: Remove target & copy:
					if (options.deleteFiles)
					{
						if (!deleteFileOrDir( itTrg->wholePath, &erasedFiles ))
						{
							SHOW_AND_LOG_ERROR2("\nERROR: Cannot delete file: ",itTrg->wholePath);
							return -1;
						}
					}
					else
					{
						SHOW_AND_LOG_ERROR("\nERROR: The command line option for deleting files is disabled and it is needed to delete the existing target file. Aborting.");
						return -1;
					}

					// Recursive copy of a directory:
					if (!copyDirectory( itSrc->wholePath, itTrg->wholePath, &newFiles))
					{
						SHOW_AND_LOG_ERROR("ERROR: Couldn't copy.");
						return -1;
					}
					else SHOW_AND_LOG_MSG("OK");
				}
				else
				{
					if (options.verbose)
					{
						SHOW_CUR_DIRECTORY_ONCE;
						SHOW_AND_LOG_MSG2("Skipping symbolic link: ",itSrc->name);
					}
				}
			}
			else
			if (!itSrc->isDir && itTrg->isDir )
			{
				if (!( (itSrc->isSymLink || itTrg->isSymLink) && options.skipSym ))  // Skip symbolic links?
				{
					SHOW_CUR_DIRECTORY_ONCE;
					SHOW_AND_LOG_MSG5_NOENDL("Copy: FILE ",itSrc->name," --> DIR ",itTrg->name,". Copying...");

					// Copy file in place of a dir!: Remove target & copy:
					if (options.deleteFiles)
					{
						if (!deleteFileOrDir(itTrg->wholePath,&erasedFiles))
						{
							SHOW_AND_LOG_ERROR2("\nERROR: Cannot delete directory :",itTrg->wholePath);
							return -1;
						}
					}
					else
					{
						SHOW_AND_LOG_ERROR("\nERROR: The command line option for deleting files is disabled and it is needed to delete the existing target directory. Aborting.");
						return -1;
					}

					if (!copyFile( itSrc->wholePath, itTrg->wholePath ))
					{
						SHOW_AND_LOG_ERROR("ERROR: Couldn't copy.");
						return -1;
					}
					else SHOW_AND_LOG_MSG("OK");

					newFiles++;
				}
				else
				{
					if (options.verbose)
					{
						SHOW_CUR_DIRECTORY_ONCE;
						SHOW_AND_LOG_MSG2("Skipping symbolic link: ",itSrc->name);
					}
				}
			}
			else
			{
				if (!( (itSrc->isSymLink || itTrg->isSymLink) && options.skipSym ))  // Skip symbolic links?
				{
					// Both are files!!
					double difTime = difftime(itSrc->modTime,itTrg->modTime);

					if (options.verbose)
					{
						SHOW_AND_LOG_MSG2("Diff time for : ",itSrc->name);
						SHOW_AND_LOG_MSG2(" is (seconds) : ", difTime);
						SHOW_AND_LOG_MSG2(" or (formated): ", formatTime((long)difTime));
					}

					// A positive difference means we have to update the file:
					if (difTime>options.timeWindow)
					{
						SHOW_CUR_DIRECTORY_ONCE;
						SHOW_AND_LOG_MSG5_NOENDL( "Update: FILE ",itSrc->name," (",formatTime((long)difTime)," newer).Copying...");

						if (logFile.is_open())
						{
							logFile << " Source file timestamps: " << endl;
							logFile << "   Access time      : " << "(" <<(long)itSrc->accessTime << ")" << ctime(&itSrc->accessTime );
							logFile << "   Modification time: " << "(" <<(long)itSrc->modTime << ")" << ctime(&itSrc->modTime);
							logFile << " Target file timestamps: " << endl;
							logFile << "   Access time      : " << "(" <<(long)itTrg->accessTime << ")" << ctime(&itTrg->accessTime ) ;
							logFile << "   Modification time: " << "(" <<(long)itTrg->modTime << ")" << ctime(&itTrg->modTime) ;
							logFile << " Difference (difftime): " << difftime(itSrc->modTime,itTrg->modTime) << endl;
						}

						if (!copyFile( itSrc->wholePath, itTrg->wholePath ))
						{
							SHOW_AND_LOG_ERROR("ERROR: Couldn't copy.");
							return -1;
						}
						else SHOW_AND_LOG_MSG("OK");

						copiedFiles++;
					}
				}
				else
				{
					if (options.verbose)
					{
						SHOW_CUR_DIRECTORY_ONCE;
						SHOW_AND_LOG_MSG2("Skipping symbolic link: ",itSrc->name);
					}
				}

			}

			// Mark source as DONE:
			*itState=1;

			// Remove target from the list:
			trgList.erase( itTrg );

		} // end if there is a match

	} // end for src

	// --------------------------------------------------------------------
	// STEP 2: Trg files still in the list: they are to be deleted
	// --------------------------------------------------------------------
	if (options.deleteFiles && trgList.size())
	{
		SHOW_CUR_DIRECTORY_ONCE;
		// Remove remaining entries:
		for (itTrg=trgList.begin();itTrg!=trgList.end();itTrg++)
		{
			SHOW_AND_LOG_MSG5_NOENDL( itTrg->isDir ? "Remove: DIR ": "Remove: FILE ", itTrg->name, ". Removing...","","");
			if (!deleteFileOrDir( itTrg->wholePath, &erasedFiles ))
			{
				SHOW_AND_LOG_ERROR("ERROR: Couldn't delete.");
				return -1;
			}
			else SHOW_AND_LOG_MSG("OK");
		}
	}
	else
	{
		if (trgList.size())
			SHOW_AND_LOG_MSG3 ("WARNING: ",trgList.size()," files should be deleted in the target but the delete option has not been added to the command line.");
	}

	// --------------------------------------------------------------------
	// STEP 3: Org files with state=INIT, copy since there are new files
	// --------------------------------------------------------------------
	string  trgPathSlash( targetPath + string("/") );
	for (itSrc=orgList.begin(),itState=orgState.begin();itSrc!=orgList.end();itSrc++,itState++)
	{
		if (*itState == 0)
		{
			if (!( itSrc->isSymLink && options.skipSym ))  // Skip symbolic links?
			{
				SHOW_CUR_DIRECTORY_ONCE;
				// Copy a new file/directory:
				SHOW_AND_LOG_MSG5_NOENDL( itSrc->isDir ? "New: DIR ": "New: FILE ", itSrc->name,". Copying...","","");
				if (itSrc->isDir)
				{
					// Copy directory:
					if (!copyDirectory( itSrc->wholePath, trgPathSlash+string(itSrc->name), &newFiles))
					{
						SHOW_AND_LOG_ERROR("ERROR: Couldn't copy.");
						return -1;
					}
					else SHOW_AND_LOG_MSG("OK");
				}
				else
				{
					if (!copyFile( itSrc->wholePath, trgPathSlash+string(itSrc->name) ))
					{
						SHOW_AND_LOG_ERROR("ERROR: Couldn't copy.");
						return -1;
					}
					else 
					{
						SHOW_AND_LOG_MSG("OK");
						newFiles++;
					}
				}
			}
			else
			{
				if (options.verbose)
				{
					SHOW_CUR_DIRECTORY_ONCE;
					SHOW_AND_LOG_MSG2("Skipping symbolic link: ",itSrc->name);
				}
			}
		} // end if the file is new
	} // end for each itSrc

	return 0; // OK
}


