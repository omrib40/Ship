#include "SimulatorValidation.h"

/**
 * This function manages to validate the whole algorithm crane instruction at given port
 */
std::optional<pair<int,int>> SimulatorValidation::validateAlgorithm(string &outputPath, string &contAtPortPath,
                                               list<string>& currAlgErrors,string& portName,int visitNumber){
    std::ifstream instructionsFile;
    string line,id,instruction;
    int errorsCount = 0,instructionsCount = 0;

    extractRawDataFromPortFile(rawDataFromPortFile, contAtPortPath,sim);
    initLoadedListAndRejected();
    initPriorityRejected();

    instructionsFile.open(outputPath);
    if(instructionsFile.fail()) {
        ERROR_READ_PATH(outputPath);
        return std::nullopt;
    }
    while(getline(instructionsFile, line)){
        vector<int> coordinates;
        extractCraneInstruction(line, instruction,id, coordinates);
        /*if the below statement pass test, then we can execute instruction or if it's reject then do nothing as we need to reject*/
        if(validateInstruction(instruction, id, coordinates)){
            if(instruction == "R"){
                eraseFromRawData(line,id);
                continue;
            }
            coordinate one = std::tuple<int,int>(coordinates[1],coordinates[2]);
            std::unique_ptr<Container> cont = createContainer(sim,rawDataFromPortFile,id,instruction,portName);
            if(instruction == "L") {
                execute(instruction.at(0), cont, one, std::forward_as_tuple(-1, -1));
                instructionsCount+=5;
            }
            else if(instruction == "U"){
                execute(instruction.at(0), cont, one, std::forward_as_tuple(-1, -1));
                instructionsCount+=5;
            }
            else if(instruction == "M"){
                coordinate two = std::tuple<int,int>(coordinates[4],coordinates[5]);
                execute(instruction.at(0), cont, one, two);
                instructionsCount+=3;

            }
            eraseFromRawData(line,id);
        }
        else{
            currAlgErrors.emplace_back(ERROR_CONT_LINE_INSTRUCTION(portName, id, visitNumber, instruction));
            errorsCount = -1;
            break;
        }
    }
    instructionsFile.close();
    /*Final checks*/
    if(errorsCount != -1)
        errorsCount  = finalChecks(currAlgErrors,portName,visitNumber);

    return {std::pair<int,int>(instructionsCount,errorsCount)};
}

/**
 * This function manages to valid all types of instructions given by algorithm to port crane
 */
bool SimulatorValidation::validateInstruction(string &instruction,string &id, vector<int> &coordinates){
    bool isValid;
    int kg = extractKgToValidate(rawDataFromPortFile,sim,id);

    if(instruction == "L")
        isValid =  validateLoadInstruction(coordinates,kg,id);

    else if(instruction == "U")
        isValid =  validateUnloadInstruction(coordinates);

    else if(instruction == "R")
        isValid = validateRejectInstruction(id,kg);

    else if(instruction == "M"){
        isValid = validateMoveInstruction(coordinates);
    }
    else
        return false;
    return isValid;
}

/**
 * This function validates reject crane instruction given by the algorithm
 */
bool SimulatorValidation::validateRejectInstruction(string& id,int kg){
    string line;
    auto &ship = sim->getShip();
    string portName = extractPortNameToValidate(rawDataFromPortFile,sim,id);
    std::tuple<int,int,int> tup = sim->getShip()->getCoordinate(id);
    if(rawDataFromPortFile.find(id) != rawDataFromPortFile.end())
        line = rawDataFromPortFile[id].front();
    /*Case the data is not validate / duplicate Id / idExist on ship*/
    if(!line.empty() && mustRejected[id].find(line) != mustRejected[id].end()){
        mustRejected[id].erase(line);
        return true;
    }
    /*Case the container with this didn't prioritize by algorithm*/
    if(!line.empty() && priorityRejected.find(id) !=  priorityRejected.end())
        return true;
    /*Case the algorithm decided to not load this container*/
    if(!line.empty() && possiblePriorityReject.find(id) != possiblePriorityReject.end())
        return true;
    /*Case there is no free space in the ship map*/
    if(ship->getFreeSpace() == 0)
        return true;
    /*Case there is an weight balance problem*/
    if(!checkIfBalanceWeightIssue(sim,kg,tup))
        return true;

    return false;
}

/**
 * This function validates load crane instruction given by the algorithm
 */
