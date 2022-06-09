//
// Created by rami on 27/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_EXPLICIT_SUBVERSION_NAKAMOTO_H
#define POSH_MDP_ANALYSIS_MODEL_EXPLICIT_SUBVERSION_NAKAMOTO_H

#include "model_explicit.h"

shared_ptr<SparseMDP> constructSparseNakamotoSubversionMDP(LD alpha, LD gamma, UI32 T, UI32 S, LD D, UI32 valuationLevel = 0, bool withChoiceLabels = false);

#endif //POSH_MDP_ANALYSIS_MODEL_EXPLICIT_SUBVERSION_NAKAMOTO_H
