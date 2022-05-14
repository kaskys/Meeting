//
// Created by abc on 21-10-11.
//

#ifndef TEXTGDB_DISPLAYMANAGER_H
#define TEXTGDB_DISPLAYMANAGER_H

#include "DisplayBasic.h"

#define DISPLAY_NOTE_HOST_POSITION  0

enum DisplayStatus{
    DISPLAY_NOTE_STATUS_DEFAULT = 0,
    DISPLAY_NOTE_STATUS_INIT,
    DISPLAY_NOTE_STATUS_UNINIT,
    DISPLAY_NOTE_STATUS_JOIN,
    DISPLAY_NOTE_STATUS_LINK,
    DISPLAY_NOTE_STATUS_SEQUENCE,
    DISPLAY_NOTE_STATUS_NORMAL,
    DISPLAY_NOTE_STATUS_PASSIVE,
    DISPLAY_NOTE_STATUS_EXIT,
    DISPLAY_NOTE_STATUS_PREVENT
};

struct DisplayParameter{
    friend bool operator==(const DisplayParameter&, const DisplayParameter&);
    friend bool operator!=(const DisplayParameter&, const DisplayParameter&);

    uint32_t high;
    uint32_t wide;
    std::pair<uint32_t, uint32_t> window_position;
};

inline bool operator==(const DisplayParameter &comp1, const DisplayParameter &comp2){
    return ((comp1.high == comp2.high) && (comp1.wide == comp2.high)
            && (comp1.window_position.first == comp2.window_position.first)
            && (comp1.window_position.second == comp2.window_position.second));
}

inline bool operator!=(const DisplayParameter &comp1, const DisplayParameter &comp2){
    return !operator==(comp1, comp2);
}

class DisplayBuffer final {
#define UPDATE_HIGH_BIT     0x80000000
public:
    explicit DisplayBuffer(uint32_t pos) : note_status(0), note_pos(pos), note_addr(), note_display_parameter() {}
    ~DisplayBuffer() = default;

    uint32_t getDisplayPos() const { return note_pos; }
    const char* getDisplayStr() const { return note_addr.c_str(); }
    std::string getDisplayString() const { return note_addr; }
    DisplayStatus getDisplayStatus() const { return static_cast<DisplayStatus>(note_status & (~UPDATE_HIGH_BIT)); }

    bool isUpdate() { int update_bit = note_status & UPDATE_HIGH_BIT; note_status &= (~UPDATE_HIGH_BIT); return update_bit; }
    void setDisplayAddr(const std::string &addr){ note_addr = addr; }
    void setDisplayAddr(std::string &&addr){ note_addr = std::move(addr); }

    void setDisplayStatus(int status) { note_status = status; }
    void updateDisplayParameter(const DisplayParameter &parameter){ note_display_parameter = parameter; }

    void onUpdateDisplayInfo(DisplayStatus status, const std::string &addr){
        if(note_addr != addr){ setDisplayAddr(addr); note_status |= UPDATE_HIGH_BIT; }
        if(note_status != status){ setDisplayStatus(status); note_status |= UPDATE_HIGH_BIT; }
    }

    bool onUpdateParameterInfo(const DisplayParameter &parameter){
        if(note_display_parameter != parameter){ updateDisplayParameter(parameter); return true; }
        else { return false;}
    }

private:
    int note_status;
    uint32_t note_pos;
    std::string note_addr;
    DisplayParameter note_display_parameter;
};


class DisplayManager : public DisplayBasic{
public:
    DisplayManager(const std::function<char*(int)>&, const std::function<void(char*,int)>&) throw(std::runtime_error, std::bad_alloc);
    ~DisplayManager() override;

    void onDisplayExtract(const std::function<void(char*, uint32_t, const std::function<void(char*,int)>&)>&);
    void onNoteMedia(char*, uint32_t, uint32_t);
    void onNoteUpdate(DisplayStatus, uint32_t, uint32_t) throw(std::bad_alloc);
    void onUpdateNoteParameter(const DisplayParameter&, uint32_t) ;
private:
    void initDisplay() throw(std::bad_alloc);
    void uinitDisplay();

    DisplayBuffer* onFindNoteOnPos(uint32_t);
    DisplayBuffer* onCreateDisplayBuffer(DisplayStatus, uint32_t, uint32_t) throw(std::bad_alloc);
    void onDestroyDisplayBuffer(DisplayBuffer *dbuffer) { delete dbuffer; }

    std::function<char*(int)> create_memory_func;
    std::function<void(char*,int)> destroy_memory_func;
    std::map<uint32_t, DisplayBuffer*> display_note_map;
};


#endif //TEXTGDB_DISPLAYMANAGER_H
