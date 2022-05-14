//
// Created by abc on 19-10-7.
//

#ifndef UNTITLED8_FITASSEMBLE_H
#define UNTITLED8_FITASSEMBLE_H

#include "MergeMemory.h"

struct MergeMemory;
struct MergeMemoryUseNote;
typedef struct AssembleMatchNote    AssembleMatchNote;

#define DEFALUT_ASSEMBLE_COMBINE_STACK_LEN  1024 * 1024

/**
 * 位置表
 *         (d)         (d)   (d)               (d)
 * |_(0)_|_(1)_|_(2)_|_(3)_|_(4)_|_(5)_|_(6)_|_(7)_|_(8)_|  (use_array)use的位置
 *    |       --|                 |       |          |
 * => |      /      - - - - - - -- ------------------
 *    |     /      /    /      /
 * |_(0)_|_(1)_|_(2)_|_(3)_|_(4)_|_(5)_|_(6)_|_(7)_|_(8)_| (use_pos)实际位置
 */
typedef struct {
    int *use_pos;       //实际位置到use的位置转换
    int *ruse_pos;      //use位置到实际位置转换（反向表）
} PosTable;

/**
 * 根节点（算法根）
 */
typedef struct{
    int match_value;                    //匹配的值
    int rank_value;                     //从0到skip_value_pos的接近值
    int skip_value_pos;                 //之前的结合值小于match_value(从0到skip_value_pos的结合值刚好大于等于match_value)
    int skip_lower_combine_pos;         //从skip_value_pos到之前的结合值小于match_value(从skip_value_pos到skip_lower_combine_pos的结合值刚好大于等于match_value)
    int lower_value_pos;                //lower_value_pos之后（包含）的值大于等于match_value

    PosTable *pos_table;                //位置表
    MergeMemoryUseNote *use_array;      //需要匹配的use数组
    MergeMemoryUseNote **match_array;   //已经匹配的use数组
} AssembleRoot;

/**
 * 匹配Note
 */
struct AssembleMatchNote{
    int use_size;               //匹配和结合的use数量
    int combine_pos;            //最后结合的pos(实际位置)
    int mrank_value;            //匹配值
    int crank_value;            //结合值
    unsigned char *match_bit;   //匹配实际的位值（用bit表示）
    unsigned char *combine_bit; //结合实际的位置（用bit表示）
    AssembleMatchNote *next;    //链表
};

/**
 * 结合元素个数信息
 */
typedef struct {
    AssembleMatchNote *max_rank_note;   //相同结合元素下最大的接近值
    AssembleMatchNote *min_size_note;   //相同结合元素下最小的匹配use数量
    AssembleMatchNote *match_note;      //与match_value相等
    AssembleMatchNote *fnote;           //相同结合元素下第一个匹配Note
} AssembleMatchLenInfo;

/**
 * 结合信息
 */
typedef struct{
    int match_len;                  //匹配长度（0到skip_value_pos-1）
    int combine_len;                //结合长度（skip_value_pos到skip_lower_combine_pos-1）
    int match_size;                 //匹配的数量

    unsigned char *bit_array;       //匹配结合实际位置的数组
    AssembleRoot *root;             //根节点
    AssembleMatchNote *note_array;  //匹配Note的数组
    AssembleMatchNote *pnote;       //相同结合元素下最后一个匹配Note
} AssembleCombine;

void MatchFitAssemble(PercolateMemory*, MergeMemoryUseNote*, void(*)(PercolateMemory*, MergeMemory*), int );
void MatchFitAssemble0(MergeMemoryUseNote*, MergeMemoryUseNote**, PosTable* , int, int, void(*)(AssembleRoot*, PosTable*, int));

void InitAssembleRootSkipSize(AssembleRoot*, PosTable*, int);
void InitAssembleRootUsePos(AssembleRoot*, PosTable*, int);
void InitPosTable(AssembleRoot*, PosTable *, int*, int);

void FindFitAssemble(AssembleRoot*, int);
void FindFitAssemble0(AssembleRoot*);

inline int RankValue(AssembleMatchNote *);
inline void InitStackData(AssembleCombine*, AssembleMatchLenInfo*, unsigned char*, AssembleMatchNote*);
void InitAssembleCombine(AssembleCombine*, int, int, int, int);
void InitMatchNote(AssembleCombine*, AssembleMatchLenInfo*, AssembleMatchNote*, int, int);

void CompareAndSet(AssembleMatchNote*, AssembleMatchNote*, AssembleMatchNote*, AssembleCombine*);
void SetBitRangeMatchNote(AssembleCombine*, AssembleMatchNote*);
int SetBitMatchNote(int, int, const unsigned char*, MergeMemoryUseNote*, MergeMemoryUseNote**, int*, int*);
void GetBitMatchNote(int, int, unsigned char *, MergeMemoryUseNote*, int*);

void SetMatchNoteBitRange(AssembleMatchNote*, AssembleMatchNote*, AssembleCombine*, int, int);
inline void SetMatchNoteBit(unsigned char*, int);
inline void CopyCombineNoteBit(AssembleCombine*, AssembleMatchNote*, unsigned char*);
#endif //UNTITLED8_FITASSEMBLE_H
