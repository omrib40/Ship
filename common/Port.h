/**
* This module represents a port
* each port has a:
* -name
* -containers:
*	# to load:
*       -priority load
*       -regular load
*	# to unload
*	# arrived
*
*
*      *******      Functions      ******
* addContainer      - adds container to port by the command type.
* get_name          - returns the name of the port.
* operator==        - returns true if this->name == p->name.
* getContainerVec   - returns pointer to the type vector(load, priority, unload or arrived) of port.
*
*/
#ifndef PORT_HEADER
#define PORT_HEADER




#include <iostream>
#include <cstring>
#include <string>
#include <iterator>
#include <vector>
#include <stack>
class Ship; class Container;

/*----------------------Prefix variables-------------------*/
enum class Type {PRIORITY = 'P', LOAD = 'L', UNLOAD = 'U', ARRIVED = 'A'};


class Port {
    std::string name;
    std::vector<Container> priority;
    std::vector<Container> load;
    std::vector<Container> unload;
    std::vector<Container> arrived;
public:
    /*C'tor*/
Port(const std::string& name) : name(name){}
~Port();

    void addContainer(Container& container,Type command);
    void removeContainer(std::string& id,Type command);
    const std::string & get_name();
    bool operator==(const Port& p);
    std::vector<Container>* getContainerVec(Type type);
};
#endif