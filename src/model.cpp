//
// Created by rami on 03/03/2021.
//

#include "model.h"

shared_ptr<const Formula> getFormula(const string &propertyString) {
    auto properties = storm::api::parseProperties(propertyString);
    auto formulae = storm::api::extractFormulasFromProperties(properties);
    shared_ptr<const Formula> formula = formulae[0];
    cout << "Formula: " << propertyString << ' ' << formula->toString() << endl;
/*//    if (formula->isAtomicExpressionFormula()) cout << "isAtomicExpressionFormula? " << formula->isAtomicExpressionFormula() << endl;
//    if (formula->isAtomicLabelFormula()) cout << "isAtomicLabelFormula? " << formula->isAtomicLabelFormula() << endl;
//    if (formula->isBinaryBooleanStateFormula()) cout << "isBinaryBooleanStateFormula? " << formula->isBinaryBooleanStateFormula() << endl;
//    if (formula->isBinaryPathFormula()) cout << "isBinaryPathFormula? " << formula->isBinaryPathFormula() << endl;
//    if (formula->isBinaryStateFormula()) cout << "isBinaryStateFormula? " << formula->isBinaryStateFormula() << endl;
//    if (formula->isBooleanLiteralFormula()) cout << "isBooleanLiteralFormula? " << formula->isBooleanLiteralFormula() << endl;
//    if (formula->isBoundedUntilFormula()) cout << "isBoundedUntilFormula? " << formula->isBoundedUntilFormula() << endl;
//    if (formula->isConditionalProbabilityFormula()) cout << "isConditionalProbabilityFormula? " << formula->isConditionalProbabilityFormula() << endl;
//    if (formula->isConditionalRewardFormula()) cout << "isConditionalRewardFormula? " << formula->isConditionalRewardFormula() << endl;
//    if (formula->isCumulativeRewardFormula()) cout << "isCumulativeRewardFormula? " << formula->isCumulativeRewardFormula() << endl;
//    if (formula->isEventuallyFormula()) cout << "isEventuallyFormula? " << formula->isEventuallyFormula() << endl;
//    if (formula->isFalseFormula()) cout << "isFalseFormula? " << formula->isFalseFormula() << endl;
//    if (formula->isGloballyFormula()) cout << "isGloballyFormula? " << formula->isGloballyFormula() << endl;
//    if (formula->isInitialFormula()) cout << "isInitialFormula? " << formula->isInitialFormula() << endl;
//    if (formula->isInstantaneousRewardFormula()) cout << "isInstantaneousRewardFormula? " << formula->isInstantaneousRewardFormula() << endl;
//    if (formula->isLongRunAverageOperatorFormula()) cout << "isLongRunAverageOperatorFormula? " << formula->isLongRunAverageOperatorFormula() << endl;
//    if (formula->isLongRunAverageRewardFormula()) cout << "isLongRunAverageRewardFormula? " << formula->isLongRunAverageRewardFormula() << endl;
//    if (formula->isMultiObjectiveFormula()) cout << "isMultiObjectiveFormula? " << formula->isMultiObjectiveFormula() << endl;
//    if (formula->isNextFormula()) cout << "isNextFormula? " << formula->isNextFormula() << endl;
//    if (formula->isOperatorFormula()) cout << "isOperatorFormula? " << formula->isOperatorFormula() << endl;
//    if (formula->isPathFormula()) cout << "isPathFormula? " << formula->isPathFormula() << endl;
//    if (formula->isProbabilityOperatorFormula()) cout << "isProbabilityOperatorFormula? " << formula->isProbabilityOperatorFormula() << endl;
//    if (formula->isProbabilityPathFormula()) cout << "isProbabilityPathFormula? " << formula->isProbabilityPathFormula() << endl;
//    if (formula->isQuantileFormula()) cout << "isQuantileFormula? " << formula->isQuantileFormula() << endl;
//    if (formula->isReachabilityProbabilityFormula()) cout << "isReachabilityProbabilityFormula? " << formula->isReachabilityProbabilityFormula() << endl;
//    if (formula->isReachabilityRewardFormula()) cout << "isReachabilityRewardFormula? " << formula->isReachabilityRewardFormula() << endl;
//    if (formula->isReachabilityTimeFormula()) cout << "isReachabilityTimeFormula? " << formula->isReachabilityTimeFormula() << endl;
//    if (formula->isRewardOperatorFormula()) cout << "isRewardOperatorFormula? " << formula->isRewardOperatorFormula() << endl;
//    if (formula->isRewardPathFormula()) cout << "isRewardPathFormula? " << formula->isRewardPathFormula() << endl;
//    if (formula->isStateFormula()) cout << "isStateFormula? " << formula->isStateFormula() << endl;
//    if (formula->isTimeOperatorFormula()) cout << "isTimeOperatorFormula? " << formula->isTimeOperatorFormula() << endl;
//    if (formula->isTimePathFormula()) cout << "isTimePathFormula? " << formula->isTimePathFormula() << endl;
//    if (formula->isTotalRewardFormula()) cout << "isTotalRewardFormula? " << formula->isTotalRewardFormula() << endl;
//    if (formula->isTrueFormula()) cout << "isTrueFormula? " << formula->isTrueFormula() << endl;
//    if (formula->isUnaryBooleanStateFormula()) cout << "isUnaryBooleanStateFormula? " << formula->isUnaryBooleanStateFormula() << endl;
//    if (formula->isUnaryPathFormula()) cout << "isUnaryPathFormula? " << formula->isUnaryPathFormula() << endl;
//    if (formula->isUnaryStateFormula()) cout << "isUnaryStateFormula? " << formula->isUnaryStateFormula() << endl;
//    if (formula->isUntilFormula()) cout << "isUntilFormula? " << formula->isUntilFormula() << endl;*/
    return formula;
}
