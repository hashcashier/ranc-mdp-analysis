//
// Created by rami on 18/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_EXPLICIT_INCENTIVE_EXTENDED_H
#define POSH_MDP_ANALYSIS_MODEL_EXPLICIT_INCENTIVE_EXTENDED_H

#include "model_explicit.h"

shared_ptr<SparseMDP> constructSparseExtendedIncentiveMDP(LD alpha, LD gamma, UI32 T, UI32 K, UI32 H, bool overpay = false, UI32 valuationLevel = 0, bool withChoiceLabels = false, bool forgive = false, bool estimate = false);


#endif //POSH_MDP_ANALYSIS_MODEL_EXPLICIT_INCENTIVE_EXTENDED_H
