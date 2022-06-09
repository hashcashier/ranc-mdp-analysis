//
// Created by rami on 17/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_EXPLICIT_H
#define POSH_MDP_ANALYSIS_MODEL_EXPLICIT_H

#include "model.h"

typedef vector<LD> ChoiceRewardVector;
typedef storm::models::sparse::StandardRewardModel<LD> SparseRewardModel;
typedef storm::models::sparse::Mdp<LD> SparseMDP;
typedef storm::models::sparse::Dtmc<LD> SparseDTMC;
typedef storm::modelchecker::SparseMdpPrctlModelChecker<SparseMDP> SparseMDPChecker;
typedef storm::modelchecker::SparseDtmcPrctlModelChecker<SparseDTMC> SparseDTMCChecker;

struct State {
    UI16 bitmaskLen;
    UI64 bitmask;
    UI16 honestBlocks;
    UI16 forkState;
    UI16 rgh;
    UI16 rga;
    UI16 mch;
    UI16 adversaryBlocks;

    static State fromTuple(tuple<UI64, UI64, UI64> const &stateTuple) {
        return {
                UI16((get<0>(stateTuple)) & 65535),
                get<1>(stateTuple),
                UI16((get<0>(stateTuple) >> 16) & 65535),
                UI16((get<0>(stateTuple) >> 32) & 65535),
                UI16((get<0>(stateTuple) >> 48) & 65535),
                UI16((get<2>(stateTuple)) & 65535),
                UI16((get<2>(stateTuple) >> 16) & 65535),
                UI16((get<2>(stateTuple) >> 32) & 65535)
        };
    }

    bool operator==(const State &o) const {
        return bitmaskLen == o.bitmaskLen
               && bitmask == o.bitmask
               && honestBlocks == o.honestBlocks
               && forkState == o.forkState
               && rgh == o.rgh
               && rga == o.rga
               && mch == o.mch
               && adversaryBlocks == o.adversaryBlocks;
    }

    tuple<UI64, UI64, UI64> asTuple() const {
        UI64 auxData_1 = (UI64(rgh) << 48ULL) | (UI64(forkState) << 32ULL) | (UI64(honestBlocks) << 16ULL) | UI64(bitmaskLen);
        UI64 auxData_2 = (UI64(adversaryBlocks) << 32ULL) | (UI64(mch) << 16ULL) | UI64(rga);
        return make_tuple(auxData_1, bitmask, auxData_2);
    }

    void printToLine(ostream &stream) const {
        stream << UI64(adversaryBlocks) << '\t' << UI64(honestBlocks) << '\t' << UI64(forkState) << '\t' << UI64(bitmaskLen) << '\t' << UI64(bitmask) << '\t' << UI64(rga) << '\t' << UI64(rgh) << '\t' << UI64(mch) << endl;
    }
};

struct StateHash
{
    std::size_t operator() (const State &state) const
    {
        auto stateTuple = state.asTuple();
        std::size_t h1 = std::hash<UI64>()(get<0>(stateTuple));
        std::size_t h2 = std::hash<UI64>()(get<1>(stateTuple));
        std::size_t h3 = std::hash<UI64>()(get<2>(stateTuple));
        return (h1 ^ (h2 << 1)) ^ (h3 << 1);
    }
};

vector<UI16> bitmaskToTimes(UI16 bitmaskLen, UI64 bitmask, bool inversion=false, bool lsbFirst=false);

pair<UI16, UI64> timesToBitmask(const vector<UI16> &times, bool lsbFirst=false);

inline UI64 getOrSetStateIndex(State &state, unordered_map<State, UI64, StateHash> &stateIndexMap) {
    UI64 mapSize = stateIndexMap.size();
    if (stateIndexMap.count(state) > 0)
        return stateIndexMap[state];
    return stateIndexMap[state] = mapSize;
}

inline void addChoiceReward(LD value, UI64 choices, ChoiceRewardVector &rewardVector, bool choiceExists = true) {
    rewardVector.resize(choiceExists ? choices - 1 : choices, 0);
    rewardVector.push_back(value);
}

void addRewardedTransition(UI64 choices,
                           vector<pair<UI64, pair<LD, LD>>> &choiceTransitions,
                           State &targetState,
                           LD probability,
                           LD reward,
                           unordered_set<State, StateHash> &visited,
                           queue<State> &bfs,
                           unordered_map<State, UI64, StateHash> &stateIndexMap);

void addRewardedChoice(
        UI64 &choices,
        vector<pair<UI64, pair<LD, LD>>> &choiceTransitions,
        storm::storage::SparseMatrixBuilder<LD> &transitionMatrixBuilder,
        storm::storage::SparseMatrixBuilder<LD> &transitionRewardMatrixBuilder,
        UI64 choice);

State confirmBlocks(UI16 blocks, vector<UI16> &tailTimes, State state, UI32 K, bool initMatch = false);

State appendBlock(State state, bool adversaryBlock, UI32 K, LD gamma, bool initMatch = false, bool withHonestTime = true, bool withAdversaryTime = true, bool overpay = false, bool estimate = false);

unordered_map<State, pair<UI16, UI16>, StateHash> constructExtendedStateChoiceMap(shared_ptr<SparseMDP> const &model, storm::storage::Scheduler<LD> const &scheduler);


#endif //POSH_MDP_ANALYSIS_MODEL_EXPLICIT_H
