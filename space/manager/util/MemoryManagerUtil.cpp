//
// Created by abc on 19-10-8.
//
#include "../MemoryManager.h"

/*
 * |__head__|__correlate__|__use_info(idle)__|__use_info(fixed)__|
 *                               |
 *                               |
 *                               _
 *                      |_______buf________|
 *                      getMemoryBufferUseLen
 */

/**
 * 获取MemoryBufferUseInfo使用的内存长度
 * @param mbuf
 * @return
 */
int getMemoryBufferUseLen(char *mbuf){
    return mbuf ? (reinterpret_cast<MemoryBufferUseInfo*>(mbuf))->len : 0;
}

/**
 * 获取MemoryBufferIdleInfo的长度
 * @param mbuf
 * @return
 */
int getMemoryBufferIdleLen(char *mbuf){
    return mbuf ? (reinterpret_cast<MemoryBufferIdleInfo*>(mbuf))->end - (reinterpret_cast<MemoryBufferIdleInfo*>(mbuf))->start : 0;
}


/**
 * 完成use_info向idle_info合并后,获取下一个MemoryBuffer的idle_info(弹出idle_info)(FILO)
 * @param minfo
 * @param mbuf
 * @return
 */
char* nextMemoryBufferIdle(char *minfo){
    MemoryBufferMergeInfo *merge_info = nullptr;

    /*
     * 转换对应的类
     */
    if((merge_info = reinterpret_cast<MemoryBufferMergeInfo*>(minfo))){

        if(merge_info->idle_info){
            if(merge_info->merge_idle_info.next) {
                /*
                 * 完成use_info的合并后,把idle_info合并于后一个ni_info(即idle_info.next)
                 * 并消除当前的idle_info,即MemoryBuffer.idle_buf_info = ni_info
                 */
                (reinterpret_cast<MemoryAdjudicator*>(merge_info->manager))->shrinkMemoryBufferIdleInfo0(&merge_info->merge_idle_info, merge_info->merge_idle_info.ipu_info, merge_info->buffer);
            }else{
                if(!merge_info->merge_idle_info.ipu_info->next){
                    //(1).先设置回收尾部空间
                    merge_info->merge_idle_info.end = merge_info->buffer->buffer_len;
                }

                //(2)判断是否回收idle_info(因为引用merge_idle_info作为参数,所以会连同回收尾部空间一起计算)
                if((merge_info->merge_idle_info.end - merge_info->merge_idle_info.start) <= sizeof(MemoryBufferIdleInfo)){
                    //移动回收该idle_info
                    (reinterpret_cast<MemoryAdjudicator*>(merge_info->manager))->shrinkMemoryBufferIdleInfo0(&merge_info->merge_idle_info, merge_info->merge_idle_info.ipu_info, merge_info->buffer);
                }else{
                    //令idle_info->start = merge_idle_info->start及ipu_info(merge_idle_info的start是已经插入use_info后更新的,idle_info的start是未更新的)
                    merge_info->idle_info->start = merge_info->merge_idle_info.start;
                    merge_info->idle_info->ipu_info = merge_info->merge_idle_info.ipu_info;

                    //不用设置end(因为merge_idle_info已经更新回收尾部空间)
                    //(3)判断是否需要回收尾部空间(需要未更新的回收尾部空间)
                    (reinterpret_cast<MemoryAdjudicator*>(merge_info->manager))->expandMemoryBufferIdleInfo(&merge_info->merge_idle_info, merge_info->buffer, merge_info->merge_idle_info.end);
                }
            }
            merge_info->idle_info = merge_info->merge_idle_info.next;
        }else{
            merge_info->idle_info = merge_info->buffer->idle_buf_info;
        }

        if(merge_info->idle_info){
            merge_info->merge_idle_info = *merge_info->idle_info;
        }
    }

    return merge_info ? reinterpret_cast<char*>(merge_info->idle_info) : nullptr;
}

/**
 * 将长度小于idle_info的use_info插入到idle_info里
 * @param minfo
 * @param uinfo
 * @param len
 * @return
 */
int insertMemoryBufferUseToIdle(char *minfo, char *uinfo, int len){
    int idle_info_len = 0;
    MemoryBufferUseInfo *ipu_info = nullptr;
    MemoryBufferMergeInfo *merge_info = nullptr;
    MergeMemoryUseNote *use_note = nullptr;

    /*
     * 转换对应的类
     */
    if((merge_info = reinterpret_cast<MemoryBufferMergeInfo*>(minfo)) && (use_note = reinterpret_cast<MergeMemoryUseNote*>(uinfo)) && (len > 0)){
        //获取idle_info的长度
        idle_info_len = merge_info->merge_idle_info.end - merge_info->merge_idle_info.start;

        //判断idle_info还有长度及大于use_info的长度
        if((idle_info_len > 0) && (len <= idle_info_len)){
            /*
             *  将use_info插入到idle_info里
             */
            ipu_info = (reinterpret_cast<MemoryAdjudicator*>(merge_info->manager))
                    ->memmoveMemoryBufferUseInfo(merge_info->buffer, reinterpret_cast<MemoryBufferUseInfo*>(use_note->use_info),
                                                 reinterpret_cast<MemoryBufferIdleInfo*>(use_note->uni_info), merge_info->merge_idle_info.ipu_info,
                                                                                             &merge_info->merge_idle_info, [&](int move_len) -> void {
                                                                                                                                merge_info->merge_idle_info.start += move_len;
                                                                                                                           });
        }
    }

    return ipu_info ? 1 : 0;
}

void clearMergeInfo(char *info, char *buffer){
    MemoryBufferMergeInfo *merge_info = nullptr;

    if((merge_info = reinterpret_cast<MemoryBufferMergeInfo*>(info))){
        *merge_info = MemoryBufferMergeInfo(merge_info->manager, reinterpret_cast<MemoryBuffer*>(buffer));
    }
}
