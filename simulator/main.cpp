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
#include "../common/Ship.h"
#include "../common/Parser.h"
#include "AlgorithmFactoryRegistrar.h"
#include <dlfcn.h>
#include <memory>

/*------------------------------Global Variables---------------------------*/

string mainTravelPath;
string mainAlgorithmsPath = fs::current_path().string();
string mainOutputPath = fs::current_path().string();

/*------------------------------Shared Objects Deleter ---------------------------*/

struct DlCloser {
    void operator()(void *dlhandle) const noexcept {
        dlclose(dlhandle);
    }
};

/*-----------------------------Utility Functions-------------------------*/

/**
 * This function creates a vector of algorithms that the simulator willing to test on
 * @param algList
 * @param ship
 */
void initAlgorithmList(vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> &algList, map<string ,std::function<std::unique_ptr<AbstractAlgorithm>()>>& map){
    for(auto &entry: map){
        algList.emplace_back(make_pair(entry.first,entry.second()));
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

void dynamicLoadSoFiles(vector<fs::path>& algPaths, vector<std::unique_ptr<void, DlCloser>>& SharedObjs){
   for(auto& path : algPaths){
       std::unique_ptr<void, DlCloser> soAlg(dlopen(path.c_str(), RTLD_LAZY));
       if(!soAlg){
           std::cerr << "dlopen failed" << dlerror() << std::endl;
       } else {
           SharedObjs.emplace_back(std::move(soAlg));
       }
   }
}

void parseRegisteredAlg(vector<fs::path>& algPaths, map<string ,std::function<std::unique_ptr<AbstractAlgorithm>()>>& map) {
    auto &vec = AlgorithmFactoryRegistrar::getRegistrar().getVec();

    for (auto &algPath : algPaths) {
        string algName = algPath.filename().string();
        algName = algName.substr(0, algName.find(".so"));
        bool isRegistered = false;

        for (auto &algFactory : vec) {
            std::string algTypeidName = typeid(*algFactory()).name();
            if (algTypeidName.find(algName) != std::string::npos) {
                isRegistered = true;
                map.insert({algName, algFactory});
                break;
            }
        }
        if (!isRegistered) P_ALGNOTREGISTER(algName);
    }
}

int main(int argc, char** argv) {

    map<string ,std::function<std::unique_ptr<AbstractAlgorithm>()>> map;
    vector<pair<string,std::unique_ptr<AbstractAlgorithm>>> algVec;
    vector<std::unique_ptr<void, DlCloser>> SharedObjs;
    vector<fs::path> algPaths;
    initPaths(argc,argv);
    SimulatorObj simulator(mainTravelPath,mainOutputPath);
    getAlgSoFiles(algPaths);
    dynamicLoadSoFiles(algPaths, SharedObjs);
    parseRegisteredAlg(algPaths, map);

    /*Cartesian Loop*/
    for (auto &travel_folder : simulator.getTravels()) {
        initAlgorithmList(algVec, map);
        string currTravelName = travel_folder.first;
        std::unique_ptr<Ship> mainShip = extractArgsForShip(currTravelName,simulator);
        if(mainShip != nullptr){
            for (auto &alg : algVec) {
                WeightBalanceCalculator algCalc;
                int errCode1 = alg.second->readShipPlan(travel_folder.second.at(PLAN).at(1).string());
                int errCode2 = alg.second->readShipRoute(travel_folder.second.at(ROUTE).at(1).string());
                int errCode3 = algCalc.readShipPlan(travel_folder.second.at(PLAN).at(1).string());
                alg.second->setWeightBalanceCalculator(algCalc);
                simulator.updateArrayOfCodes(errCode1 + errCode2 + errCode3,"alg");
                simulator.setShipAndCalculator(mainShip,travel_folder.second.at(PLAN).at(1).string());
                simulator.runCurrentAlgorithm(alg,currTravelName);
                simulator.getShip().reset(nullptr);
            }
        }
        simulator.addOutputInfo(currTravelName);
        simulator.prepareForNewTravel();
        mainShip.reset(nullptr);
        destroyAlgVec(algVec);
    }
    simulator.createResultsFile(mainTravelPath);
    simulator.createErrorsFile(mainTravelPath);
    std::cerr << "IN MAIN LAST ROW" << endl;
    std::cerr << "NOTICE: this core dump happens only at the end of the program" << endl;
    return (EXIT_SUCCESS);
 }

