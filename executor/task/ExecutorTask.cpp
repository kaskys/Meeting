//
// Created by abc on 21-11-2.
//
#include "ExecutorTask.h"

ReleaseTaskHolder release_holder = ReleaseTaskHolder();

void HolderInterior::onReduce() {
    if (release_flag.fetch_sub(1, std::memory_order_release) <= 1) {
        release_holder(this);
    }
}
