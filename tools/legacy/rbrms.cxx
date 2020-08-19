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

// Calculates atomic RMSD of each SD record with record #1

#include <functional>
#include <iostream>
#include <sstream>

#include "rxdock/MdlFileSink.h"
#include "rxdock/MdlFileSource.h"
#include "rxdock/Model.h"
#include "rxdock/ModelError.h"

using namespace rxdock;

namespace rxdock {

typedef std::vector<CoordList> CoordListList;
typedef CoordListList::iterator CoordListListIter;
typedef CoordListList::const_iterator CoordListListConstIter;

// Struct for holding symmetric bond params
class SymBond {
public:
  SymBond(BondPtr bond, int n, bool swap) : m_bond(bond), m_n(n), m_swap(swap) {
    m_dih = (m_n > 0) ? 360.0 / m_n : 360.0;
  }
  BondPtr m_bond; // The smart pointer to the bond itself
  int m_n;        // The symmetry operator (n-fold rotation)
  bool m_swap;    // false = spin atom 2 in bond; true = spin atom 1 in bond
  double m_dih;   // The dihedral step (360/n)
};

typedef SmartPtr<SymBond> SymBondPtr;
typedef std::vector<SymBondPtr> SymBondList;
typedef SymBondList::iterator SymBondListIter;
typedef SymBondList::const_iterator SymBondListConstIter;

// Class to enumerate all symmetry-related coordinate sets for a Model
class EnumerateSymCoords {
public:
  EnumerateSymCoords(ModelPtr spModel);
  // Main method to get the sym coords
  void GetSymCoords(CoordListList &cll);

private:
  void Setup();
  // Recursive operator to traverse the sym bond list
  void operator()(SymBondListConstIter symIter, CoordListList &cll);

