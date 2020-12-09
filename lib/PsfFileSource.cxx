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

#include "rxdock/PsfFileSource.h"
#include "rxdock/ElementFileSource.h"
#include "rxdock/FileError.h"

#include <loguru.hpp>

#include <functional>

using namespace rxdock;

// Constructors
// PsfFileSource::PsfFileSource(const char* fileName) :
//  BaseMolecularFileSource(fileName,"PSF_FILE_SOURCE") //Call base class
//  constructor
//{
//  _RBTOBJECTCOUNTER_CONSTR_("PsfFileSource");
//}

PsfFileSource::PsfFileSource(const std::string &fileName,
                             const std::string &strMassesFile,
                             bool bImplHydrogens)
    : BaseMolecularFileSource(fileName,
                              "PSF_FILE_SOURCE"), // Call base class constructor
      m_bImplHydrogens(bImplHydrogens) {
  // Open a Charmm data source for converting atom type number to the more
  // friendly string variety
  m_spCharmmData = CharmmDataSourcePtr(new CharmmDataSource(strMassesFile));
  // DM 13 Jun 2000 - new separate prm file for receptor ionic atom definitions
  m_spParamSource = ParameterFileSourcePtr(
      new ParameterFileSource(GetDataFileName("data/sf", "IonicAtoms.prm")));
  // Open an Element data source
  m_spElementData = ElementFileSourcePtr(
      new ElementFileSource(GetDataFileName("data", "elements.json")));
  _RBTOBJECTCOUNTER_CONSTR_("PsfFileSource");
}

// Default destructor
PsfFileSource::~PsfFileSource() { _RBTOBJECTCOUNTER_DESTR_("PsfFileSource"); }

