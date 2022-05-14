//
// Created by abc on 19-10-21.
//
#include "Combination.h"

/**
 * 组合数,从总数m从抽取元素n的组合数量 ==> C(n,m)
 * @param number    元素个数
 * @param total     总数
 * @return
 */
uint32_t Combination(uint32_t number, uint32_t total){
    uint32_t dis = total - number;
    return (dis > number) ? (Factorial(dis + 1, total) / Factorial(1, number)) : (Factorial(number + 1, total) / Factorial(1, dis));
}

/**
 * 阶剩 ==> begin * begin + 1 * begin + 2 * begin + 3 * ... * rank
 * @param begin 开始值
 * @param rank  结束值
 * @return
 */
uint32_t Factorial(uint32_t begin, uint32_t rank){
    uint32_t res = 1;
    if((rank > 0) && (begin > 0) && (rank >= begin)){
        for(; begin <= rank; begin++){
            res *= begin;
        }
    }
    return res;
}

/**
 * 获取skip_value_pos到lower_value_pos的组合结果小于match_value的最小组合数
 * @param use_array
 * @param use_pos
 * @param match_array
 * @return
 */
int MinCombinationNumber(AssembleRoot *root, MergeMemoryUseNote *use_array, int *use_pos, MergeMemoryUseNote **match_array){
    CombineElement combine_element = { .min_number = 0,.begin_combine_pos = root->skip_lower_combine_pos,
                                       .use_pos = use_pos, .root = root, .use_array = use_array, .match_array = match_array};
    CombineElementNumber(&combine_element);
    return combine_element.min_number;
}

/**
 * 移动begin指向Note的use_len >= value
 * @param combine_element
 * @param value
 * @return
 */
int ExceedMatchValue(CombineElement *combine_element, int value){
    for(; combine_element->begin_combine_pos < combine_element->root->lower_value_pos; combine_element->begin_combine_pos++){
        if((combine_element->use_array + combine_element->use_pos[combine_element->begin_combine_pos])->use_len >= value){
            return 0;
        }
    }
    return 1;
}

/**
 * 设置匹配数组(从匹配和结合节点信息转换到对应的use_note,并设置到配置数组中)(*Combination)
 * @param combine_element
 * @param combine_element_size
 * @param combine_value
 */
void SetFitAssemble(CombineElement *combine_element, int combine_element_size, int combine_value){
    for(int i = 0; i < combine_element_size; i++){
        combine_element->match_array[i] = (combine_element->use_array + combine_element->use_pos[combine_element->root->skip_value_pos + i]);
        combine_value -= (combine_element->use_array + combine_element->use_pos[combine_element->root->skip_value_pos + i])->use_len;
    }

    if(combine_value){
        combine_element->match_array[combine_element_size++] = (combine_element->use_array + combine_element->use_pos[combine_element->begin_combine_pos]);
    }

    combine_element->match_array[combine_element_size] = NULL;
    combine_element->min_number = 0;
}

/**
 * 从组合数skip_value_pos到skip_lower_combine_pos获取结果小于match_value的组合数
 * 从skip_value_pos到lower_value_pos,设len = n
 * 从skip_value_pos到skip_lower_combine_pos的结合数大于match_value,设len = x,
 * 从skip_lower_combine_pos到lower_value_pos,设len = y
 * ==> n = x + y;
 *      最大组合数 ==> C(1, n) + C(2, n) + C(3, n) + ... + C(x - 1, n)
 * @param root
 * @param combine_element
 */