bool SimulatorValidation::validateLoadInstruction(vector<int> &coordinates,int kg,string& id){
    int z = coordinates[0],x = coordinates[1], y = coordinates[2];
    auto &ship = sim->getShip();
    auto &map = ship->getMap();
    /*Check if the position of the x,y axis is out of bounds*/
    if((x < 0 || x >= ship->getAxis("x")) || (y < 0 || y >= ship->getAxis("y")))
        return false;
    /*Check if the position of z axis is out of bounds or loading on the air*/
    if((int)(map).at(x).at(y).size() != z)
        return false;
    /*Check if algorithm loaded un prioritized container*/
    if(priorityRejected.find(id) != priorityRejected.end())
        return false;
    /*Check if an error found while parsing the id*/
    if(mustRejected.find(id) != mustRejected.end() && !isIdAwaitAtPort(id))
        return false;
    /*Check if the weight balance is approved*/
    return sim->getCalc().tryOperation('L', kg, x, y) == APPROVED;
}

/**
 * This function validate's unload crane instruction. given by the algorithm
 */
bool SimulatorValidation::validateUnloadInstruction(vector<int> &coordinates){
    int z = coordinates[0],x = coordinates[1], y = coordinates[2];
    auto &ship = sim->getShip();
    auto &map = ship->getMap();
    /*Check if the position of the x,y axis is out of bounds*/
    if((x < 0 || x >= ship->getAxis("x")) || (y < 0 || y >= ship->getAxis("y")))
        return false;
    /*Check if the position of z axis is out of bounds*/
    if((int)(map).at(x).at(y).size() != z+1)
        return false;
    /*Check if the container at x,y,z is block container*/
    if((map).at(x).at(y).at(z).getId() == "block")
        return false;
        /*Check if weight balance is approved*/
    else {
        int kg = map.at(x).at(y).at(z).getWeight();
        return sim->getCalc().tryOperation('U', kg, x, y) == APPROVED;
    }
}


/**
 * This function checks if algorithm move crane instruction is valid, logically it checks if the unload operation
 * and the load operation of the move are legal
 */
bool SimulatorValidation::validateMoveInstruction(vector<int> &coordinates){
    int z1 = coordinates[0],x1 = coordinates[1],y1 = coordinates[2];
    int z2 = coordinates[3],x2 = coordinates[4],y2 = coordinates[5];
    int realX = sim->getShip()->getAxis("x"), realY = sim->getShip()->getAxis("y");
    auto &map = (sim->getShip()->getMap());
    int kg = 0;
    /*Case one of the (x,y,z) dimensions exceeding the dimensions of the ship*/
    if((x1 < 0 || x1 >= realX) || (y1 < 0 || y1 >= realY))
        return false;
    if((x2 < 0 || x2 >= realX) || (y2 < 0 || y2 >= realY))
        return false;
    /*Check if the position of z axis is out of bounds*/
    if((int)(map.at(x1).at(y1).size()) != z1 + 1 || (int)(map.at(x2).at(y2).size()) != z2)
        return false;
    /*Check if weight balance approved for unload && for load*/
    kg = map.at(x1).at(y1).at(z1).getWeight();
    return sim->getCalc().tryOperation('U', kg, x1, y1) == APPROVED && sim->getCalc().tryOperation('L', kg, x2, y2) == APPROVED;
}

/**
 * This function checks a reject instruction happened because of weight issue
 * if coordinates(x,y,z) are != -1 then --> container is on ship so we want to unload this
 * else container is on port and we want to load it
 */
bool SimulatorValidation::checkIfBalanceWeightIssue(SimulatorObj* sim, int kg,std::tuple<int,int,int> &coordinates){
    if(std::get<0>(coordinates) >= 0){
        return sim->getCalc().tryOperation('U',kg,std::get<0>(coordinates),std::get<1>(coordinates)) != BalanceStatus::APPROVED;
    }
    else{
        for(int i = 0; i < sim->getShip()->getAxis("x"); i++){
            for(int j = 0; j < sim->getShip()->getAxis("y"); j++){
                if((int)sim->getShip()->getMap()[i][j].size() < sim->getShip()->getAxis("z"))
                    if(sim->getCalc().tryOperation('L',kg,i,j) == BalanceStatus::APPROVED)
                        /*Found a position that the container can be loaded at*/
                        return false;
            }
        }
    }
    return true;
}

/**
 * This function extracts the kg of a given id from possible existence of this id
 */
