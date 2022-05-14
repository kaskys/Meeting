//
// Created by abc on 19-10-12.
//
#include "Combination.h"

/**
 * 匹配合适的值
 * @param match_memory
 * @param use_array
 * @param size
 */
void MatchFitAssemble(PercolateMemory *pmemory, MergeMemoryUseNote *use_array, void(*percolate_down)(PercolateMemory*, MergeMemory*), int array_size){
    char *idle_info = NULL;

    /**
     * 循环匹配MergeMemory的idle_memory
     */
    for(;;){
        if(!(idle_info = pmemory->info_func(pmemory->merge_info)) || (pmemory->use_size <= 0)){
            break;
        }

        MergeMemoryUseNote *match_array[pmemory->use_size];
        memset(match_array, 0, sizeof(MergeMemoryUseNote*) * pmemory->use_size);

        MatchFitAssemble0(use_array, match_array, NULL, array_size, pmemory->len_func(idle_info), InitAssembleRootSkipSize);
        /**
         * 结合匹配的use_memory
         * 减少use_memory数量
         * 减少use_memory的MergeMemory的rank并增加MergeMemory的rank值即下滤MergeMemory
         */
        for(int i = 0; ;i++, pmemory->use_size--){
            if(match_array[i] == NULL){
                break;
            }

            //插入匹配的use_info
            if(pmemory->insert_func(pmemory->merge_info, (char*)match_array[i], match_array[i]->use_len)) {
                //设置该use_info为不可用并跳过该use_info
                match_array[i]->flag = NULL;
                match_array[i]->skip_note = NULL;

                //增加该use_info的MemoryBuffer的rank值
                match_array[i]->location_merge_memory->merge_memory->rank += match_array[i]->use_len;
                //减少该use_info的MemoryBuffer的note_size数量
                match_array[i]->location_merge_memory->merge_memory->note_size -= 1;

                //下滤merge_memory
                percolate_down(pmemory, match_array[i]->location_merge_memory->merge_memory);
            }
        }
    }

}

/**
 * 实现
 * @param use_array
 * @param match_array
 * @param pos_table
 * @param use_size
 * @param match_value
 * @param find_func
 */
void MatchFitAssemble0(MergeMemoryUseNote *use_array, MergeMemoryUseNote **match_array, PosTable *pos_table, int use_size, int match_value,
                       void (*find_func)(AssembleRoot*, PosTable*, int)){
    AssembleRoot fit_root = {.match_value = match_value, .rank_value = 0, .skip_value_pos = -1, .skip_lower_combine_pos = -1, .lower_value_pos = -1,
                             .pos_table = NULL, .use_array = use_array, .match_array = match_array};
    find_func(&fit_root, pos_table, use_size);
}

/**
 * 初始化AssembleRoot,并计算位置表
 * @param root
 * @param null_table 该函数不需要参数PosTable
 * @param use_size   use的数量
 */
void InitAssembleRootSkipSize(AssembleRoot *root, PosTable *null_table, int use_size){
    int skip_combine_value = 0;
    int pos_array[use_size * 2];
    MergeMemoryUseNote *use_note = NULL, *unote = NULL;
    PosTable pos_table;

    InitPosTable(root, &pos_table, pos_array, use_size);
    /*
     * 初始化AssembleRoot信息
     * 初始化位置表
     */
    for(int i = 0, pos = 0; i < use_size; i++){
        use_note = use_note ? (root->use_array + i) : root->use_array;

        /*
         * 判断use_note是否被结合或use_note的MergeMemory被选中为结合
         * 是=>则跳到skip_note,并跳过该范围的结合use_note
         * 否=>计算skip_value_pos、skip_lower_combine_pos、lower_value_pos
         */
        if(!use_note->flag || !(*use_note->flag)){
            if(!unote){
                unote = use_note;
            }

            if(use_note->skip_note){
                use_note = use_note->skip_note;
            }
            unote->skip_note = use_note;
            //跳到当前连续结合use_note的最后一个的位置
            i = (int)(use_note - root->use_array);
            continue;
        }

        unote = NULL;
        root->pos_table->use_pos[pos] = i;
        root->pos_table->ruse_pos[i] = pos;

        if(use_note->use_len <= root->match_value){
            root->lower_value_pos = (pos + 1);
        }else{
            break;
        }

        if(root->skip_value_pos < 0){
            if((root->rank_value + use_note->use_len) > root->match_value){
                root->skip_value_pos = pos;
            }else {
                root->rank_value += use_note->use_len;
            }
        }

        if((root->skip_value_pos >= 0) && (root->skip_lower_combine_pos < 0)){
            if((root->skip_lower_combine_pos < 0) && ((skip_combine_value + use_note->use_len) > root->match_value)){
                root->skip_lower_combine_pos = pos;
            }else {
                skip_combine_value += use_note->use_len;
            }
        }

        pos++;
    }
    /*
     * 如果从0到use_size的结合值小于match_value
     * 则令skip_value_pos = lower_value_pos
     */
    if(root->skip_value_pos < 0){
        root->skip_value_pos = root->lower_value_pos;
    }
    /*
     * 如果从skip_value_pos到skip_lower_combine_pos的结合值小于match_value
     * 则令skip_lower_combine_pos = lower_value_pos
     */
    if(root->skip_lower_combine_pos < 0){
        root->skip_lower_combine_pos = root->lower_value_pos;
    }
    FindFitAssemble(root, skip_combine_value);
}

