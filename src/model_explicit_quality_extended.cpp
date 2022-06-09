//
// Created by rami on 22/03/2021.
//

#include "model_explicit_quality_extended.h"

shared_ptr<SparseMDP> constructSparseExtendedQualityMDP(LD alpha, LD gamma, UI32 T, UI32 K, UI32 H, UI32 valuationLevel, bool withChoiceLabels) {
    auto expressionManager = make_shared<storm::expressions::ExpressionManager>(
            storm::expressions::ExpressionManager());
    Variable s1Var = expressionManager->declareIntegerVariable("S1");
    Variable s2Var = expressionManager->declareIntegerVariable("S2");
    Variable s3Var = expressionManager->declareIntegerVariable("S3");
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
            true,
            0);
    storm::storage::SparseMatrixBuilder<LD> transitionRewardMatrixBuilder(
            0,
            0,
            0,
            false,
            true,
            0);

    auto stateValuationsBuilder = storm::storage::sparse::StateValuationsBuilder();
    if (valuationLevel == 1) {
        stateValuationsBuilder.addVariable(s1Var);
        stateValuationsBuilder.addVariable(s2Var);
        stateValuationsBuilder.addVariable(s3Var);
    } else if (valuationLevel == 2) {
        for (auto const &var : auxVars) {
            stateValuationsBuilder.addVariable(var);
        }
    }

    auto choiceVector = vector<uint8>();
    auto choiceRewardVector = ChoiceRewardVector();
    auto stateIndexMap = unordered_map<State, UI64, StateHash>();
    auto visited = unordered_set<State, StateHash>();
    auto bfs = queue<State>();
    auto rewardedChoiceTransitions = vector<pair<UI64, pair<LD, LD>>>();
    UI64 choices = 0;

    State initialState{};

    State exitState = State::fromTuple({~0ULL, ~0ULL, ~0ULL});

//    stringstream buff;
    UI64 threateningStates = 0;

    bfs.push(initialState);
    while (!bfs.empty()) {
        State state = bfs.front();
        bfs.pop();
        auto insertion = visited.insert(state);
        if (!insertion.second) { // this state was already visited
            continue;
        }
        transitionMatrixBuilder.newRowGroup(choices);
        transitionRewardMatrixBuilder.newRowGroup(choices);
        UI64 stateIndex = getOrSetStateIndex(state, stateIndexMap);
        if (state == exitState) {
            if (valuationLevel == 1) {
                auto stateTuple = state.asTuple();
                stateValuationsBuilder.addState(stateIndex, {}, {SI64(get<0>(stateTuple)), SI64(get<1>(stateTuple)), SI64(get<2>(stateTuple))});
            } else if (valuationLevel == 2) {
                stateValuationsBuilder.addState(stateIndex, {}, vector<int64_t>(auxVars.size(), T));
            }
            if (withChoiceLabels) {
                choiceVector.push_back('e');
            }
            addRewardedTransition(choices, rewardedChoiceTransitions, state, 1.0, 0, visited, bfs, stateIndexMap);
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder,
                              transitionRewardMatrixBuilder, 0);
            continue;
        }

        assert(state.bitmaskLen == 0);
        assert(state.bitmask == 0);
        assert(state.rga == 0);
        assert(state.rgh == 0);
        assert(state.mch == 0);

        vector<UI16> tailTimes{UI16(0)};

        UI16 a = state.adversaryBlocks; // total adversary blocks mined
        auto h = state.honestBlocks; // honest blocks mined this round
        auto f = state.forkState; // forkability

        if (valuationLevel == 1) {
            auto stateTuple = state.asTuple();
            stateValuationsBuilder.addState(stateIndex, {}, {SI64(get<0>(stateTuple)), SI64(get<1>(stateTuple)), SI64(get<2>(stateTuple))});
        } else if (valuationLevel == 2) {
            stateValuationsBuilder.addState(stateIndex, {}, {
                    SI64(a),
                    SI64(state.honestBlocks),
                    SI64(state.forkState),
            });
        }

        assert(f != 2 || (a >= h && h > 0));

        bool canAdopt = h > 0;
        bool canOverride = a > h;
        bool canWait = max(a, h) < T;
