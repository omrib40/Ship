#include "Parser.h"

/**
 * This function gets the number from the <port_symbol>_<num>.<filetype> deceleration
 * @param fileName
 * @return the number
 */
int extractPortNumFromFile(const string& fileName){
    if(fileName.size() < 5) return 0;
    int dot = fileName.find(".");
    int dash = fileName.find("_") + 1;
    string numPort = fileName.substr(dash, dot - dash);
    return atoi(numPort.data());

}

/**
 * This function get a travel folder map:= <portName,list of files with same portName>
 * and assigns at list[portNum] the given entry which corresponds to map[portName][portNum-1] --> portName_portNum.cargo_data
 * @param travelMap - the travel_name map
 * @param portName - the current portName file
 * @param portNum - the current portName number
 * @param entry - the entry path
 */
string extractPortNameFromFile(string fileName){
    int index = fileName.find_first_of("_");
    string portName = fileName.substr(0,index);
    return portName;
}

/**
 * This function checks if port already exist in the vector list, if exists it return 0 but
 * sets a pointer to the same port at the the end of the vector.
 * @param vec - the vector of ports
 * @param str - the string of the port to be check
 * @return 0 iff exist already port with same name
 */
int portAlreadyExist(std::vector<std::shared_ptr<Port>>& vec,string &str){
    for(const auto &element : vec ){
        if(element->get_name() == str){
            vec.emplace_back(element);
            return 1;
        }
    }
    return 0;
}

/**
 * This function parsing the dimensions of a ship/container location from file/string
 * @param arr - the array to store the dimensions in
 * @param inFile - the file we are working on to parse the data from
 * @param str - byFile means we parse from file, otherwise parse from the str itself
 */
void getDimensions(std::array<int,3> &arr, std::istream &inFile,string str){
    vector<string> vec;

    /*if we read from file then we want to get first line which is not comment*/
    if(str == "byFile"){
        str = "";
        while(getline(inFile,str)) {
            if(!str.empty() && str.at(0) != '#') break; //comment symbol
        }
    }

    vec = stringSplit(str,delim);
    if((int)vec.size() != 3) {
        arr[0] = -1; /*indicates caller function that bad line format*/
        return;
    }
    for(int i = 0; i < 3; i++){
        if(isValidInteger(vec[i]))
            arr[i] = atoi(vec[i].data());
        else
            arr[i] = -1;
        }
}


/**
 * This function gets a string(a line from the file) parse the line to get 3 ints (x,y,z) such ship.getMap()[x][y][z]
 * will be initialized to be block container.
 * @param str - the string to parse
 * @param ship - the ship to get it's map from.
 * @param lineNumber - current line number reading from file optional --> algorithm might
 * @return returns empty string iff no error happened
 */
pair<string,int> setBlocksByLine(string &str,std::unique_ptr<Ship>& ship,int lineNumber) {
    auto map = ship->getMap();
    std::ifstream inFile;
    std::array<int,3> dim{};
    pair<string,int> pair;
    getDimensions(dim,inFile,str);

    if(dim[0] > ship->getAxis("x") || dim[1] > ship->getAxis("y") || dim[2] > ship->getAxis("z")){
        if(dim[0] > ship->getAxis("x") || dim[1] > ship->getAxis("y")){
            std::get<1>(pair) = Plan_XYError;
        }
        if(dim[2] > ship->getAxis("z"))
            std::get<1>(pair) = Plan_ZError;

        std::get<0>(pair) = "Error: at line number " + std::to_string(lineNumber) + " One of the provided ship plan constraints exceeding the dimensions of the ship";
        return pair;
    }

    else if(dim[0] < 0 || dim[1] < 0 || dim[2] < 0){
        std::get<0>(pair) = "Error: at line number " + std::to_string(lineNumber) + "bad line format";
        std::get<1>(pair) = Plan_BadLine;
    }
    else if(!(*map)[dim[0]][dim[1]].empty()){
        if((*map)[dim[0]][dim[1]].size() == ship->getAxis("z")-dim[2]) {
            std::get<0>(pair) = "Error: at line number " + std::to_string(lineNumber) + " constraint at (" +
                                std::to_string(dim[0]) + "," + std::to_string(dim[1]) +
                                ") already given with diff value";
            std::get<1>(pair) = Plan_Con;
        }
            else {
            std::get<0>(pair) = "Error: at line number " + std::to_string(lineNumber) + " constraint at (" +
                                std::to_string(dim[0]) + "," + std::to_string(dim[1]) +
                                ") already given with same value";
            std::get<1>(pair) = Plan_BadLine;
        }
    }
    else{
        for(int i = 0; i < ship->getAxis("z")-dim[2]; i++){
            (*map)[dim[0]][dim[1]].emplace_back(Block());
            ship->updateFreeSpace(-1);
        }
    }
    return pair;
}

