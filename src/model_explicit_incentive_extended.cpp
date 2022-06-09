//
// Created by rami on 18/03/2021.
//

#include "model_explicit_incentive_extended.h"

shared_ptr<SparseMDP> constructSparseExtendedIncentiveMDP(LD alpha, LD gamma, UI32 T, UI32 K, UI32 H, bool overpay, UI32 valuationLevel, bool withChoiceLabels, bool forgive, bool estimate) {
    auto expressionManager = make_shared<storm::expressions::ExpressionManager>(storm::expressions::ExpressionManager());
    Variable s1Var = expressionManager->declareIntegerVariable("S1");
    Variable s2Var = expressionManager->declareIntegerVariable("S2");
    Variable s3Var = expressionManager->declareIntegerVariable("S3");
    Variable aVar = expressionManager->declareIntegerVariable("a");
    Variable hVar = expressionManager->declareIntegerVariable("h");
    Variable fVar = expressionManager->declareIntegerVariable("f");
    Variable rghVar = expressionManager->declareIntegerVariable("rgh");
    Variable rgaVar = expressionManager->declareIntegerVariable("rga");
    Variable mchVar = expressionManager->declareIntegerVariable("mch");
    Variable bmlVar = expressionManager->declareIntegerVariable("bml");
    Variable bmkVar = expressionManager->declareIntegerVariable("bmk");
    vector<Variable> auxVars{
            aVar,
            hVar,
            fVar,
            rghVar,
            rgaVar,
            mchVar,
            bmlVar,
            bmkVar,
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
    State exitState_1 = State::fromTuple({~0ULL, ~0ULL - 1, ~0ULL});
    State exitState_2 = State::fromTuple({~0ULL, ~0ULL - 2, ~0ULL});

//    stringstream buff;
    UI64 threateningStates = 0;

     bfs.push(initialState);
    while(!bfs.empty()) {
        State state = bfs.front();
        bfs.pop();
        auto insertion = visited.insert(state);
        if (!insertion.second) { // this state was already visited
            continue;
        }
        transitionMatrixBuilder.newRowGroup(choices);
        transitionRewardMatrixBuilder.newRowGroup(choices);
        UI64 stateIndex = getOrSetStateIndex(state, stateIndexMap);
        if (state == exitState or state == exitState_1 or state == exitState_2) {
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
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder, transitionRewardMatrixBuilder, 0);
            continue;
        }

        auto tailTimes = bitmaskToTimes(state.bitmaskLen, state.bitmask);

        UI16 a = state.adversaryBlocks + tailTimes.size() - 1; // total adversary blocks mined
        auto h = state.honestBlocks; // honest blocks mined this round
        auto f = state.forkState; // forkability
        auto bmLen = state.bitmaskLen; // length of bitmask maintaining race at a >= K
        auto bm = state.bitmask; // bitmask of race events at a >= K
        auto rgh = state.rgh; // honest mining time at h > K
        auto rga = state.rga; // adversary mining time at K < a <= h
        auto mch = state.mch; // time spent by honest miners breaking tie at h=K

        if (valuationLevel == 1) {
            auto stateTuple = state.asTuple();
            stateValuationsBuilder.addState(stateIndex, {}, {SI64(get<0>(stateTuple)), SI64(get<1>(stateTuple)), SI64(get<2>(stateTuple))});
        } else if (valuationLevel == 2) {
            stateValuationsBuilder.addState(stateIndex, {}, {
                    SI64(a),
                    SI64(state.honestBlocks),
                    SI64(state.forkState),
                    SI64(state.rgh),
                    SI64(state.rga),
                    SI64(state.mch),
                    SI64(state.bitmaskLen),
                    SI64(state.bitmask)
            });
        }

/*
        // count honest blocks that arrived at a >= K
        UI64 bmH = UI64(bmLen + rga) - (tailTimes.size() - 1);
        // count unrecorded honest blocks from a < K
        UI64 CPA = bmH >= h
                ? 0
                : UI64(h) - UI64(bmH);
        // count total uncompressed bitmask size
        UI32 activeRaceLength = CPA + rga + bmLen + state.adversaryBlocks;
*/
        assert(f != 2 || (a >= h && h > 0));

        bool canAdopt = h > 0;
        bool canOverride = a > h;
        bool canWait = max(a, h) < T;
//        bool canMatch = canWait and a >= h and h <= K and h > 0 and f == 1 and gamma > 0;
        bool canMatch = canWait and a >= h and h > 0 and f == 1 and gamma > 0;

//        bool honestTimeAtStake = (h > K) or ((h == K) and (f == 1));
        bool honestTimeAtStake = (h > K) or ((h >= K) and (f == 1));
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
            LD ra = alpha * (1);
            LD rh = (1 - alpha) * (rgh + (1 - gamma) * mch + 1);
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
            State outcome_2 = appendBlock(initialState, true, K, gamma, false, true, !forgive, overpay, estimate);
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
            State outcome_3 = appendBlock(initialState, false, K, gamma, false, true, !forgive, overpay, estimate);
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
            UI32 tailSumLimit = state.adversaryBlocks < K
                    ? 0
                    : (h+1+K) - state.adversaryBlocks;
            UI16 cpa = safeSum(tailTimes, 0, tailSumLimit);
            LD ra = forgive
                    ? alpha * (1)
                    : alpha * (rga + cpa + ((a-(h+1)) < K));
            LD rh = (1 - alpha) * (1 + gamma * mch);
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
            State outcome_2 = appendBlock(postOverride, true, K, gamma, false, true, !forgive, overpay, estimate);
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
            State outcome_3 = appendBlock(postOverride, false, K, gamma, false, true, !forgive, overpay, estimate);
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
                LD ra = forgive
                        ? alpha * (1)
                        : alpha * (a < K);
                LD rh = (1 - alpha) * (h < K);
                LD pE = (ra + rh) / H;
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
                State outcome_2 = appendBlock(state, true, K, gamma, false, true, !forgive, overpay, estimate);
//                outcome_2.printToLine(buff);
                addRewardedTransition(
                        choices,
                        rewardedChoiceTransitions,
                        outcome_2,
                        alpha * (1 - pE),
                        0,
                        visited,
                        bfs,
                        stateIndexMap);
                // outcome 3: next block arrival is for the honest miners
                State outcome_3 = appendBlock(state, false, K, gamma, false, true, !forgive, overpay, estimate);
//                outcome_3.printToLine(buff);
                addRewardedTransition(
                        choices,
                        rewardedChoiceTransitions,
                        outcome_3,
                        (1 - alpha) * (1 - pE),
                        0,
                        visited,
                        bfs,
                        stateIndexMap);
                addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder, transitionRewardMatrixBuilder, 3);
                addChoiceReward(ra, choices, choiceRewardVector, true);
            }
            else { // match active
//                buff << "WAIT -> ";
                goto match_code;
                wait_match_code:
                addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder, transitionRewardMatrixBuilder, 3);
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
            UI32 tailSumLimit = state.adversaryBlocks < K
                                ? 0
                                : (h+K) - state.adversaryBlocks;
            UI16 cpa = safeSum(tailTimes, 0, tailSumLimit);

            LD matchRa = forgive
                    ? alpha * (1)
                    : alpha * (rga + cpa + ((a-h)<K));

            assert(h >= K or mch == 0); // h < K implies mch == 0
            LD matchRh = h < K
                         ? (1 - alpha) * (1)
                         : (1 - alpha) * (gamma * (mch + 1));
            LD matchPE = (matchRa + matchRh) / H;

            assert(!estimate or mch == 0); // estimate implies mch == 0

            LD mineRa = forgive
                    ? alpha * (1)
                    : alpha * (a < K);
