/***********************************************************************
 * The rDock program was developed from 1998 - 2006 by the software team
 * at RiboTargets (subsequently Vernalis (R&D) Ltd).
 * In 2006, the software was licensed to the University of York for
 * maintenance and distribution.
 * In 2012, Vernalis and the University of York agreed to release the
 * program as Open Source software.
 * This version is licensed under GNU-LGPL version 3.0 with support from
 * the University of Barcelona.
 * http://rdock.sourceforge.net/
 ***********************************************************************/

#ifndef _RBTDIRECTORYSOURCE_H_
#define _RBTDIRECTORYSOURCE_H_

#include "rxdock/Config.h"
#include "rxdock/FileError.h" // for exception types
#include <dirent.h>           // for scandir()
#include <fstream>            // ditto
#include <sys/stat.h>         // for stat()
#include <sys/types.h>

namespace rxdock {

const unsigned int PATH_SIZE = 1024; // max path width
const unsigned int LINE_WIDTH = 256; // max num of chars in a line
const unsigned int ALL = 0;          // read all files in directory

/**
 * DirectorySource
 * Reads all (or the given number) of files into a <vector>
 * that are in a directory. To extend with other file formasts
 * include other ReadFiles () methods
 */
class DirectorySource {
protected:
  struct stat fStat; // stat struct for file access check
#ifdef COMMENT
#ifdef linux
  struct dirent **fNameList; // file name list in directory
#endif
#ifdef sun                   // for gcc
  struct direct **fNameList; // file name list in directory
#endif
#endif                                // COMMENT
  int fNum;                           // number of files in the dir
  struct dirent **fNameList;          // file name list in directory
  std::ifstream inFile;               // an actual file to read
  std::string thePath;                // leading path
  void CheckDirectory(std::string &); // to check directory access

public:
  DirectorySource(const std::string &);
  virtual ~DirectorySource();

  static const std::string _CT;

  /**
   * ReadFiles should be re-defined by derived classes to match argument
   * Ie ReadFile(std::vector<string> anStrVect) reads file and stores result in
   * anStrVect, or ReadFile(std::map< ... > aMap) in aMap, etc.
   *
   */
  // virtual void ReadFiles()=0;
  // virtual void ParseLines()=0;
};

} // namespace rxdock

#endif // _RBTDIRECTORYSOURCE_H_