int SimulatorValidation::extractKgToValidate(map<string,list<string>>& rawData,SimulatorObj* sim,string& id){
    vector<string> parsedInfo;
    int kg = -1;
    bool found = false;

    /*First check if container exist in priority list*/
    for(auto &cont : *(sim->getPort()->getContainerVec(Type::PRIORITY))){
        if(cont.getId() == id){
            kg = cont.getWeight();
            found = true;
        }
    }
    /*If not in priority list check in load list*/
    if(!found){
        for(auto &cont : *(sim->getPort()->getContainerVec(Type::LOAD))){
            if(cont.getId() == id){
                kg = cont.getWeight();
                found = true;
            }
        }
    }
    /*Check if container id exist in the container at port file*/
    if(!found && rawData.find(id) != rawData.end()){
        parsedInfo = stringSplit(rawData[id].front(),delim);
        if(isValidInteger(parsedInfo.at(1))){
            kg = atoi(parsedInfo.at(1).data());
            found = true;
        }
    }
    /*Check if container is on ship map --> case we extract kg to unload operation*/
    if(!found){
        for(auto &vX : (sim->getShip()->getMap()))
            for(auto &vY : vX)
                for(auto &cont : vY)
                    if(cont.getId() == id){
                        kg = cont.getWeight();
                    }
    }
    return kg;
}

/**
 * This function extracts the port name of the the container id
 */
string SimulatorValidation::extractPortNameToValidate(map<string,list<string>>& rawData,SimulatorObj* sim,string& id){
    vector<string> parsedInfo;
    string portName;
    if(rawData.find(id) != rawData.end()){
        parsedInfo = stringSplit(rawData.find(id)->second.front(),delim);
        portName = parsedInfo.at(2);
    } else{
        for(auto &cont : *(sim->getPort()->getContainerVec(Type::PRIORITY))){
            if(cont.getId() == id){
                portName = cont.getDest()->get_name();
            }
        }
    }
    return portName;
}

/**
 * This function checks if there were containers at port file that were not handled at all
 */
int SimulatorValidation::checkContainersDidntHandle(map<string, list<string>> &idAndRawLine,list<string> &currAlgErrors,string &portName,int visitNum) {
    int err = 0;
    for(auto& idInstruction : idAndRawLine){
        if(!idInstruction.second.empty() && !softCheckId(idInstruction.first)){
            for(auto &line : idInstruction.second){
                currAlgErrors.emplace_back(ERROR_LINE_NOT_HANDLE(line, portName, visitNum));
            }
            err= -1;
        }
    }
    return err;
}

/**
 * This function checks if there were containers that unloaded at this port
 * that the destination of the container != current port
 * also checks if there is a free space at the ship and containers that didn't loaded
 */
int SimulatorValidation::checkIfContainersLeftOnPort(SimulatorObj* sim , list<string> &currAlgErrors){
    auto currPort = sim->getPort();
    string currPortName = currPort->get_name();
    int err = 0;
    for(auto &cont : *currPort->getContainerVec(Type::PRIORITY)){
        err = -1;
        string id = cont.getId();
        string dstPortName = cont.getDest()->get_name();
        currAlgErrors.emplace_back(ERROR_CONT_NOT_INDEST(id, currPortName, dstPortName));
    }
    /*Case there is still space on ship*/
    if(sim->getShip()->getFreeSpace() > 0 && err != -1){
        for(auto& cont : *currPort->getContainerVec(Type::LOAD)){
            err = -1;
            string id = cont.getId();
            string dstPortName = cont.getDest()->get_name();
            currAlgErrors.emplace_back(ERROR_CONT_LEFT_ONPORT(id, currPortName, dstPortName));
        }
    }
    return err;
}

/**
 * This function iterates over the current ship map and checks if
 * at the end of this port stop we have containers left on ship map such that for container x
 * that left on ship it's destination is this port stop but it didnt unloaded.
 */
int  SimulatorValidation::checkForContainersNotUnloaded(SimulatorObj* sim, list<string> &currAlgErrors){
    auto currPort = sim->getPort();
    int err = 0;
    for(auto& cont : sim->getShip()->getContainersByPort()[currPort]){
        currAlgErrors.emplace_back(ERROR_CONT_LEFT_ONSHIP(cont.getId()));
        err = -1;
    }
    return err;
}

