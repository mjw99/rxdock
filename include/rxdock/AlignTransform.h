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

// Aligns ligand with the principal axes of one of the active site cavities

#ifndef _RBTALIGNTRANSFORM_H_
#define _RBTALIGNTRANSFORM_H_

#include "rxdock/BaseBiMolTransform.h"
#include "rxdock/Cavity.h"
#include "rxdock/Rand.h"

namespace rxdock {

class AlignTransform : public BaseBiMolTransform {
public:
  // Static data member for class type
  static const std::string _CT;
  // Parameter names
  static const std::string _COM;
  static const std::string _AXES;

  ////////////////////////////////////////
  // Constructors/destructors
  AlignTransform(const std::string &strName = "ALIGN");
  virtual ~AlignTransform();

  ////////////////////////////////////////
  // Public methods
  ////////////////

protected:
  ////////////////////////////////////////
  // Protected methods
  ///////////////////
  virtual void SetupReceptor(); // Called by Update when receptor is changed
  virtual void SetupLigand();   // Called by Update when ligand is changed
  virtual void
  SetupTransform(); // Called by Update when either model has changed
  virtual void Execute();

private:
  ////////////////////////////////////////
  // Private methods
  /////////////////
  AlignTransform(
      const AlignTransform &); // Copy constructor disabled by default
  AlignTransform &
  operator=(const AlignTransform &); // Copy assignment disabled by default

protected:
  ////////////////////////////////////////
  // Protected data
  ////////////////

private:
  ////////////////////////////////////////
  // Private data
  //////////////
  Rand &m_rand; // keep a reference to the singleton random number generator
  CavityList m_cavities;        // List of active site cavities to choose from
  std::vector<int> m_cumulSize; // Cumulative sizes, for weighted probabilities
  int m_totalSize;              // Total size of all cavities
};

// Useful typedefs
typedef SmartPtr<AlignTransform> AlignTransformPtr; // Smart pointer

} // namespace rxdock

#endif //_RBTALIGNTRANSFORM_H_
