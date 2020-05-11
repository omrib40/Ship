#include "Common.h"

/**
 * This function executes the command on the simulator ship map if all validation passed
 * @param ship
 * @param command
 * @param container
 * @param origin
 * @param dest
 */
void execute(std::unique_ptr<Ship>& ship, char command, std::unique_ptr<Container>& container, coordinate origin, coordinate dest, std::shared_ptr<Port> port) {
    switch (command) {
        case 'L':
            ship->addContainer(*container, origin);
            break;
        case 'U':
            port->addContainer(*container, Type::PRIORITY);
            ship->removeContainer(origin);
            break;
        case 'M':
            ship->moveContainer(origin, dest);
            break;
        default:
            cout << CLANG_TIDY << endl;
    }
}

/**
 * This function manages to validate the whole algorithm cranse instruction at given port
 * @param outputPath - the path of the crane instructions file
 * @param contAtPortPath - the input path of the port data of containers to be tested
 * @param simShip - the simulator ship
 * @param portNumber - the current port number(to get reference were we are at the route)
 * @param currAlgErrors - the algorithm errors list to update
 */
std::optional<pair<int,int>> validateAlgorithm(string &outputPath, string &contAtPortPath,
        int portNumber, list<string>& currAlgErrors,SimulatorObj* simulator,string& portName,int visitNumber){
    std::ifstream instructionsFile;
    string line,id,instruction;
    pair<string,string> idAndInstruction;
    map<string,list<string>> linesFromPortFile;
    int errorsCount = 0,instructionsCount = 0;
//    int kg = 0; ex. 3

    parseDataFromPortFile(linesFromPortFile, contAtPortPath);
    instructionsFile.open(outputPath);
    if(instructionsFile.fail()) {
        P_ERROR_READPATH(outputPath);
        return std::nullopt;
    }
    while(getline(instructionsFile, line)){
        vector<int> coordinates;
        extractCraneInstruction(line, idAndInstruction, coordinates);
        instruction = std::get<0>(idAndInstruction);
        id = std::get<1>(idAndInstruction);
        /*if the below statement pass test, then we can execute instruction or if it's reject then do nothing as we need to reject*/
        if(validateInstruction(instruction, id, coordinates, simulator, linesFromPortFile, portNumber)){
            if(instruction == "R"){
                linesFromPortFile[id].clear();
                continue;
            }
            coordinate one = std::tuple<int,int>(coordinates[0],coordinates[1]);
            std::unique_ptr<Container> cont = std::make_unique<Container>(id);
            if(instruction == "L") {
                execute(simulator->getShip(), instruction.at(0), cont, one, std::forward_as_tuple(-1, -1), simulator->getPort());
            }
            else if(instruction == "U"){
                execute(simulator->getShip(), instruction.at(0), cont, one, std::forward_as_tuple(-1, -1), simulator->getPort());
                cont.reset(nullptr);
            }
            else if(instruction == "M"){
                coordinate two = std::tuple<int,int>(coordinates[3],coordinates[4]);
                execute(simulator->getShip(), instruction.at(0), cont, one, two, simulator->getPort());
                cont.reset(nullptr);
            }
            instructionsCount++;
            linesFromPortFile[id].clear();
        }
        else{
            currAlgErrors.emplace_back(ERROR_CONTLINEINSTRUCTION(portName,visitNumber,instruction));
            errorsCount = -1;
            break;
        }
    }
    instructionsFile.close();
    /*Final check, if there are any containers were on containers at port file that the algorithm didnt handle properly*/
    if(errorsCount != -1 && portNumber != (int)simulator->getShip()->getRoute().size() - 1)
            errorsCount = SimulatorObj::checkContainersDidntHandle(linesFromPortFile,currAlgErrors,portName,visitNumber);

    return {std::pair<int,int>(instructionsCount,errorsCount)};
}

/**
 * This function gets a string to extract the data from and arrange it.
 * @param toParse - the string to parse from
 * @param instruction
 * @param coordinates
 */
void extractCraneInstruction(string &toParse, std::pair<string,string> &instruction, vector<int> &coordinates){
    auto parsedInfo = stringSplit(toParse,delim);

    for(int i = 0; i < (int)parsedInfo.size(); i++){
        if(i == 0)
            std::get<0>(instruction) = parsedInfo.at(0);
        else if(i == 1)
            std::get<1>(instruction) = parsedInfo.at(1);
        else
            coordinates.emplace_back(stoi(parsedInfo.at(i)));
    }
}

