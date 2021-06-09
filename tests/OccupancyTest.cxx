#include "OccupancyTest.h"
#include "rxdock/FlexAtomFactory.h"
#include "rxdock/LigandFlexData.h"
#include "rxdock/MdlFileSink.h"
#include "rxdock/MdlFileSource.h"
#include "rxdock/PRMFactory.h"
#include "rxdock/PolarIdxSF.h"
#include "rxdock/SAIdxSF.h"
#include "rxdock/SFAgg.h"
#include "rxdock/SetupPolarSF.h"
#include "rxdock/Variant.h"
#include "rxdock/VdwIdxSF.h"

#include <chrono>

using namespace rxdock;
using namespace rxdock::unittest;

double OccupancyTest::TINY = 1E-4;

void OccupancyTest::SetUp() {
  try {
    // Create the docking site, receptor, and ligand objects
    const std::string &wsName = "1YET";
    std::string prmFileName = GetDataFileName("", wsName + ".json");
    std::string ligFileName = GetDataFileName("", wsName + "_c.sd");
    std::string dockingSiteFileName =
        GetDataFileName("", wsName + "-docking-site.json");
    ParameterFileSourcePtr spPrmSource(new ParameterFileSource(prmFileName));
    MolecularFileSourcePtr spMdlFileSource(
        new MdlFileSource(ligFileName, true, true, true));
    m_workSpace = new BiMolWorkSpace();
    std::ifstream dockingSiteFile(dockingSiteFileName.c_str());
    json siteData;
    dockingSiteFile >> siteData;
    dockingSiteFile.close();
    m_workSpace->SetDockingSite(new DockingSite(siteData.at("docking-site")));
    PRMFactory prmFactory(spPrmSource, m_workSpace->GetDockingSite());
    m_workSpace->SetReceptor(prmFactory.CreateReceptor());
    m_workSpace->SetLigand(prmFactory.CreateLigand(spMdlFileSource));
    m_workSpace->SetSolvent(prmFactory.CreateSolvent());
    m_solventList = m_workSpace->GetSolvent();
  } catch (Error &e) {
    std::cout << e.what() << std::endl;
  }
}

void OccupancyTest::TearDown() {
  m_workSpace.SetNull();
  m_solventList.clear();
}

// 1) Check that the total desolvation score for receptor / ligand / (disabled)
// solvent matches the total score with no solvent present
TEST_F(OccupancyTest, SolvationSF) {
  BaseSF *sfAgg = new SFAgg(GetMetaDataPrefix() + "score");
  BaseSF *sfInter = new SFAgg("inter");
  BaseSF *sfSolv = new SAIdxSF("solv");
  sfAgg->Add(sfInter);
  sfInter->Add(sfSolv);
  m_workSpace->SetSF(sfAgg);

  ASSERT_LT(CompareScoresForDisabledAndNoSolvent(), TINY);
  sfInter->Remove(sfSolv);
  sfAgg->Remove(sfInter);
  delete sfAgg;
  delete sfInter;
  delete sfSolv;
}

// 2) Check that the total polar score for receptor / ligand / (disabled)
// solvent matches the total score with no solvent present
TEST_F(OccupancyTest, PolarSF) {
  BaseSF *sfAgg = new SFAgg(GetMetaDataPrefix() + "score");
  BaseSF *sfInter = new SFAgg("inter");
  BaseSF *sfSetupPolar = new SetupPolarSF("setup");
  BaseSF *sfPolar = new PolarIdxSF("polar");
  sfAgg->Add(sfInter);
  // For this term we have to set the interaction range and increment manually
  // These values match those in the standard scoring function
  sfPolar->SetRange(5.31);
  sfPolar->SetParameter(PolarIdxSF::GetIncr(), 3.36);
  sfInter->Add(sfSetupPolar);
  sfInter->Add(sfPolar);
  m_workSpace->SetSF(sfAgg);

  ASSERT_LT(CompareScoresForDisabledAndNoSolvent(), TINY);
  sfInter->Remove(sfPolar);
  sfInter->Remove(sfSetupPolar);
  sfAgg->Remove(sfInter);
  delete sfAgg;
  delete sfInter;
  delete sfSetupPolar;
  delete sfPolar;
}

// 3) Check that the total vdW score for receptor / ligand / (disabled) solvent
// matches the total score with no solvent present
TEST_F(OccupancyTest, VdwSF) {
  BaseSF *sfAgg = new SFAgg(GetMetaDataPrefix() + "score");
  BaseSF *sfInter = new SFAgg("inter");
  BaseSF *sfVdw = new VdwIdxSF("vdw");
  sfAgg->Add(sfInter);
  sfInter->Add(sfVdw);
  m_workSpace->SetSF(sfAgg);

  ASSERT_LT(CompareScoresForDisabledAndNoSolvent(), TINY);
  sfInter->Remove(sfVdw);
  sfAgg->Remove(sfInter);
  delete sfAgg;
  delete sfInter;
  delete sfVdw;
}

