#ifndef SHIP3_ABSTRACTALGORITHM_H
#define SHIP3_ABSTRACTALGORITHM_H

#pragma once

#include <string>
class WeightBalanceCalculator;

class AbstractAlgorithm {
public:
    virtual ~AbstractAlgorithm()= default;

    enum class Action { LOAD = 'L', UNLOAD = 'U', MOVE = 'M', REJECT = 'R'};

// methods below return int for returning 0 for success
// and any other number as error code
    virtual int readShipPlan(const std::string& full_path_and_file_name) = 0;
    virtual int readShipRoute(const std::string& full_path_and_file_name) = 0;
    virtual int setWeightBalanceCalculator(WeightBalanceCalculator& calculator) = 0;
    virtual int getInstructionsForCargo(
            const std::string& input_full_path_and_file_name,
            const std::string& output_full_path_and_file_name) = 0;
};


#endif //SHIP3_ABSTRACTALGORITHM_H
