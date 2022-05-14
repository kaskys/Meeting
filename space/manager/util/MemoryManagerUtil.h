//
// Created by abc on 19-10-8.
//

#ifndef UNTITLED8_MEMORYMANAGERUTIL_H
#define UNTITLED8_MEMORYMANAGERUTIL_H

#include "../../struct/SpaceHead.h"
#include "../../../util/list/UnLockList.h"

#ifdef __cplusplus
extern "C"
{
#endif
#include "../../algorithm/Combination.h"
#ifdef __cplusplus
};
#endif

/**
 * 获取MemoryBufferUseInfo指向的buf
 *
 * |__head__|______buf_______|__use_info__| (obtainBuffer)
 * @param use_info
 * @return
 */
inline char* getMemoryBufferUseInfoBuf(MemoryBufferUseInfo *use_info){
    return use_info->parent->buffer + use_info->start;
}

/**
 * 从buf中获取FIXED指向的MemoryBufferUseInfo
 * @param gp
 * @param len
 * @return
 *
 * |(gp)__head__|___correlate___|__use_info(idle)__|__use_info(fixed)__|
 */
inline MemoryBufferUseInfo* getMemoryBufferUseInfo(char *gp, int len) {
    return reinterpret_cast<MemoryBufferUseInfo*>(gp + (len - sizeof(MemoryBufferUseInfo)));
}

/**
 * 从MemoryCorrelate中获取MemoryBufferUseInfo
 * |__head__|__correlate__|__use_info(idle)__|__use_info(fixed)__|
 *                              ->
 * @param correlate
 * @return
 */
inline MemoryBufferUseInfo* getMemoryBufferUseInCorrelate(MemoryCorrelate *correlate){
    return reinterpret_cast<MemoryBufferUseInfo*>(++correlate);
}

/**
 * 从buf中提取MemoryBufferIdleInfo
 * @param gp
 * @param len
 * @return
 *
 * |__gp_______________|__idle_info__|
 * |                                 |
 * |----------------len--------------|
 */
inline MemoryBufferIdleInfo* getMemoryBufferIdleInfo(char *gp, int len) {
    return reinterpret_cast<MemoryBufferIdleInfo*>(gp + (len - sizeof(MemoryBufferIdleInfo)));
}

/**
 * 从buf中提取MemoryBufferSubstituteInfo
 * @param gp
 * @param len
 * @return
 *
 * |__gp_______________|__substitute_info__|
 * |                                       |
 * |----------------len--------------------|
 */
inline MemoryBufferSubstituteInfo* getMemoryBufferSubstituteInfo(char *gp, int len){
    return reinterpret_cast<MemoryBufferSubstituteInfo*>(gp + (len - sizeof(MemoryBufferSubstituteInfo)));
}

int getMemoryBufferUseLen(char*);
int getMemoryBufferIdleLen(char*);

char* nextMemoryBufferIdle(char*);

int insertMemoryBufferUseToIdle(char*,char*,int);
void clearMergeInfo(char*, char*);

#endif //UNTITLED8_MEMORYMANAGERUTIL_H