/**
 * 初始化AssembleRoot,不需要计算位置表(同上)
 * @param root
 * @param pos_table
 * @param use_size
 */
void InitAssembleRootUsePos(AssembleRoot *root, PosTable *pos_table, int use_size){
    int skip_combine_value = 0;
    MergeMemoryUseNote *use_note = NULL;

    root->pos_table = pos_table;

    for(int i = 0; i < use_size; i++){
        use_note = root->use_array + root->pos_table->use_pos[i];

        if(use_note->use_len <= root->match_value){
            root->lower_value_pos = (i + 1);
        }else{
            break;
        }

        if(root->skip_value_pos < 0){
            if((root->rank_value + use_note->use_len) >= root->match_value){
                root->skip_value_pos = i;
            }else {
                root->rank_value += use_note->use_len;
            }
        }
        if(root->skip_value_pos >= 0){
            if((root->skip_lower_combine_pos < 0) && ((skip_combine_value + use_note->use_len) >= root->match_value)){
                root->skip_lower_combine_pos = i;
            }else {
                skip_combine_value += use_note->use_len;
            }
        }
    }
    if(root->skip_value_pos < 0){
        root->skip_value_pos = root->lower_value_pos;
    }
    if(root->skip_lower_combine_pos < 0){
        root->skip_lower_combine_pos = root->lower_value_pos;
    }

    FindFitAssemble(root, skip_combine_value);
}

/**
 * 赋值位置表
 * @param root
 * @param pos_table
 * @param pos_array
 * @param size
 */
void InitPosTable(AssembleRoot *root, PosTable *pos_table, int *pos_array, int size){
    memset(pos_array, 0, sizeof(*pos_array) * (size * 2));

    pos_table->use_pos = pos_array;
    pos_table->ruse_pos = (pos_array + size);

    root->pos_table = pos_table;
}

/**
 * 寻找合适的匹配值
 * @param root
 * @param combine_value
 */
void FindFitAssemble(AssembleRoot *root, int combine_value){
    if(root->rank_value > 0) {
        /*
         * 从0到skip_value_pos的结合值等于match_value
         */
        if(root->rank_value == root->match_value){
            for(int i = 0; i < root->skip_value_pos; i++){
                root->match_array[i] = root->use_array + (root->pos_table->use_pos[i]);
            }
            root->match_array[root->skip_value_pos] = NULL;
        }
        /*
         * lower_value_pos - 1的值等于match_value
         */
        else if((root->lower_value_pos >= 0) && (root->use_array + root->pos_table->use_pos[root->lower_value_pos - 1])->use_len == root->match_value) {
            root->match_array[0] = root->use_array + root->pos_table->use_pos[root->lower_value_pos - 1];
            root->match_array[1] = NULL;
        }
        /*
         * 从skip_value_pos到skip_lower_combine_pos的结合值等于match_value
         */
        else if(combine_value == root->match_value){
            for(int i = root->skip_value_pos; i < root->skip_lower_combine_pos; i++){
                root->match_array[i] = root->use_array + (root->pos_table->use_pos[root->skip_value_pos + i]);
            }
            root->match_array[root->skip_lower_combine_pos] = NULL;
        }
        /*
         * 实现函数
         */
        else{
            FindFitAssemble0(root);
        }
    }else{
        /*
         * 从0到use_size的值都小于match_value
         */
        root->match_array[0] = NULL;
    }
}

