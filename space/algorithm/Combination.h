//
// Created by abc on 19-10-21.
//

#ifndef UNTITLED8_COMBINATION_H
#define UNTITLED8_COMBINATION_H

#include "FitAssemble.h"

//把一个数升到8的倍数
#define size_round_up(size)     ((size > 0) ? (((size) + 8 - 1) & ~(8 - 1)) : 8)

/**
 * 结合元素
 */
typedef struct {
    int min_number;                     //该算法最小结合个数
    int begin_combine_pos;              //开始结合位置
    int *use_pos;                       //实际位置到use的位置转换
    AssembleRoot *root;                 //根节点
    MergeMemoryUseNote *use_array;      //需要匹配的use数组
    MergeMemoryUseNote **match_array;   //已经匹配的use数组
} CombineElement;

uint32_t Combination(uint32_t, uint32_t);
uint32_t Factorial(uint32_t, uint32_t);

int MinCombinationNumber(AssembleRoot*, MergeMemoryUseNote*, int*, MergeMemoryUseNote**);

int ExceedMatchValue(CombineElement*, int);
void SetFitAssemble(CombineElement*, int, int);
void CombineElementNumber(CombineElement*);

void CombineElement2(AssembleCombine*, AssembleMatchLenInfo*);
void CombineElementN(AssembleCombine*, AssembleMatchLenInfo*, AssembleMatchLenInfo*);

#endif //UNTITLED8_COMBINATION_H