void PsfFileSource::Parse() {
  // Expected string constants in PSF files
  const std::string strPsfKey("PSF");
  const std::string strTitleKey("!NTITLE");
  const std::string strAtomKey("!NATOM");
  // DM 19 Feb 2002 - InsightII writes PSF files with !NBONDS: string rather
  // than !NBOND: so need to cope with either
  const std::string strBondKey("!NBOND:");
  // const std::string strBondKey("!NBOND");
  std::string strKey;

  // Only parse if we haven't already done so
  if (!m_bParsedOK) {
    ClearMolCache(); // Clear current cache
    Read();          // Read the file

    try {
      FileRecListIter fileIter = m_lineRecs.begin();
      FileRecListIter fileEnd = m_lineRecs.end();

      //////////////////////////////////////////////////////////
      // 1. Check for PSF string on line 1
      if ((fileIter == fileEnd) || ((*fileIter).substr(0, 3) != strPsfKey))
        throw FileParseError(_WHERE_, "Missing " + strPsfKey + " string in " +
                                          GetFileName());

      //////////////////////////////////////////////////////////
      // 2a Read number of title lines and check for correct title key...
      unsigned int nTitleRec;
      fileIter += 2;
      std::istringstream(*fileIter) >> nTitleRec >> strKey;
      if (strKey != strTitleKey)
        throw FileParseError(_WHERE_, "Missing " + strTitleKey + " string in " +
                                          GetFileName());

      // 2b ...and store them
      fileIter++;
      m_titleList.reserve(nTitleRec); // Allocate enough memory for the vector
      while ((m_titleList.size() < nTitleRec) && (fileIter != fileEnd)) {
        m_titleList.push_back(*fileIter++);
      }

      // 2c ..and check we read them all before reaching the end of the file
      if (m_titleList.size() != nTitleRec)
        throw FileParseError(_WHERE_,
                             "Incomplete title records in " + GetFileName());

      //////////////////////////////////////////////////////////
      // 3a. Read number of atoms and check for correct atom key...
      unsigned int nAtomRec;
      fileIter++;
      std::istringstream(*fileIter) >> nAtomRec >> strKey;
      if (strKey != strAtomKey)
        throw FileParseError(_WHERE_, "Missing " + strAtomKey + " string in " +
                                          GetFileName());

      // 3b ...and store them
      fileIter++;
      m_atomList.reserve(nAtomRec); // Allocate enough memory for the vector

      int nAtomId;                // original atom number in PSF file
      std::string strSegmentName; // segment name in PSF file
      std::string strSubunitId;   // subunit(residue) ID in PSF file
      std::string strSubunitName; // subunit(residue) name in PSF file
      std::string strAtomName;    // atom name from PSF file
      std::string strFFType;      // force field type from PSF file
      // Int nFFType; //force field type from PSF file (integer)
      double dPartialCharge; // partial charge
      double dAtomicMass;    // atomic mass from PSF file

      while ((m_atomList.size() < nAtomRec) && (fileIter != fileEnd)) {
        std::istringstream istr(*fileIter++);
        istr >> nAtomId >> strSegmentName >> strSubunitId >> strSubunitName >>
            strAtomName >> strFFType >> dPartialCharge >> dAtomicMass;

        // Construct a new atom (constructor only accepts the 2D params)
        AtomPtr spAtom(new Atom(nAtomId,
                                0, // Atomic number undefined (gets set below)
                                strAtomName, strSubunitId, strSubunitName,
                                strSegmentName));

        // Now set the remaining 2-D and 3-D params
        // Use the Charmm data source to provide some params not in the PSF file
        //
        // DM 4 Jan 1999 - check if force field type is numeric (parm22) or
        // string (parm19)
        int nFFType = std::atoi(strFFType.c_str());
        if (nFFType > 0)
          strFFType = m_spCharmmData->AtomTypeString(
              nFFType); // Convert atom type from int to string
        spAtom->SetFFType(strFFType);
        spAtom->SetAtomicNo(m_spCharmmData->AtomicNumber(
            strFFType)); // Now we can set the atomic number correctly
        spAtom->SetNumImplicitHydrogens(m_spCharmmData->ImplicitHydrogens(
            strFFType)); // Set # of implicit hydrogens
        spAtom->SetHybridState(m_spCharmmData->HybridState(
            strFFType)); // DM 8 Dec 1998 Set hybridisation state
        // spAtom->SetVdwRadius(m_spCharmmData->VdwRadius(strFFType));
        // spAtom->SetFormalCharge(m_spCharmmData->FormalCharge(strFFType));
        spAtom->SetGroupCharge(0.0);
        spAtom->SetPartialCharge(dPartialCharge);
        spAtom->SetAtomicMass(dAtomicMass);

        m_atomList.push_back(spAtom);
        m_segmentMap[strSegmentName]++; // increment atom count in segment map
      }

      // 3c ..and check we read them all before reaching the end of the file
      if (m_atomList.size() != nAtomRec)
        throw FileParseError(_WHERE_,
                             "Incomplete atom records in " + GetFileName());

      //////////////////////////////////////////////////////////
      // 4a. Read number of bonds and check for correct bond key...
      unsigned int nBondRec;
      fileIter++;
      std::istringstream(*fileIter) >> nBondRec >> strKey;
      LOG_F(1, "strKey {} strBondKey {}", strKey, strBondKey);
      // if(strBondKey.compare(strKey,0,6)) {// 6 for "!NBOND" old
      // style API
      int iCmp = strBondKey.compare(0, 5, strKey);
      LOG_F(1, "iCmp = {}", iCmp);
      if (iCmp > 0) { // 6 for "!NBOND" new API
        throw FileParseError(_WHERE_, "Missing " + strBondKey + " string in " +
                                          GetFileName());
      }

      // 4b ...and store them
      fileIter++;
      m_bondList.reserve(nBondRec); // Allocate enough memory for the vector

      int nBondId(0);
      unsigned int idxAtom1;
      unsigned int idxAtom2;
      while ((m_bondList.size() < nBondRec) && (fileIter != fileEnd)) {
        std::istringstream istr(*fileIter++);

        // Read bonds slightly differently to atoms as we have 4 bonds per line
        // Current method assumes atoms are numbered consectively from 1
        // May be better to store a map rather than a vector ??
        while (istr >> idxAtom1 >> idxAtom2) {
          if ((idxAtom1 > nAtomRec) ||
              (idxAtom2 > nAtomRec)) { // Check for indices in range
            throw FileParseError(_WHERE_,
                                 "Atom index out of range in bond records in " +
                                     GetFileName());
          }
          AtomPtr spAtom1(
              m_atomList[idxAtom1 -
                         1]); // Decrement the atom index as the atoms are
                              // numbered from zero in our atom vector
          AtomPtr spAtom2(
              m_atomList[idxAtom2 -
                         1]); // Decrement the atom index as the atoms are
                              // numbered from zero in our atom vector
          BondPtr spBond(
              new Bond(++nBondId, // Store a nominal bond ID starting from 1
                       spAtom1, spAtom2));
          m_bondList.push_back(spBond);
        }
      }
      // 4c ..and check we read them all before reaching the end of the file
      if (m_bondList.size() != nBondRec)
        throw FileParseError(_WHERE_,
                             "Incomplete bond records in " + GetFileName());

      // Setup the atomic params not stored in the file
      SetupAtomParams();

      //////////////////////////////////////////////////////////
      // If we get this far everything is OK
      m_bParsedOK = true;
    }

    catch (Error &error) {
      ClearMolCache(); // Clear the molecular cache so we don't return
                       // incomplete atom and bond lists
      throw;           // Rethrow the Error
    }
  }
}

