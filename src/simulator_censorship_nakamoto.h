//
// Created by rami on 27/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_SIMULATOR_CENSORSHIP_NAKAMOTO_H
#define POSH_MDP_ANALYSIS_SIMULATOR_CENSORSHIP_NAKAMOTO_H

#include "simulator.h"

LD simulateCensorshipNakamoto(unordered_map<State, pair<UI16, UI16>, StateHash> &stateChoiceMap, LD alpha, LD gamma, UI32 T, bool compareChoiceChar = false, UI64 duration =
1ULL << 20, UI64 seed = 31337) {
    UI16 a = 0, h = 0, f = 0;
    LD rh = 0;
    std::default_random_engine generator(seed);
    discrete_distribution<UI16> p{alpha, (1 - alpha) * (1 - gamma), (1 - alpha) * gamma};

    UI16 choices[] = {'a', 'o', 'w', 'm'};

    stringstream buff;

    for (UI32 i = 0; i < duration; i++) {
        UI16 outcome = p(generator);

        State repState = {0, 0, h, f, 0, 0, 0, a};

        bool possibleState = stateChoiceMap.count(repState) > 0;
        if (!possibleState) {
            buff << "UNREACHABLE MDP STATE" << endl;
            buff << 't' << ' ' << i << ' ' << UI64(a) << ' ' << UI64(h) << ' ' << UI64(f) << endl;
            repState.printToLine(buff);
            cout << buff.str() << endl;
        }
        assert(possibleState);

        pair<UI16, UI16> stateChoice = stateChoiceMap[repState];

        bool canAdopt = h > 0;
        bool canOverride = a > h;
        bool canWait = max(a, h) < T;
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
                buff << 't' << ' ' << i << ' ' << UI64(a) << ' ' << UI64(h) << ' ' << UI64(f) << ' ' << choices[choice] << '-' << stateChoice.second << endl;
                repState.printToLine(buff);
                cout << buff.str() << endl;
            }
            assert(choices[choice] == stateChoice.second);
            stringstream().swap(buff);
            buff << endl;
            buff << 't' << ' ' << i << ' ' << UI64(a) << ' ' << UI64(h) << ' ' << UI64(f) << ' ' << choices[choice] << '-' << stateChoice.second << endl;
            repState.printToLine(buff);
        }

//        buff << i << ' '; repState.printToLine(buff); buff << ' ' << UI64(choice) << endl;
//        buff << UI64(a) << ' ' << UI64(h) << endl;

        assert(choicesPossible[choice]);
        if (choice == 0) { // adopt
            rh += h;
            a = 0;
            h = 0;
            f = 0;
            if (outcome == 0) { // adversary block arrives after action
                a++;
            } else { // honest block arrives after action
                h++;
            }
        }
        else if (choice == 1) { // override
            a -= h+1;
            h = 0;
            if (outcome == 0) { // adversary block arrives after action
                a++;
                f = 0;
            } else { // honest block arrives after action
                h++;
                f = (gamma > 0) and (a >= h);
            }
        }
        else if (choice == 2) { // wait
            if (f != 2) {
                if (outcome == 0) { // adversary block arrives after action
                    a++;
                    f = 0;
                } else { // honest block arrives after action
                    h++;
                    f = (gamma > 0) and (a >= h);
                }
            } else {
                goto match_code;
            }
        }
        else if (choice == 3) { // match
            match_code:
            if (outcome == 0) { // adversary block arrives after action
                a++;
                f = 2;
            } else if (outcome == 1) { // honest block arrives after action
                h++;
                f = a >= h;
            } else if (outcome == 2) { // compromised block arrives after action
                a -= h;
                h = 1;
                f = (a >= h);
            }
        }
    }
    return rh / duration;
}

#endif //POSH_MDP_ANALYSIS_SIMULATOR_CENSORSHIP_NAKAMOTO_H
