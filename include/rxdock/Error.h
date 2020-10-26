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

// rxdock exception base class
// Errors can be thrown for unspecified errors, or derived classes can be
// used for particular exception types.
// Has the following methods:
//
// Name      - exception name (set by constructor)
// File      - source file where error occurred
// Line      - source line no.
// Message   - description of the error
// isOK      - if true, status is OK, not an error
// AddMessage- append new message to existing message, e.g. when rethrowing
// Insertion operator defined for dumping exception details to a stream

#ifndef _RBTERROR_H_
#define _RBTERROR_H_

#include <exception>
#include <sstream>

// Useful macro for reporting errors
//__FILE__ and __LINE__ are standard macros for source code file name
// and line number respectively
#define _WHERE_ __FILE__, __LINE__

namespace rxdock {

// General errors
const std::string IDS_ERROR = "RBT_ERROR";
const std::string IDS_OK = "RBT_OK";
const std::string IDS_INVALID_REQUEST = "RBT_INVALID_REQUEST";
const std::string IDS_BAD_ARGUMENT = "RBT_BAD_ARGUMENT";
const std::string IDS_ASSERT = "RBT_ASSERT";
const std::string IDS_BAD_RECEPTOR_FILE = "BAD_RECEPTOR_FILE";

// Template for throwing exceptions from assert failures
//(The C++ Programming Language, 3rd Ed, p750)
template <class X, class A> inline void Assert(A assertion) {
  if (!assertion) {
    throw X();
  }
}
/////////////////////////////////////
// BASE ERROR CLASS
// Note: This is a concrete class so can be instantiated
/////////////////////////////////////

class Error : public std::exception {
  ///////////////////////
  // Public methods
public:
  // Default constructor
  // Use to create an "error" with status=OK, no line, no file, no message
  // All other constructors set the OK flag to false
  Error()
      : m_strName(IDS_OK), m_strFile(""), m_strMessage(""), m_nLine(0),
        m_bOK(true) {}

  // Parameterised constructor
  // Use to create unspecified rxdock Errors
  Error(const std::string &strFile, int nLine,
        const std::string &strMessage = "")
      : Error(IDS_ERROR, strFile, nLine, strMessage) {}

  // Default destructor
  virtual ~Error() {}

  virtual const char *what() const noexcept { return m_strWhat.c_str(); }

  // Attribute methods
  std::string File() const { return m_strFile; }       // Get filename
  int Line() const { return m_nLine; }                 // Get line number
  std::string Message() const { return m_strMessage; } // Get message
  std::string Name() const { return m_strName; }       // Get error name
  bool isOK() const { return m_bOK; } // If true, status is OK (not an error)

  // Append new message to existing message
  void AddMessage(const std::string &strMessage) {
    m_strMessage += strMessage;
    m_strWhat += "\n" + strMessage;
  }

  ///////////////////////
  // Protected methods
protected:
  // Protected constructor to allow derived error classes to set error name
  Error(const std::string &strName, const std::string &strFile, int nLine,
        const std::string &strMessage = "")
      : m_strName(strName), m_strFile(strFile), m_strMessage(strMessage),
        m_nLine(nLine), m_bOK(false) {
    std::ostringstream oss;
    oss << Name();
    if (!File().empty())
      oss << " at " << File() << ", line " << Line();
    if (!Message().empty())
      oss << std::endl << Message();
    oss.flush();
    m_strWhat = oss.str();
  }

  ///////////////////////
  // Private data
private:
  std::string m_strName;
  std::string m_strFile;
  std::string m_strMessage;
  std::string m_strWhat;
  int m_nLine;
  bool m_bOK;
};

/////////////////////////////////////
// DERIVED ERROR CLASSES - General
/////////////////////////////////////

// Invalid request - object does not supported the request
class InvalidRequest : public Error {
public:
  InvalidRequest(const std::string &strFile, int nLine,
                 const std::string &strMessage = "")
      : Error(IDS_INVALID_REQUEST, strFile, nLine, strMessage) {}
};
// Bad argument - e.g. empty atom list
class BadArgument : public Error {
public:
  BadArgument(const std::string &strFile, int nLine,
              const std::string &strMessage = "")
      : Error(IDS_BAD_ARGUMENT, strFile, nLine, strMessage) {}
};
// Assertion failure
// Assert template doesn't pass params to exception class so can't pass file and
// line number
class Assertion : public Error {
public:
  Assertion() : Error(IDS_ASSERT, "unspecified file", 0, "Assertion failed") {}
};

// bad receptor type (ie not PSF/CRD either mol2
class BadReceptorFile : public Error {
public:
  BadReceptorFile(const std::string &strFile, int nLine,
                  const std::string &strMessage = "")
      : Error(IDS_BAD_RECEPTOR_FILE, strFile, nLine, strMessage) {}
};

} // namespace rxdock

#endif //_RBTERROR_H_
