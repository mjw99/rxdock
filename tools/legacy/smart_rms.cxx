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

// Calculates atomic RMSD of each SD record with reference structure
// Requires Daylight SMARTS toolkit to check all symmetry-related atomic
// numbering paths Reference structure can be a substructural fragment of each
// ligand in the SD file if desired No requirement for consistent numbering
// scheme between reference structure and SD file

#include <sstream>

#include "rxdock/MdlFileSink.h"
#include "rxdock/MdlFileSource.h"
#include "rxdock/Model.h"
#include "rxdock/ModelError.h"
#include "rxdock/daylight/Smarts.h"

using namespace rxdock;

namespace rxdock {

typedef std::vector<CoordList> CoordListList;
typedef CoordListList::iterator CoordListListIter;
typedef CoordListList::const_iterator CoordListListConstIter;

// RMSD calculation between two coordinate lists
double rmsd(const CoordList &rc, const CoordList &c) {
  int nCoords = rc.size();
  if (c.size() != nCoords) {
    return 0.0;
  } else {
    double rms(0.0);
    for (int i = 0; i < nCoords; i++) {
      rms += Length2(rc[i], c[i]);
    }
    rms = std::sqrt(rms / float(nCoords));
    return rms;
  }
}

} // namespace rxdock

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "smart_rms <ref sdfile> <input sdfile> [<output sdfile>]"
              << std::endl;
    std::cout
        << "RMSD is calculated for each record in <input sdfile> against <ref "
           "sdfile> (heavy atoms only)"
        << std::endl;
    std::cout
        << "If <output sdfile> is defined, records are written to output file "
           "with RMSD data field"
        << std::endl;
    std::cout << std::endl << "NOTE:" << std::endl;
    std::cout << "\tThis version uses the Daylight SMARTS toolkit to check all "
                 "symmetry-related atom numbering paths"
              << std::endl;
    std::cout << "\tto ensure that the true RMSD is reported. Will only run on "
                 "licensed machines (giles)"
              << std::endl;
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
  std::ios_base::fmtflags oldflags = std::cout.flags(); // save state

  std::vector<double> scoreVec;
  std::vector<double> rmsVec;

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
    std::string strSmarts;
    std::string strSmiles;
    AtomListList pathset = DT::QueryModel(spRefModel, strSmarts, strSmiles);
    std::cout << "Reference SMILES: " << strSmiles << std::endl;
    std::cout << "Paths found = " << pathset.size() << std::endl;
    if (pathset.empty()) {
      std::exit(0);
    }
    // Use the SMILES string for the reference to query each record in the SD
    // file This has the useful side effect that the numbering scheme in the SD
    // file does not have to be consistent with that in the reference structure
    // Also allows the reference to be a substructural fragment of each ligand
    strSmarts = strSmiles;
    for (AtomListListConstIter iter = pathset.begin(); iter != pathset.end();
         iter++) {
      CoordList coords;
      GetCoordList(*iter, coords);
      cll.push_back(coords);
    }
    int nCoords = cll.front().size();

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
    for (int nRec = 1; spMdlFileSource->FileStatusOK();
         spMdlFileSource->NextRecord(), nRec++) {
      Error molStatus = spMdlFileSource->Status();
      if (!molStatus.isOK()) {
        std::cout << molStatus << std::endl;
        continue;
      }
      // DM 16 June 2006 - remove any solvent fragments from each record
      spMdlFileSource->SetSegmentFilterMap(ConvertStringToSegmentMap("H"));
      ModelPtr spModel(new Model(spMdlFileSource));
      AtomListList pathset1 = DT::QueryModel(spModel, strSmarts, strSmiles);
      // std::cout << "SMILES: " << strSmiles << std::endl;
      // std::cout << "Paths found = " << pathset1.size() << std::endl;
      if (pathset1.empty()) {
        continue;
      }
      // we only need to retrieve the coordinate list for the first matching
      // atom numbering path as we have already stored all the alternative
      // numbering schemes for the reference structure
      CoordList coords;
      GetCoordList(pathset1.front(), coords);

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

        std::cout << nRec << "\t" << score << "\t" << scoreInter << "\t"
                  << scoreIntra << "\t" << rms << "\t" << 0.0 << std::endl;
        if (bOutput) {
          spMdlFileSink->SetModel(spModel);
          spMdlFileSink->Render();
        }
      }
    }
    // END OF MAIN LOOP OVER LIGAND RECORDS
    ////////////////////////////////////////////////////
    std::vector<double>::const_iterator sIter = scoreVec.begin();
    std::vector<double>::const_iterator rIter = rmsVec.begin();
    double minScore = *std::min_element(scoreVec.begin(), scoreVec.end());
    double zGood(0.0);
    double zPartial(0.0);
    double zBad(0.0);
    for (; (sIter != scoreVec.end()) && (rIter != rmsVec.end());
         sIter++, rIter++) {
      double de = (*sIter) - minScore;
      double z = std::exp(-de / (8.314e-3 * 298.0));
      double r = (*rIter);
      if (r < 2.05)
        zGood += z;
      else if (r < 3.05)
        zPartial += z;
      else
        zBad += z;
    }
    // std::cout << "zGood,zPartial,zBad = " << zGood << "," << zPartial << ","
    // << zBad << std::endl;
  } catch (Error &e) {
    std::cout << e << std::endl;
  } catch (...) {
    std::cout << "Unknown exception" << std::endl;
  }

  std::cout.flags(oldflags); // Restore state

  _RBTOBJECTCOUNTER_DUMP_(std::cout)

  return 0;
}
