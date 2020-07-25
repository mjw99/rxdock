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

#include "InteractionGrid.h"
#include "FileError.h"
#include "PseudoAtom.h"

using namespace rxdock;

// Could be a useful general function
// If pAtom is a pseudo atom, then pushes all the constituent atoms onto list
// else, push pAtom itself onto the list
void InteractionCenter::AccumulateAtomList(const Atom *pAtom,
                                           AtomRList &atomList) const {
  if (!pAtom)
    return;
  PseudoAtom *pPseudo = dynamic_cast<PseudoAtom *>(const_cast<Atom *>(pAtom));
  if (pPseudo) {
    AtomList constituents = pPseudo->GetAtomList();
    std::copy(constituents.begin(), constituents.end(),
              std::back_inserter(atomList));
  } else {
    atomList.push_back(const_cast<Atom *>(pAtom));
  }
}

// Returns list of constituent atoms
//(deconvolutes pseudoatoms into their constituent Atom* lists)
AtomRList InteractionCenter::GetAtomList() const {
  AtomRList atomList;
  AccumulateAtomList(m_pAtom1, atomList);
  AccumulateAtomList(m_pAtom2, atomList);
  AccumulateAtomList(m_pAtom3, atomList);
  return atomList;
}

// An interaction is selected if any of its constituent atoms are selected
// If any of the constituent atoms are pseudo-atoms, then check these also
bool InteractionCenter::isSelected() const {
  AtomRList atomList = GetAtomList();
  return (std::count_if(atomList.begin(), atomList.end(), isAtomSelected()) >
          0);
}

// Select/deselect the interaction center (selects all constituent atoms)
void rxdock::SelectInteractionCenter::operator()(InteractionCenter *pIC) {
  AtomRList atomList = pIC->GetAtomList();
  std::for_each(atomList.begin(), atomList.end(), SelectAtom(b));
}

// Static data members
std::string InteractionGrid::_CT("InteractionGrid");

////////////////////////////////////////
// Constructors/destructors
// Construct a NXxNYxNZ grid running from gridMin at gridStep resolution
InteractionGrid::InteractionGrid(const Coord &gridMin, const Coord &gridStep,
                                 unsigned int NX, unsigned int NY,
                                 unsigned int NZ, unsigned int NPad)
    : BaseGrid(gridMin, gridStep, NX, NY, NZ, NPad) {
  CreateMap();
  _RBTOBJECTCOUNTER_CONSTR_("InteractionGrid");
}

// Constructor reading params from binary stream
InteractionGrid::InteractionGrid(std::istream &istr) : BaseGrid(istr) {
  CreateMap();
  OwnRead(istr);
  _RBTOBJECTCOUNTER_CONSTR_("InteractionGrid");
}

// Default destructor
InteractionGrid::~InteractionGrid() {
  ClearInteractionLists();
  _RBTOBJECTCOUNTER_DESTR_("InteractionGrid");
}

// Copy constructor
InteractionGrid::InteractionGrid(const InteractionGrid &grid) : BaseGrid(grid) {
  CreateMap();
  CopyGrid(grid);
  _RBTOBJECTCOUNTER_COPYCONSTR_("InteractionGrid");
}

// Copy constructor taking base class argument
InteractionGrid::InteractionGrid(const BaseGrid &grid) : BaseGrid(grid) {
  CreateMap();
  _RBTOBJECTCOUNTER_COPYCONSTR_("InteractionGrid");
}

// Copy assignment
InteractionGrid &InteractionGrid::operator=(const InteractionGrid &grid) {
  if (this != &grid) {
    // Clear the current lists
    ClearInteractionLists();
    // In this case we need to explicitly call the base class operator=
    BaseGrid::operator=(grid);
    CopyGrid(grid);
  }
  return *this;
}
// Copy assignment taking base class argument
InteractionGrid &InteractionGrid::operator=(const BaseGrid &grid) {
  if (this != &grid) {
    // Clear the current lists
    ClearInteractionLists();
    // In this case we need to explicitly call the base class operator=
    BaseGrid::operator=(grid);
  }
  return *this;
}

////////////////////////////////////////
// Virtual functions for reading/writing grid data to streams in
// text and binary format
// Subclasses should provide their own private OwnPrint,OwnWrite, OwnRead
// methods to handle subclass data members, and override the public
// Print,Write and Read methods

// Text output
void InteractionGrid::Print(std::ostream &ostr) const {
  BaseGrid::Print(ostr);
  OwnPrint(ostr);
}

// Binary output
void InteractionGrid::Write(std::ostream &ostr) const {
  BaseGrid::Write(ostr);
  OwnWrite(ostr);
}

// Binary input
void InteractionGrid::Read(std::istream &istr) {
  ClearInteractionLists();
  BaseGrid::Read(istr);
  OwnRead(istr);
}

////////////////////////////////////////
// Public methods
////////////////

