#include "Parser.h"


/**
 * This function gets the number from the <port_symbol>_<num>.<filetype> decleration
 * @param file_name
 * @return the number
 */
int getPortNumFile(const string& file_name){
    if(file_name.size() < 6) return 0;
    int dot = file_name.find(".");
    int dash = file_name.find("_") + 1;
    string numPort = file_name.substr(dash,dot-dash);
    return atoi(numPort.data());

}

bool isValidPortExpressionFile(const string& file_name){
    std::regex reg("\\s*[A-Z]{2}\\s+[A-Z]{3}_[1-9]+.[a-z]*");
    return std::regex_match(file_name,reg);
}

bool isValidPortName(const string& name){
    std::regex reg("\\s*[A-Z]{2}\\s+[A-Z]{3}s*$");
    return std::regex_match(name,reg);
}

bool isValidTravelName(const string& name){
    std::regex reg("Travel[1-9]*");
    return std::regex_match(name,reg);
}

std::vector<std::vector<fs::path>> orderListOfDir(std::list<std::list<fs::path>> &unOrdered){
    std::vector<std::vector<fs::path>> result(unOrdered.size());
    int ind = 0;
    int numFile = 0;
    string file_name;
    for(auto &list : unOrdered){
        result[ind].resize(list.size());
        for(auto &path : list){
            file_name = path.filename().string();
            if(file_name == "ship_plan.txt"){
                result[ind][0] = path;
            }
            else if(file_name == "ship_route.txt"){
                result[ind][1] = path;
            }
            else if(isValidPortExpressionFile(file_name)){
                numFile = getPortNumFile(file_name)+1; //as 0,1 places are for plan and route.
                if(!result[ind][numFile].empty()){
                    std::cout << "Error: there's already another port file with this number" << std::endl;
                }
                else{
                    result[ind][numFile] = path;
                }
            }
            else{
                std::cout << "Error:" + file_name + " doesn't match any file format" << std::endl;
            }
        }
        result[ind].shrink_to_fit();
        ind++;
    }
    return result;

}
void initListDirectories(string &path,std::vector<std::vector<fs::path>> &vecOfPaths) {
    string msg = " Only sub folders allowed in main folder, file won't be included in the program..";
    std::list<std::list<fs::path>> unOrderedList;
    for (const auto &entry : fs::directory_iterator(path)) {
        if (!entry.is_directory()) {
            std::cout <<  entry.path().filename()  << msg << std::endl;
            continue;
        }
        if(!isValidTravelName(entry.path().filename().string())) continue;
        unOrderedList.emplace_back(std::list<fs::path>());
        for (const auto &deep_entry : fs::directory_iterator(entry.path())) {
            unOrderedList.back().emplace_back((deep_entry.path()));
        }
    }
    vecOfPaths = orderListOfDir(unOrderedList);
    validateSequenceDirectories(vecOfPaths);
}

/**
 * This function gets the paths matrix and validate that files exist.
 * @param direct
 */
void validateSequenceDirectories(vector<vector<fs::path>> &direct) {
    string msg;
    int num = 0;
    setActualSize(direct);
    for (size_t i = 0; i < direct.size(); i++) {
        num++;
        for (size_t j = 0; j < direct[i].size(); j++) {
            if (direct[i][j].empty()) {
                msg.clear();
                if (j == 0) msg = "ship_plan";
                else if (j == 1) msg = "ship_route";
                else msg = ("port num " + std::to_string(j-1));
                std::cout << "Folder Travel " << num << " lack of " << msg
                          << " file, this travel folder isn't going to be tested" << std::endl;
                direct[i].clear();
                break;
            }
        }
    }
}

/**
 * This function sets each sub folder to it's actual size vector
 * @post vec[i][vec[i].size()-1] != empty for every i=0..direct.size()-1
 * @param direct
 */
void setActualSize(std::vector<std::vector<fs::path>> &direct){
    for(size_t i = 0; i < direct.size(); i++){
        for(size_t j = direct[i].size()-1; j > 0; j--){
            if(!direct[i][j].empty()){
                direct[i].resize(j+1);
                break;
            }
        }
    }
}


