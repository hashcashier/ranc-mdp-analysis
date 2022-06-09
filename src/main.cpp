#include <chrono>

#include "model_explicit_quality_extended.h"
#include "model_explicit_incentive_extended.h"
#include "model_explicit_subversion_extended.h"
#include "model_explicit_censorship_extended.h"
#include "model_explicit_reachability_extended.h"

#include "simulator_quality_extended.h"
#include "simulator_incentive_extended.h"
#include "simulator_subversion_extended.h"
#include "simulator_censorship_extended.h"

#include "model_explicit_subversion_nakamoto.h"
#include "model_explicit_censorship_nakamoto.h"

#include "simulator_censorship_nakamoto.h"
#include "simulator_subversion_nakamoto.h"


int main (int argc, char *argv[]) {
    if (argc < 6) {
        cout << "Usage: nakamoto quality 0 <T:uint> <alpha:0..100> <gamma:0..100>           <H:uint>                                                                                                <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: nakamoto incentive 0 <T:uint> <alpha:0..100> <gamma:0..100>         <H:uint>                                                                                                <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: nakamoto subversion 0 <T:uint> <alpha:0..100> <gamma:0..100>        <confirmations:0..T>    <reward:uint>                                                                   <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: nakamoto censorship 0 <T:uint> <alpha:0..100> <gamma:0..100>                                                                                                                <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: extended reachability <K:uint> <T:uint> <alpha:0..100> <gamma:0..100>" << endl;
        cout << "Usage: extended quality <K:uint> <T:uint> <alpha:0..100> <gamma:0..100>    <H:uint>                                                                                                <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: extended incentive <K:uint> <T:uint> <alpha:0..100> <gamma:0..100>  <H:uint>                                    <overpay:(y)/n>     <forgive:y/(n)>     <estimate:y/(n)>    <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: extended subversion <K:uint> <T:uint> <alpha:0..100> <gamma:0..100> <confirmations:0..T>    <reward:uint>       <overpay:(y)/n>     <forgive:y/(n)>                         <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        cout << "Usage: extended censorship <K:uint> <T:uint> <alpha:0..100> <gamma:0..100>                                                                                                         <scheduler:y/(n)>   <choiceLabels:y/(n)>" << endl;
        return 1;
    }
    UI32 K(strtoumax(argv[3], nullptr, 10));
    UI32 T(strtoumax(argv[4], nullptr, 10));
    LD alpha(strtoumax(argv[5], nullptr, 10) / 100.0);
    LD gamma(strtoumax(argv[6], nullptr, 10) / 100.0);

    bool nakamoto = strcmp(argv[1], "nakamoto") == 0;
    bool extended = strcmp(argv[1], "extended") == 0;

    bool qualityAnalysis = strcmp(argv[2], "quality") == 0;
    bool incentiveAnalysis = strcmp(argv[2], "incentive") == 0;
    bool subversionAnalysis = strcmp(argv[2], "subversion") == 0;
    bool censorshipAnalysis = strcmp(argv[2], "censorship") == 0;
    bool reachabilityAnalysis = strcmp(argv[2], "reachability") == 0;

    storm::utility::setUp();

    storm::settings::initializeAll("posh-mdp-analysis", "posh-mdp-analysis");
    storm::settings::SettingsManager::manager().setFromString("--general:verbose");

    storm::settings::SettingsManager::manager().setFromString("--enable-tbb");
    assert(storm::settings::getModule<storm::settings::modules::CoreSettings>().isUseIntelTbbSet());

    storm::settings::SettingsManager::manager().setFromString("--core:cuda");
    assert(storm::settings::getModule<storm::settings::modules::CoreSettings>().isUseCudaSet());

    storm::settings::SettingsManager::manager().setFromString("--minmax:method topological");
    storm::settings::SettingsManager::manager().setFromString("--topological:minmax pi");
    storm::settings::SettingsManager::manager().setFromString("--topological:eqsolver gmm++");

    cout << setprecision(4);

    if (nakamoto) {
        cout << "NAKAMOTO" << endl;
        if (qualityAnalysis or incentiveAnalysis) {
            cout << "Chain Quality / Incentive Compatibility Analysis" << endl;
            if (argc < 7 or K != 0) {
                cout << "Usage: nakamoto quality 0 <T:uint> <alpha:0..100> <gamma:0..100> <H:uint> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                cout << "Usage: nakamoto incentive 0 <T:uint> <alpha:0..100> <gamma:0..100> <H:uint> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }
            UI32 H(strtoumax(argv[7], nullptr, 10));

            bool withScheduler = argc >= 9 && (strcmp(argv[8], "y") == 0);
            bool withChoiceLabels = argc >= 10 && (strcmp(argv[9], "y") == 0);

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "H=" << H << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;

            auto propertyString = R"(R{"blocks"}max=? [ F "exit" ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseExtendedQualityMDP(
                    alpha,
                    gamma,
                    T,
                    K,
                    H,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
            cout << explicitResult[0] / H << endl;
            cout << 1 - (explicitResult[0] / H) << endl;
            cout << (1 - alpha) - (1 - (explicitResult[0] / H)) << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<LD> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateQualityExtended(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            K,
                            withChoiceLabels,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back() << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0;
                for (auto &elem : experimentResults) {
                    avg += elem;
                }
                cout << avg/n << endl;
            }

        }
        else if (subversionAnalysis) {
            cout << "Subversion Gain Analysis" << endl;
            if (argc < 8 or K != 0) {
                cout << "Usage: nakamoto subversion 0 <T:uint> <alpha:0..100> <gamma:0..100> <confirmations:0..T> <reward:uint> <overpay:(y)/n> <forgive:y/(n)> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }

            UI32 S(strtoumax(argv[7], nullptr, 10));
            LD D(strtoumax(argv[8], nullptr, 10) / 100.0);
            bool withScheduler = argc >= 10 && (strcmp(argv[9], "y") == 0);
            bool withChoiceLabels = argc >= 11 && (strcmp(argv[10], "y") == 0);

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "k=" << S << '\t' << "D=" << D << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;


            auto propertyString = R"(R{"revenue"}max=? [ LRA ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseNakamotoSubversionMDP(
                    alpha,
                    gamma,
                    T,
                    S,
                    D,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
            cout << explicitResult[0] << endl;
            cout << explicitResult[0] - alpha << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<LD> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateSubversionNakamoto(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            S,
                            D,
                            withChoiceLabels,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back() << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0, avg_t = 0;
                for (auto &elem : experimentResults) {
                    avg += elem;
                }
                cout << avg/n << endl;
            }
        }
        else if (censorshipAnalysis) {
            cout << "Censorship Susceptibility Analysis" << endl;
            if (argc < 6 or K != 0) {
                cout << "Usage: nakamoto censorship 0 <T:uint> <alpha:0..100> <gamma:0..100> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }

            bool withScheduler = argc >= 8 && (strcmp(argv[7], "y") == 0);
            bool withChoiceLabels = argc >= 9 && (strcmp(argv[8], "y") == 0);

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;

            auto propertyString = R"(R{"loss"}max=? [ LRA ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseNakamotoCensorshipMDP(
                    alpha,
                    gamma,
                    T,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
//        cout << explicitResult[0] << endl;
//        cout << (1 - alpha) - explicitResult[0] << endl;
//        cout << ((1 - alpha) - explicitResult[0]) / ((1 - alpha)) << endl;
            cout << (1 - alpha) - explicitResult[0] << endl;
            cout << explicitResult[0] << endl;
            cout << (explicitResult[0]) / ((1 - alpha)) << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<LD> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateCensorshipNakamoto(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            withChoiceLabels,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back() << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0;
                for (auto &elem : experimentResults) {
                    avg += elem;
                }
                cout << avg/n << endl;
            }
        }
        else {
            cout << "Unsupported analysis." << endl;
            return 1;
        }
    }
    else if (extended) {
        cout << "EXTENDED" << endl;
        if (qualityAnalysis) {
            cout << "Chain Quality Analysis" << endl;
            if (argc < 7) {
                cout << "Usage: extended quality <K:uint> <T:uint> <alpha:0..100> <gamma:0..100> <H:uint> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }
            UI32 H(strtoumax(argv[7], nullptr, 10));

            bool withScheduler = argc >= 9 && (strcmp(argv[8], "y") == 0);
            bool withChoiceLabels = argc >= 10 && (strcmp(argv[9], "y") == 0);

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "H=" << H << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;

            auto propertyString = R"(R{"blocks"}max=? [ F "exit" ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseExtendedQualityMDP(
                    alpha,
                    gamma,
                    T,
                    K,
                    H,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
            cout << explicitResult[0] / H << endl;
            cout << 1 - (explicitResult[0] / H) << endl;
            cout << (1 - alpha) - (1 - (explicitResult[0] / H)) << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<LD> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateQualityExtended(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            K,
                            withChoiceLabels,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back() << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0;
                for (auto &elem : experimentResults) {
                    avg += elem;
                }
                cout << avg/n << endl;
            }

        }
        else if (incentiveAnalysis) {
            cout << "Incentive Compatibility Analysis" << endl;
            if (argc < 7) {
                cout << "Usage: extended incentive <K:uint> <T:uint> <alpha:0..100> <gamma:0..100> <H:uint> <overpay:(y)/n> <forgive:y/(n)> <estimate:y/(n)> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }
            UI32 H(strtoumax(argv[7], nullptr, 10));

            bool overpay = argc < 9 || (strcmp(argv[8], "y") == 0);
            bool forgive = argc >= 10 && (strcmp(argv[9], "y") == 0);
            bool estimate = argc >= 11 && (strcmp(argv[10], "y") == 0);
            bool withScheduler = argc >= 12 && (strcmp(argv[11], "y") == 0);
            bool withChoiceLabels = argc >= 13 && (strcmp(argv[12], "y") == 0);
            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "H=" << H << '\t' << "O=" << overpay << '\t' << "F=" << forgive << '\t' << "E=" << estimate << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;

            auto propertyString = R"(R{"revenue"}max=? [ F "exit" ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseExtendedIncentiveMDP(
                    alpha,
                    gamma,
                    T,
                    K,
                    H,
                    overpay,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels,
                    forgive,
                    estimate);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
            cout << explicitResult[0] / H << endl;
            cout << 1 - (explicitResult[0] / H) << endl;
            cout << (1 - alpha) - (1 - (explicitResult[0] / H)) << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<pair<LD, LD>> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateIncentiveExtended(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            K,
                            overpay,
                            withChoiceLabels,
                            forgive,
                            estimate,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back().first << ',' << experimentResults.back().second << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0, avg_t = 0;
                for (auto &elem : experimentResults) {
                    avg += elem.first;
                    avg_t += elem.second;
                }
                cout << avg/n << ',' << avg_t/n << endl;
            }

        }
        else if (subversionAnalysis) {
            cout << "Subversion Gain Analysis" << endl;
            if (argc < 8) {
                cout << "Usage: extended subversion <K:uint> <T:uint> <alpha:0..100> <gamma:0..100> <confirmations:0..T> <reward:uint> <overpay:(y)/n> <forgive:y/(n)> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }

            UI32 S(strtoumax(argv[7], nullptr, 10));
            LD D(strtoumax(argv[8], nullptr, 10) / 100.0);
            bool overpay = argc < 10 || (strcmp(argv[9], "y") == 0);
            bool forgive = argc >= 11 && (strcmp(argv[10], "y") == 0);
            bool withScheduler = argc >= 12 && (strcmp(argv[11], "y") == 0);
            bool withChoiceLabels = argc >= 13 && (strcmp(argv[12], "y") == 0);

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "k=" << S << '\t' << "D=" << D << '\t' << "O=" << overpay << '\t' << "F=" << forgive << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;


            auto propertyString = R"(R{"revenue"}max=? [ LRA ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseExtendedSubversionMDP(
                    alpha,
                    gamma,
                    T,
                    K,
                    S,
                    D,
                    overpay,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels,
                    forgive);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
            cout << explicitResult[0] << endl;
            cout << explicitResult[0] - alpha << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<pair<LD, LD>> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateSubversionExtended(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            K,
                            S,
                            D,
                            overpay,
                            withChoiceLabels,
                            forgive,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back().first << ',' << experimentResults.back().second << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0, avg_t = 0;
                for (auto &elem : experimentResults) {
                    avg += elem.first;
                    avg_t += elem.second;
                }
                cout << avg/n << ',' << avg_t/n << endl;
            }
        }
        else if (censorshipAnalysis) {
            cout << "Censorship Susceptibility Analysis" << endl;
            if (argc < 6) {
                cout << "Usage: extended censorship <K:uint> <T:uint> <alpha:0..100> <gamma:0..100> <scheduler:y/(n)> <choiceLabels:y/(n)>" << endl;
                return 1;
            }

            bool withScheduler = argc >= 8 && (strcmp(argv[7], "y") == 0);
            bool withChoiceLabels = argc >= 9 && (strcmp(argv[8], "y") == 0);

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << '\t' << "S=" << withScheduler << '\t' << "C=" << withChoiceLabels << endl;

//        auto propertyString = R"(R{"revenue"}min=? [ LRA ])";
            auto propertyString = R"(R{"loss"}max=? [ LRA ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseMDP> sparseMDP = constructSparseExtendedCensorshipMDP(
                    alpha,
                    gamma,
                    T,
                    K,
                    withChoiceLabels || withScheduler,
                    withChoiceLabels);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseMDP->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseMDP, SparseMDPChecker, LD>(*sparseMDP, targetFormula, withScheduler);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
//        cout << explicitResult[0] << endl;
//        cout << (1 - alpha) - explicitResult[0] << endl;
//        cout << ((1 - alpha) - explicitResult[0]) / ((1 - alpha)) << endl;
            cout << (1 - alpha) - explicitResult[0] << endl;
            cout << explicitResult[0] << endl;
            cout << (explicitResult[0]) / ((1 - alpha)) << endl;

            if (withScheduler) {
                cout << "Simulation:" << endl;
                sparseStart = chrono::high_resolution_clock::now();
                auto scheduler = explicitResult.getScheduler();
                auto stateChoiceMap = constructExtendedStateChoiceMap(sparseMDP, scheduler);
                vector<pair<LD, LD>> experimentResults;
                UI32 n = 20;
                experimentResults.reserve(n);
                for (int i = 0; i < n; i++) {
                    experimentResults.push_back(simulateCensorshipExtended(
                            stateChoiceMap,
                            alpha,
                            gamma,
                            T,
                            K,
                            withChoiceLabels,
                            (1ULL << 20),
                            i * time(nullptr)));
//                        i));
                    if (i > 0 && (i%5) == 0) {
                        cout << endl;
                    }
                    cout << experimentResults.back().first << ',' << experimentResults.back().second << "\t\t";
                    cout.flush();
                }
                cout << endl;
                sparseStop = chrono::high_resolution_clock::now();
                sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
                cout << "Seconds: " << sparseDelta.count() << endl;
                LD avg = 0, avg_t = 0;
                for (auto &elem : experimentResults) {
                    avg += elem.first;
                    avg_t += elem.second;
                }
                cout << avg/n << ',' << avg_t/n << endl;
            }
        }
        else if (reachabilityAnalysis) {
            cout << "Threat Reachability Analysis" << endl;
            if (argc != 6) {
                cout << "Usage: extended reachability <K:uint> <T:uint> <alpha:0..100> <gamma:0..100>" << endl;
                return 1;
            }

            cout << "K=" << K << '\t' << "T=" << T << '\t' << "A=" << alpha << '\t' << "G=" << gamma << endl;

//        auto propertyString = R"(Pmax=? [ F<)" + to_string(H)  + R"( "threat" ])";
            auto propertyString = R"(P=? [ F "threat" ])";
            auto targetFormula = getFormula(propertyString);
            cout << "Explicit Model Construction:" << endl;
            auto sparseStart = chrono::high_resolution_clock::now();
            shared_ptr<SparseDTMC> sparseDTMC = constructSparseExtendedReachabilityDTMC(
                    alpha,
                    gamma,
                    T,
                    K);
            auto sparseStop = chrono::high_resolution_clock::now();
            auto sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            mem_usage();
            sparseDTMC->printModelInformationToStream(cout); cout << endl;

            cout << "Model Checking:" << endl;
            sparseStart = chrono::high_resolution_clock::now();
            auto result = checkModel<SparseDTMC, SparseDTMCChecker, LD>(*sparseDTMC, targetFormula, false);
            sparseStop = chrono::high_resolution_clock::now();
            sparseDelta = chrono::duration_cast<chrono::seconds>(sparseStop - sparseStart);
            cout << "Seconds: " << sparseDelta.count() << endl;
            assert(result->isExplicitQuantitativeCheckResult());
            auto explicitResult = result->asExplicitQuantitativeCheckResult<LD>();
            cout << explicitResult[0] << endl;
        }
        else {
            cout << "Unsupported analysis." << endl;
            return 1;
        }
    } else {
        cout << "Unsupported protocol." << endl;
        return 1;
    }

    return 0;
}