/////////////////////////
// Get attribute functions
/////////////////////////

const InteractionCenterList &
InteractionGrid::GetInteractionList(unsigned int iXYZ) const {
  // InteractionListMapConstIter iter = m_intnMap.find(iXYZ);
  // if (iter != m_intnMap.end())
  //  return iter->second;
  // else
  //  return m_emptyList;

  // DM 3 Nov 2000 - map replaced by vector
  if (isValid(iXYZ)) {
    return m_intnMap[iXYZ];
  } else {
    return m_emptyList;
  }
}

const InteractionCenterList &
InteractionGrid::GetInteractionList(const Coord &c) const {
  // if (isValid(c))
  //  return GetInteractionList(GetIXYZ(c));
  // else
  //  return m_emptyList;

  // DM 3 Nov 2000 - map replaced by vector
  if (isValid(c)) {
    return m_intnMap[GetIXYZ(c)];
  } else {
    // std::cout << _CT << "::GetInteractionList," << c << " is off grid" <<
    // std::endl;
    return m_emptyList;
  }
}

/////////////////////////
// Set attribute functions
/////////////////////////
void InteractionGrid::SetInteractionLists(InteractionCenter *pIntn,
                                          double radius) {
  // Index using atom 1 coords - check if atom 1 is present
  Atom *pAtom1 = pIntn->GetAtom1Ptr();
  if (pAtom1 == nullptr)
    return;

  // DM 27 Oct 2000 - GetCoords now returns by reference
  const Coord &c = pAtom1->GetCoords();
  std::vector<unsigned int> sphereIndices;
  GetSphereIndices(c, radius, sphereIndices);

  for (std::vector<unsigned int>::const_iterator sphereIter =
           sphereIndices.begin();
       sphereIter != sphereIndices.end(); sphereIter++) {
    // InteractionListMapIter mapIter = m_intnMap.find(*sphereIter);
    // if (mapIter != m_intnMap.end()) {
    //  (mapIter->second).push_back(pIntn);
    //}
    // else {
    //  m_intnMap[*sphereIter] = InteractionCenterList(1,pIntn);
    //}
    m_intnMap[*sphereIter].push_back(pIntn);
  }
}

void InteractionGrid::ClearInteractionLists() {
  // m_intnMap.clear();
  // Clear each interaction center list separately, without clearing the whole
  // map (vector)
  for (InteractionListMapIter iter = m_intnMap.begin(); iter != m_intnMap.end();
       iter++) {
    (*iter).clear();
  }
}

void InteractionGrid::UniqueInteractionLists() {
  for (InteractionListMapIter iter = m_intnMap.begin(); iter != m_intnMap.end();
       iter++) {
    // std::cout << _CT << ": before = " << (*iter).size();
    std::sort((*iter).begin(), (*iter).end(), InteractionCenterCmp());
    InteractionCenterListIter uniqIter =
        std::unique((*iter).begin(), (*iter).end());
    (*iter).erase(uniqIter, (*iter).end());
    // std::cout << "; After = " << (*iter).size() << std::endl;
  }
}

///////////////////////////////////////////////////////////////////////////
// Protected methods

// Protected method for writing data members for this class to text stream
void InteractionGrid::OwnPrint(std::ostream &ostr) const {
  ostr << std::endl << "Class\t" << _CT << std::endl;
  ostr << "No. of entries in the map: " << m_intnMap.size() << std::endl;
  // TO BE COMPLETED - no real need for dumping the interaction list info
}

// Protected method for writing data members for this class to binary stream
//(Serialisation)
void InteractionGrid::OwnWrite(std::ostream &ostr) const {
  // Write all the data members
  // NO MEANS OF WRITING THE INTERACTION LISTS IN A WAY WHICH CAN BE READ BACK
  // IN i.e. we are holding pointers to atoms which would need to be recreated
  // What we need is some kind of object database I guess.
  // Note: By not writing the title key here, it enables a successful read
  // of ANY grid subclass file. e.g. An InteractionGrid can be constructed
  // from an RealGrid output stream, so will have the same grid dimensions
  // The RealGrid data array will simply be ignored
}

// Protected method for reading data members for this class from binary stream
// WARNING: Assumes grid data array has already been created
// and is of the correct size
void InteractionGrid::OwnRead(std::istream &istr) {
  // Read all the data members
  // NOTHING TO READ - see above
  CreateMap();
}

///////////////////////////////////////////////////////////////////////////
// Private methods
// Helper function called by copy constructor and assignment operator
void InteractionGrid::CopyGrid(const InteractionGrid &grid) {
  // This copies the interaction lists, but of course the atoms themselves are
  // not copied
  m_intnMap = grid.m_intnMap;
}

// DM 3 Nov 2000 - create InteractionListMap of the appropriate size
void InteractionGrid::CreateMap() { m_intnMap = InteractionListMap(GetN()); }