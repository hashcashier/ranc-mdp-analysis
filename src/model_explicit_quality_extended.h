//
// Created by rami on 22/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_EXPLICIT_QUALITY_EXTENDED_H
#define POSH_MDP_ANALYSIS_MODEL_EXPLICIT_QUALITY_EXTENDED_H

#include "model_explicit.h"

shared_ptr<SparseMDP> constructSparseExtendedQualityMDP(LD alpha, LD gamma, UI32 T, UI32 K, UI32 H, UI32 valuationLevel = 0, bool withChoiceLabels = false);

#endif //POSH_MDP_ANALYSIS_MODEL_EXPLICIT_QUALITY_EXTENDED_H