/**
 * This function parse line by line from the file, and initialized the block containers in the shipmap
 * @param ship - the ship object to get it's shipMap and update it
 * @param inFile - the file pointer
 * @return 0 if succeeded, specified return code otherwise.
 */
int extractArgsForBlocks(std::unique_ptr<Ship>& ship,const std::string& filePath, list<string> &generalErrors){
    string line;
    int lineNumber = 2, returnStatement = 0,num;
    std::ifstream inFile;
    pair<string,int> pair;

    inFile.open(filePath);
    if (inFile.fail()) {
        std::cerr << FAIL_TO_READ_PATH + filePath << endl;
        returnStatement = Plan_Fatal;
    }
    else {
        getline(inFile,line); /*first line is ship dimensions we already got them*/
        while (getline(inFile, line)){
            if(!line.empty() && line.at(0) != '#') {/*if not commented line*/
                pair = setBlocksByLine(line, ship, lineNumber);
                num = std::get<1>(pair);
                if (num != 0) {
                    updateErrorNum(&returnStatement, num);
                    generalErrors.emplace_back(std::get<0>(pair));
                }
            }
            lineNumber++;
        }
    }
    inFile.close();
    return returnStatement;
}

/**
 * overloaded function without the errors for the algorithm
 */
int extractArgsForBlocks(std::unique_ptr<Ship>& ship,const std::string& filePath) {
    list<string> tempListForAlg;
    return extractArgsForBlocks(ship, filePath, tempListForAlg);
}

int extractShipPlan(const std::string& filePath, std::unique_ptr<Ship>& ship){
    std::array<int, 3> dimensions{};
    std::ifstream inFile;
    int returnStatement = 0;

    inFile.open(filePath);
    if (inFile.fail()) {
        std::cerr << FAIL_TO_READ_PATH + filePath << endl;
        returnStatement = Plan_Fatal;
    }
    else {
        getDimensions(dimensions, inFile,"byFile");
        if(dimensions[0] < 0 || dimensions[1] < 0 || dimensions[2] < 0) {
            returnStatement = Plan_Fatal;
        } else {
            ship = std::make_unique<Ship>(dimensions[1]+1, dimensions[2]+1, dimensions[0]+1);
        }
    }

    inFile.close();
    return returnStatement;
}

/**
 * This function parse the ship map and the ship route files and init a new ship object with
 * the information it parsed.
 * @param folder - the folder that contains ship_route, ship_plan files
 * @return the constructed ship iff folder is not empty.
 */
