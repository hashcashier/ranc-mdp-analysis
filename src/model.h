//
// Created by rami on 03/03/2021.
//

#ifndef POSH_MDP_ANALYSIS_MODEL_H
#define POSH_MDP_ANALYSIS_MODEL_H

#include <storm/utility/initialize.h>
#include <storm/settings/modules/GeneralSettings.h>
#include <storm/api/storm.h>
#include <storm-pars/api/storm-pars.h>
#include "storm/storage/expressions/ExpressionManager.h"
#include <storm-parsers/api/storm-parsers.h>
#include <storm/storage/jani/Property.h>
#include <storm/modelchecker/results/CheckResult.h>
#include <storm/modelchecker/results/ExplicitQuantitativeCheckResult.h>

#include "util.h"

typedef storm::expressions::Variable Variable;
typedef storm::logic::Formula Formula;

shared_ptr<const Formula> getFormula(const string &propertyString);

template <typename ModelType, typename CheckerType, typename VT>
std::unique_ptr<storm::modelchecker::CheckResult> checkModel(ModelType &model, const shared_ptr<const Formula> &formula, bool withScheduler) {
    // Create a check task with the formula. Run this task with the model checker.
    auto checkTask = storm::modelchecker::CheckTask<storm::logic::Formula, VT>(*formula, true);
    auto checker = make_shared<CheckerType>(model);
    if (withScheduler) {
        checkTask.setProduceSchedulers(withScheduler);
    }
    assert(checker->canHandle(checkTask));
    return checker->check(checkTask);
}

#endif //POSH_MDP_ANALYSIS_MODEL_H
