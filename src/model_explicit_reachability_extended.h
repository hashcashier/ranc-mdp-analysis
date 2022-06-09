//
// Created by rami on 23/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_EXPLICIT_REACHABILITY_EXTENDED_H
#define POSH_MDP_ANALYSIS_MODEL_EXPLICIT_REACHABILITY_EXTENDED_H

#include "model_explicit.h"

shared_ptr<SparseDTMC> constructSparseExtendedReachabilityDTMC(LD alpha, LD gamma, UI32 T, UI32 K, UI32 valuationLevel = 0);

#endif //POSH_MDP_ANALYSIS_MODEL_EXPLICIT_REACHABILITY_EXTENDED_H
