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

// Miscellaneous non-member functions in rxdock namespace
#ifndef _RBT_H_
#define _RBT_H_

#include "rxdock/support/Export.h"

#include <istream>
#include <map>
#include <ostream>
#include <vector>

namespace rxdock {

// Segment is a named part of an Model (usually an intact molecule)
// For now, a segment is defined as just an String
// SegmentMap holds a map of (key=unique segment name, value=number of atoms
// in segment)
typedef std::string Segment;
typedef std::map<Segment, unsigned int> SegmentMap;
typedef SegmentMap::iterator SegmentMapIter;
typedef SegmentMap::const_iterator SegmentMapConstIter;

////////////////////////////////////////////////////////////////
// RESOURCE HANDLING FUNCTIONS
//
// GetRoot - returns value of RBT_ROOT env variable
RBTDLL_EXPORT std::string GetRoot();
// GetHome - returns value of RBT_HOME env variable
//(or HOME if RBT_HOME is undefined)
std::string GetHome();
// GetProgramName - returns program name
RBTDLL_EXPORT std::string GetProgramName();
// GetMetaDataPrefix - returns meta data prefix
RBTDLL_EXPORT std::string GetMetaDataPrefix();
// GetCopyright - returns legalese statement
RBTDLL_EXPORT std::string GetCopyright();
// GetProgramVersion - returns current library version
RBTDLL_EXPORT std::string GetProgramVersion();
// GetProduct - returns library product name
RBTDLL_EXPORT std::string GetProduct();
// GetTime - returns current time and date as an String
std::string GetTime();
// GetCurrentWorkingDirectory - returns current working directory
RBTDLL_EXPORT std::string GetCurrentWorkingDirectory();
//
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// FILE/DIRECTORY HANDLING FUNCTIONS
//
// GetDirName
// Returns the full path to a subdirectory in the rDock directory structure
//
// For example, if RBT_ROOT environment variable is ~dave/ribodev/molmod/ribodev
// then GetDirName("data") would return ~dave/ribodev/molmod/ribodev/data/
//
// If RBT_ROOT is not set, then GetDirName returns ./ irrespective of the
// subdirectory asked for. Thus the fall-back position is that parameter files
// are read from the current directory if RBT_ROOT is not defined.
std::string GetDirName(const std::string &strSubdir = "");

// GetDataFileName
// As GetDirName but returns the full path to a file in the rDock directory
// structure
RBTDLL_EXPORT std::string GetDataFileName(const std::string &strSubdir,
                                          const std::string &strFile);

// GetFileType
// Returns the string following the last "." in the file name.
// e.g. GetFileType("receptor.psf") would return "psf"
std::string GetFileType(const std::string &strFile);

// GetDirList
// Returns a list of files in a directory (strDir) whose names begin with
// strFilePrefix (optional) and whose type is strFileType (optional, as returned
// by GetFileType)
std::vector<std::string> GetDirList(const std::string &strDir,
                                    const std::string &strFilePrefix = "",
                                    const std::string &strFileType = "");
//
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// CONVERSION ROUTINES
//
// Converts (comma)-delimited string of segment names to segment map
RBTDLL_EXPORT SegmentMap ConvertStringToSegmentMap(
    const std::string &strSegments, const std::string &strDelimiter = ",");
// Converts segment map to (comma)-delimited string of segment names
std::string ConvertSegmentMapToString(const SegmentMap &segmentMap,
                                      const std::string &strDelimiter = ",");

// Returns a segment map containing the members of map1 which are not in map2
// I know, should really be a template so as to be more universal...one day
// maybe. Or maybe there is already an STL algorithm for doing this.
SegmentMap SegmentDiffMap(const SegmentMap &map1, const SegmentMap &map2);

// DM 30 Mar 1999
// Converts (comma)-delimited string to string list (similar to
// ConvertStringToSegmentMap, but returns list not map)
RBTDLL_EXPORT std::vector<std::string>
ConvertDelimitedStringToList(const std::string &strValues,
                             const std::string &strDelimiter = ",");
// Converts string list to (comma)-delimited string (inverse of above)
std::string
ConvertListToDelimitedString(const std::vector<std::string> &listOfValues,
                             const std::string &strDelimiter = ",");

// Detect terminal width and wrap text to that width
std::string WrapTextToTerminalWidth(const std::string &text);

//
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
// I/O ROUTINES
//
// PrintStdHeader
// Print a standard header to an output stream (for log files etc)
// Contains copyright info, library version, date, time etc
// DM 19 Feb 1999 - include executable information
RBTDLL_EXPORT std::ostream &
PrintStdHeader(std::ostream &s, const std::string &strExecutable = "");

RBTDLL_EXPORT std::ostream &
PrintBibliographyItem(std::ostream &s, const std::string &publicationKey = "");

// Helper functions to read/write chars from iostreams
// Throws error if stream state is not Good() before and after the read/write
// It appears the STL std::ios_base exception throwing is not yet implemented
// at least on RedHat 6.1, so this is a temporary workaround (yeah right)
//
// DM 25 Sep 2000 - with MIPSPro CC compiler, streamsize is not defined,
// at least with the iostreams library we are using, so typedef it here
// it is not in gcc 3.2.1 either SJ
//#ifdef __sgi
typedef int streamsize;
//#endif

RBTDLL_EXPORT void WriteWithThrow(std::ostream &ostr, const char *p,
                                  streamsize n);
RBTDLL_EXPORT void ReadWithThrow(std::istream &istr, char *p, streamsize n);

} // namespace rxdock

#endif //_RBT_H_
