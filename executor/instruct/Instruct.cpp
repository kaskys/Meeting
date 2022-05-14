//
// Created by abc on 20-5-9.
//
#include "Instruct.h"

void Instruct::releaseInstruct(Instruct *instruct) {
    if(instruct){ free(instruct); }
}