int SimulatorValidation::checkIfContainerLeftOnShipFinalPort(SimulatorObj* sim,list<string> &currAlgErrors){
    int err = 0;
    auto& shipMap = sim->getShip()->getMap();
    if(sim->getPortNum() != (int)sim->getShip()->getRoute().size() - 1)
        return 0;
    else{
        for(auto& vX : shipMap)
            for(auto& vY : vX)
                for(auto& cont : vY) {
                    if(cont.getId() != "block"){
                        currAlgErrors.emplace_back(ERROR_CONT_LEFT_LAST_PORT(cont.getId()));
                        err = -1;
                    }
                }
    }
    return err;
}

/**
 * This function initialize the loaded list and rejected list of this simulator validation object
 * based on the raw data from cargo_data port file
 */
void SimulatorValidation::initLoadedListAndRejected() {
    auto &shipMap = sim->getShip();
    auto currPort = sim->getPort();
    for (auto &outterPair : rawDataFromPortFile) {
        bool alreadyFound = false;
        for (auto &info : outterPair.second) {
            VALIDATION reason = VALIDATION::Valid;
            string id;
            int weight = -1;
            std::shared_ptr<Port> dest;
            std::pair<string, VALIDATION> p;
            vector<string> parsedInfo = stringSplit(info, delim);
            validateContainerDataForReject(info, reason, sim);
            if (reason != VALIDATION::Valid) {
                p.first = info;
                p.second = reason;
                mustRejected[parsedInfo[0]].insert({info,reason});
                alreadyFound = true;
            } else if(!alreadyFound) {
                extractContainersData(info, id, weight, dest, shipMap);
                Container con(id, weight, currPort, dest);
                currPort->addContainer(con, Type::LOAD);
            }
            else{
                reason = VALIDATION::DuplicatedIdOnPort;
                mustRejected[parsedInfo[0]].insert({info,reason});
            }
        }
    }

}

/**
 * This function is a shallow validation for a given raw line from cargo_data port file and checks if it's valid
 * or not, if not it returns the reason for invalidation.
 */
void SimulatorValidation::validateContainerDataForReject(string& line,VALIDATION &reason,SimulatorObj* sim){
    auto parsedInfo = stringSplit(line,delim);
    auto& simShip = sim->getShip();
    auto srcPort = sim->getPort();
    int portNum = sim->getPortNum();
    if(parsedInfo.size() != 3){
        reason = VALIDATION::InvalidNumParameters;
        return;
    }
    else {
        if(!isValidId(parsedInfo[0])){
            reason = VALIDATION ::InvalidID;
            return;
        }
        else if(!isValidInteger(parsedInfo[1])){
            reason = VALIDATION ::InvalidWeight;
            return;
        }
        else if(!isValidPortName(parsedInfo[2])){
            reason = VALIDATION ::InvalidPort;
            return;
        }
        else if(!isPortInRoute(parsedInfo[2],sim->getShip()->getRoute(),portNum)){
            reason = VALIDATION :: InvalidPort;
            return;
        }
        else if(idExistOnShip(parsedInfo[0],simShip)){
            reason = VALIDATION :: ExistID;
            return;
        }
    }
    reason = VALIDATION :: Valid;
}

/**
 * This function initialize the priority reject map and the possible priority reject map
 * it's initialization is pre to any operation made on the given port
 * init loadCapacity at this current port --> freeSpace on ship + amount containers to be unloaded
 */
void SimulatorValidation::initPriorityRejected(){
    auto& simShip = sim->getShip();
    auto currPort = sim->getPort();
    auto currPortLoadVec = currPort->getContainerVec(Type::LOAD);
    int amountToUnload = (int)simShip->getContainersByPort()[currPort].size();
    int amountToLoad = (int)currPortLoadVec->size();
    loadCapacity = simShip->getFreeSpace() + amountToUnload;
    if(loadCapacity < amountToLoad) {
        sim->sortContainersByPriority(currPortLoadVec);
        for (int i = loadCapacity; i < amountToLoad; i++) {
            string id = currPortLoadVec->at(i).getId();
            priorityRejected.insert({id, currPortLoadVec->at(i)});
        }
        /*This block handles the case that we have k containers that part of them were assigned rejected
         * and part of them were assigned as not rejected --> algorithm have the ability to choose randomly
         * which to load and which to decline*/
        if (loadCapacity > 0) {
            string suffixPort = (*currPortLoadVec->at(loadCapacity).getDest()).get_name();
            string prevPort = (*currPortLoadVec->at(loadCapacity - 1).getDest()).get_name();
            if (prevPort == suffixPort) {
                int i = 0;
                while ((*currPortLoadVec->at(i).getDest()).get_name() != suffixPort) {
                    i++;
                }
                while ((*currPortLoadVec->at(i).getDest()).get_name() == suffixPort) {
                    string id = currPortLoadVec->at(i).getId();
                    possiblePriorityReject.insert({id, currPortLoadVec->at(i)});
                    if (priorityRejected.find(id) != priorityRejected.end()) {
                        priorityRejected.erase(id);
                    }
                    i++;
                }
            }
        }
    }
}