/**
 * This function manages to valid all types of instructions given by algorithm to port crane
 * @param instruction - current instruction
 * @param id
 * @param coordinates
 * @param ship
 * @param portContainers
 * @param portNum
 * @return true iff the validation of instruction went successfully
 */
bool validateInstruction(string &instruction,string &id, vector<int> &coordinates,SimulatorObj* sim,std::map<string,list<string>>& portContainers,int portNum){
    bool isValid;
    /*TODO problem here --> i try to get the kg of the container from the
     * port containers raw data but if unload operation then container id won;t appear here it will crash.
     * furthermore, if it's load operation of container that the algorithm just unloaded then it also won't appear here,
     * and then at the load operation i won't able to know what is the weight*/
    //auto parsedInfo = stringSplit(portContainers.at(id).front(),delim);
    int kg = 1;
    if(instruction == "L"){
        isValid =  validateLoadInstruction(coordinates, sim,kg);
    }
    else if(instruction == "U"){
        isValid =  validateUnloadInstruction(coordinates, sim);
    }
    else if(instruction == "R"){
        isValid = validateRejectInstruction(portContainers, id, sim, portNum);
    }
    else {
        isValid = validateMoveInstruction(coordinates, sim);
    }
    return isValid;
}

/**
 * This function validates reject crane instruction given by the algorithm
 * @pre assuming this function can't be reached if it's the last port in the route validation
 * @param portContainers - this list contains the current port file data(raw data)
 * @param id - the container id we want to check why rejected
 * @param ship - to get the ship map
 * @param portNum - the current port number
 * @return true iff one of the tests failes
 */
bool validateRejectInstruction(std::map<string,list<string>>& portContainers, string& id,SimulatorObj* sim,int portNum){
    string line = portContainers[id].front();
    auto parsedInfo = stringSplit(line,delim);
    auto &ship = sim->getShip();
    string portName = parsedInfo.at(2);
//    int kg = 0; ex.3
    VALIDATION reason = VALIDATION::Valid;/*might be used in exercise 2 to be more specific*/
//    if(isValidInteger(parsedInfo.at(1)))
//        kg = atoi(parsedInfo.at(1).data());

    bool test1 = !validateContainerData(line,reason,id,ship);
    bool test3 = (ship->getRoute().at(portNum)->get_name() == portName);
    bool test2 = !isPortInRoute(portName,ship->getRoute(),portNum);
    bool test4 = ship->getFreeSpace() == 0; /*if != 0 that means this test didn't passed --> reject at this test failed*/
    bool test5 = portContainers[id].size() > 1; /*case there are 2 lines with same id*/
    //bool test6 = BalanceStatus ::APPROVED != sim->getCalc().tryOperation('L',kg,-1,-1);
    //TODO test 6 not good, as it need to be load operation but it doesn't check all options!!!
    /*if one of the test fails, that means --> that reject at this instruction is necessary*/
    return (test1 || test2 || test3 || test4 || test5);
}

/**
 * This function validates load crane instruction given by the algorithm
 * @param coordinates - given coordinates that algorithm did load to
 * @param ship - to get ship map
 * @return true iff all tests were failed
 */
bool validateLoadInstruction(vector<int> &coordinates,SimulatorObj* sim,int kg){
    int x = coordinates[0],y = coordinates[1], z = coordinates[2];
    auto &ship = sim->getShip();
    auto map = ship->getMap();
    /*Check if the position of the x,y axis is out of bounds*/
    if((x < 0 || x > ship->getAxis("x")) && (y < 0 || y > ship->getAxis("y"))) return false;
    /*Check if the position of z axis is out of bounds*/
    if((int)(*map).at(x).at(y).size() != z) return false;
    /*Check if the weight balance is approved*/
    return sim->getCalc().tryOperation('L', kg, x, y) == APPROVED;
}

/**
 * This function validate's unload crane instruction. given by the algorithm
 * @param coordinates - given coordinates that algorithm did unload to
 * @param ship - to get ship map
 * @return true iff all tests were failed
 */
bool validateUnloadInstruction(vector<int> &coordinates,SimulatorObj* sim){
    int x = coordinates[0],y = coordinates[1], z = coordinates[2];
    auto &ship = sim->getShip();
    auto &map = *ship->getMap();
    /*Check if the position of the x,y axis is out of bounds*/
    if((x < 0 || x > ship->getAxis("x")) && (y < 0 || y > ship->getAxis("y"))) return false;
    /*Check if the position of z axis is out of bounds*/
    if((int)(map).at(x).at(y).size() != z+1) return false;
    /*Check if the container at x,y,z is block container*/
    if((map).at(x).at(y).at(z).getId() == "block") return false;
    /*Check if weight balance is approved*/
    else {
        int kg = map.at(x).at(y).at(z).getWeight();
        return sim->getCalc().tryOperation('U', kg, x, y) == APPROVED;
    }
}

