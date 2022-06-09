//
// Created by rami on 22/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_SIMULATOR_H
#define POSH_MDP_ANALYSIS_SIMULATOR_H

#include "model_explicit.h"
#include <random>
#include <ctime>

State getRepresentativeState(vector<UI16> w_a, UI16 h, UI16 f, UI16 rgh, UI16 mch, UI32 K, bool withHonestTime = true, bool withAdversaryTime = true, bool overpay = false, bool estimate = false);

#endif //POSH_MDP_ANALYSIS_SIMULATOR_H