/**
 * This function checks if port already exist in the vector list, if exists it return 0 but
 * sets a pointer to the same port at the the end of the vector.
 * @param vec - the vector of ports
 * @param str - the string of the port to be check
 * @return 0 iff exist already port with same name
 */
int portAlreadyExist(std::vector<Port*> &vec,string &str){
    for(const auto &element : vec ){
        if(element->get_name() == str){
            vec.emplace_back(element);
            return 1;
        }
    }
    return 0;
}

void getDimensions(std::array<int,3> &arr, std::istream &inFile,string str){
    string line;
    char* input;
    if(str == "byFile"){
        while(getline(inFile,line)) {
            if(line.at(0) != '#') break; //comment symbol
        }
        input = strtok(line.data(),delim);
        for(int i = 0; i < 3; i++) {
            arr[i] = atoi(input);
            input = strtok(NULL, delim);
        }
    }
    else {
        input = strtok(str.data(),delim);
        for(int i = 0; i < 3; i++) {
            arr[i] = atoi(input);
            input = strtok(NULL, delim);
        }
    }
}


void getTravelRoute(Ship* &ship, std::istream &inFile) {
    std::vector<Port *> *vec = new std::vector<Port *>();
    string line;
    while (getline(inFile, line)) {
        if (line.at(0) == '#') continue; //comment symbol
        else if (isValidPortName(line)) {
            if (!portAlreadyExist(*vec, line)) {
                Port *p1 = new Port(line);
                vec->emplace_back(p1);
            }
        }
    }
    ship->setRoute(*vec);
}

/**
 * This function gets a string(a line from the file) parse the line to get 3 ints (x,y,z) such ship.getMap()[x][y][z]
 * will be initialized to be block container.
 * @param str - the string to parse
 * @param ship - the ship to get it's map from.
 */
void setBlocksByLine(std::string &str, Ship* &ship) {
    auto map = ship->get_map();
    std::ifstream inFile;
    std::array<int,3> dim{};
    getDimensions(dim,inFile,str);
    if(dim[0] > ship->getAxis("x") || dim[1] > ship->getAxis("y") || dim[2] > ship->getAxis("z")){
        cout << "One of the provided ship plan constraints exceeding the dimensions of the ship, ignoring..." << endl;
        return;
    }
    else{
        for(int i = 0; i < ship->getAxis("z")-dim[2]; i++){
            (*map)[dim[0]][dim[1]].emplace_back(Block());
        }
        ship->update_free_space(-(ship->getAxis("z") - dim[2]));
    }
}

/**
 * This function parse line by line from the file, and initialized the block containers in the shipmap
 * @param ship - the ship object to get it's shipMap and update it
 * @param inFile - the file pointer
 */
void extractArgsForBlocks(Ship* &ship, std::istream &inFile) {
    string line;
    while (getline(inFile, line)){
        if(line.at(0) == '#') continue;
        setBlocksByLine(line, ship);
    }
}
/**
 * This function parse the ship map and the ship route files and init a new ship object with
 * the information it parsed.
 * @param folder - the folder that contains ship_route, ship_plan files
 * @return the constructed ship iff folder is not empty.
 */
Ship* extractArgsForShip(std::vector<fs::path> &folder) {
    std::ifstream inFile;
    string line, file_path;
    std::array<int, 3> dimensions{};
    std::vector<Port *> travelRoute;
    Ship* ship = nullptr;
    if(folder.empty()) return nullptr;
    for (int i = 0; i < 2; i++) {
        file_path = folder[i].string();
        inFile.open(file_path);
        if (inFile.fail()) {
            cout << "Failed to read from this file path - " + file_path << endl;
            return nullptr;
        } else if (i == 0) {
            getDimensions(dimensions, inFile,"byFile");
            ship = new Ship(dimensions[0], dimensions[1], dimensions[2]);
            extractArgsForBlocks(ship, inFile);
        } else {
            getTravelRoute(ship,inFile);
        }
        inFile.close();
    }
    return ship;
}