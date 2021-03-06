#ifndef SHIP_HEADER
#define SHIP_HEADER

/**
* This header is a container of a ship that holds 3D vector of containers
* and the route of the current ship
*
*/
class Container; class Port;

#include <iostream>
#include <cstring>
#include <string>
#include <list>
#include <iterator>
#include <map>
#include "Port.h"
#include "Container.h"
#include <stack>
#include <set>
#include <algorithm>
#include <memory>
#include "../interfaces/WeightBalanceCalculator.h"

typedef std::tuple<int,int> coordinate;
const char delim[] = {',','\t','\r',' ','\n','\0'};

class Ship {
    std::vector<std::vector<std::vector<Container>>> shipMap;
    std::map<std::shared_ptr<Port>, std::vector<Container>> containersByPort;
    std::vector<std::shared_ptr<Port>> route;
    int freeSpace;
    int x, y, z;
public:
    /*given a route of ports, the C'tor parses the containers of any port to a map*/
    Ship(int x, int y, int z) {
        std::vector<std::vector<std::vector<Container>>> map;
        map.resize(x);
        for(int i = 0; i < x; i++){
            map[i].resize(y);
            for(int j = 0; j < y; j++){
                map[i][j].reserve(z);
            }
        }
        this->shipMap = map;
        for(int i = 0; i < x; i++){
            for(int j = 0; j < y; j++){
                this->shipMap[i][j].reserve(z);
            }
        }
        this->x = x;
        this->y = y;
        this->z = z;
        freeSpace = x*y*z;
    }
    Ship(const Ship* shipToCopy){
        x = shipToCopy->getAxis("x");
        y = shipToCopy->getAxis("y");
        z = shipToCopy->getAxis("z");
        freeSpace = shipToCopy->getFreeSpace();
        this->shipMap.resize(x);
        for(int i = 0; i < x; i++){
            this->shipMap[i].resize(y);
            for(int j = 0; j < y; j++){
                //Note that this c'tor only for copying the blocks
                this->shipMap[i][j] = shipToCopy->shipMap[i][j];
                this->shipMap[i][j].reserve(z);
            }
        }
        bool found = false;
        for(auto& p_out : shipToCopy->route){
            for(auto& p_this : this->route){
                if(p_out->get_name() == p_this->get_name()){
                    this->route.emplace_back(p_this);
                    found = true;
                    break;
                }
            }
            if(!found){//Case we didn't found a port that already exist...need to insert this one.
                this->route.emplace_back(std::make_shared<Port>(p_out->get_name()));
            }
            found = false;
        }
    }
    ~Ship();
    std::tuple<int, int, int> getCoordinate(const Container& container);
    std::tuple<int,int,int> getCoordinate(std::string& contName);
    std::vector<std::shared_ptr<Port>> getRoute();
    std::vector<std::vector<std::vector<Container>>>& getMap();
    std::shared_ptr<Port> getPortByName(const std::string &name);
    std::map<std::shared_ptr<Port>,std::vector<Container>>& getContainersByPort();
    void initContainersByPort(std::vector<std::shared_ptr<Port>>& vector);
    void setRoute(std::vector<std::shared_ptr<Port>>& route);
    int getAxis(const std::string& str) const;
    void getCoordinatesToHandle(std::set<coordinate> &coordinates_to_handle, std::vector<Container>& containers_to_unload);
    int getLowestFloorOfRelevantContainer(std::shared_ptr<Port>& pPort, coordinate coor);
    int getTopFloor(coordinate coor);
    void getColumn(coordinate coor, std::vector<Container>** column);
    int getFreeSpace() const;
    void getContainersToUnload(std::shared_ptr<Port>& port, std::vector<Container>** unload);
    bool findColumnToMoveTo(coordinate old_coor, coordinate& new_coor, std::vector<Container>& containersToUnload, int weight, WeightBalanceCalculator& calc);
    void findColumnToLoad(coordinate &coor, bool &found, int kg, WeightBalanceCalculator& calc);
    void addContainer(Container& container, std::tuple<int,int> coordinate);
    void removeContainer(coordinate coor);
    void moveContainer(coordinate origin, coordinate dest);
    void updateFreeSpace(int num);
    bool isOnShip(Container &con);

};

#endif