// Sets up all the atomic attributes that are not explicitly stored in the PSF
// file
void PsfFileSource::SetupAtomParams() {
  // In the PSF file we have:
  //  Force field type
  //  X,Y,Z coords
  //  Partial charges
  //  Atomic mass
  //
  // CharmmDataSource has already provided (based on force field type):
  //  Element type
  //  Hybridisation state - sp3,sp2,sp,trigonal planar
  //  Number of implicit hydrogens

  // We need to define:
  //  Corrected vdW radii on H-bonding hydrogens and extended atoms
  //  Interaction group charges

  // DM 16 Feb 2000 - remove all non-polar hydrogens if required
  if (m_bImplHydrogens)
    RemoveNonPolarHydrogens();
  SetupVdWRadii();
  SetupPartialIonicGroups(); // Partial ionic groups (N7 etc)
  RenumberAtomsAndBonds();   // DM 30 Oct 2000 - tidy up atom and bond numbering
}

// Defines vdW radius, correcting for extended atoms and H-bond donor hydrogens
void PsfFileSource::SetupVdWRadii() {
  // Radius increment for atoms with implicit hydrogens
  // DM 22 Jul 1999 - only increase the radius for sp3 atoms with implicit
  // hydrogens For sp2 and aromatic, leave as is
  isHybridState_eq bIsSP3(Atom::SP3);
  isHybridState_eq bIsTri(Atom::TRI);
  isCoordinationNumber_eq bTwoBonds(2);

  double dImplRadIncr = m_spElementData->GetImplicitRadiusIncr();
  // Element data for hydrogen
  ElementData elHData = m_spElementData->GetElementData(1);
  // Radius increment and predicate for H-bonding hydrogens
  isAtomHBondDonor bIsHBondDonor;
  double dHBondRadius =
      elHData.vdwRadius + m_spElementData->GetHBondRadiusIncr();

  for (AtomListIter iter = m_atomList.begin(); iter != m_atomList.end();
       iter++) {
    // Get the element data for this atom
    int nAtomicNo = (*iter)->GetAtomicNo();
    // DM 04 Dec 2003 - correction to hybrid state for Ntri
    // Problem is Charmm type 34 is used ambiguously for "Nitrogen in 5-membered
    // ring" Some should be Ntri (3 bonds), others should Nsp2 (2 bonds)
    // masses.rtf defines hybrid state as TRI so switch here if necessary
    if ((nAtomicNo == 7) && bIsTri(*iter) && bTwoBonds(*iter)) {
      (*iter)->SetHybridState(Atom::SP2);
      LOG_F(1, "Switch from N_tri to N_sp2: {}", (*iter)->GetFullAtomName());
    }
    ElementData elData = m_spElementData->GetElementData(nAtomicNo);
    double vdwRadius = elData.vdwRadius;
    int nImplH = (*iter)->GetNumImplicitHydrogens();
    // Adjust atomic mass and vdw radius for atoms with implicit hydrogens
    if (nImplH > 0) {
      (*iter)->SetAtomicMass(elData.mass +
                             (nImplH * elHData.mass)); // Adjust atomic mass
      if (bIsSP3(*iter)) {
        vdwRadius +=
            dImplRadIncr; // adjust vdw radius (for sp3 implicit atoms only)
      }
    }
    // Adjust vdw radius for H-bonding hydrogens
    else if (bIsHBondDonor(*iter)) {
      vdwRadius = dHBondRadius;
    }
    // Finally we can set the radius
    (*iter)->SetVdwRadius(vdwRadius);
    LOG_F(1, "{}: #H= {}; vdwR={}; mass={}", (*iter)->GetFullAtomName(), nImplH,
          (*iter)->GetVdwRadius(), (*iter)->GetAtomicMass());
  }
}

