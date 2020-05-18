#include "Port.h"
#include "Container.h"
#include "Common.h"
#include <algorithm>
#include <fstream>

void Port::addContainer(Container container, Type command) {
    switch(command){
        case Type::LOAD: load.emplace_back(container); break;
        case Type::UNLOAD: unload.emplace_back(container); break;
        case Type::ARRIVED: arrived.emplace_back(container); break;
        case Type::PRIORITY: priority.emplace_back(container); break;
        default: std::cout << "Invalid command, please enter L/U/A/P." << std::endl;
    }
}

const std::string&  Port::get_name() {
    return name;
}

bool Port::operator==(const Port& p){
    return this->get_name() == p.name;
}

Port::~Port(){
    unload.clear();
    load.clear();
    priority.clear();
    arrived.clear();
}

std::vector<Container>* Port::getContainerVec(Type type){
    switch(type){
        case Type::LOAD: return &this->load;
        case Type::UNLOAD: return &this->unload;
        case Type::PRIORITY: return &this->priority;
        case Type::ARRIVED: return &this->arrived;
        default: return nullptr;
    }
}

void Port::removeContainer(std::string &id, Type command) {
    int i = 0;
    bool found = false;
    if(command == Type::LOAD){
        for(;i < (int)load.size(); i++){
            if(load[i].getId() == id){
                found = true;
                break;
            }
        }
        if(found)
            load.erase(load.begin()+i);
    }
    else if(command == Type::PRIORITY){
        for(; (int)priority.size(); i++){
            if(priority[i].getId() == id){
                found = true;
                break;
            }
        }
        if(found)
            priority.erase(priority.begin()+i);
    }

}
