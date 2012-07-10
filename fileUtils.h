// --------------------------------------------------------------
//  Program: synchpath
//  File: dirExplorer.h
//  Author: Jose Luis Blanco Claraco <jlblanco@ctima.uma.es>
//  License: GPL v3
// ---------------------------------------------------------------

#ifndef _DIR_EXPLORER_H
#define _DIR_EXPLORER_H

#if defined(_WIN32) || defined(_WIN32_)  || defined(WIN32)
    #define IS_WINDOWS
#else
#if defined(unix) || defined(__unix__) || defined(__unix)
    #define IS_LINUX
#else
    #error Unsupported Implementation (Found neither _WIN32_ nor __unix__)
#endif
#endif

#include <string>
#include <deque>
#include <cstdio>
#include <time.h>
#include <fstream>

using namespace std;


/** Returns true if path exists and we have permissions to read it.
  */
bool pathExists(const std::string &path);

/** Returns true if path is a directory.
  */
bool isDirectory( const std::string &fileName);

/** Exits the program with a given error code
  */
void exitProgram(int errCode, bool waitKey=true);

/** copyFileAttributes (WIN only) - Returns true on success, false on error.
  */
bool copyFileAttributes(const string& org, const string &trg);


/** Replaces / with \.
  */
string winBackSlashes(const string&str);

/** Returns true on success (retVal==0), false on error (retVal!=0).
  */
bool SHOW_ERROR_IF_ANY( int retVal );


/** copyFile - Copy a file to a target filename (the source & target MUST be filenames, not directories) 
  *  This function preserves the original file timestamps - Returns true on success, false on error.
  */
bool copyFile( const std::string &org, const std::string &trg );

/** copyDirectory - Recursively copy files and directories (the source & target MUST be directories - if target directory does not exist it will be created) 
  *  This function preserves the original file timestamps - Returns true on success, false on error.
  *  If "copyCount" is not NULL, its value will be INCREMENTED (initialize before calling!) by the number of copied files/directories.
  */
bool copyDirectory( const std::string &org, const std::string &trg, long *copyCount = NULL );

/**  deleteFileOrDir - Delete a file or a directory - Returns true on success, false on error.
  *   If "deleteCount" is not NULL, it returns the total amount of files/directories deleted in the whole tree (INITIALIZE THIS VARIABLE TO ZERO BEFORE CALL!!).
  */
bool deleteFileOrDir( const std::string &filename, long * deleteCount = NULL );

/** Creates a new directory (does not fail if if already exists) - In linux, the directory is created with RWX permisions for everyone - Returns true on success, false on error.
  */
bool createDirectory( const std::string &path );

/** changeFileTimes - Returns true on success, false on error.
  */
bool changeFileTimes( const std::string &fileName, time_t accessTime, time_t modifTime );

struct TFileInfo
{
    std::string     name,wholePath;
    time_t          accessTime,modTime;
    bool            isDir, isSymLink;
};

typedef std::deque<TFileInfo> TDirListing;

/** Explores a given path and return the list of its contents. Returns true if OK, false on any error.
  */
bool dirExplorer( const std::string &inPath, TDirListing &outList );

/** Waits for a key
  */
void waitForKey();

/** Returns a string representation of a time ellapse
  */
string formatTime( long secs );


extern ofstream logFile;

#define SHOW_AND_LOG_ERROR(M) { cerr << (M) << endl; if (logFile.is_open()) logFile << (M) << endl; }
#define SHOW_AND_LOG_ERROR2(M1,M2) { cerr << (M1) << (M2) << endl; if (logFile.is_open()) logFile << (M1) << (M2) << endl; }
#define SHOW_AND_LOG_ERROR2_NOENDL(M1,M2) { cerr << (M1) << (M2); if (logFile.is_open()) logFile << (M1) << (M2); }
#define SHOW_AND_LOG_ERROR4(M1,M2,M3,M4) { cerr << (M1) << (M2) << (M3) << (M4) << endl; if (logFile.is_open()) logFile << (M1) << (M2) << (M3) << (M4) << endl; }

#define SHOW_AND_LOG_MSG(M) { cout << (M) << endl; if (logFile.is_open()) logFile << (M) << endl; }
#define SHOW_AND_LOG_MSG_NOENDL(M) { cout << (M); if (logFile.is_open()) logFile << (M); }
#define SHOW_AND_LOG_MSG2(M1,M2) { cout << (M1) << (M2) << endl; if (logFile.is_open()) logFile << (M1) << (M2) << endl; }
#define SHOW_AND_LOG_MSG3(M1,M2,M3) { cout << (M1) << (M2) << (M3) << endl; if (logFile.is_open()) logFile << (M1) << (M2) << (M3) << endl; }
#define SHOW_AND_LOG_MSG4(M1,M2,M3,M4) { cout << (M1) << (M2) << (M3) << (M4) << endl; if (logFile.is_open()) logFile << (M1) << (M2) << (M3) << (M4) << endl; }

#define SHOW_AND_LOG_MSG5_NOENDL(M1,M2,M3,M4,M5) { cout << (M1) << (M2) << (M3) << (M4) << (M5); if (logFile.is_open()) logFile << (M1) << (M2) << (M3) << (M4) << (M5); }


#endif

