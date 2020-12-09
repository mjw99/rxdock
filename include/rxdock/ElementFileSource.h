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

// File reader for rxdock elemental data file (atomic no,element name,vdw radii)
// Provides the data as a vector of structs

#ifndef _RBTELEMENTFILESOURCE_H_
#define _RBTELEMENTFILESOURCE_H_

#include "rxdock/BaseFileSource.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace rxdock {

// Simple struct for holding the element data
class ElementData {
public:
  ElementData()
      : atomicNo(0), element(""), minVal(0), maxVal(0), commonVal(0), mass(0.0),
        vdwRadius(0.0) {}
  ElementData(json j) { j.get_to(*this); }

  friend void to_json(json &j, const ElementData &e) {
    j = json{{"atomic-number", e.atomicNo}, {"element-name", e.element},
             {"minimum-value", e.minVal},   {"maximum-value", e.maxVal},
             {"common-value", e.commonVal}, {"atomic-mass", e.mass},
             {"vdw-radius", e.vdwRadius}};
  }

  friend void from_json(const json &j, ElementData &e) {
    j.at("atomic-number").get_to(e.atomicNo);
    j.at("element-name").get_to(e.element);
    j.at("minimum-value").get_to(e.minVal);
    j.at("maximum-value").get_to(e.maxVal);
    j.at("common-value").get_to(e.commonVal);
    j.at("atomic-mass").get_to(e.mass);
    j.at("vdw-radius").get_to(e.vdwRadius);
  }

  int atomicNo;
  std::string element;
  int minVal;
  int maxVal;
  int commonVal;
  double mass;
  double vdwRadius; // Regular vdW radius
};

// Map with element data indexed by element name
typedef std::map<std::string, ElementData> StringElementDataMap;
typedef StringElementDataMap::iterator StringElementDataMapIter;
typedef StringElementDataMap::const_iterator StringElementDataMapConstIter;
// Map with element data indexed by atomic number
typedef std::map<int, ElementData> IntElementDataMap;
typedef IntElementDataMap::iterator IntElementDataMapIter;
typedef IntElementDataMap::const_iterator IntElementDataMapConstIter;

class ElementFileSource : public BaseFileSource {
public:
  // Constructors
  // ElementFileSource(const char* fileName);
  ElementFileSource(const std::string &fileName);

  // Destructor
  virtual ~ElementFileSource();

  ////////////////////////////////////////
  // Public methods
  RBTDLL_EXPORT std::string GetTitle();
  std::string GetVersion();
  unsigned int GetNumElements();
  std::vector<std::string> GetElementNameList(); // List of element names
  std::vector<int> GetAtomicNumberList();        // List of atomic numbers
  // Get element data for a given element name, throws error if not found
  ElementData GetElementData(const std::string &strElementName);
  // Get element data for a given atomic number, throws error if not found
  ElementData GetElementData(int nAtomicNumber);
  // Check if given element name is present
  bool isElementNamePresent(const std::string &strElementName);
  // Check if given atomic number is present
  bool isAtomicNumberPresent(int nAtomicNumber);

  // Get vdw radius increment for hydrogen-bonding donors
  double GetHBondRadiusIncr();
  // Get vdw radius increment for atoms with implicit hydrogens
  double GetImplicitRadiusIncr();

protected:
  // Protected methods

private:
  // Private methods
  ElementFileSource(); // Disable default constructor
  ElementFileSource(
      const ElementFileSource &); // Copy constructor disabled by default
  ElementFileSource &
  operator=(const ElementFileSource &); // Copy assignment disabled by default

  // Pure virtual in BaseFileSource - needs to be defined here
  virtual void Parse();
  void ClearElementDataCache();

protected:
  // Protected data

private:
  // Private data
  std::string m_inputFileName;
  std::string m_strTitle;
  std::string m_strVersion;
  double m_dHBondRadiusIncr;    // Increment to add to vdW radius for H-bonding
                                // hydrogens
  double m_dImplicitRadiusIncr; // Increment to add to vdW radius for atoms
                                // with implicit hydrogens
  StringElementDataMap m_elementNameMap;
  IntElementDataMap m_atomicNumberMap;
};

// useful typedefs
typedef SmartPtr<ElementFileSource> ElementFileSourcePtr; // Smart pointer

} // namespace rxdock

#endif //_RBTELEMENTFILESOURCE_H_