//        bool canMatch = canWait and a >= h and (K == 0 or h <= K) and h > 0 and f == 1 and gamma > 0;
        bool canMatch = canWait and a >= h and h > 0 and f == 1 and gamma > 0;

//        bool honestTimeAtStake = (K == 0 and h > 0) or (K > 0 and ((h > K) or ((h == K) and (f == 1))));
        bool honestTimeAtStake = (K == 0 and h > 0) or (K > 0 and ((h > K) or ((h >= K) and (f == 1))));
        bool canThreaten = honestTimeAtStake and (canOverride or canMatch);
        if (canThreaten) {
            threateningStates++;
        }

        bool canAct = (canAdopt and !canOverride) or canOverride or canWait or canMatch;
        assert(canAct);

//        buff << "EXTENDED STATE REPORT" << endl;
//        state.printToLine(buff);
//        buff << "LEN: " << activeRaceLength << endl;
//        buff << "ACT: ";
//        printVector(vector<bool>{(canAdopt and !canOverride), canOverride, canWait, canMatch}, buff);
//        buff << endl;

        if (canAdopt and !canOverride) {
//            buff << "ADOPT" << endl;
            LD ra = 0;
            LD rh = h;
            LD pE = (ra + rh) / H; // exit after immediate rewards
            // outcome 1: horizon is reached after immediate rewards -> exit
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    exitState,
                    pE,
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 2: next block arrival is for adversary
            State outcome_2 = appendBlock(initialState, true, K, gamma, false, false, false);
//            outcome_2.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_2,
                    (1 - pE) * alpha,
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3: next block arrival is for the honest miners
            State outcome_3 = appendBlock(initialState, false, K, gamma, false, false, false);
//            outcome_3.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_3,
                    (1 - pE) * (1 - alpha),
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // finalize cases for adoption choice
            if (withChoiceLabels) {
                choiceVector.push_back('a');
            }
            addRewardedChoice(
                    choices,
                    rewardedChoiceTransitions,
                    transitionMatrixBuilder,
                    transitionRewardMatrixBuilder,
                    1);
            addChoiceReward(ra, choices, choiceRewardVector, true);
        }
        if (canOverride) {
//            buff << "OVERRIDE -> " << endl;
            LD ra = h + 1;
            LD rh = 0;
            LD pE = (ra + rh) / H; // exit after immediate rewards
            State postOverride = confirmBlocks(h + 1, tailTimes, state, K);
//            postOverride.printToLine(buff);
            // outcome 1: horizon is reached after immediate rewards -> exit
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    exitState,
                    pE,
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 2: next block arrival is for adversary
            State outcome_2 = appendBlock(postOverride, true, K, gamma, false, false, false);
//            outcome_2.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_2,
                    (1 - pE) * alpha,
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3: next block arrival is for the honest miners
            State outcome_3 = appendBlock(postOverride, false, K, gamma, false, false, false);
//            outcome_3.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_3,
                    (1 - pE) * (1 - alpha),
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // finalize cases for override choice
            if (withChoiceLabels) {
                choiceVector.push_back('o');
            }
            addRewardedChoice(
                    choices,
                    rewardedChoiceTransitions,
                    transitionMatrixBuilder,
                    transitionRewardMatrixBuilder,
                    2);
            addChoiceReward(ra, choices, choiceRewardVector, true);
        }
        if (canWait) {
//            buff << "WAIT" << endl;
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
                addChoiceReward(0, choices, choiceRewardVector, true);
            } else { // match active
//                buff << "WAIT -> ";
                goto match_code;
                wait_match_code:
                addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder,
                                  transitionRewardMatrixBuilder, 3);
                addChoiceReward(0, choices, choiceRewardVector, true);
            }
            // finalize cases for wait choice
            if (withChoiceLabels) {
                choiceVector.push_back('w');
            }
        }
        if (canMatch) {
//            buff << "MATCH -> ";
            match_code:
            LD matchRa = h;
            LD matchRh = 0;
            LD matchPE = (matchRa + matchRh) / H;

            State postMatch = confirmBlocks(h, tailTimes, state, K, true);
//            postMatch.printToLine(buff);
            // outcome 1: next block arrival is for adversary
            State outcome_1 = appendBlock(state, true, K, gamma, true, false, false);
//            outcome_1.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_1,
                    alpha,
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 2: next block arrival is for uncompromised honest miners, and match fails
            State outcome_2 = appendBlock(state, false, K, gamma, true, false, false);
//            outcome_2.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_2,
                    (1 - alpha) * (1 - gamma),
                    0,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3e: exit after immediate rewards
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    exitState,
                    (1 - alpha) * gamma * matchPE,
                    matchRa,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3: next block arrival is for compromised honest miners
            State outcome_3 = appendBlock(postMatch, false, K, gamma, false, false, false);
//            outcome_3.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_3,
                    (1 - alpha) * gamma * (1 - matchPE),
                    matchRa,
                    visited,
                    bfs,
                    stateIndexMap);
            if (f == 2) {
                goto wait_match_code;
            }
            // finalize cases for match choice
            if (withChoiceLabels) {
                choiceVector.push_back('m');
            }
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder,
                              transitionRewardMatrixBuilder, 4);
            addChoiceReward(0, choices, choiceRewardVector, true);
        }
    }