void CombineElementNumber(CombineElement *combine_element){
    int combine_value = 0, element_pos = 0, element_size = 0;

    /**
     * 如果lower_value_pos之前,skip_value_pos之后的元素之和小于match_value
     * 则需要全部元素的组合数之和 --> (2^n - 1)
     */
    if(combine_element->root->skip_lower_combine_pos == combine_element->root->lower_value_pos){
        combine_element->min_number = (int)pow(2, combine_element->root->lower_value_pos - combine_element->root->skip_value_pos) - 1;
        return;
    }

    /**
     * 跳过最后的一个元素,并计算元素个数及skip_lower_combine_pos - 1的元素的位置
     * 因为最后一个元素没有和skip_lower_combine_pos之后的元素能匹配,所以需要跳过最后一个元素
     */
    for(int i = combine_element->root->skip_value_pos; i < (combine_element->root->skip_lower_combine_pos - 1); i++, element_size++){
        combine_value += (combine_element->use_array + combine_element->use_pos[(element_pos = i)])->use_len;
    }

    /**
     * 计算当前结合元素个数的begin_combine_pos的元素位置信息,并计算当前元素个数的最小结合数量
     */
    for(; element_pos >= combine_element->root->skip_value_pos;
          element_pos--, element_size--, combine_value -= (combine_element->use_array + combine_element->use_pos[element_pos])->use_len){
        //匹配结合值大于等于匹配值的位置
        if(ExceedMatchValue(combine_element, combine_element->root->match_value - combine_value)){
            break;
        }

        //结合值满足匹配值,所以需要提前退出
        if((combine_value == combine_element->root->match_value) ||
          ((combine_value + (combine_element->use_array + combine_element->use_pos[combine_element->begin_combine_pos])->use_len) == combine_element->root->match_value)){
            SetFitAssemble(combine_element, element_size, combine_value);
            combine_element->min_number = -1;
            goto end;
        }

        //计算begin_combine_pos的结合数量
        combine_element->min_number += Combination(element_size + 1, combine_element->begin_combine_pos - combine_element->root->skip_value_pos);
    }

    /**
     * 因为当前元素的所有组合都符合match_value,所以只需增加当前算数的组合数即可
     */
    for(; element_pos >= combine_element->root->skip_value_pos;
          element_pos--, element_size--, combine_value -= (combine_element->use_array + combine_element->use_pos[element_pos])->use_len){
        if(combine_value == combine_element->root->match_value){
            SetFitAssemble(combine_element, element_size, combine_value);
            combine_element->min_number = -1;
            goto end;
        }

        /**
         * += C(当前组合元素个素, 全部组合元素个素)
         */
        combine_element->min_number += Combination(element_size + 1, combine_element->root->lower_value_pos - combine_element->root->skip_value_pos);
    }
    /**
     * 计算只有一个元素的次数
     */
    combine_element->min_number += (combine_element->root->lower_value_pos - combine_element->root->skip_value_pos);
end:
    return;
}

/**
 * 2个元素个数的结合匹配
 * @param assemble_combine
 * @param len_info
 */
void CombineElement2(AssembleCombine *assemble_combine, AssembleMatchLenInfo *len_info){
    for(int i = assemble_combine->root->skip_value_pos; i < assemble_combine->root->lower_value_pos; i++){
        //匹配
        MatchFitAssemble0(assemble_combine->root->use_array, assemble_combine->root->match_array, assemble_combine->root->pos_table, assemble_combine->root->skip_value_pos,
                          assemble_combine->root->match_value - (assemble_combine->root->use_array + assemble_combine->root->pos_table->use_pos[i])->use_len, InitAssembleRootUsePos);
        //初始化
        InitMatchNote(assemble_combine, len_info, NULL, i, 1);

        //判断
        if(RankValue(assemble_combine->pnote) == assemble_combine->root->match_value){
            len_info->match_note = assemble_combine->pnote;
            break;
        }
        //没有匹配数量,提前退出
        if(assemble_combine->match_size <= 0){
            break;
        }
    }
}

/**
 * 2个以上元素个数的结合匹配
 * @param assemble_combine
 * @param len_info
 * @param combine_info
 */
void CombineElementN(AssembleCombine *assemble_combine, AssembleMatchLenInfo *len_info, AssembleMatchLenInfo *combine_info){
    int combine_value = 0, combine_size = 0;
    AssembleMatchNote *combine_note = combine_info->fnote;

    combine_size = combine_note->combine_pos - assemble_combine->root->skip_value_pos + 1;
    /**
     * 循环结合Note
     */
    while(combine_note){

        for(int i = combine_note->combine_pos + 1; i < assemble_combine->root->lower_value_pos; i++){
            if((combine_value = (combine_note->crank_value + (assemble_combine->root->use_array + assemble_combine->root->pos_table->use_pos[i])->use_len))
                <= assemble_combine->root->match_value){
                //匹配
                MatchFitAssemble0(assemble_combine->root->use_array, assemble_combine->root->match_array, assemble_combine->root->pos_table, assemble_combine->root->skip_value_pos,
                                  assemble_combine->root->match_value - combine_value, InitAssembleRootUsePos);
                //初始化
                InitMatchNote(assemble_combine, len_info, combine_note, i, combine_size);

                //判断
                if(RankValue(assemble_combine->pnote) == assemble_combine->root->match_value){
                    len_info->match_note = assemble_combine->pnote;
                    return;
                }
                //没有匹配数量,提前退出
                if(assemble_combine->match_size <= 0){
                    return;
                }
            }else{
                break;
            }
        }

        combine_note = combine_note->next;
    }
}
