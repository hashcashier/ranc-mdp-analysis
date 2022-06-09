//
// Created by rami on 17/03/2021.
//

#include "model_explicit.h"

vector<UI16> bitmaskToTimes(UI16 bitmaskLen, UI64 bitmask, bool inversion, bool lsbFirst) {
    vector<UI16> res{0};
    res.reserve(bitmaskLen);
    if (lsbFirst) {
        for(int i = 0; i < bitmaskLen; i++) {
            res[res.size() - 1]++;
            bool myArrival = ((1ULL << i) & bitmask) > 0;
            if (myArrival ^ inversion) {
                res.push_back(0);
            }
        }
    } else {
        for(int i = bitmaskLen - 1; i >= 0; i--) {
            res[res.size() - 1]++;
            bool myArrival = ((1ULL << i) & bitmask) > 0;
            if (myArrival ^ inversion) {
                res.push_back(0);
            }
        }
    }

    return res;
}

pair<UI16, UI64> timesToBitmask(const vector<UI16> &times, bool lsbFirst) {
    UI16 bmLen = 0;
    UI64 bm = 0;
    if (!times.empty()) {
        if (lsbFirst) {
            for (auto t : times) {
                if (t > 0) {
                    bmLen += t-1;
                }
                bm |= (1ULL<<(bmLen++));
            }
            bm &= ~((1ULL<<(--bmLen)));
            if (times.back() > 0) {
                bmLen++;
            }
        } else {
            for (auto t : times) {
                if (t > 0) {
                    bmLen += t-1;
                    bm <<= t-1;
                }
                bm <<= 1;
                bm++;
                bmLen++;
            }
            bm >>= 1;
            bmLen--;
            if (times.back() > 0) {
                bm <<= 1;
                bmLen++;
            }
        }
    }
    return {bmLen, bm};
}

void addRewardedTransition(UI64 choices,
                           vector<pair<UI64, pair<LD, LD>>> &choiceTransitions,
                           State &targetState,
                           LD probability,
                           LD reward,
                           unordered_set<State, StateHash> &visited,
                           queue<State> &bfs,
                           unordered_map<State, UI64, StateHash> &stateIndexMap) {
    if (probability > 0) {
        UI64 targetStateIndex = getOrSetStateIndex(targetState, stateIndexMap);
        choiceTransitions.emplace_back(targetStateIndex, make_pair(reward, probability));
        if (visited.count(targetState) == 0) { // target state was not visited before
            bfs.push(targetState);
        }
    }
}

void addRewardedChoice(
        UI64 &choices,
        vector<pair<UI64, pair<LD, LD>>> &choiceTransitions,
        storm::storage::SparseMatrixBuilder<LD> &transitionMatrixBuilder,
        storm::storage::SparseMatrixBuilder<LD> &transitionRewardMatrixBuilder,
        UI64 choice) {
    sort(choiceTransitions.begin(), choiceTransitions.end());
    for (auto t : choiceTransitions) {
        auto destination = t.first;
        auto reward = t.second.first;
        auto probability = t.second.second;
        transitionMatrixBuilder.addNextValue(choices, destination, probability);
        if (reward > 0) {
            transitionRewardMatrixBuilder.addNextValue(choices, destination, reward);
        }
    }
    choiceTransitions.clear();
    choices++;
}

State confirmBlocks(UI16 blocks, vector<UI16> &tailTimes, State state, UI32 K, bool initMatch) {
    assert(blocks >= state.honestBlocks);
    assert(blocks <= state.adversaryBlocks + tailTimes.size() - 1);
    state.honestBlocks = 0;
    state.rgh = 0;
    state.rga = 0;
    state.mch = 0;
    if (initMatch) {
        state.forkState = 2;
    } else {
        state.forkState = 0;
    }
    // Remove blocks from adversaryBlocks of height < K
    if (blocks <= state.adversaryBlocks) {
        state.adversaryBlocks -= blocks;
        blocks = 0;
    } else {
        blocks -= state.adversaryBlocks;
        state.adversaryBlocks = 0;
    }
    if (blocks > 0) {
        // Remove blocks from adversaryBlocks of height > K
        if (blocks == tailTimes.size() - 1) {
            state.bitmaskLen = 0;
            state.bitmask = 0;
        } else {
            vector<UI16> subTailTimes(tailTimes.begin() + blocks, tailTimes.end());
            auto newBitmask = timesToBitmask(subTailTimes);
            state.bitmaskLen = newBitmask.first;
            state.bitmask = newBitmask.second;
        }
    }
    // shift remaining blocks left
    if (state.bitmaskLen > 0 and state.adversaryBlocks < K) {
        auto subTailTimes = bitmaskToTimes(state.bitmaskLen, state.bitmask);
        UI32 n = subTailTimes.size() - 1;
        UI32 rem = K - state.adversaryBlocks;
        if (n <= rem) {
            state.adversaryBlocks += n;
            state.bitmaskLen = n < rem ? 0 : subTailTimes.back();
            state.bitmask = 0;
        } else {
            state.adversaryBlocks = K;
            subTailTimes.erase(subTailTimes.begin(), subTailTimes.begin() + rem);
            auto newBitmask = timesToBitmask(subTailTimes);
            state.bitmaskLen = newBitmask.first;
            state.bitmask = newBitmask.second;
        }
    }

    return state;
}

