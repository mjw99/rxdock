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

#include "rxdock/GATransform.h"
#include "rxdock/Population.h"
#include "rxdock/SFRequest.h"
#include "rxdock/WorkSpace.h"

#include <loguru.hpp>

#include <iomanip>

using namespace rxdock;

const std::string GATransform::_CT = "GATransform";
const std::string GATransform::_NEW_FRACTION = "fraction-of-new-individuals";
const std::string GATransform::_PCROSSOVER = "crossover-probability";
const std::string GATransform::_XOVERMUT = "crossover-mutation";
const std::string GATransform::_CMUTATE = "crossover-mutate";
const std::string GATransform::_STEP_SIZE = "step-size";
const std::string GATransform::_EQUALITY_THRESHOLD = "equality-threshold";
const std::string GATransform::_NCYCLES = "number-of-cycles";
const std::string GATransform::_NCONVERGENCE = "number-for-convergence";
const std::string GATransform::_HISTORY_FREQ = "history-frequency";

GATransform::GATransform(const std::string &strName)
    : BaseBiMolTransform(_CT, strName), m_rand(GetRandInstance()) {
  AddParameter(_NEW_FRACTION, 0.5);
  AddParameter(_PCROSSOVER, 0.4);
  AddParameter(_XOVERMUT, true);
  AddParameter(_CMUTATE, false);
  AddParameter(_STEP_SIZE, 1.0);
  AddParameter(_EQUALITY_THRESHOLD, 0.1);
  AddParameter(_NCYCLES, 100);
  AddParameter(_NCONVERGENCE, 6);
  AddParameter(_HISTORY_FREQ, 0);
  _RBTOBJECTCOUNTER_CONSTR_(_CT);
}

GATransform::~GATransform() { _RBTOBJECTCOUNTER_DESTR_(_CT); }

void GATransform::SetupReceptor() {}

void GATransform::SetupLigand() {}

void GATransform::SetupTransform() {}

void GATransform::Execute() {
  WorkSpace *pWorkSpace = GetWorkSpace();
  if (pWorkSpace == nullptr) {
    return;
  }
  BaseSF *pSF = pWorkSpace->GetSF();
  if (pSF == nullptr) {
    return;
  }
  PopulationPtr pop = pWorkSpace->GetPopulation();
  if (pop.Null() || (pop->GetMaxSize() < 1)) {
    return;
  }
  // Remove any partitioning from the scoring function
  // Not appropriate for a GA
  pSF->HandleRequest(new SFPartitionRequest(0.0));
  // This forces the population to rescore all the individuals in case
  // the scoring function has changed
  pop->SetSF(pSF);

  double newFraction = GetParameter(_NEW_FRACTION);
  double pcross = GetParameter(_PCROSSOVER);
  bool xovermut = GetParameter(_XOVERMUT);
  bool cmutate = GetParameter(_CMUTATE);
  double relStepSize = GetParameter(_STEP_SIZE);
  double equalityThreshold = GetParameter(_EQUALITY_THRESHOLD);
  int nCycles = GetParameter(_NCYCLES);
  int nConvergence = GetParameter(_NCONVERGENCE);
  int nHisFreq = GetParameter(_HISTORY_FREQ);

  double popsize = static_cast<double>(pop->GetMaxSize());
  int nrepl = static_cast<int>(newFraction * popsize);
  bool bHistory = nHisFreq > 0;

  double bestScore = pop->Best()->GetScore();
  // Number of consecutive cycles with no improvement in best score
  int iConvergence = 0;

  LOG_F(INFO, "CYCLE CONV      BEST      MEAN       VAR");
  LOG_F(INFO, " Init    -{:10.3f}{:10.3f}{:10.3f}", bestScore,
        pop->GetScoreMean(), pop->GetScoreVariance());

  for (int iCycle = 0; (iCycle < nCycles) && (iConvergence < nConvergence);
       ++iCycle) {
    if (bHistory && ((iCycle % nHisFreq) == 0)) {
      pop->Best()->GetChrom()->SyncToModel();
      pWorkSpace->SaveHistory(true);
    }
    pop->GAstep(nrepl, relStepSize, equalityThreshold, pcross, xovermut,
                cmutate);
    double score = pop->Best()->GetScore();
    if (score > bestScore) {
      bestScore = score;
      iConvergence = 0;
    } else {
      iConvergence++;
    }
    LOG_F(INFO, "{:5d}{:5d}{:10.3f}{:10.3f}{:10.3f}", iCycle, iConvergence,
          score, pop->GetScoreMean(), pop->GetScoreVariance());
  }
  pop->Best()->GetChrom()->SyncToModel();
  int ri = GetReceptor()->GetCurrentCoords();
  GetLigand()->SetDataValue(GetMetaDataPrefix() + "ri", ri);
}
