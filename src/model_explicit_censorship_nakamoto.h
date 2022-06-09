//
// Created by rami on 27/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_EXPLICIT_CENSORSHIP_NAKAMOTO_H
#define POSH_MDP_ANALYSIS_MODEL_EXPLICIT_CENSORSHIP_NAKAMOTO_H

#include "model_explicit.h"

shared_ptr<SparseMDP> constructSparseNakamotoCensorshipMDP(LD alpha, LD gamma, UI32 T, UI32 valuationLevel = 0, bool withChoiceLabels = false);

#endif //POSH_MDP_ANALYSIS_MODEL_EXPLICIT_CENSORSHIP_NAKAMOTO_H