/**
 * This function checks if algorithm move crane instruction is valid, logically it checks if the unload operation
 * and the load operation of the move are legal
 * @param coordinates - the vector of coordinates to unload and load , x1-z1 are unload coordinates.
 * @param map - ship's map
 * @return true iff it passed all tests
 */
bool validateMoveInstruction(vector<int> &coordinates, SimulatorObj* sim){
    int x1 = coordinates[0],y1 = coordinates[1],z1 = coordinates[2];
    int x2 = coordinates[3],y2 = coordinates[4],z2 = coordinates[5];
    std::tuple<bool,bool> tup(true,true);
    auto &map = *(sim->getShip()->getMap());
    int kg = 0;
    /*Check if the position of z axis is out of bounds*/
    if((int) (map.at(x1).at(y1).size()) != z1 + 1 || (int) (map.at(x2).at(y2).size()) != z2) return false;
    /*Check if weight balance approved for unload*/
    kg = map.at(x1).at(y1).at(z1).getWeight();
    return sim->getCalc().tryOperation('U', kg, x1, y1) == APPROVED && sim->getCalc().tryOperation('L', kg, x2, y2) == APPROVED;
}

/**
 * This function checks if the given line and is is valid
 * the validation checks are - existing id, bad id, bad port name, port not in route.
 * @param line - the container data line
 * @param reason - assigning the reason if the validation failed
 * @param id - the container id
 * @param ship - current ship
 * @return true iff reason == valid
 */
bool validateContainerData(const std::string& line, VALIDATION& reason, std::string& id, std::unique_ptr<Ship>& ship) {
    int i=-1;
    auto data = stringSplit(line, delim);
    if(data.size() != 3)
        return false;
    std::string port_name;

    for(const std::string& item : data){
        ++i;
        if (i == 0) {
            id = item;
            bool idBool = isValidId(item);
            if(!idBool){
                reason = VALIDATION::InvalidID;
                return false;
            }
            else {
                if(idExistOnShip(item, ship)){
                    reason = VALIDATION::ExistID;
                    return false;
                }
            }
        }
        if(i == 1) {
            bool weight = isNumber(item);
            if(!weight){
                std::cout << item << std::endl;
                reason = VALIDATION::InvalidWeight;
                return false;
            }
        }
        else if (i == 2) {
            port_name = item;
        }
    }
    bool dest = isValidPortName(port_name);
    if(!dest){
        reason = VALIDATION::InvalidPort;
        return false;
    }
    return true;
}

/**
 * This function validates if given port portName is valid port name according to seaport code
 * @param portName
 * @return true iff it's valid port name
 */
bool isValidPortName(const std::string& portName){
    std::regex reg("\\s*[A-Za-z]{5}\\s*");
    return std::regex_match(portName, reg);
}
/**
 * This function checks if a given string is a valid number aka include's only 0-9 chars
 * @param s
 * @return true iff it's a valid number
 */
