//
// Created by abc on 19-10-12.
//
#include "Combination.h"

/**
 * 初始化MergeMemory
 * @param merge_memory
 * @param buffer
 * @param len_func
 * @param info_func
 * @param insert_func
 */
void InitMergeMemoryAndConvertTable(MergeMemory *merge_memory, MergeLocationTable *convert_memory, char *buffer, int rank, int note_size){
    if(merge_memory && convert_memory && buffer){
        merge_memory->rank = rank;
        merge_memory->note_size = note_size;
        merge_memory->buffer = buffer;
        merge_memory->location_table = convert_memory;

        convert_memory->use_flag = 1;
        convert_memory->merge_memory = merge_memory;
    }else{
        memset(merge_memory, 0, sizeof(MergeMemory));
        memset(convert_memory, 0, sizeof(MergeLocationTable));
    }
}

/**
 * 二叉堆插入
 * @param merge_memory
 * @param convert_table
 * @param buffer
 * @param pos
 * @param rank
 * @param note_size
 */
void InsertSortMemory(MergeMemory *merge_memory, MergeLocationTable *convert_table, char *buffer, int pos, int rank, int note_size){
    InitMergeMemoryAndConvertTable(merge_memory + pos, convert_table + pos,  buffer, rank, note_size);
    if(pos > 0) {
        PercolateUp(merge_memory, merge_memory + pos);
    }
}

/**
 * 二分插入排序
 * @param merge_memory
 * @param use_info
 * @param use_array
 * @param flag
 * @param ulen
 * @param size
 * @return
 */
int InsertSortUse(MergeLocationTable *convert_memory, char *use_info, MergeMemoryUseNote *use_array, int ulen, int pos){
    int start = 0 , end = pos - 1,
        mid = 0, search_size = pos;

    for(;;){
        if((search_size <= 0) || (start > end)){
            mid = start;
            break;
        }

        mid = (start + end) / 2;
        if(use_array[mid].use_len > ulen){
            search_size -= (end - mid + 1);
            end = mid - 1;
        }else if(use_array[mid].use_len < ulen){
            search_size -= (mid - start + 1);
            start = mid + 1;
        }else{
            break;
        }
    }

    if(mid < pos){
        memmove(use_array + (mid + 1), use_array + mid, sizeof(MergeMemoryUseNote) * (pos - mid));
    }
    use_array[mid].use_len = ulen;
    use_array[mid].use_info = use_info;
    use_array[mid].uni_info = NULL;
    use_array[mid].flag = &convert_memory->use_flag;
    use_array[mid].location_merge_memory = convert_memory;
    use_array[mid].skip_note = NULL;

    return mid;
}


/**
 * 合并MergeMemory
 * @param memory_array
 * @param use_array
 * @param caller
 * @param response
 * @param memory_size
 * @param use_size
 */
void MergeMemoryFunc(MergeMemory *memory_array, MergeMemoryUseNote *use_array, char *merge_info,
                  int(*len_func)(char*),  char*(*info_func)(char*), int (*insert_func)(char*,char*,int),
                     void(*clear_merge_info)(char*, char*), int memory_size, int use_size){
    //memory_size - 1 => 因为memory_heap[0]不计算
    PercolateMemory percolate_memory = {.use_size = use_size, .heap_size = memory_size - 1, .merge_info = merge_info,
                 .len_func = len_func, .info_func = info_func, .insert_func = insert_func, .memory_heap = memory_array};

    for(;;){
        if((percolate_memory.heap_size <= 0) || (percolate_memory.use_size <= 0)){
            break;
        }
        clear_merge_info(merge_info, memory_array[0].buffer);
        SkipMergeUse(&percolate_memory);

        MatchFitAssemble(&percolate_memory, use_array, PercolateDown, use_size);

        ExtractMin(&percolate_memory);
    }
}

/**
 * 跳过合并MergeMemory的所有use_note
 * @param skip_memory
 */
void SkipMergeUse(PercolateMemory *percolate_memory){
    //设置合并的MergeMemory的use_note为不可选
    percolate_memory->memory_heap->location_table->use_flag = 0;
    //减少对应MergeMemory的use_note的数量
    percolate_memory->use_size -= percolate_memory->memory_heap->location_table->merge_memory->note_size;
    //percolate_memory->use_size_func(percolate_memory->memory_heap->buffer);
}

/**
 * 抽取最小rank的MergeMemory
 * @param memory_heap
 */
void ExtractMin(PercolateMemory *memory_heap){
#define BINARY_HEAP_MIN_VALUE   1

    SwapMergeMemory(memory_heap->memory_heap + 0, memory_heap->memory_heap + BINARY_HEAP_MIN_VALUE);
    SwapMergeMemory(memory_heap->memory_heap + BINARY_HEAP_MIN_VALUE, memory_heap->memory_heap + (memory_heap->heap_size--));

    PercolateDown(memory_heap, memory_heap->memory_heap + BINARY_HEAP_MIN_VALUE);
}

/**
 * 下滤MergeMemory
 * @param memory_heap
 * @param down_memory
 */
void PercolateDown(PercolateMemory *memory_heap, MergeMemory *down_memory){
    int hold = (int)(down_memory - memory_heap->memory_heap);
    MergeMemory swap_memory = memory_heap->memory_heap[hold];

    for(int pos = 0; (hold * 2) <= memory_heap->heap_size; hold = pos){
        pos = hold * 2;
        if((pos != memory_heap->heap_size) && (memory_heap->memory_heap[pos + 1].rank < memory_heap->memory_heap[pos].rank)){
            pos++;
        }

        if(memory_heap->memory_heap[pos].rank < swap_memory.rank){
            SwapMergeMemory(memory_heap->memory_heap + hold, memory_heap->memory_heap + pos);
        }else{
            break;
        }
    }

    SwapMergeMemory(memory_heap->memory_heap + hold, &swap_memory);
}

/**
 * 上滤MergeMemory
 * @param root_memory
 * @param up_memory
 */
void PercolateUp(MergeMemory *root_memory, MergeMemory *up_memory){
    int hold = (int)(up_memory - root_memory);
    MergeMemory swap_memory = root_memory[hold];

    for(int pos; hold > 0 ; hold /= 2){
        if(root_memory[hold].rank < root_memory[(pos = hold / 2)].rank){
            SwapMergeMemory(root_memory + hold, root_memory + pos);
        }else{
            break;
        }
    }

    SwapMergeMemory(root_memory + hold, &swap_memory);
}

/**
 * 交换MergeMemory
 * @param dsc
 * @param src
 */
void SwapMergeMemory(MergeMemory *dsc, MergeMemory *src){
    *dsc = *src;
    //src已经移动到dsc,新的dsc(旧src)的定位表指向src,所以需要重新指向dsc
    dsc->location_table->merge_memory = dsc;
}