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
#include "ship.h"
#include "Parser.h"
#include "AbstractAlgorithm.h"
#include "lifo_algorithm.h"
#include "Unsorted_Lifo_Algorithm.h"
#include "erroneous_algorithm.h"
#include "outputHandler.h"
#include "common.h"
#include <memory>
#include <bitset>


/*------------------------------Global Variables---------------------------*/

string mainTravelPath;
string mainAlgorithmsPath;
string mainOutputPath;

/*-----------------------------Utility Functions-------------------------*/

/**
 * This function creates a vector of algorithms that the simulator willing to test on
 * @param algList
 * @param ship
 */
void initAlgorithmList(vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> &algList,map<string,vector<fs::path>> &travelFolder){
        string routePath = travelFolder.at(ROUTE).at(0).string();
        string planPath = travelFolder.at(PLAN).at(0).string();
//    //TODO make polymorphic algorithm factory & change to smart pointers
//    std::unique_ptr<AbstractAlgorithm> lifoAlgorithm = std::make_unique<Lifo_algorithm>();
//    std::unique_ptr<AbstractAlgorithm> unsortedLifoAlgorithm = std::make_unique<Unsorted_Lifo_Algorithm>();
//    std::unique_ptr<AbstractAlgorithm> erroneousAlgorithm = std::make_unique<Erroneous_algorithm>();
//    algList.emplace_back(std::move(lifoAlgorithm));
//    algList.emplace_back(unsortedLifoAlgorithm);
//    algList.emplace_back(erroneousAlgorithm);
//    //init alg data
    for(auto &alg : algList){
        alg.second->readShipPlan(planPath);
        alg.second->readShipRoute(routePath);
    }
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
    if(argc == 0)
        exit(EXIT_FAILURE);
    else if(argc == 2){
        mainAlgorithmsPath = basePath;
        mainOutputPath = basePath;
    }
    else if(argc == 3)
        mainAlgorithmsPath = argv[2];
    else{
        mainAlgorithmsPath = argv[2];
        mainOutputPath = argv[3];
    }
    mainTravelPath = argv[1];
}

void initArrayOfErrors(std::array<bool,19> &arr,int num){
    string binary = std::bitset<19>(num).to_string();
    int index = 0;
    //TODO doesnt work properly yet.
    for(string::iterator it = binary.end(); it != binary.begin(); it--){
        arr[index] = *it;
        index++;
    }
}
/**
 * This function handles current port and algorithm interaction
 * @param portName          - the current port name
 * @param portPath          - the full <port name>_<number>.cargo_data path
 * @param portNum           - tracking the current port in the route list
 * @param alg               - current alg
 * @param simShip           - the current simulator ship
 * @param simCurrAlgErrors  - the list of current algorithm errros
 * @param algOutputFolder   - the alg output folder to write the output file to
 * @param visitNumber       - current port visit number
 * @param algInfo           - stores the data of instructions count and errors count
 */
void runCurrentPort(string &portName,fs::path &portPath,int portNum,pair<string,std::unique_ptr<AbstractAlgorithm>> &alg,std::unique_ptr<Ship> &simShip,
        list<string> &simCurrAlgErrors,string &algOutputFolder,int visitNumber,map<string,pair<int,int>> &algInfo){

    string inputPath,outputPath;
    int instructionsCount, errorsCount, algReturnValue;
    std::optional<pair<int,int>> result;
    pair<int,int> intAndError;
    std::array<bool,19> errors{false};

    if(portPath.empty())
        inputPath = "";
    else
        inputPath =  portPath.string();

    outputPath = algOutputFolder + PATH_SEPARATOR + portName + "_" + std::to_string(visitNumber) + ".crane_instructions";
    algReturnValue = alg.second->getInstructionsForCargo(inputPath,outputPath);
    initArrayOfErrors(errors,algReturnValue);
    result = validateAlgorithm(outputPath,inputPath,simShip,portNum,simCurrAlgErrors);
    if(!result) return; //case there was an error in validateAlgorithm

    /*Incrementing the instructions count and errors count*/
    intAndError = result.value();
    instructionsCount = std::get<0>(intAndError);
    errorsCount = std::get<1>(intAndError);
    if(algInfo.find(alg.first) == algInfo.end()){
        algInfo.insert(make_pair(alg.first,pair<int,int>()));
    }
    std::get<0>(algInfo[alg.first]) += instructionsCount;
    std::get<1>(algInfo[alg.first]) += errorsCount;
}

int main(int argc, char** argv) {

    vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> algVec;
    map<string,map<string,pair<int,int>>> outputResultsInfo;
    map<string,map<string,list<string>>> outputSimulatorErrors;
    map<string,map<string,vector<fs::path>>> inputFiles;
    initPaths(argc,argv);
    initListDirectories(mainTravelPath, inputFiles);

    /*Cartesian Loop*/
    for (auto &travel_folder : inputFiles) {
        list<string> currTravelGeneralErrors;
        map<string,list<string>> simCurrTravelErrors;
        string currTravelName = travel_folder.first;
        std::unique_ptr<Ship> mainShip = extractArgsForShip(travel_folder.second,currTravelGeneralErrors);
        if(mainShip == nullptr){
            simCurrTravelErrors.insert(make_pair("general",currTravelGeneralErrors));
            outputSimulatorErrors.insert(make_pair(currTravelName,simCurrTravelErrors));
            continue; /* can happen if either route/map files are erroneous*/
        }
        mainShip->initCalc();
        initAlgorithmList(algVec,travel_folder.second);
        map<string,pair<int,int>> algInfo; /*This is a list of algorithms and counting their errors and instruction per travel*/
        for (auto &alg : algVec) {
            std::unique_ptr<Ship> simShip = std::make_unique<Ship>(*mainShip);
            list<string> simCurrAlgErrors;
            map<string,int> visitNumbersByPort;
            for(int portNum = 0; portNum < (int)simShip->getRoute().size(); portNum++){
                string portName = simShip->getRoute()[portNum]->get_name();
                int visitNumber = visitNumbersByPort[portName];
                fs::path portPath(travel_folder.second[portName][visitNumber]);
                string algOpFolder = createAlgorithmOutDirectory(alg.first,mainOutputPath,currTravelName);
                runCurrentPort(portName,portPath,portNum,alg,simShip,simCurrAlgErrors,algOpFolder,++visitNumbersByPort[portName],algInfo);
            }
            simCurrTravelErrors.insert(make_pair(typeid(alg).name(),simCurrAlgErrors));
            simShip.reset(nullptr);
        }
        outputSimulatorErrors.insert(make_pair(currTravelName,simCurrTravelErrors));
        outputResultsInfo.insert(make_pair(currTravelName,algInfo));
        destroyAlgVec(algVec);
        mainShip.reset(nullptr);
    }
    createResultsFile(outputResultsInfo, mainTravelPath);
    createErrorsFile(outputSimulatorErrors, mainTravelPath);
    return (EXIT_SUCCESS);
 }