std::unique_ptr<Ship> extractArgsForShip(string &travelName,SimulatorObj &simulator) {
    string file_path;
    vector<std::shared_ptr<Port>> travelRoute;
    std::unique_ptr<Ship> ship;
    list<string> generalErrors;
    auto &travelFolder = simulator.getInputFiles()[travelName];

    if(travelFolder.find(ROUTE) == travelFolder.end() || travelFolder.find(PLAN) == travelFolder.end()){
        if(travelFolder.find(ROUTE) == travelFolder.end())
            simulator.addNewErrorToGeneralErrors("Error: Lack of route file, ignoring this travel");
        else
            simulator.addNewErrorToGeneralErrors("Error: Lack of plan file, ignoring this travel");
        return nullptr;
    }
    file_path = travelFolder[PLAN].at(1).string();
    int resultInt = extractShipPlan(file_path,ship);
    if(resultInt == 0){
        resultInt = extractArgsForBlocks(ship,file_path,generalErrors);
        simulator.addListOfGeneralErrors(generalErrors);
    }
    else {
        simulator.addNewErrorToGeneralErrors("Error: Fatal error occurred in plan file, ignoring this travel");
        simulator.updateArrayOfCodes(resultInt, "sim");
        return nullptr;
    }
    generalErrors.clear();
    file_path = travelFolder[ROUTE].at(1).string();
    resultInt = extractTravelRoute(ship,file_path,generalErrors);
    if(resultInt == Route_Fatal || ship->getRoute().size() <= 1){
        if(resultInt == Route_Fatal)
            simulator.addNewErrorToGeneralErrors("Error: Fatal error occurred in route file, ignoring this travel");
        else
            simulator.addNewErrorToGeneralErrors("Error: Route file contains less then 2 valid ports, ignoring this travel");
        simulator.addListOfGeneralErrors(generalErrors);
        return nullptr;
    }

    simulator.updateArrayOfCodes(resultInt, "sim");
    return ship;
}

//TODO walkthrough todo's and update in above function
//Ship* extractArgsForShip(map<string,vector<fs::path>> &travelFolder,list<string> &generalErrors){
//    std::ifstream inFile;
//    string line, file_path;
//    std::array<int, 3> dimensions{};
//    vector<Port *> travelRoute;
//    Ship* ship = nullptr;
//    if(travelFolder.find(ROUTE) == travelFolder.end()){
//        generalErrors.emplace_back("Error: Lack of route file, ignoring this travel folder");
//        return ship;
//    }
//    else if(travelFolder.find(PLAN) == travelFolder.end()){
//        generalErrors.emplace_back("Error: Lack of plan file, ignoring this travel folder");
//        return ship;
//    }
//    file_path = travelFolder[PLAN].at(0).string();
//    inFile.open(file_path);
//    if(inFile.fail()){
//        generalErrors.emplace_back("Error: opening plan file failed, ignoring this travel folder");
//        return ship;
//    }
//    else{
//        //TODO handle creating a ship from the plan file.
//    }
//    inFile.close();
//    file_path = travelFolder[ROUTE].at(0).string();
//    inFile.open(file_path);
//    if(inFile.fail()){
//        generalErrors.emplace_back("Error: opening route file failed, ignoring this travel folder");
//        delete ship;
//        return nullptr;
//    }
//    else{
//        //TODO handle creating route vector from route file.
//    }
//    inFile.close();
//    ship->initCalc();
//    return ship;
//
//}

/**
 * This function parses the data from a port file, it saves it by container id and the data line of this id in a map
 * @param map - the given map to save instruction by port id
 * @param inputPath - the input path of the port file
 */
void parseDataFromPortFile(std::map<string,string>& map, string& inputPath){
    std::ifstream inFile;
    string line;
    inFile.open(inputPath);
    if(inFile.fail()){
        std::cerr << FAIL_TO_READ_PATH + inputPath << endl;
        return;
    }
    while(getline(inFile,line)){
        if(!line.empty() && line.at(0) == '#')continue;
        vector<string> parsedInfo = stringSplit(line,delim);
        if(parsedInfo.size() != 4)continue; /*case not enough information or too much*/
        string contID = parsedInfo.at(0);
        map.insert(make_pair(contID,line));
    }
    inFile.close();
}

//This function saved to usage in exercise 2
/*string* getPortNameFromFile(string filePath){
    string* portName = new string();
    int i = 0,j;
    for(i = filePath.size()-1; i > 0; i--){
        if(filePath.at(i) == '_'){
            j = i;
        }
        if(filePath.at(i) == '\\')
            break;
    }
    portName->append(filePath.substr(filePath.size() - i+1,j-i));
    return portName;
}*/