/**
 * This function erasing the given id and line from the raw data map
 */
void SimulatorValidation::eraseFromRawData(string &line, string &id) {
    if(line.empty()){}
    if(rawDataFromPortFile[id].size() > 1)
        rawDataFromPortFile[id].remove(rawDataFromPortFile[id].back());
    else
        rawDataFromPortFile.erase(id);
}

/**
 * This function manages to make the final validation after all instructions handled by the simulator
 * checks if prioritized containers handled by it's prioritization
 * checks if there were containers that left on port with not valid reason
 * checks if there were containers that left on ship that was supposed to unloaded
 * checks if there were lines at cargo_data port file that were not hanlded at all
 */
int SimulatorValidation::finalChecks(list<string> &currAlgErrors, string &portName, int visitNumber) {
    int errorsCount = 0;
    auto currPortPriority = sim->getPort()->getContainerVec(Type::PRIORITY);
    sim->sortContainersByPriority(currPortPriority);
    if(checkPrioritizedHandledProperly(currAlgErrors) == -1)
        return -1;
    else if(checkIfContainersLeftOnPort(sim,currAlgErrors) == -1)
        return -1;
    else if(checkForContainersNotUnloaded(sim, currAlgErrors) == -1)
        return -1;
    else if(checkIfContainerLeftOnShipFinalPort(sim,currAlgErrors) == -1)
        return -1;
    /*Final check, if there are any containers were on containers at port file that the algorithm didnt handle properly*/
    else if(sim->getPortNum() != (int)sim->getShip()->getRoute().size() - 1)
            errorsCount = checkContainersDidntHandle(rawDataFromPortFile,currAlgErrors,portName,visitNumber);
    return errorsCount;
}

/**
 * This function executes the command on the simulator ship map if all validation passed
 */
void SimulatorValidation::execute(char command, std::unique_ptr<Container>& container, coordinate origin, coordinate dest) {
    string id;
    auto& ship = sim->getShip();
    auto port = sim->getPort();
    if(command == 'L'){
        id = container->getId();
        ship->addContainer(*container, origin);
        port->removeContainer(id,Type::LOAD);
        port->removeContainer(id,Type::PRIORITY);
    }
    else if(command == 'U'){
        if((container->getDest()->get_name() == port->get_name()))
            port->addContainer(*container, Type::ARRIVED);
        else
            port->addContainer(*container, Type::PRIORITY);
        ship->removeContainer(origin);
    }
    else
        ship->moveContainer(origin, dest);
}

/**
 * This function checks that enough prioritized containers loaded from this port
 */
int SimulatorValidation::checkPrioritizedHandledProperly(list<string> &currAlgErrors) {
    auto& simShip = sim->getShip();
    int err = 0;
    std::shared_ptr<Port> port;
    if(!possiblePriorityReject.empty() && loadCapacity > 0){
        for(auto& pair : possiblePriorityReject){
            port = pair.second.getDest();
            break;
        }
        auto &contOnShipByPort = simShip->getContainersByPort()[port];
        for(auto& pair : possiblePriorityReject){
            for(int i = 0; i < (int)contOnShipByPort.size(); i++){
                if(contOnShipByPort.at(i).getId() == pair.second.getId() && loadCapacity > 0){
                    loadCapacity--;
                }
            }
        }
        if(loadCapacity > 0){
            currAlgErrors.emplace_back(ERROR_LEFT_PRIORITY_ONPORT);
            err = -1;
        }
    }
    return err;
}

/**
 * This function checks if the length of the id is not 11 --> then algorithm able to ignore it
 * error is not neccessary
 */
bool SimulatorValidation::softCheckId(string id){
    if(id.length() != 11)
        return true;
    return false;
}

/**
 * This function checks if a given id await at port priorirty vector or load vector
 */
bool SimulatorValidation::isIdAwaitAtPort(string& id){
    for(auto& cont : *sim->getPort()->getContainerVec(Type::PRIORITY))
        if(id == cont.getId())
            return true;
    for(auto& cont : *sim->getPort()->getContainerVec(Type::LOAD))
        if(id == cont.getId())
            return true;
    return false;
}