  ModelPtr m_spModel;
  CoordListList m_cll;
  SymBondList m_symBondList;
  AtomList m_heavyAtomList;
  MolecularFileSinkPtr m_sink;
};

EnumerateSymCoords::EnumerateSymCoords(ModelPtr spModel) : m_spModel(spModel) {
  Setup();
  m_sink = new MdlFileSink("rmsd_ref_sym.sd", m_spModel);
}

void EnumerateSymCoords::operator()(SymBondListConstIter symIter,
                                    CoordListList &cll) {
  // If we are not at the end of the sym bond list, then spin the current bond
  // At each dihedral step, recursively spin all remaining sym bonds
  if (symIter != m_symBondList.end()) {
    SymBondPtr spSymBond = *symIter;
    for (int i = 0; i < spSymBond->m_n; i++) {
      m_spModel->RotateBond(spSymBond->m_bond, spSymBond->m_dih,
                            spSymBond->m_swap);
      (*this)(symIter + 1, cll); // Recursion
    }
  }
  // Once we get to the end of the sym bond list, append the conformation to the
  // ref coord list
  else {
    CoordList coords;
    GetCoordList(m_heavyAtomList, coords);
    cll.push_back(coords);
    m_sink->Render();
  }
  return;
}

// Main public method to enumerate the ref coords for the model
void EnumerateSymCoords::GetSymCoords(CoordListList &cll) {
  cll.clear();
  (*this)(m_symBondList.begin(), cll);
}

void EnumerateSymCoords::Setup() {
  m_heavyAtomList = GetAtomListWithPredicate(m_spModel->GetAtomList(),
                                             std::not1(isAtomicNo_eq(1)));
  m_symBondList.clear();
  BondList bondList = m_spModel->GetBondList();
  std::vector<std::string> symBonds =
      m_spModel->GetDataValue("SYMMETRIC_BONDS");
  for (std::vector<std::string>::const_iterator iter = symBonds.begin();
       iter != symBonds.end(); iter++) {
    int atomId1(1);
    int atomId2(1);
    int nSym(1);
    std::istringstream istr((*iter).c_str());
    istr >> atomId1 >> atomId2 >> nSym;
    bool bMatch = false;
    // Find the bond which matches these two atom IDs
    for (BondListConstIter bIter = bondList.begin();
         bIter != bondList.end() && !bMatch; bIter++) {
      if (((*bIter)->GetAtom1Ptr()->GetAtomId() == atomId1) &&
          ((*bIter)->GetAtom2Ptr()->GetAtomId() == atomId2)) {
        SymBondPtr spSymBond(new SymBond(*bIter, nSym, false));
        m_symBondList.push_back(spSymBond);
        bMatch = true;
#ifdef _DEBUG
        std::cout << "Matched bond ID " << (*bIter)->GetBondId()
                  << " for atoms " << atomId1 << ", " << atomId2
                  << ", swap=false" << std::endl;
#endif //_DEBUG
      } else if (((*bIter)->GetAtom1Ptr()->GetAtomId() == atomId2) &&
                 ((*bIter)->GetAtom2Ptr()->GetAtomId() == atomId1)) {
        SymBondPtr spSymBond(new SymBond(*bIter, nSym, true));
        m_symBondList.push_back(spSymBond);
        bMatch = true;
#ifdef _DEBUG
        std::cout << "Matched bond ID " << (*bIter)->GetBondId()
                  << " for atoms " << atomId1 << ", " << atomId2
                  << ", swap=true" << std::endl;
#endif //_DEBUG
      }
    }
    if (bMatch == false) {
      std::cout << "Bond " << atomId1 << " - " << atomId2 << " not found"
                << std::endl;
    }
  }
}

// RMSD calculation between two coordinate lists
double rmsd(const CoordList &rc, const CoordList &c) {
  unsigned int nCoords = rc.size();
  if (c.size() != nCoords) {
    return 0.0;
  } else {
    double rms(0.0);
    for (unsigned int i = 0; i < nCoords; i++) {
      rms += Length2(rc[i], c[i]);
    }
    rms = std::sqrt(rms / float(nCoords));
    return rms;
  }
}

} // namespace rxdock

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "rbrms <ref sdfile> <input sdfile> [<output sdfile>] [<RMSD "
                 "threshold>]"
              << std::endl;
    std::cout
        << "RMSD is calculated for each record in <input sdfile> against <ref "
           "sdfile> (heavy atoms only)"
        << std::endl;
    std::cout
        << "If <output sdfile> is defined, records are written to output file "
           "with RMSD data field"
        << std::endl;
    std::cout
        << "If RMSD threshold is defined, records are removed which have an "
           "RMSD < threshold with any"
        << std::endl;
    std::cout << "previous record in <input sdfile>" << std::endl;
    return 1;
  }

  std::string strRefSDFile(argv[1]);
  std::string strInputSDFile(argv[2]);
  std::string strOutputSDFile;
  bool bOutput(false);
  if (argc > 3) {
    strOutputSDFile = argv[3];
    bOutput = true;
  }
  bool bRemoveDups(false);
  double threshold(1.0);
  if (argc > 4) {
    threshold = std::atof(argv[4]);
    bRemoveDups = true;
  }

  // std::ios_base::fmtflags oldflags = std::cout.flags();//save state
  std::ios_base::fmtflags oldflags = std::cout.flags(); // save state

  std::vector<double> scoreVec;
  std::vector<double> rmsVec;
  double minScore(9999.9);

  try {
    MolecularFileSourcePtr spRefFileSource(new MdlFileSource(
        GetDataFileName("data/ligands", strRefSDFile), false, false, true));
    // DM 16 June 2006 - remove any solvent fragments from reference
    // The largest fragment in each SD record always has segment name="H"
    // for reasons lost in the mists of rDock history
    spRefFileSource->SetSegmentFilterMap(ConvertStringToSegmentMap("H"));
    // Get reference ligand (first record)
    ModelPtr spRefModel(new Model(spRefFileSource));
    CoordListList cll;
    EnumerateSymCoords symEnumerator(spRefModel);
    symEnumerator.GetSymCoords(cll);
    unsigned int nCoords = cll.front().size();

    std::cout << "molv_	rms rms	rmc rmc"
              << std::endl; // Dummy header line to be like do_anal

    std::cout.precision(3);
    std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);

    ///////////////////////////////////
    // MAIN LOOP OVER LIGAND RECORDS
    ///////////////////////////////////
    MolecularFileSourcePtr spMdlFileSource(new MdlFileSource(
        GetDataFileName("data/ligands", strInputSDFile), false, false, true));
    MolecularFileSinkPtr spMdlFileSink;
    if (bOutput) {
      spMdlFileSink = new MdlFileSink(strOutputSDFile, ModelPtr());
    }
    ModelList previousModels;
    for (int nRec = 1; spMdlFileSource->FileStatusOK();
         spMdlFileSource->NextRecord(), nRec++) {
      Error molStatus = spMdlFileSource->Status();
      if (!molStatus.isOK()) {
        std::cout << molStatus.what() << std::endl;
        continue;
      }
      // DM 16 June 2006 - remove any solvent fragments from each record
      spMdlFileSource->SetSegmentFilterMap(ConvertStringToSegmentMap("H"));
      ModelPtr spModel(new Model(spMdlFileSource));
      CoordList coords;
      GetCoordList(GetAtomListWithPredicate(spModel->GetAtomList(),
                                            std::not1(isAtomicNo_eq(1))),
                   coords);

      if (coords.size() ==
          nCoords) { // Only calculate RMSD if atom count is same as reference
        double rms(9999.9);
        for (CoordListListConstIter cIter = cll.begin(); cIter != cll.end();
             cIter++) {
          double rms1 = rmsd(*cIter, coords);
          // std::cout << "\tRMSD = " << rms1 << std::endl;
          rms = std::min(rms, rms1);
        }
        spModel->SetDataValue("RMSD", rms);
        double score = spModel->GetDataValue(GetMetaDataPrefix() + "score");
        double scoreInter =
            spModel->GetDataValue(GetMetaDataPrefix() + "score.inter");
        double scoreIntra =
            spModel->GetDataValue(GetMetaDataPrefix() + "score.intra");

        scoreVec.push_back(score);
        rmsVec.push_back(rms);
        minScore = std::min(minScore, score);

        bool bIsUnique(true);
        // Duplicate check
        if (bRemoveDups) {
          for (unsigned int i = 0; i < previousModels.size() && bIsUnique;
               i++) {
            CoordList prevCoords;
            GetCoordList(
                GetAtomListWithPredicate(previousModels[i]->GetAtomList(),
                                         std::not1(isAtomicNo_eq(1))),
                prevCoords);
            double rms0 = rmsd(prevCoords, coords);
            bIsUnique = (rms0 > threshold);
          }
        }
        // If we are not in 'remove duplicate' mode then bIsUnique is always
        // true
        if (bIsUnique) {
          std::cout << nRec << "\t" << score << "\t" << scoreInter << "\t"
                    << scoreIntra << "\t" << rms << "\t" << 0.0 << std::endl;
          if (bRemoveDups) {
            previousModels.push_back(spModel);
          }
          if (bOutput) {
            spMdlFileSink->SetModel(spModel);
            spMdlFileSink->Render();
          }
        }
      }
    }
    // END OF MAIN LOOP OVER LIGAND RECORDS
    ////////////////////////////////////////////////////
    std::vector<double>::const_iterator sIter = scoreVec.begin();
    std::vector<double>::const_iterator rIter = rmsVec.begin();
    double zTot(0.0);
    double zMean(0.0);
    double zMean2(0.0);
    // std::cout << std::endl << "Bolztmann-weighted RMSD calculation" <<
    // std::endl;
    for (; (sIter != scoreVec.end()) && (rIter != rmsVec.end());
         sIter++, rIter++) {
      double de = (*sIter) - minScore;
      double z = std::exp(-de / (8.314e-3 * 298.0));
      zTot += z;
      zMean += (*rIter) * z;
      zMean2 += (*rIter) * (*rIter) * z;
      // std::cout << *sIter << "\t" << de << "\t" << z << "\t" << zTot << "\t"
      // <<
      // (*rIter) << std::endl;
    }
    zMean /= zTot;
    // double zVar = zMean2 / zTot - (zMean * zMean);
    // std::cout << "zRMSD," << zTot << "," << zMean << "," << std::sqrt(zVar)
    // << std::endl;
  } catch (Error &e) {
    std::cout << e.what() << std::endl;
  } catch (...) {
    std::cout << "Unknown exception" << std::endl;
  }

  std::cout.flags(oldflags); // Restore state

  _RBTOBJECTCOUNTER_DUMP_(std::cout)

  return 0;
}