void PsfFileSource::SetupPartialIonicGroups() {
  // Break the atom list up into substructures and call the base class
  // SetupPartialIonicGroups to assign group charges by residue Completely
  // general code, does not assume that all atoms in each substructure are
  // continguous
  //
  // Deselect all atoms
  std::for_each(m_atomList.begin(), m_atomList.end(), SelectAtom(false));
  // The loop increment searches for the next unselected atom
  for (AtomListIter iter = m_atomList.begin(); iter != m_atomList.end();
       iter = std::find_if(iter + 1, m_atomList.end(),
                           std::not1(isAtomSelected()))) {
    // Copy all atoms that belong to same substructure as head atom
    AtomList ss;
    std::copy_if(iter, m_atomList.end(), std::back_inserter(ss),
                 isSS_eq(*iter));
    LOG_F(1, "Psf SetupPartialIonicGroups: SS from {} to {} ({}) atoms",
          ss.front()->GetFullAtomName(), ss.back()->GetFullAtomName(),
          ss.size());
    // Assign group charges for this residue
    BaseMolecularFileSource::SetupPartialIonicGroups(ss, m_spParamSource);
    // Mark each atom in substructure as having been processed
    std::for_each(ss.begin(), ss.end(), SelectAtom(true));
  }
}

// DM 16 Feb 2000 - remove all non-polar hydrogens, adjusts number of implicit
// hydrogens
void PsfFileSource::RemoveNonPolarHydrogens() {
  // Get list of all carbons
  AtomList cList = GetAtomListWithPredicate(m_atomList, isAtomicNo_eq(6));

  for (AtomListIter cIter = cList.begin(); cIter != cList.end(); cIter++) {
    // Get list of all bonded hydrogens
    AtomList hList =
        GetAtomListWithPredicate(GetBondedAtomList(*cIter), isAtomicNo_eq(1));
    int nImplH = hList.size();
    if (nImplH > 0) {
      for (AtomListIter hIter = hList.begin(); hIter != hList.end(); hIter++) {
        RemoveAtom(*hIter);
      }
      // Adjust number of implicit hydrogens
      (*cIter)->SetNumImplicitHydrogens((*cIter)->GetNumImplicitHydrogens() +
                                        nImplH);
      LOG_F(1, "Removing {} hydrogens from {}", nImplH,
            (*cIter)->GetFullAtomName());
    }
  }
}