State appendBlock(State state, bool adversaryBlock, UI32 K, LD gamma, bool initMatch, bool withHonestTime, bool withAdversaryTime, bool overpay, bool estimate) {
    if (initMatch) {
        state.forkState = 2 ;
    }
    // record honest mining time
    if (withHonestTime) {
        if (state.forkState == 2 and state.honestBlocks >= K) {
            if (!estimate) {
                state.mch++;
            }
        } else if (state.honestBlocks >= K) {
            state.rgh++;
        }
    }
    // record appended block
    if (adversaryBlock) {
        // maintain active fork or signal fork irrelevance
        state.forkState = state.forkState == 1 or gamma == 0 ? 0 : state.forkState;
        if (state.adversaryBlocks < K or !withAdversaryTime) {
            // mining time is already rewarded, no record needed
            state.adversaryBlocks++;
            return state;
        }
        // record block arrival order
        state.bitmaskLen += 1;
        state.bitmask = (state.bitmask<<1)+1;
    } else {
        state.honestBlocks++;
        // signal fork relevance
        UI16 a = state.adversaryBlocks + countOnes(state.bitmask);
//        state.forkState = gamma > 0 and (K == 0 or state.honestBlocks <= K) and a >= state.honestBlocks;
        state.forkState = gamma > 0 and a >= state.honestBlocks;
        if (state.adversaryBlocks < K or !withAdversaryTime) {
            // mining time is already rewarded, no record needed
            return state;
        }
        // record block arrival order
        state.bitmaskLen += 1;
        state.bitmask <<= 1;
    }
    assert(withAdversaryTime and state.bitmaskLen > 0 and state.adversaryBlocks >= K);
    if (overpay) {
        assert(state.bitmaskLen == 1);
        if (adversaryBlock) {
            assert(state.bitmask == 1);
            state.adversaryBlocks++;
        } else {
            state.rga++;
        }
        state.bitmaskLen = 0;
        state.bitmask = 0;
        return state;
    }
    // compress order of arrival information
    auto adversaryTimes = bitmaskToTimes(state.bitmaskLen, state.bitmask);
    for (UI32 i = 0; i < state.honestBlocks && i < adversaryTimes.size(); i++) {
        if (adversaryTimes[i] == 0) {
            continue;
        }
        bool isLastEntry = i == adversaryTimes.size() - 1;
        state.rga += adversaryTimes[i] - !isLastEntry; // move zeroes in bitmask to rga
        adversaryTimes[i] = !isLastEntry; // retain 1 if it exists
    }
    auto remaining = timesToBitmask(adversaryTimes);
    state.bitmaskLen = remaining.first;
    state.bitmask = remaining.second;
    return state;
}

unordered_map<State, pair<UI16, UI16>, StateHash> constructExtendedStateChoiceMap(shared_ptr<SparseMDP> const &model, storm::storage::Scheduler<LD> const &scheduler) {
    unordered_map<State, pair<UI16, UI16>, StateHash> stateChoice;
    UI32 n = model->getNumberOfStates();
    stateChoice.reserve(n);
    bool withLabel = model->hasChoiceLabeling();
//    assert(withLabel);
    for (UI32 i = 0; i < n; i++) {
        auto it = model->getStateValuations().at(i).begin();
        auto et = model->getStateValuations().at(i).end();
        UI64 auxData_1 = it.getIntegerValue();
        ++it;
        UI64 bitmask = it == et ? 0ULL : it.getIntegerValue();
        ++it;
        UI64 auxData_2 = it == et ? 0ULL : it.getIntegerValue();
        auto stateTuple = make_tuple(auxData_1, bitmask, auxData_2);
        State state = State::fromTuple(stateTuple);
//        cout << "PAIR: " << statePair.first << '\t' << statePair.second << endl;
//        cout << "STATE: "; state.printToLine(cout);
        UI16 a = scheduler.getChoice(i).getDeterministicChoice();
        UI16 c = 0;
        if (withLabel) {
            UI64 idx = model->getChoiceIndex(storm::storage::StateActionPair(i, a));
            auto labels = model->getChoiceLabeling().getLabelsOfChoice(idx);
            assert(labels.size() == 1);
            c = (unsigned char)(*(labels.begin()->begin()));
//            state.printToLine(cout); cout << UI64(a) << ' ' << idx << ' ' << c << endl;
        }
        stateChoice[state] = make_pair(a, c);
    }
    return stateChoice;
}
