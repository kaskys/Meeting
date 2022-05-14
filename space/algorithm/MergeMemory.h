//
// Created by abc on 19-10-8.
//

#ifndef UNTITLED8_MERGEMEMORY_H
#define UNTITLED8_MERGEMEMORY_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

typedef struct MergeMemory  MergeMemory;
typedef struct MemoryBufferUseInfo  MemoryBufferUseInfo;
typedef struct MemoryBufferIdleInfo MemoryBufferIdleInfo;

/**
 * 转换表
 */
typedef struct{
    int use_flag;                       //标记（当该memory被选中为被结合的内存时,flag = 0,否则flag = 1）
    MergeMemory *merge_memory;          //对应的MergeMemory
} MergeLocationTable;

/**
 * 合并内存
 */
struct MergeMemory{
    int rank;                               //提取idle_memory的个数
    int note_size;                          //关联该Memory的UseInfo的数量
    char *buffer;                           //指向需要合并的memory
    MergeLocationTable *location_table;     //指向转换表
};

/**
 * 合并内存中使用的内存
 */
struct MergeMemoryUseNote{
    int use_len;                                    //use的长度
    int *flag;                                      //标记（没有被结合,flag != NULL,否则flag == NULL）
    char *use_info;                                 //use_info的信息
    char *uni_info;                                 //如果有idle_info.ipu_info指向该use_info,那么uni_info指向idle_info,否则为NULL
    MergeLocationTable *location_merge_memory;      //指向MergeMemory转换表,获取实际的合并内存
    struct MergeMemoryUseNote *skip_note;           //指向连续被结合的最后一个MergeMemoryUseNote
};
typedef struct MergeMemoryUseNote   MergeMemoryUseNote;


/**
 * 二叉堆
 */
typedef struct{
    int use_size;               //use_info数量
    int heap_size;              //数组数量

    char *merge_info;
    int (*len_func)(char*);     //提取idle_info长度的函数
    /*
     * 返回下一个idle_info
     * 如果char* == NULL,则返回第一个idle_info
     */
    char* (*info_func)(char*);
    /*
     * 插入use_info到idle_info的函数
     * 如果use_info填满idle_info(+use_infos.len >= (idle_info.len - sizeof(MemoryBufferIdleInfo))),需要删除该idle_info
     *  返回 该idle_info->prev
     * 否则填充不满idle_info,该idle_info还存在MemoryBuffer中
     *  返回 该idle_info
     */
    int (*insert_func)(char*, char*, int);
    MergeMemory *memory_heap;   //数组
} PercolateMemory;

void InitMergeMemoryAndConvertTable(MergeMemory*, MergeLocationTable*, char*, int, int);
void InsertSortMemory(MergeMemory*, MergeLocationTable*, char*, int, int, int);
int InsertSortUse(MergeLocationTable*, char*, MergeMemoryUseNote*, int, int);

void MergeMemoryFunc(MergeMemory*, MergeMemoryUseNote*, char*, int(*)(char*), char*(*)(char*), int(*)(char*,char*,int),  void(*)(char*,char*), int, int);
void SkipMergeUse(PercolateMemory*);
void ExtractMin(PercolateMemory*);
void PercolateDown(PercolateMemory*, MergeMemory*);
void PercolateUp(MergeMemory*, MergeMemory*);

void SwapMergeMemory(MergeMemory*, MergeMemory*);

#endif //UNTITLED8_MERGEMEMORY_H
