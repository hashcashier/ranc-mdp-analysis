//
// Created by rami on 22/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_SIMULATOR_CENSORSHIP_EXTENDED_H
#define POSH_MDP_ANALYSIS_SIMULATOR_CENSORSHIP_EXTENDED_H

#include "simulator.h"

pair<LD, LD> simulateCensorshipExtended(unordered_map<State, pair<UI16, UI16>, StateHash> &stateChoiceMap, LD alpha, LD gamma, UI32 T, UI32 K, bool compareChoiceChar = false, UI64 duration =
1ULL << 20, UI64 seed = 31337) {
    UI16 f = 0;
    vector<UI16> w_a{0}, w_h{0};
    vector<LD> w_a_t{0}, w_h_t{0};
    UI16 mch = 0;
    LD mch_t = 0;
    LD ra = 0, rh = 0;
    LD ra_t = 0, rh_t = 0;
    std::default_random_engine generator(seed);
    discrete_distribution<UI16> p{alpha, (1 - alpha) * (1 - gamma), (1 - alpha) * gamma};
    LD lambda = 0.0001;
    exponential_distribution<LD> t(lambda);

    UI16 choices[] = {'a', 'o', 'w', 'm'};

    stringstream buff;

    for (UI32 i = 0; i < duration; i++) {
        UI16 outcome = p(generator);
        UI16 a = w_a.size() - 1;
        UI16 h = w_h.size() - 1;
        UI16 rgh = safeSum(w_h, K);
        LD ibt = t(generator);

        State repState = getRepresentativeState(w_a, h, f, rgh, mch, K, true, false);

        bool possibleState = stateChoiceMap.count(repState) > 0;
        if (!possibleState) {
            buff << "UNREACHABLE MDP STATE" << endl;
            buff << 't' << ' ' << i << ' '; printVector(w_a, buff); buff << " (" << UI64(repState.rga) << ") "; printVector(w_h, buff); buff << " (" << UI64(rgh) << ", " << UI64(mch) << ')' << endl;
            buff << 's' << ' ' << UI64(a) << ' ' << UI64(h) << ' ' << UI64(f) << endl;
            repState.printToLine(buff);
            repState.printToLine(buff);
            cout << buff.str() << endl;
        }
        assert(possibleState);

        pair<UI16, UI16> stateChoice = stateChoiceMap[repState];


/*
        UI16 headBlocks = a < K ? a : K;
        UI16 tailBlocks = a < K ? 0 : a - K;
        UI64 bmH = UI64(repState.bitmaskLen + repState.rga) - tailBlocks;
        UI64 CPA = bmH >= h ? 0 : UI64(h) - UI64(bmH);
        UI32 historyLength = CPA + repState.rga + repState.bitmaskLen + headBlocks;
*/

        bool canAdopt = h > 0;
        bool canOverride = a > h;
        bool canWait = max(a, h) < T;
//        bool canMatch = canWait and a >= h and h <= K and h > 0 and f == 1 and gamma > 0;
        bool canMatch = canWait and a >= h and h > 0 and f == 1 and gamma > 0;

        bool canAct = (canAdopt and !canOverride) or canOverride or canWait or canMatch;
        assert(canAct);

        vector<UI16> choicesPossible{
                canAdopt and !canOverride,
                canOverride,
                canWait,
                canMatch};

        vector<UI16> availableChoices;
        availableChoices.reserve(3);
        for (UI32 j = 0; j < choicesPossible.size(); j++) {
            if (choicesPossible[j]) {
                availableChoices.push_back(j);
            }
        }

        UI16 choice = availableChoices[stateChoice.first];
        if (compareChoiceChar) {
            if (choices[choice] != stateChoice.second) {
                buff << 't' << ' ' << i << ' '; printVector(w_a, buff); buff << " (" << UI64(repState.rga) << ") "; printVector(w_h, buff); buff << " (" << UI64(rgh) << ", " << UI64(mch) << ')' << endl;
                buff << 's' << ' ' << UI64(a) << ' ' << UI64(h) << ' ' << UI64(f) << ' ' << choices[choice] << '-' << stateChoice.second << endl;
                repState.printToLine(buff);
                cout << buff.str() << endl;
            }
            assert(choices[choice] == stateChoice.second);
            stringstream().swap(buff);
            buff << endl;
            buff << 't' << ' ' << i << ' '; printVector(w_a, buff); buff << " (" << UI64(repState.rga) << ") "; printVector(w_h, buff); buff << " (" << UI64(rgh) << ", " << UI64(mch) << ')' << endl;
            buff << 's' << ' ' << UI64(a) << ' ' << UI64(h) << ' ' << UI64(f) << ' ' << choices[choice] << '-' << stateChoice.second << endl;
            repState.printToLine(buff);
        }


//        buff << i << ' '; repState.printToLine(buff); buff << ' ' << UI64(choice) << endl;
//        buff << UI64(a) << ' ' << UI64(h) << endl;

        assert(choicesPossible[choice]);
        if (choice == 0) { // adopt
            ra += alpha * (safeSum(w_a, 0, K));
            ra_t += alpha * (safeSum(w_a_t, 0, K));
            rh += (1 - alpha) * (safeSum(w_h) + ((1 - gamma) * mch));
            rh_t += (1 - alpha) * (safeSum(w_h_t) + ((1 - gamma) * mch_t));
            w_a = {1};
            w_a_t = {ibt};
            w_h = {1};
            w_h_t = {ibt};
            mch = 0;
            mch_t = 0;
            f = 0;
            if (outcome == 0) { // adversary block arrives after action
                w_a.push_back(0);
                w_a_t.push_back(0);
            } else { // honest block arrives after action
                w_h.push_back(0);
                w_h_t.push_back(0);
            }
        }
        else if (choice == 1) { // override
            ra += alpha * (safeSum(w_a, 0, h+1));
            ra_t += alpha * (safeSum(w_a_t, 0, h+1));
            rh += (1 - alpha) * (safeSum(w_h, 0, K) + (gamma * mch));
            rh_t += (1 - alpha) * (safeSum(w_h_t, 0, K) + (gamma * mch_t));
            w_a.erase(w_a.begin(), w_a.begin()+(h+1));
            w_a_t.erase(w_a_t.begin(), w_a_t.begin()+(h+1));
            w_a.back()++;
            w_a_t.back() += ibt;
            w_h = {1};
            w_h_t = {ibt};
            mch = 0;
            mch_t = 0;
            if (outcome == 0) { // adversary block arrives after action
                w_a.push_back(0);
                w_a_t.push_back(0);
                f = 0;
            } else { // honest block arrives after action
                w_h.push_back(0);
                w_h_t.push_back(0);
//                f = gamma > 0 and (w_h.size() - 1) <= K and (w_a.size() - 1) >= (w_h.size() - 1);
                f = gamma > 0 and (w_a.size() - 1) >= (w_h.size() - 1);
            }
        }
        else if (choice == 2) { // wait
            if (f != 2) {
                w_a.back()++;
                w_a_t.back() += ibt;
                w_h.back()++;
                w_h_t.back() += ibt;
                if (outcome == 0) { // adversary block arrives after action
                    w_a.push_back(0);
                    w_a_t.push_back(0);
                    f = 0;
                } else { // honest block arrives after action
                    w_h.push_back(0);
                    w_h_t.push_back(0);
//                    f = gamma > 0 and (w_h.size() - 1) <= K and (w_a.size() - 1) >= (w_h.size() - 1);
                    f = gamma > 0 and (w_a.size() - 1) >= (w_h.size() - 1);
                }
            } else {
                goto match_code;
            }
        }
        else if (choice == 3) { // match
            match_code:
            w_a.back()++;
            w_a_t.back() += ibt;
//            if (h == K) {
            if (h >= K) {
                mch++;
                mch_t += ibt;
            } else {
                w_h.back()++;
                w_h_t.back() += ibt;
            }
            if (outcome == 0) { // adversary block arrives after action
                w_a.push_back(0);
                w_a_t.push_back(0);
                f = 2;
            } else if (outcome == 1) { // honest block arrives after action
                w_h.push_back(0);
                w_h_t.push_back(0);
//                f = (w_h.size() - 1) <= K and (w_a.size() - 1) >= (w_h.size() - 1);
                f = (w_a.size() - 1) >= (w_h.size() - 1);
            } else if (outcome == 2) { // compromised block arrives after action
                ra += alpha * (safeSum(w_a, 0, h));
                ra_t += alpha * (safeSum(w_a_t, 0, h));
                rh += (1 - alpha) * (safeSum(w_h, 0, K) + (gamma * mch));
                rh_t += (1 - alpha) * (safeSum(w_h_t, 0, K) + (gamma * mch_t));
                mch = 0;
                mch_t = 0;
                w_a.erase(w_a.begin(), w_a.begin()+h);
                w_a_t.erase(w_a_t.begin(), w_a_t.begin()+h);
                w_h = {0, 0};
                w_h_t = {0, 0};
//                f = (w_h.size() - 1) <= K and (w_a.size() - 1) >= (w_h.size() - 1);
                f = (w_a.size() - 1) >= (w_h.size() - 1);
            }
        }
    }
    return make_pair(rh / duration, rh_t / (duration / lambda));
}

#endif //POSH_MDP_ANALYSIS_SIMULATOR_CENSORSHIP_EXTENDED_H