void FindFitAssemble0(AssembleRoot *root){
    //获取该算法认为的最小结合数量（时间与空间）
    int min_combine_size = MinCombinationNumber(root, root->use_array, root->pos_table->use_pos, root->match_array);

    AssembleMatchNote *max_rank = NULL, *min_size = NULL;
    AssembleMatchLenInfo *len_info = NULL;
    AssembleCombine assemble_combine = {.match_len = 0, .combine_len = 0, .bit_array = NULL, .root = root, .note_array = NULL};

    if(min_combine_size > 0){
        {
            /**
             * 构造初始化栈变量
             */
            InitAssembleCombine(&assemble_combine, root->skip_value_pos, root->lower_value_pos - root->skip_value_pos,
                                sizeof(AssembleMatchNote), min_combine_size);

            unsigned char bit_array[(assemble_combine.match_len + assemble_combine.combine_len) * assemble_combine.match_size];
            AssembleMatchNote note_array[assemble_combine.match_size];
            AssembleMatchLenInfo info_array[root->skip_lower_combine_pos - root->skip_value_pos];

            InitStackData(&assemble_combine, info_array, bit_array, note_array);

            /*
             * 循环元素个素结合匹配
             */
            for(int info_pos = 0, info_end = root->skip_lower_combine_pos - root->skip_value_pos; info_pos < info_end; info_pos++){
                len_info = info_array + info_pos;

                if(!info_pos){
                    //2个元素的结合匹配
                    CombineElement2(&assemble_combine, len_info);
                }else{
                    //2个以上的元素结合匹配
                    CombineElementN(&assemble_combine, len_info, info_array + (info_pos - 1));
                };

                /*
                 * 判断结合匹配的值是否等于match_note
                 */
                if(len_info->match_note){
                    SetBitRangeMatchNote(&assemble_combine, len_info->match_note);
                    break;
                }

                /*
                 * 判断最大匹配值是否等于match_note
                 */
                if(RankValue(len_info->max_rank_note) == root->match_value){
                    SetBitRangeMatchNote(&assemble_combine, len_info->max_rank_note);
                    break;
                }

                /*
                 * 判断最小匹配数是否等于match_note
                 */
                if(RankValue(len_info->min_size_note) == root->match_value){
                    SetBitRangeMatchNote(&assemble_combine, len_info->min_size_note);
                    break;
                }

                /*
                 * 比较并设置最大匹配值
                 */
                if(len_info->max_rank_note && (!max_rank || (RankValue(len_info->max_rank_note) > RankValue(max_rank)) ||
                  ((RankValue(len_info->max_rank_note) == RankValue(max_rank)) && (len_info->max_rank_note->use_size < max_rank->use_size)))){
                    max_rank = len_info->max_rank_note;
                }

                /*
                 * 比较并设置最小匹配数
                 */
                if(len_info->min_size_note && (!min_size || (len_info->min_size_note->use_size < min_size->use_size) ||
                  ((len_info->min_size_note->use_size == min_size->use_size) && (RankValue(len_info->min_size_note) > RankValue(min_size))))){
                    min_size = len_info->min_size_note;
                }

                //没有匹配数量,退出匹配循环
                if(assemble_combine.match_size <= 0){
                    break;
                }
            }

            /**
             * 设置最接近的匹配结合值
             */
            CompareAndSet(max_rank, min_size, len_info ? len_info->match_note : NULL, &assemble_combine);
        }
    }
    /*
     * 最小结合数量为0,则返回root的从0到skip_value_pos的接近值
     */
    else if(!min_combine_size){
        for(int i = 0; i < root->skip_value_pos; i++){
            root->match_array[i] = root->use_array + root->pos_table->use_pos[i];
        }
        root->match_array[root->skip_value_pos] = NULL;
    }
}

/**
 * 返回匹配结合值
 * @param match_note
 * @return
 */
int RankValue(AssembleMatchNote *match_note){
    return (match_note->crank_value + match_note->mrank_value);
}

/**
 * 初始化栈变量
 * @param assemble_combine
 * @param info_array
 * @param min_combine_size
 */
