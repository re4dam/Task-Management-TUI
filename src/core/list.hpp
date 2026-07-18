#ifndef LIST_HPP
#define LIST_HPP

#include <string>
#include <vector>
#include "task.hpp"

struct List {
    std::string name;
    std::vector<Task> tasks;
};

#endif // LIST_HPP
