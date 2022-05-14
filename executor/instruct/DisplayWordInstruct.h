//
// Created by abc on 21-10-9.
//

#ifndef TEXTGDB_DISPLAYWORDINSTRUCT_H
#define TEXTGDB_DISPLAYWORDINSTRUCT_H

#include "Instruct.h"

class DisplayThreadUtil;

struct CorrelateDisplayInstruct final : public Instruct{
    CorrelateDisplayInstruct(BasicThread *thread, DisplayThreadUtil *util) : Instruct(INSTRUCT_DISPLAY_CORRELATE, thread),
                                                                             thread_util(util), correlate_promise() {}
    ~CorrelateDisplayInstruct() override = default;

    uint32_t classSize() const override { return sizeof(CorrelateDisplayInstruct); }

    DisplayThreadUtil *thread_util;
    std::promise<void> correlate_promise;
};

#endif //TEXTGDB_DISPLAYWORDINSTRUCT_H