void InitStackData(AssembleCombine *assemble_combine, AssembleMatchLenInfo *info_array, unsigned char *bit_array, AssembleMatchNote *note_array){
    memset(bit_array, 0, (size_t)(assemble_combine->match_len + assemble_combine->combine_len) * assemble_combine->match_size);
    memset(note_array, 0, sizeof(AssembleMatchNote) * assemble_combine->match_size);
    memset(info_array, 0, sizeof(AssembleMatchLenInfo) * (assemble_combine->root->skip_lower_combine_pos - assemble_combine->root->skip_value_pos));

    assemble_combine->bit_array = bit_array;
    assemble_combine->note_array = note_array;
}

/**
 * 初始化AssembleCombine并计算匹配数量
 * @param assemble_combine
 * @param match_len
 * @param combine_len
 * @param note_len
 * @param min_combine_size
 */
void InitAssembleCombine(AssembleCombine *assemble_combine, int match_len, int combine_len, int note_len, int min_combine_size){
    note_len += (assemble_combine->match_len = (size_round_up(match_len) / 8));
    note_len += (assemble_combine->combine_len = (size_round_up(combine_len) / 8));

    if((note_len * min_combine_size) > DEFALUT_ASSEMBLE_COMBINE_STACK_LEN){
        assemble_combine->match_size = DEFALUT_ASSEMBLE_COMBINE_STACK_LEN / note_len;
    }else{
        assemble_combine->match_size = min_combine_size;
    }
}

/**
 * 初始化匹配Note
 * @param assemble_combine  结合信息
 * @param len_info          结合元素个数信息
 * @param combine_note      被结合的Note
 * @param combine_pos       匹配note的位置
 * @param combine_size      结合元素个数
 */
void InitMatchNote(AssembleCombine *assemble_combine, AssembleMatchLenInfo *len_info, AssembleMatchNote *combine_note, int combine_pos, int combine_size){
    int array_pos = (--assemble_combine->match_size);
    AssembleMatchNote *match_note = assemble_combine->note_array + array_pos;

    match_note->use_size = combine_size;
    match_note->combine_pos = combine_pos;
    match_note->next = NULL;
    /*
     * 设置匹配元素和结合元素的位置信息,并计算匹配值和结合值
     */
    SetMatchNoteBitRange(match_note, combine_note, assemble_combine, combine_pos, array_pos);

    /*
     * 计算结合元素个数信息的最大匹配值和最小匹配数
     */
    if(!len_info->fnote){
        len_info->max_rank_note = len_info->min_size_note = len_info->fnote = match_note;
    }else{
        assemble_combine->pnote->next = match_note;

        if((RankValue(match_note) > RankValue(len_info->max_rank_note)) ||
          ((RankValue(match_note) == RankValue(len_info->max_rank_note)) && (match_note->use_size < len_info->max_rank_note->use_size))){
            len_info->max_rank_note = match_note;
        }

        if((match_note->use_size < len_info->min_size_note->use_size) ||
          ((match_note->use_size == len_info->min_size_note->use_size) && (RankValue(match_note) > RankValue(len_info->min_size_note)))){
            len_info->min_size_note = match_note;
        }
    }
    //令assemble_combine的pnote指向当前的匹配note
    assemble_combine->pnote = match_note;
}

/**
 * 设置匹配元素和结合元素的位置信息,并计算匹配值和结合值
 * @param match_note        匹配Note
 * @param combine_note      被结合的Note
 * @param assemble_combine  结合信息
 * @param combine_pos       匹配Note的位置
 * @param array_pos         数组的使用位置
 */
void SetMatchNoteBitRange(AssembleMatchNote *match_note, AssembleMatchNote *combine_note, AssembleCombine *assemble_combine, int combine_pos, int array_pos){
    MergeMemoryUseNote *use_note = NULL;

    /*
     * 赋值结合值,获取匹配值和结合值的位值bit
     */
    match_note->crank_value = (combine_note ? combine_note->crank_value : 0);
    match_note->mrank_value = 0;
    match_note->match_bit = assemble_combine->bit_array + ((assemble_combine->match_len + assemble_combine->combine_len) * array_pos);
    match_note->combine_bit = assemble_combine->bit_array + ((assemble_combine->match_len + assemble_combine->combine_len) * array_pos + assemble_combine->match_len);

    /*
     * 循环计算匹配值并设置匹配节点位置
     */
    for(int i = 0; ; i++){
        if(!(use_note = (assemble_combine->root->match_array[i]))){
            break;
        }

        match_note->mrank_value += use_note->use_len;
        match_note->use_size++;

        SetMatchNoteBit(match_note->match_bit, assemble_combine->root->pos_table->ruse_pos[use_note - assemble_combine->root->use_array]);
    }

    /*
     * 拷贝被结合Note的结合节点位置
     */
    if(combine_note) {
        CopyCombineNoteBit(assemble_combine, combine_note, match_note->combine_bit);
    }
    /*
     * 设置当前结合点的信息
     */
    match_note->crank_value += (assemble_combine->root->use_array + assemble_combine->root->pos_table->use_pos[combine_pos])->use_len;
    SetMatchNoteBit(match_note->combine_bit , combine_pos - assemble_combine->root->skip_value_pos);
}

