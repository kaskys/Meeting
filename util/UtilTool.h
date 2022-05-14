//
// Created by abc on 22-2-28.
//

#ifndef TEXTGDB_UTILTOOL_H
#define TEXTGDB_UTILTOOL_H

#include <list>
#include <atomic>
#include <memory>
#include <utility>
#include <iostream>
#include <string.h>

inline std::string onRuntimeErrorValue(const char *class_name, const char *func_name, int up, int down){
    std::string error_value(class_name);
    return error_value.append("::").append(func_name).append("->runtime_error:")
            .append(std::to_string(down)).append(" up ").append(std::to_string(up)).c_str();
}

#endif //TEXTGDB_UTILTOOL_H
