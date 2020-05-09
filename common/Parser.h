/**
* This header represents a module that involves in reading and parsing(extracting) files or information given in
* any constellation.
*
*      *******      Functions      *******
* extractPortNumFromFile                - gets the number from the port file, <port_symbol>_<num>.<filetype>.
* isValidPortFileName     - checks if the port expression file is valid
* isValidTravelName             - check if the name of the travel folder is valid name
* portAlreadyExists             - checks in a given list of ports if the given port already exists, and pushes it back if so
* orderListDirectories          - arranging the list of the travel directories with the following format map,route,port1,port2 etc..
* initListOfTravels           - initialize the list of the directories from a given root path and store it in a data structure
* validateSequenceDirectories   - validating the each travel folder has a all valid files exists
* setActualSize                 - gets a vector and "shrink" it to it's actual size
* extractArgsForShip            - given ship_map and ship_route files it's builds a ship with a map and route from them
* extractArgsForBlocks          - used by above function and it sets the block containers in the ship map from file information
* setBlocksByLine               - used as a utility function of extractArgsForBlocks
* getDimensions                 - reads a line of 3 number <x y z> and parse the numbers out of it
 */
#ifndef PARSER_HEADER
#define PARSER_HEADER

class Common;

#include <string>
#include <vector>
#include <list>
#include <regex>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "Common.h"
#include "../interfaces/ErrorsInterface.h"

using std::cout;
using std::endl;
using std::string;
using std::list;
using std::vector;
using std::pair;
using std::map;
namespace fs = std::filesystem;

#define CONTAINER_NOT_IN_ROUTE "This container's destination is not in the ship's route"
#define FAIL_TO_READ_PATH "Failed to read from this file path "
#define ROUTE "route"
#define PLAN "plan"


int extractPortNumFromFile(const string& fileName);
int extractArgsForBlocks(std::unique_ptr<Ship>& ship, const std::string& file_path,list<string> &generalErrors);
int extractArgsForBlocks(std::unique_ptr<Ship>& ship,const std::string& filePath);
std::unique_ptr<Ship> extractArgsForShip(string &travelName,SimulatorObj &simulator);
pair<string,int> setBlocksByLine(std::string &str, std::unique_ptr<Ship> &ship,int lineNumber);
void getDimensions(std::array<int,3> &arr, std::istream &inFile,string str);
int portAlreadyExist(std::vector<std::shared_ptr<Port>>& vec,string &str);
void parseDataFromPortFile(std::map<string,list<string>>& map, string &inputPath);
int extractShipPlan(const std::string& filePath, std::unique_ptr<Ship>& ship);
string extractPortNameFromFile(const string& fileName);
//string* getPortNameFromFile(string &filePath);

#endif