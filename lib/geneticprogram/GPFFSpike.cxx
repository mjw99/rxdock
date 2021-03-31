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

#include "rxdock/geneticprogram/GPFFSpike.h"
#include "rxdock/CellTokenIter.h"
#include "rxdock/Debug.h"
#include "rxdock/FilterExpression.h"
#include "rxdock/FilterExpressionVisitor.h"
#include "rxdock/Parser.h"
#include "rxdock/TokenIter.h"
#include "rxdock/geneticprogram/GPGenome.h"
#include "rxdock/geneticprogram/GPParser.h"

#include <loguru.hpp>

#include <cassert>
#include <fstream>
#include <sstream>

using namespace rxdock;
using namespace rxdock::geneticprogram;

const std::string GPFFSpike::_CT = "GPFFSpike";
void GPFFSpike::ReadTables(std::istream &in, ReturnTypeArray &it,
                           ReturnTypeArray &sft) {
  ReturnType value;
  int nip, nsfi;
  int i = 0, j, recordn;
  in >> nip;
  in.get();
  int nctes = 15;
  GPGenome::SetNIP(nip + nctes);
  in >> nsfi;
  GPGenome::SetNSFI(nsfi);
  in >> recordn;
  inputTable.clear();
  SFTable.clear();
  char name[80];
  CreateRandomCtes(nctes);
  while (!in.eof()) {
    // read name, don't store it for now
    in.get();
    in.getline(name, 80, ',');
    in >> value;
    ReturnTypeList ivalues;
    ivalues.push_back(new ReturnType(value));
    for (j = 1; j < nip; j++) {
      in.get();
      in >> value;
      ivalues.push_back(new ReturnType(value));
    }
    for (j = 0; j < nctes; j++) {
      ivalues.push_back(new ReturnType(ctes[j]));
    }
    ReturnTypeList sfvalues;
    for (j = 0; j < nsfi; j++) {
      in.get();
      in >> value;
      sfvalues.push_back(new ReturnType(value));
    }
    inputTable.push_back(ReturnTypeList(ivalues));
    SFTable.push_back(ReturnTypeList(sfvalues));
    i++;
    in >> recordn;
  }
  LOG_F(1, "Read: {}", inputTable[0][0]);
  it = inputTable;
  LOG_F(1, "Input table row size: {}", it[0].size());
  sft = SFTable;
}

double GPFFSpike::CalculateFitness(GPGenomePtr g, ReturnTypeArray &it,
                                   ReturnTypeArray &sft, bool function) {
  //    GPParser p(GPGenome::GetNIP(), GPGenome::GetNIF(),
  //                GPGenome::GetNN(), GPGenome::GetNO());
  ReturnTypeList o;
  o.push_back(new ReturnType(1.1));
  double good = 0.0;
  double bad = 0.0;
  double hitlimit = 0.0;
  GPChromosomePtr c(g->GetChrom());
  Parser p2;
  CellTokenIterPtr ti(new CellTokenIter(c, contextp));
  FilterExpressionPtr fe(p2.Parse(ti, contextp));
  //    CellTokenIterPtr ti(new CellTokenIter(c, contextp));
  for (unsigned int i = 0; i < it.size(); i++) {
    ReturnTypeList inputs(it[i]);
    for (unsigned int j = 0; j < inputs.size(); j++)
      contextp->Assign(j, *(inputs[j]));
    ReturnTypeList SFValues = sft[i];
    double hit = *(SFValues[SFValues.size() - 1]);
    //        Parser p2;
    //        CellTokenIterPtr ti(new CellTokenIter(c, contextp));
    //        FilterExpressionPtr fe(p2.Parse(ti, contextp));
    EvaluateVisitor visitor(contextp);
    fe->Accept(visitor);
    *(o[0]) = fe->GetValue();
    LOG_F(1, "Filter expression value: {}", *(o[0]));
    for (int j = 0; j < GPGenome::GetNO(); j++)
      if (function) {
        LOG_F(ERROR, "No function possible with spike");
        std::exit(1);
      } else if (*(o[j]) < 0.0) {
        if (hit < hitlimit)
          good++; // += 1.2;
        else
          bad++;
      } else if (hit >= hitlimit)
        ; // good++;
      else
        good--;
    //        c->Clear();
  }
  // objective value always an increasing function
  objective = good * 1.5 - bad;
  // For now, I am using tournament selection. So the fitness
  // function doesn't need to be scaled in any way
  fitness = objective;
  g->SetFitness(fitness);
  return fitness;
}

double GPFFSpike::CalculateFitness(GPGenomePtr g, ReturnTypeArray &it,
                                   ReturnTypeArray &sft, double hitlimit,
                                   bool function) {
  //    GPParser p(g->GetNIP(), g->GetNIF(), g->GetNN(), g->GetNO());
  ReturnTypeList o;
  o.push_back(new ReturnType(1.1));
  // ReturnType oldo;
  double truehits = 0.0;
  double falsehits = 0.0;
  double truemisses = 0.0;
  double falsemisses = 0.0;
  GPChromosomePtr c(g->GetChrom());
  Parser p2;
  TokenIterPtr ti(new CellTokenIter(c, contextp));
  FilterExpressionPtr fe(p2.Parse(ti, contextp));
  for (unsigned int i = 0; i < it.size(); i++) {
    ReturnTypeList inputs = it[i];
    for (unsigned int j = 0; j < inputs.size(); j++)
      contextp->Assign(j, *(inputs[j]));
    ReturnTypeList SFValues = sft[i];
    double hit = *(SFValues[SFValues.size() - 1]);
    //        o = p.Parse(c, inputs);
    //        oldo = *(o[0]);
    //        Parser p2; //new CellTokenIter(c, contextp));
    //        TokenIterPtr ti(new CellTokenIter(c, contextp));
    //        FilterExpressionPtr fe(p2.Parse(ti, contextp));
    EvaluateVisitor visitor(contextp);
    fe->Accept(visitor);
    *(o[0]) = fe->GetValue();
    LOG_F(1, "Filter expression value: {}", *(o[0]));
    //        assert(oldo == *(o[0]));
    double limit = 0.0;
    for (int j = 0; j < GPGenome::GetNO(); j++) {
      if (hit < hitlimit)
        if (*(o[j]) < limit)
          // true hit
          truehits++;
        else
          // false miss
          falsemisses++;
      else if (*(o[j]) < limit)
        // false hit
        falsehits++;
      else
        // true miss
        truemisses++;
    }
    //        c->Clear();
  }
  // objective value always an increasing function
  LOG_F(1, "True hits: {}, false hits: {}, true misses: {}, false misses: {}",
        truehits, falsehits, truemisses, falsemisses);
  objective = truehits / (truehits + falsehits);
  fitness = objective;
  // g->SetFitness(fitness);
  // For now, I am using tournament selection. So the fitness
  // function doesn't need to be scaled in any way
  return fitness;
}

void GPFFSpike::CreateRandomCtes(int nctes) {
  if (ctes.size() == 0) // it hasn't already been initialized
  {
    int a, b;
    double c;
    ctes.push_back(0.0);
    ctes.push_back(1.0);
    LOG_F(1, "c0=0.0");
    LOG_F(1, "c1=1.0");
    for (int i = 0; i < (nctes - 2); i++) {
      a = m_rand.GetRandomInt(200) - 100;
      b = m_rand.GetRandomInt(10) - 5;
      c = (a / 10.0) * std::pow(10, b);
      LOG_F(1, "c{}={}", i + 2, c);
      ctes.push_back(c);
    }
  }
}