//    cout << buff.str() << endl;
    cout << "Threatening states: " << threateningStates << endl;
    // build transition matrix
    auto transitionMatrix = transitionMatrixBuilder.build();
    // build state labels
    auto n = stateIndexMap.size();
    auto stateLabeling = storm::models::sparse::StateLabeling(n);
    auto stateLabels = vector<string>{"exit", "init"};
    for (auto const &lab : stateLabels) {
        stateLabeling.addLabel(lab);
    }
    stateLabeling.addLabelToState("exit", getOrSetStateIndex(exitState, stateIndexMap));
    stateLabeling.addLabelToState("init", getOrSetStateIndex(initialState, stateIndexMap));
    // build reward matrix
    auto transitionRewardMatrix = transitionRewardMatrixBuilder.build();
    auto standardRewardModel = SparseRewardModel(boost::none, choiceRewardVector, transitionRewardMatrix);
    assert(not standardRewardModel.hasStateRewards());
    assert(standardRewardModel.hasStateActionRewards());
    assert(standardRewardModel.hasTransitionRewards());
    standardRewardModel.reduceToStateBasedRewards(transitionMatrix, false);
    assert(not standardRewardModel.hasStateRewards());
    assert(standardRewardModel.hasStateActionRewards());
    assert(not standardRewardModel.hasTransitionRewards());
    auto rewardModelsMap = unordered_map<string, SparseRewardModel>{{"blocks", standardRewardModel}};
    // build model components
    auto modelComponents = storm::storage::sparse::ModelComponents<LD>(
            transitionMatrix,
            stateLabeling,
            rewardModelsMap);
    // append state valuations
    if (valuationLevel > 0) {
        modelComponents.stateValuations = stateValuationsBuilder.build(n);
    }
    if (withChoiceLabels) {
        modelComponents.choiceLabeling = storm::models::sparse::ChoiceLabeling(choiceVector.size());
        modelComponents.choiceLabeling->addLabel("e");
        modelComponents.choiceLabeling->addLabel("a");
        modelComponents.choiceLabeling->addLabel("o");
        modelComponents.choiceLabeling->addLabel("w");
        modelComponents.choiceLabeling->addLabel("m");
        for (UI32 i = 0; i < choiceVector.size(); i++) {
            modelComponents.choiceLabeling->addLabelToChoice(string(1, choiceVector[i]), i);
        }
    }
    // return mdp
    auto mdp = make_shared<storm::models::sparse::Mdp<LD>>(modelComponents);
    if (n <= 128) {
        remove("sparseModel.gv");
        ofstream mdpDotFile;
        mdpDotFile.open("sparseModel.gv");
        mdp->writeDotToStream(mdpDotFile);
        mdpDotFile.close();
    }
    return mdp;
}