bool isNumber(const std::string& s) {
    return !s.empty() && std::find_if(s.begin(),
                                      s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}


/**
 * util function that split strings by a given delimiter
 * @param s - string to be splitted
 * @param delimiter - delimiters to be splitted by
 * @return vector of words in s
 */
std::vector<std::string> stringSplit(std::string s, const char* delimiter) {
    if(iscntrl(s[s.length() - 1])){
        s = s.substr(0, s.length() - 1);
    }
    size_t start = 0;
    size_t end = s.find_first_of(delimiter);

    std::vector<std::string> output;

    while (end <= std::string::npos)
    {
        output.emplace_back(s.substr(start, end - start));

        if (end == std::string::npos)
            break;

        start = end + 1;
        end = s.find_first_of(delimiter, start);

        while(start == end){
            start = end + 1;
            end = s.find_first_of(delimiter, start);
        }
    }

    return output;
}

/**
 * This function validate's that the id is in the right format according to ISO 6346
 * @param str - the checked id
 * @return true iff it's in the right format
 */
bool isValidId(const std::string& str) {
    std::regex reg("[A-Z]{3}[UJZ][0-9]{7}");
    return std::regex_match(str, reg);
}

/**
 * This function checks if the given container id already exist in the ship map
 * @param id
 * @param ship
 * @return true iff it already exist
 */
bool idExistOnShip(const std::string& id, std::unique_ptr<Ship>& ship){
    auto map = ship->getContainersByPort();
    for(auto & pPort : map){
        for(auto con_it : pPort.second) {
            if(con_it.getId() == id){
                return true;
            }
        }
    }
    return false;
}


/**
 * This function checks if the given portName exist in the following port's in the route file
 * from the current route location
 * @param portName
 * @param route
 * @param portNum
 * @return true iff it's in the following route
 */
bool isPortInRoute(const std::string& portName, const std::vector<std::shared_ptr<Port>>& route, int portNum) {
    bool found = false;
    for(auto port_it = route.begin() + portNum + 1; port_it != route.end(); ++port_it){
        if(((*port_it)->get_name()) == portName){
            found = true;
            break;
        }
    }
    return found && portName != "NOT_IN_ROUTE";
}


/**
 * This function extracting the travel route from a file and sets it to be the given ship travel's route
 * @param ship
 * @param inFile
 */
int extractTravelRoute(std::unique_ptr<Ship>& ship, const std::string& filePath,list<string> &generalErrors) {
    std::unique_ptr<std::vector<std::shared_ptr<Port>>> vec = std::make_unique<std::vector<std::shared_ptr<Port>>>();
    string line;
    std::ifstream inFile;
    int returnStatement = 0,temporalStatement = 0;

    inFile.open(filePath);
    if (inFile.fail()) {
        P_ERROR_READPATH(filePath);
        returnStatement = Route_Fatal;
    }
    else {
        while (getline(inFile, line)) {
            temporalStatement = 0;
            if (isCommentLine(line)) continue;
            else if (isValidPortName(line)) {
                if(iscntrl(line[line.length() - 1])){
                    line = line.substr(0, line.length() - 1);
                }
                if(vec->size() > 0 && vec->at(vec->size()-1) && vec->at(vec->size()-1)->get_name() == line){
                    generalErrors.emplace_back(ERROR_PORTTWICE(line));
                    temporalStatement = Route_PortTwice;
                }
                else if (!portAlreadyExist(*vec, line)) {
                    std::shared_ptr<Port> p1 = std::make_shared<Port>(line);
                    vec->emplace_back(p1);
                }
            }
            else{
                temporalStatement = Route_badPortS;
            }
            updateErrorNum(&returnStatement,temporalStatement);
        }
        if((int)vec->size() <= 1){
            returnStatement += Route_SingleP;
        }
        ship->setRoute(*vec);
    }

    inFile.close();
    return returnStatement;
}

/**
 * overloaded function without error log for algorithm usage
 */
int extractTravelRoute(std::unique_ptr<Ship>& ship, const std::string& filePath){
    list<string> tempListForAlg;
    return extractTravelRoute(ship, filePath, tempListForAlg);
}


/*************************************************************************
************************ MERGE WITH STOWAGE ALGORITHM ********************
*************************************************************************/


void writeToOutput(std::ofstream& output, AbstractAlgorithm::Action command, const std::string& id, const std::tuple<int,int,int> pos, const std::tuple<int,int,int>& movedTo){
    switch(command) {
        case AbstractAlgorithm::Action::REJECT:
            output << static_cast<char>(command) << ", " << id << std::endl;
            break;
        case AbstractAlgorithm::Action::MOVE:
            output << static_cast<char>(command) << ", " << id << ", " << std::get<0>(pos) <<
                   ", " << std::get<1>(pos) << ", " << std::get<2>(pos) <<
                   ", " << std::get<0>(movedTo) << ", " << std::get<1>(movedTo) <<
                   ", " << std::get<2>(movedTo) << std::endl;
            break;
        case  AbstractAlgorithm::Action::LOAD:
        case  AbstractAlgorithm::Action::UNLOAD:
            output << static_cast<char>(command) << ", " << id << ", " << std::get<0>(pos) << ", " << std::get<1>(pos) << ", "
                   << std::get<2>(pos) << std::endl;
            break;
    }
}

/**
 * Parses the containers data and connecting it to the "load" list of the port
 * @param input_full_path_and_file_name
 */
bool parseDataToPort(const std::string& inputFullPathAndFileName, std::ofstream &output,
        std::unique_ptr<Ship>& ship, std::shared_ptr<Port>& port) {
    std::string line;
    std::ifstream input;

    if(inputFullPathAndFileName == "") return true;
    input.open(inputFullPathAndFileName);

    if(input.fail()){
        P_ERROR_READPATH(inputFullPathAndFileName);
        return false;
    }

    while(getline(input,line)){
        if(isCommentLine(line)) continue; //comment symbol
        std::string id; int weight;
        std::shared_ptr<Port> dest;
        VALIDATION reason = VALIDATION::Valid;
        if(validateContainerData(line, reason, id, ship)) {
            extractContainersData(line, id, weight, dest, ship);
            if(dest != nullptr && !(*dest == *port) && dest->get_name() != "NOT_IN_ROUTE") {
                std::unique_ptr<Container> con = std::make_unique<Container>(id, weight,port, dest);
                port->addContainer(*con, Type::LOAD);
            }
            else {
                writeToOutput(output,AbstractAlgorithm::Action::REJECT, id);
            }
        }
        else {
            if(reason != VALIDATION::Valid){
                writeToOutput(output, AbstractAlgorithm::Action::REJECT, id);
            }
        }
    }
    input.close();
    return true;
}

/**
 * parses the data from a given line
 * @param line - data line of container
 * @param id - container's ID
 * @param weight - container's weight
 * @param dest - container's destination
 */
void extractContainersData(const std::string& line, std::string &id, int &weight, std::shared_ptr<Port>& dest, std::unique_ptr<Ship>& ship) {
    int i=0;
    auto data = stringSplit(line, delim);
    std::string port_name;
    for(const std::string& item : data){
        switch(i){
            case 0:
                id = item;
                ++i;
                break;
            case 1:
                weight = stoi(item);
                ++i;
                break;
            case 2:
                port_name = item;
                ++i;
                break;
//            case 3:
//                port_name += " " + item; //no spaces anymore
        }
    }
    auto dest_temp = ship->getPortByName(port_name);
    if(dest_temp){
        dest = dest_temp;
    }
}
/**
 * This function checks if the port file is valid aka <port_symbol>_<num>.<filetype>
 * @param fileName
 * @return true iff it's in the right format
 */
bool isValidPortFileName(const string& fileName){
    std::regex reg("[A-Za-z]{5}_[1-9]+\\.cargo_data");
    return std::regex_match(fileName, reg);
}

bool isValidShipRouteFileName(const string& fileName){
    std::regex reg(".*\\.ship_route");
    return std::regex_match(fileName,reg);
}

bool isValidShipMapFileName(const string& fileName){
    std::regex reg(".*\\.ship_plan");
    return std::regex_match(fileName,reg);
}

bool isCommentLine(const string& line){
    std::regex commentLine("\\s*[#]+.*");
    std::regex whiteSpaces(R"(\s*\t*\r*\n*)");
    return (std::regex_match(line,commentLine) || std::regex_match(line,whiteSpaces));
}

/**
 * This function checks if the travel name folder is in the right format aka "Travel" followed by any valid numbers
 * @param travelName
 * @return true iff it's in the right format
 */
bool isValidTravelName(const string& travelName){
    std::regex reg("Travel\\s*[1-9a-zA-Z]+");
    return std::regex_match(travelName, reg);
}

/**
 * This function gets the array of 19 available errors, and the number that returned from the algorithm run
 * over the current port and transform this number into bit number, if bit at index i is 1 then there's an error
 * and array at index length() - i will get the value true as there's an error.
 * @param arr - the given array
 * @param num - the number returned from the algortihm run
 */
void initArrayOfErrors(std::array<bool,NUM_OF_ERRORS> &arr,int num){
    string binary = std::bitset<NUM_OF_ERRORS>(num).to_string();
    int index = 0, length = (int)binary.length()-1;
    for(index = 0; index <= length; index++){
        arr[index] = binary.at(length-index) - 48;
    }
}

/**
 * This function gets a curr error number which is a sum of 2^i distinct values only
 * @param currError - the current error number which we want to update
 * @param newError - the new error number which we want to compare to, and assign only values which not exist
 * in the curr error number
 */
void updateErrorNum(int* currError,int newError){
    std::array<bool,NUM_OF_ERRORS> currErrorArr {false};
    std::array<bool,NUM_OF_ERRORS> newErrorArr  {false};
    initArrayOfErrors(currErrorArr,*currError);
    initArrayOfErrors(newErrorArr,newError);
    for(int i = 0; i < NUM_OF_ERRORS; i++){
        if(!currErrorArr[i] && newErrorArr[i]){ /*if currError is false then no error yet found, and the other is true then found*/
            (*currError) += (1 << i);
        }
    }
}

bool isValidInteger(const string str){
    return std::regex_match(str, std::regex("[(-|+)|]*[0-9]+"));
}
