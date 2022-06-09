//
// Created by rami on 23/03/2021.
//

#include "model_explicit_reachability_extended.h"

shared_ptr<SparseDTMC> constructSparseExtendedReachabilityDTMC(LD alpha, LD gamma, UI32 T, UI32 K, UI32 valuationLevel) {
    auto expressionManager = make_shared<storm::expressions::ExpressionManager>(
            storm::expressions::ExpressionManager());
    Variable aVar = expressionManager->declareIntegerVariable("a");
    Variable hVar = expressionManager->declareIntegerVariable("h");
    Variable fVar = expressionManager->declareIntegerVariable("f");
    vector<Variable> auxVars{
            aVar,
            hVar,
            fVar,
    };

    storm::storage::SparseMatrixBuilder<LD> transitionMatrixBuilder(
            0,
            0,
            0,
            false,
            false,
            0);
    storm::storage::SparseMatrixBuilder<LD> transitionRewardMatrixBuilder(
            0,
            0,
            0,
            false,
            false,
            0);

    auto stateValuationsBuilder = storm::storage::sparse::StateValuationsBuilder();
    if (valuationLevel == 2) {
        for (auto const &var : auxVars) {
            stateValuationsBuilder.addVariable(var);
        }
    }

    auto stateIndexMap = unordered_map<State, UI64, StateHash>();
    auto visited = unordered_set<State, StateHash>();
    auto bfs = queue<State>();
    auto rewardedChoiceTransitions = vector<pair<UI64, pair<LD, LD>>>();
    UI64 choices = 0;

    State initialState{};
    State exitState = State::fromTuple({~0ULL, ~0ULL, ~0ULL});

//    stringstream buff;

    vector<UI64> threateningStates;

    bfs.push(initialState);
    while (!bfs.empty()) {
        State state = bfs.front();
        bfs.pop();
        auto insertion = visited.insert(state);
        if (!insertion.second) { // this state was already visited
            continue;
        }
//        transitionMatrixBuilder.newRowGroup(choices);
        UI64 stateIndex = getOrSetStateIndex(state, stateIndexMap);
        if (state == exitState) {
            if (valuationLevel == 2) {
                stateValuationsBuilder.addState(stateIndex, {}, vector<int64_t>(auxVars.size(), T));
            }
            addRewardedTransition(choices, rewardedChoiceTransitions, state, 1.0, 0, visited, bfs, stateIndexMap);
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder, transitionRewardMatrixBuilder, 0);
            continue;
        }

        UI16 a = state.adversaryBlocks; // total adversary blocks mined
        UI16 h = state.honestBlocks; // honest blocks mined this round
        UI16 f = state.forkState; // forkability
        assert(state.bitmaskLen == 0);
        assert(state.bitmask == 0);
        assert(state.rga == 0);
        assert(state.rgh == 0);
        assert(state.mch == 0);

        if (valuationLevel == 2) {
            stateValuationsBuilder.addState(stateIndex, {}, {
                    SI64(a),
                    SI64(h),
                    SI64(f),
            });
        }

        bool canOverride = a > h;
        bool canWait = max(a, h) < T;
//        bool canMatch = canWait and a >= h and h <= K and h > 0 and f == 1 and gamma > 0;
        bool canMatch = canWait and a >= h and h > 0 and f == 1 and gamma > 0;

//        bool honestTimeAtStake = (h > K) or ((h == K) and (f == 1));
        bool honestTimeAtStake = (h > K) or ((h >= K) and (f == 1));
        bool canThreaten = honestTimeAtStake and (canOverride or canMatch);
        if (canThreaten) {
            threateningStates.push_back(stateIndex);
        }

        if ((!canWait) or canThreaten) {
            addRewardedTransition(choices, rewardedChoiceTransitions, exitState, 1.0, 0, visited, bfs, stateIndexMap);
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder, transitionRewardMatrixBuilder, 0);
            continue;
        }

        if (f != 2) { // unmatched
            // outcome 2: next block arrival is for adversary
            State outcome_2 = appendBlock(state, true, K, gamma, false, false, false);
//                outcome_2.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_2,
                    alpha,
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3: next block arrival is for the honest miners
            State outcome_3 = appendBlock(state, false, K, gamma, false, false, false);
//                outcome_3.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_3,
                    (1 - alpha),
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder,
                              transitionRewardMatrixBuilder, 3);
        }
    }
//    cout << buff.str() << endl;
    // build transition matrix
    auto transitionMatrix = transitionMatrixBuilder.build();
    // build state labels
    auto n = stateIndexMap.size();
    auto stateLabeling = storm::models::sparse::StateLabeling(n);
    auto stateLabels = vector<string>{"init", "threat", "exit"};
    for (auto const &lab : stateLabels) {
        stateLabeling.addLabel(lab);
    }
    stateLabeling.addLabelToState("init", getOrSetStateIndex(initialState, stateIndexMap));
    stateLabeling.addLabelToState("exit", getOrSetStateIndex(exitState, stateIndexMap));
    for (auto const &v : threateningStates) {
        stateLabeling.addLabelToState("threat", v);
    }
    // build reward matrix
    auto rewardModelsMap = unordered_map<string, SparseRewardModel>{};
    // build model components
    auto modelComponents = storm::storage::sparse::ModelComponents<LD>(
            transitionMatrix,
            stateLabeling,
            rewardModelsMap);
    // append state valuations
    if (valuationLevel > 0) {
        modelComponents.stateValuations = stateValuationsBuilder.build(n);
    }
    // return mdp
    auto mdp = make_shared<SparseDTMC>(modelComponents);
    if (n <= 128) {
        remove("sparseModel.gv");
        ofstream mdpDotFile;
        mdpDotFile.open("sparseModel.gv");
        mdp->writeDotToStream(mdpDotFile);
        mdpDotFile.close();
    }
    return mdp;

}