TEST_F(OccupancyTest, VdwSFSolventModes) {
  BaseSF *sfAgg = new SFAgg(GetMetaDataPrefix() + "score");
  BaseSF *sfInter = new SFAgg("inter");
  BaseSF *sfSystem = new SFAgg("system");
  BaseSF *sfVdw = new VdwIdxSF("vdw");
  sfAgg->Add(sfInter);
  sfAgg->Add(sfSystem);
  sfInter->Add(sfVdw);
  m_workSpace->SetSF(sfAgg);

  ASSERT_LT(CompareScoresForSolventModes(), TINY);
  sfInter->Remove(sfVdw);
  sfAgg->Remove(sfSystem);
  sfAgg->Remove(sfInter);
  delete sfAgg;
  delete sfInter;
  delete sfSystem;
  delete sfVdw;
}

// 5) Checks the results of FlexAtomFactory for receptor flexibility modes
TEST_F(OccupancyTest, FlexAtomFactoryReceptor) {
  int expected[2][3] = {{2034, 10, 0}, {2044, 0, 0}};
  ModelPtr spReceptor = m_workSpace->GetReceptor();
  // Test two modes:
  // mode 0 is as read from PRM file (flex OH/NH3+)
  // mode 1 is fixed receptor
  for (int mode = 0; mode < 2; ++mode) {
    if (mode == 1) {
      spReceptor->SetFlexData(nullptr);
    }
    FlexAtomFactory flexAtomFactory(spReceptor);
    AtomRList fixedAtomList = flexAtomFactory.GetFixedAtomList();
    AtomRList tetheredAtomList = flexAtomFactory.GetTetheredAtomList();
    AtomRList freeAtomList = flexAtomFactory.GetFreeAtomList();
    int nFixed = fixedAtomList.size();
    int nTethered = tetheredAtomList.size();
    int nFree = freeAtomList.size();
    // std::cout << "Receptor: #fixed = " << nFixed << "; #tethered = " <<
    // nTethered
    // << "; #free = " << nFree << std::endl; for (AtomRListConstIter iter =
    // tetheredAtomList.begin(); iter != tetheredAtomList.end(); ++iter) {
    //  std::cout << "Tethered: " << (*iter)->GetAtomName() << ": " <<
    //  (*iter)->GetUser1Value() << std::endl;
    //}
    ASSERT_EQ(nFixed, expected[mode][0]);
    ASSERT_EQ(nTethered, expected[mode][1]);
    ASSERT_EQ(nFree, expected[mode][2]);
  }
}

// 6) Checks the results of FlexAtomFactory for solvent flexibility modes
TEST_F(OccupancyTest, FlexAtomFactorySolvent) {
  // Expected sizes for fixed, tethered and free atom lists
  // for each value of model flexibility mode
  int expected[9][3] = {{3, 0, 0}, {0, 3, 0}, {0, 3, 0}, {0, 3, 0}, {0, 3, 0},
                        {0, 3, 0}, {0, 0, 3}, {0, 0, 3}, {0, 0, 3}};
  ModelList solventList = m_workSpace->GetSolvent();
  if (!solventList.empty()) {
    ModelPtr solvent = solventList.front();
    CheckFlexAtomFactory(solventList.front(), expected);
  }
}

// 7) Checks the results of FlexAtomFactory for ligand flexibility modes
TEST_F(OccupancyTest, FlexAtomFactoryLigand) {
  // Expected sizes for fixed, tethered and free atom lists
  // for each value of model flexibility mode
  int expected[9][3] = {{0, 0, 44}, {0, 0, 44}, {0, 0, 44},
                        {0, 0, 44}, {0, 0, 44}, {0, 0, 44},
                        {0, 0, 44}, {0, 0, 44}, {0, 0, 44}};
  ModelPtr spLigand = m_workSpace->GetLigand();
  if (!spLigand.Null()) {
    CheckFlexAtomFactory(spLigand, expected);
  }
}