/**
 * 比较并设置最大匹配值或最小匹配数、一致匹配note
 *      匹配数和匹配值的一半比较？？？
 * @param max_rank
 * @param min_size
 * @param identical_note    一致匹配note
 * @param assemble_combine
 */
void CompareAndSet(AssembleMatchNote *max_rank, AssembleMatchNote *min_size, AssembleMatchNote *identical_note, AssembleCombine *assemble_combine){
    /**
     * 有则设置一致匹配note
     * 否则默认最大匹配值
     */
    if(max_rank || min_size || identical_note) {
        SetBitRangeMatchNote(assemble_combine, identical_note ? identical_note : max_rank);
    }else{
        assemble_combine->root->match_array[0] = NULL;
    }
}

/**
 * 设置匹配数组(从匹配和结合节点信息转换到对应的use_note,并设置到配置数组中)
 * @param assemble_combine
 * @param match_note
 */
void SetBitRangeMatchNote(AssembleCombine *assemble_combine, AssembleMatchNote *match_note){
    int match_pos = 0;

    SetBitMatchNote(assemble_combine->match_len, 0, match_note->match_bit, assemble_combine->root->use_array, assemble_combine->root->match_array,
                    assemble_combine->root->pos_table->use_pos, &match_pos);
    SetBitMatchNote(assemble_combine->combine_len, assemble_combine->root->skip_value_pos, match_note->combine_bit, assemble_combine->root->use_array, assemble_combine->root->match_array,
                    assemble_combine->root->pos_table->use_pos, &match_pos);

    assemble_combine->root->match_array[match_pos] = NULL;
}

/**
 * 设置匹配数组
 * @param len           bit长度
 * @param off_set       偏移值
 * @param bit           bit信息
 * @param use_array     use数组
 * @param match_array   匹配数组
 * @param use_pos       实际数组
 * @param pos           设置数量
 * @return
 */
int SetBitMatchNote(int len, int off_set, const unsigned char *bit, MergeMemoryUseNote *use_array, MergeMemoryUseNote **match_array, int *use_pos, int *pos){
#define BIT_SIZE    8
    unsigned char bit_value = 0;

    if(*bit) {
        for (int i = 0; i < len; i++) {
            bit_value = *(bit + i);
            for (int k = 0; k < BIT_SIZE; k++) {
                if (bit_value & (1 << (BIT_SIZE - k - 1))) {
                    match_array[(*pos)++] = use_array + use_pos[((i * BIT_SIZE) + k) + off_set];
                }
            }
        }
    }
}

void GetBitMatchNote(int len, int off_set, unsigned char *bit, MergeMemoryUseNote *use_array, int *use_pos){
#define BIT_SIZE    8
    unsigned char bit_value = 0;

    if(*bit) {
        for (int i = 0; i < len; i++) {
            bit_value = *(bit + i);
            for (int k = 0; k < BIT_SIZE; k++) {
                if (bit_value & (1 << (BIT_SIZE - k - 1))) {
                    printf(",%d", (use_array + use_pos[((i * BIT_SIZE) + k) + off_set])->use_len);
                }
            }
        }
    }
}

/**
 * 设置匹配或结合位置信息(bit表示)
 * @param match_bit
 * @param pos
 */
void SetMatchNoteBit(unsigned char *match_bit, int pos){
    unsigned char bit_value = (1 << (7 - (7 & pos)));
    match_bit += (pos / 8);

    (*match_bit) |= bit_value;
}

/**
 * 拷贝
 * @param assemble_combine
 * @param combine_note
 * @param combine_bit
 */
void CopyCombineNoteBit(AssembleCombine *assemble_combine, AssembleMatchNote *combine_note, unsigned char *combine_bit){
    memcpy(combine_bit, combine_note->combine_bit, (size_t)assemble_combine->combine_len);
}
