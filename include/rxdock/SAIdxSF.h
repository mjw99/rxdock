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

// Desolvation scoring function based on Weighted Solvent Accessible Surface
// Area

#ifndef _RBTSAIDXSF_H_
#define _RBTSAIDXSF_H_

#include "rxdock/AnnotationHandler.h"
#include "rxdock/BaseIdxSF.h"
#include "rxdock/BaseInterSF.h"
#include "rxdock/NonBondedHHSGrid.h"
#include "rxdock/ParameterFileSource.h"
#include "rxdock/SATypes.h"

namespace rxdock {

class SAIdxSF : public BaseInterSF, public BaseIdxSF, public AnnotationHandler {
public:
  RBTDLL_EXPORT SAIdxSF(const std::string &strName = "SAIdxSF");
  virtual ~SAIdxSF();
  // write score components
  virtual void ScoreMap(StringVariantMap &scoreMap) const;

  static const std::string _CT;
  static const std::string _INCR;

  // Request Handling method
  // Handles the Partition request
  virtual void HandleRequest(RequestPtr spRequest);

protected:
  virtual void SetupReceptor();
  virtual void SetupLigand();
  virtual void SetupSolvent();
  virtual void SetupScore();
  virtual double RawScore(void) const;

  // clear indexed grids
  void ClearReceptor(void);
  void ClearLigand(void);
  void ClearSolvent(void);

  double GetASP(HHSType::eType, double) const;
  double GetP_i(HHSType::eType) const;
  double GetR_i(HHSType::eType) const;
  void PrintWeightMatrix() const;

private:
  struct solvprms {
    solvprms(double r0, double p0, double asp0, bool b)
        : r(r0), p(p0), asp(asp0), chg_scaling(b) {}
    double r;
    double p;
    double asp;
    bool chg_scaling;
  };

  typedef std::vector<SAIdxSF::solvprms> SolvTable;
  void Setup();
  HHS_SolvationRList CreateInteractionCenters(const AtomList &atomList) const;
  void BuildIntraMap(HHS_SolvationRList &ICList) const;
  void BuildIntraMap(HHS_SolvationRList &ICList1,
                     HHS_SolvationRList &ICList2) const;

  // Sum the surface energies (ASP*area) for the list of solvation interaction
  // centers
  double TotalEnergy(const HHS_SolvationRList &intnCenters) const;
  void Partition(HHS_SolvationRList &intnCenters, double dist = 0.0);

  HHS_SolvationRList theLSPList; // All ligand solvation interaction centers
  HHS_SolvationRList
      theRSPList; // All rigid receptor solvation interaction centers
  HHS_SolvationRList
      theFlexList; // All flexible receptor solvation interaction centers
  HHS_SolvationRList
      theCavList; // Subset of rigid receptor list within range of docking site
  // Subset of rigid receptor list NOT within range of docking site
  // but within range of the flexible atoms
  // Must remember to restore the invariant areas for these atoms
  // before updating the overlap of the flexible atoms
  HHS_SolvationRList thePeriphList;
  HHS_SolvationRList
      theSolventList; // DM 21 Dec 2005 - explicit solvent interaction centers
  NonBondedHHSGridPtr theIdxGrid;
  SolvTable m_solvTable;
  ParameterFileSourcePtr m_spSolvSource; // File source for solvation params
  double m_maxR;   // Maximum radius of any atom type, used to adjust Range()
                   // dynamically
  bool m_bFlexRec; // Is receptor flexible?
  mutable double
      m_lig_0; // Solvation energy of the free ligand (initial conformation)
  mutable double
      m_lig_free; // Solvation energy of the free ligand (current conformation)
  mutable double m_lig_bound;     // Solvation energy of the bound ligand
                                  // (current conformation)
  mutable double m_site_0;        // Solvation energy of the free docking site
                                  // (initial conformation)
  mutable double m_site_free;     // Solvation energy of the free docking site
                                  // (current conformation)
  mutable double m_site_bound;    // Solvation energy of the bound docking site
                                  // (current conformation)
  mutable double m_solvent_0;     // Solvation energy of the free explicit
                                  // solvent (initial conformation)
  mutable double m_solvent_free;  // Solvation energy of the free explicit
                                  // solvent (current conformation)
  mutable double m_solvent_bound; // Solvation energy of the bound explicit
                                  // solvent (current conformation)
};

} // namespace rxdock

#endif // _RBTSAIDXSF_H_