double OccupancyTest::CompareScoresForDisabledAndNoSolvent() {
  double retVal = 0.0;
  BaseSF *pSF = m_workSpace->GetSF();
  if (pSF) {
    // A) No solvent present
    m_workSpace->RemoveSolvent();
    double scoreNoSolvent = pSF->Score();
    // B) With disabled solvent present
    for (ModelListIter iter = m_solventList.begin();
         iter != m_solventList.end(); ++iter) {
      (*iter)->SetOccupancy(0.0, 0.5);
    }
    m_workSpace->SetSolvent(m_solventList);
    double scoreDisabledSolvent = pSF->Score();
    // C) With enabled solvent present (not used at present)
    for (ModelListIter iter = m_solventList.begin();
         iter != m_solventList.end(); ++iter) {
      (*iter)->SetOccupancy(1.0, 0.5);
    }
    m_workSpace->SetSolvent(m_solventList);
    double scoreEnabledSolvent = pSF->Score();
    std::cout << "Score no solvent = " << scoreNoSolvent << std::endl;
    std::cout << "Score enabled solvent = " << scoreEnabledSolvent << std::endl;
    std::cout << "Score disabled solvent = " << scoreDisabledSolvent
              << std::endl;
    retVal = std::fabs(scoreNoSolvent - scoreDisabledSolvent);
  }
  return retVal;
}

void OccupancyTest::CheckFlexAtomFactory(Model *pModel, int expected[9][3]) {
  if (pModel) {
    FlexData *pFlexData = pModel->GetFlexData();
    FlexAtomFactory flexAtomFactory;
    for (int mode = 0; mode < 9; ++mode) {
      ChromElement::eMode eTransMode = (ChromElement::eMode)(mode / 3);
      ChromElement::eMode eRotMode = (ChromElement::eMode)(mode % 3);
      std::string strTransMode = ChromElement::ModeToStr(eTransMode);
      std::string strRotMode = ChromElement::ModeToStr(eRotMode);
      pFlexData->SetParameter(LigandFlexData::GetTransMode(), strTransMode);
      pFlexData->SetParameter(LigandFlexData::GetRotMode(), strRotMode);
      pModel->SetFlexData(pFlexData);
      FlexAtomFactory flexAtomFactory(pModel);
      AtomRList fixedAtomList = flexAtomFactory.GetFixedAtomList();
      AtomRList tetheredAtomList = flexAtomFactory.GetTetheredAtomList();
      AtomRList freeAtomList = flexAtomFactory.GetFreeAtomList();
      int nFixed = fixedAtomList.size();
      int nTethered = tetheredAtomList.size();
      int nFree = freeAtomList.size();
      // std::cout << std::endl << "Trans = " << strTransMode << ", Rot = " <<
      // strRotMode
      // << std::endl; for (AtomRListConstIter iter = fixedAtomList.begin();
      // iter
      // != fixedAtomList.end(); ++iter) {
      //  std::cout << "Fixed: " << (*iter)->GetAtomName() << ": " <<
      //  (*iter)->GetUser1Value() << std::endl;
      //}
      // for (AtomRListConstIter iter = tetheredAtomList.begin(); iter !=
      // tetheredAtomList.end(); ++iter) {
      //  std::cout << "Tethered: " << (*iter)->GetAtomName() << ": " <<
      //  (*iter)->GetUser1Value() << std::endl;
      //}
      ASSERT_EQ(nFixed, expected[mode][0]);
      ASSERT_EQ(nTethered, expected[mode][1]);
      ASSERT_EQ(nFree, expected[mode][2]);
    }
  }
}

double OccupancyTest::CompareScoresForSolventModes() {
  double retVal = 0.0;
  double lastScore = 0.0;
  BaseSF *pSF = m_workSpace->GetSF();
  if (pSF) {
    ModelList solventList = m_workSpace->GetSolvent();
    for (int mode = 0; mode < 9; ++mode) {
      ChromElement::eMode eTransMode = (ChromElement::eMode)(mode / 3);
      ChromElement::eMode eRotMode = (ChromElement::eMode)(mode % 3);
      std::string strTransMode = ChromElement::ModeToStr(eTransMode);
      std::string strRotMode = ChromElement::ModeToStr(eRotMode);
      // Change the modes of all except the first solvent model, to ensure a mix
      // of tethered / free solvent
      for (ModelListIter iter = solventList.begin() + 1;
           iter != solventList.end(); ++iter) {
        FlexData *pFlexData = (*iter)->GetFlexData();
        pFlexData->SetParameter(LigandFlexData::GetTransMode(), strTransMode);
        pFlexData->SetParameter(LigandFlexData::GetRotMode(), strRotMode);
        (*iter)->SetFlexData(pFlexData);
      }
      m_workSpace->RemoveSolvent();
      m_workSpace->SetSolvent(solventList);
      double score = pSF->Score();
      // std::cout << "mode = " << mode << "; score = " << score << std::endl;
      if (mode > 0) {
        double diff = std::fabs(score - lastScore);
        retVal = std::max(diff, retVal);
      }
      lastScore = score;
      // StringVariantMap scoreMap;
      // pSF->ScoreMap(scoreMap);
      // for (StringVariantMapConstIter vIter = scoreMap.begin(); vIter !=
      // scoreMap.end(); vIter++) { std::cout << (*vIter).first << " = " <<
      // (*vIter).second << std::endl;
      //}
    }
  }
  return retVal;
}
