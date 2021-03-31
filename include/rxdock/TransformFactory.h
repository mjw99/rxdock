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

// Factory class for creating transform objects. Transforms are specified by
// string names (as defined in each class's static _CT string)
// Directly analogous to SFFactory for creating scoring function objects

#ifndef _RBTTRANSFORMFACTORY_H_
#define _RBTTRANSFORMFACTORY_H_

#include "rxdock/Config.h"
#include "rxdock/ParameterFileSource.h"
#include "rxdock/TransformAgg.h"

namespace rxdock {

class TransformFactory {
  // Parameter name which identifies a scoring function definition
  static const std::string _TRANSFORM;

public:
  ////////////////////////////////////////
  // Constructors/destructors

  RBTDLL_EXPORT TransformFactory(); // Default constructor

  virtual ~TransformFactory(); // Default destructor

  ////////////////////////////////////////
  // Public methods
  ////////////////

  // Creates a single transform object of type strTransformClass, and name
  // strName e.g. strTransformClass = SimAnnTransform
  virtual BaseTransform *Create(const std::string &strTransformClass,
                                const std::string &strName);

  // Creates an aggregate transform from a parameter file source
  // Each component transform is in a named section, which should minimally
  // contain a TRANSFORM parameter whose value is the class name to instantiate
  // strTransformClasses contains a comma-delimited list of transform class
  // names to instantiate If strTransformClasses is empty, all named sections in
  // spPrmSource are scanned for valid transform definitions Transform
  // parameters and scoring function requests are set from the list of
  // parameters in each named section
  virtual TransformAgg *
  CreateAggFromFile(ParameterFileSourcePtr spPrmSource,
                    const std::string &strName,
                    const std::string &strTransformClasses = std::string());

protected:
  ////////////////////////////////////////
  // Protected methods
  ///////////////////

private:
  ////////////////////////////////////////
  // Private methods
  /////////////////

  TransformFactory(
      const TransformFactory &); // Copy constructor disabled by default

  TransformFactory &
  operator=(const TransformFactory &); // Copy assignment disabled by default

protected:
  ////////////////////////////////////////
  // Protected data
  ////////////////

private:
  ////////////////////////////////////////
  // Private data
  //////////////
};

// Useful typedefs
typedef SmartPtr<TransformFactory> TransformFactoryPtr; // Smart pointer

} // namespace rxdock

#endif //_RBTTRANSFORMFACTORY_H_
