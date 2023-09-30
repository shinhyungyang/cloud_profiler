#include "time_tsc.h"

#include "globals.h"
#include "cloud_profiler.h"

#include <vector>
#include <iostream>
#include <list>

std::vector<long> * targetNodes;

void printStrPtrVector(std::vector<std::string *> & strptrVec) {
  for ( auto it = strptrVec.begin(); it != strptrVec.end(); ++it ) {
    std::cout << *(*it) << std::endl;
  }
}

void deleteStrPtrVector(std::vector<std::string *> & strptrVec) {
  for ( auto it = strptrVec.begin(); it != strptrVec.end(); ++it ) {
    delete *it;
  }
}

void printStringVector(std::vector<std::string> & node_list) {
  for ( auto it = node_list.begin(); it != node_list.end(); ++it ) {
    std::cout << *it << std::endl;
  }
}

std::string * newString(std::string str1) {
  return new std::string(str1);
}

void addToStrPtrVec(std::vector<std::string *> & strptrVec, std::string str2) {
  strptrVec.push_back(new std::string(std::move(str2)));
}