//            LD mineRh = (1 - alpha) * (h < K);
            LD mineRh = h < K
                    ? (1 - alpha)
                    : estimate ? (1 - alpha) * gamma : 0;
            LD minePE = (mineRa + mineRh) / H;

            State postMatch = confirmBlocks(h, tailTimes, state, K, true);
//            postMatch.printToLine(buff);
            // outcome 1e: exit after immediate rewards
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    exitState,
                    alpha * minePE,
                    mineRa,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 1: next block arrival is for adversary
            State outcome_1 = appendBlock(state, true, K, gamma, true, true, !forgive, overpay, estimate);
//            outcome_1.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_1,
                    alpha * (1 - minePE),
                    mineRa,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 2e: exit after immediate rewards
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    exitState_1,
                    (1 - alpha) * (1 - gamma) * minePE,
                    mineRa,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 2: next block arrival is for uncompromised honest miners, and match fails
            State outcome_2 = appendBlock(state, false, K, gamma, true, true, !forgive, overpay, estimate);
//            outcome_2.printToLine(buff);
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    outcome_2,
                    (1 - alpha) * (1 - gamma) * (1 - minePE),
                    mineRa,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3e: exit after immediate rewards
            addRewardedTransition(
                    choices,
                    rewardedChoiceTransitions,
                    exitState_2,
                    (1 - alpha) * gamma * matchPE,
                    matchRa,
                    visited,
                    bfs,
                    stateIndexMap);
            // outcome 3: next block arrival is for compromised honest miners
            State outcome_3 = appendBlock(postMatch, false, K, gamma, false, true, !forgive, overpay, estimate);
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
            addRewardedChoice(choices, rewardedChoiceTransitions, transitionMatrixBuilder, transitionRewardMatrixBuilder, 4);
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
    auto stateLabels = vector<string>{"init", "exit"};
    for (auto const &lab : stateLabels) {
        stateLabeling.addLabel(lab);
    }
    stateLabeling.addLabelToState("init", getOrSetStateIndex(initialState, stateIndexMap));
    stateLabeling.addLabelToState("exit", getOrSetStateIndex(exitState, stateIndexMap));
    if (gamma > 0) {
        if (stateIndexMap.count(exitState_1) > 0) {
            stateLabeling.addLabelToState("exit", getOrSetStateIndex(exitState_1, stateIndexMap));
        }
        if (stateIndexMap.count(exitState_2) > 0) {
            stateLabeling.addLabelToState("exit", getOrSetStateIndex(exitState_2, stateIndexMap));
        }
    }
    // build reward matrix
    auto transitionRewardMatrix = transitionRewardMatrixBuilder.build();
    auto standardRewardModel = SparseRewardModel(boost::none, choiceRewardVector, transitionRewardMatrix);
    assert(not standardRewardModel.hasStateRewards());
    assert(standardRewardModel.hasStateActionRewards());
    assert(standardRewardModel.hasTransitionRewards());
/*
    if (bisimulateable) {
        standardRewardModel.reduceToStateBasedRewards(transitionMatrix, false);
        assert(not standardRewardModel.hasStateRewards());
        assert(standardRewardModel.hasStateActionRewards());
        assert(not standardRewardModel.hasTransitionRewards());
    }
*/
    auto rewardModelsMap = unordered_map<string, SparseRewardModel>{{"revenue", standardRewardModel}};
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
        mdpDotFile.open ("sparseModel.gv");
        mdp->writeDotToStream(mdpDotFile);
        mdpDotFile.close();
    }
    return mdp;
}
