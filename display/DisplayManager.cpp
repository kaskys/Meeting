//
// Created by abc on 21-10-11.
//
#include "DisplayManager.h"


void DisplayMediaData::onAnalysis() {
    //暂未实现
}

void DisplayMediaData::onCompile() {
    //暂未实现
}

//--------------------------------------------------------------------------------------------------------------------//

DisplayManager::DisplayManager(const std::function<char*(int)> &cfunc, const std::function<void(char*,int)> &dfunc) throw(std::runtime_error, std::bad_alloc)
        : DisplayBasic(), create_memory_func(cfunc), destroy_memory_func(dfunc), display_note_map() {
    if(!create_memory_func || !destroy_memory_func){
        throw std::runtime_error("not memory func!");
    }

    initDisplay();
}

DisplayManager::~DisplayManager() {
    uinitDisplay();
}

void DisplayManager::initDisplay() throw (std::bad_alloc) {
    onNoteStatus(onCreateDisplayBuffer(DISPLAY_NOTE_STATUS_DEFAULT, 0, DISPLAY_NOTE_HOST_POSITION));
}

void DisplayManager::uinitDisplay() {
    for(auto iterator : display_note_map){
        if(iterator.second){ onDestroyDisplayBuffer(iterator.second); }
    }
    display_note_map.clear();
}

DisplayBuffer* DisplayManager::onFindNoteOnPos(uint32_t pos) {
    auto display_iterator = display_note_map.find(pos);
    return ((display_iterator != display_note_map.end()) ? display_iterator->second : nullptr);
}

DisplayBuffer* DisplayManager::onCreateDisplayBuffer(DisplayStatus status, uint32_t addr, uint32_t pos) throw(std::bad_alloc) {
    DisplayBuffer *display_buffer = nullptr;
    auto display_iterator = display_note_map.find(pos);

    if(display_iterator != display_note_map.end()){
        display_buffer = display_iterator->second;
    }else{
        if(!display_note_map.insert({pos, (display_buffer = new DisplayBuffer(pos))}).second){
            onDestroyDisplayBuffer(display_buffer); throw std::bad_alloc();
        }
    }
    display_buffer->onUpdateDisplayInfo(status, onIntToString(addr));

    return display_buffer;
}



void DisplayManager::onNoteUpdate(DisplayStatus status, uint32_t addr, uint32_t pos) throw(std::bad_alloc) {
    DisplayBuffer *display_buffer = onFindNoteOnPos(pos);


    if((display_buffer || (display_buffer = onCreateDisplayBuffer(status, addr, pos))) && display_buffer->isUpdate()){
        onNoteStatus(display_buffer);
    }
}

void DisplayManager::onNoteMedia(char *media_buffer, uint32_t media_len, uint32_t pos) {
    DisplayBuffer *display_buffer = onFindNoteOnPos(pos);

    if(!display_buffer){ return; }

    DisplayMediaData media_data{};
    try {
        media_data = onAnalysisDisplayMedia(media_buffer, media_len);
    }catch (std::logic_error &e){
        std::cout << "DisplayManager->onAnalysisDisplayMedia->" << display_buffer->getDisplayStr() << std::endl;
    }

    if(media_data.media_buffer){
        onNoteMediaUpdate(display_buffer, &media_data);
    }
}

void DisplayManager::onUpdateNoteParameter(const DisplayParameter &parameter, uint32_t note_pos) {
    DisplayBuffer *display_buffer = onFindNoteOnPos(note_pos);

    if(display_buffer && display_buffer->onUpdateParameterInfo(parameter)){
        onNoteParameter(display_buffer);
    }

}

void DisplayManager::onDisplayExtract(const std::function<void(char*, uint32_t, const std::function<void(char*,int)>&)> &extract_func) {
    if(extract_func){
        DisplayMediaData media_data = onDisplayExtract0();
        extract_func(media_data.media_buffer, media_data.media_len, destroy_memory_func);
    }
}

