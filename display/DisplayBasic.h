//
// Created by abc on 21-10-11.
//

#ifndef TEXTGDB_DISPLAYBASIC_H
#define TEXTGDB_DISPLAYBASIC_H

#include "DisplayUtil.h"

struct DisplayMediaData final{
    DisplayMediaData() = default;
    explicit DisplayMediaData(uint32_t len, char *buffer) : media_len(len), media_buffer(buffer) {}
    ~DisplayMediaData() = default;

    void onAnalysis();
    void onCompile();

    uint32_t media_len;
    char *media_buffer;
};

class DisplayBasic {
public:
    DisplayBasic();
    virtual ~DisplayBasic();
protected:
    static std::string onIntToString(uint32_t addr){
#define INT_SHIFT_CHAR  8
#define CHAR_BIT_VALUE  0x000000FF
        std::string saddr;

        for(int start = sizeof(addr) - 1, value = 0; start >= 0; start--){
            value = (addr >> (start * INT_SHIFT_CHAR) & CHAR_BIT_VALUE);
            saddr.append(std::to_string(value));

            if(start > 0) {
                saddr.append(".");
            }
        }

        return saddr;
    }

    void onNoteStatus(DisplayBuffer*);
    void onNoteParameter(DisplayBuffer*);
    void onNoteMediaUpdate(DisplayBuffer*, DisplayMediaData*);
    DisplayMediaData onDisplayExtract0();
    DisplayMediaData onAnalysisDisplayMedia(char*, uint32_t);
private:
    DisplayCodecUtil   codec_util;
    DisplayMonitorUtil monitor_util;
    DisplayCameraUtil  camera_util;
};


#endif //TEXTGDB_DISPLAYBASIC_H
