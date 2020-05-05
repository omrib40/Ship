/**
 * This module is includes the main function which simulates the cartesian loop such that for every travel folder
 * and every algorithm in runs the simulation. given a list of {Alg1,Alg2,Alg3...} and a list of {Travel1,Travel2..}
 * for every i,and j the simulator runs Algi over Travelj, every iteration over Travelj the algorithm gets from the
 * simulator the next port cargo file, which includes containers awaiting at this port then :
 * 1. the algorithm generates crane instructions file including Load/Unload/Move/Reject operations
 * 2. the simulator validates the crane instructions file generated by the algorithm, and gather erroneous operations
 * after finishing Travelj and prior to run Travelx the simulator saves the information it gathered in a data structure.
 * at the end while every Algi iterated over every Travelj the simulator creates 2 output files :
 * 1. Simulation.errors  - including the general errors the simulator found, so the errors it found validating the algs.
 * 2. Simulation.results - representing the total number of L/R/M instructions of every algorithm on every Travel, also
 * the total number of errors a given algorithm made over all the travel folders, sorted by following criteria :
 * 1. first will be shown the algorithm with the lowest errors occurred(generated by the simulator).
 * 2. second will be compared only iff 2 algorithms errors count is same the it will be sorted by number of instructions.
 * Note* - if no output path given -> output files will be at the directory the main program runs from.
 */
#include <string>
#include "Ship.h"
#include "Parser.h"
#include "AbstractAlgorithm.h"
#include "UnsortedLifoAlgorithm.h"
#include "SimulatorObj.h"
#include "LifoAlgorithm.h"
#include <memory>

/*------------------------------Global Variables---------------------------*/

string mainTravelPath;
string mainAlgorithmsPath = fs::current_path().string();
string mainOutputPath = fs::current_path().string();

/*-----------------------------Utility Functions-------------------------*/

/**
 * This function creates a vector of algorithms that the simulator willing to test on
 * @param algList
 * @param ship
 */
void initAlgorithmList(vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> &algList){
////    //TODO make polymorphic algorithm factory & change to smart pointers
//    std::unique_ptr<AbstractAlgorithm> lifoAlgorithm = std::make_unique<Lifo_algorithm>();
//    std::unique_ptr<AbstractAlgorithm> unsortedLifoAlgorithm = std::make_unique<UnsortedLifoAlgorithm>();
//    pair<string,std::unique_ptr<AbstractAlgorithm>> pair("Lifo_algorithm",std::make_unique<Lifo_algorithm>());
    algList.emplace_back(make_pair("Lifo_algorithm",std::make_unique<LifoAlgorithm>()));
    algList.emplace_back(make_pair("UnsortedLifoAlgorithm",std::make_unique<UnsortedLifoAlgorithm>()));


    /*TODO need to init the algList with alg name and the algorithm object*/
}

/**
 * This function iterate through the vector and delete each algorithm
 * @param algVec
 */
void destroyAlgVec(vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> &algVec){
    for(auto &alg : algVec)
        alg.second.reset(nullptr);

    algVec.clear();
}

/**
 * This function gets the paths or sets them to be the current working directory
 * @param argc
 * @param argv
 */
void initPaths(int argc,char** argv){
    string basePath = fs::current_path().string();
    const string travelFlag = "-travel_path";
    const string outputFlag = "-output_path";
    const string algorithmFlag = "-algorithm_path";

    for(int i = 1; i+1 < argc; i++){
        if(argv[i] == travelFlag)
            mainTravelPath = argv[i+1];
        else if(argv[i] == outputFlag)
            mainOutputPath = argv[i+1];
        else if(argv[i] == algorithmFlag)
            mainAlgorithmsPath = argv[i+1];
    }

    if(mainTravelPath.empty()) {
        ERROR_NOTRAVELPATH;
        exit(EXIT_FAILURE);
    }
}

/**
 * This function gets the algorithms.so files from the mainAlgorithms path (if given or from current path)
 * and saves the paths in the given vector.
 * @param algPaths
 */
void getAlgSoFiles(vector<fs::path> &algPaths){
    std::regex reg("_[0-9]+_[a-z]+\\.so");
    for(const auto &entry : fs::directory_iterator(mainAlgorithmsPath)) {
        if (!entry.is_directory()) {
            if (std::regex_match(entry.path().filename().string(), reg)) {
                algPaths.emplace_back(entry);
            }
        }
    }
}


int main(int argc, char** argv) {

    vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> algVec;
    vector<fs::path> algPaths;
    initPaths(argc,argv);
    SimulatorObj simulator(mainTravelPath,mainOutputPath);
    getAlgSoFiles(algPaths);
    initAlgorithmList(algVec);

    /*Cartesian Loop*/
    for (auto &travel_folder : simulator.getInputFiles()) {
        string currTravelName = travel_folder.first;
        std::unique_ptr<Ship> mainShip = extractArgsForShip(currTravelName,simulator);
        if(mainShip != nullptr){
            for (auto &alg : algVec) {
                WeightBalanceCalculator algCalc;
                int errCode1 = alg.second->readShipPlan(travel_folder.second.at(PLAN).at(1).string());
                int errCode2 = alg.second->readShipRoute(travel_folder.second.at(ROUTE).at(1).string());
                int errCode3 = algCalc.readShipPlan(travel_folder.second.at(PLAN).at(1).string());
                simulator.updateArrayOfCodes(errCode1 + errCode2 + errCode3,"alg");
                simulator.setShip(mainShip);
                simulator.runCurrentAlgorithm(alg,currTravelName);
                simulator.getShip().reset(nullptr);
            }
        }
        simulator.addOutputInfo(currTravelName);
        simulator.prepareForNewTravel();
        mainShip.reset(nullptr);
    }
    simulator.createResultsFile(mainTravelPath);
    simulator.createErrorsFile(mainTravelPath);
    destroyAlgVec(algVec);
    return (EXIT_SUCCESS);
 }
