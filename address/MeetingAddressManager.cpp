//
// Created by abc on 20-4-5.
//
#include "MeetingAddressManager.h"

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------MeetingAddressBase--------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

int MeetingAddressBase::getDiffOnFirstTrue(uint32_t addr1, uint32_t addr2, int start_off, int end_off) {
    uint32_t diff = (addr1 ^ addr2) << start_off;

    if(diff){
        for(int i = 0, end = end_off - start_off; i < end; i++){
            if(diff & (1 << (end_off - 1 - i))){
                return (i + start_off);
            }
        }
    }

    return -1;
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------MeetingAddressModifier----------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

void MeetingAddressModifier::insertMeetingAddressNote(MeetingAddressNote *root_note, MeetingAddressNote *note_info) {
    try {
        //初始化leaf节点
        initLeaf(note_info);
        //插入节点
        pushAddressInNote(root_note, note_info);
    }catch (PushFinal &e){
        updateMeetingAddressLeaf(root_note);
    }
}

/**
 * 删除leaf节点
 * @param root_note     根节点(note的父类节点)
 * @param note          删除节点
 * @param rm            保留或删除
 */
void MeetingAddressModifier::deleteMeetingAddressNote(MeetingAddressNote *root_note, MeetingAddressNote *note_info, const RemoteFunc &callback) {
    MeetingAddressNote *parent = nullptr;
    MeetingAddressNote *child = nullptr, *brother_note = nullptr;

    if((parent = note_info->man_p)){
        if(parent->man_p){
            //parent是非root_note节点,所用删除需要连同parent一起移除,将parent的左或右子节点链接parent->man_p
            (brother_note = (parent->man_right == note_info) ? parent->man_left : parent->man_right)->man_p = parent->man_p;
            parent = (child = parent)->man_p;
        }else{
            //parent是root_note节点,该节点没有man_p,所用需要设置为null
            child = note_info;
            brother_note = nullptr;
        }

        //修改信息
        if(parent->man_right == child){
            parent->man_right = brother_note;
        }else{
            parent->man_left = brother_note;
        }

        //删除或保留
        if(callback){
            callback(note_info, child);
        }

        //减少运行的数量
        reduceMeetingAddress();
        //上升减少数量
        onRise(root_note, parent, onNoteReduce);
        //更新左右节点
        updateMeetingAddressLeaf(root_note);
    }
}

/**
 * 插入节点
 * @param root_note
 * @param push_note
 */
void MeetingAddressModifier::pushAddressInNote(MeetingAddressNote *root_note, MeetingAddressNote *note_info) throw(PushFinal) {
    PushInfo pinfo = PushInfo(INADDR_NONE, note_info);
    try {
        pushAddressInNote0(root_note, &pinfo);
    }catch (PushFinal &e) {
        //上升增加节点数量
        onRise(root_note, note_info, onNoteIncrease);
        throw;
    }
}

/**
 * 插入Leaf
 * 从偏差点往后寻找的点或创建Note节点插入
 * @param parent
 * @param push_info     插入信息
 * @param off_set       偏差点
 */
void MeetingAddressModifier::pushAddressInNote0(MeetingAddressNote *parent, PushInfo *push_info) throw(PushFinal) {
    for(;;){
        push_info->next_off = parent->man_off + 1;
        if(!(parent = pushLeafToFixedNote(parent, push_info, PushInfo::getAddrBit(push_info->note_info->man_key, parent->man_off) ? &parent->man_right : &parent->man_left))){
            break;
        }
    }
}

/**
 * 插入Leaf到固定的Parent的节点中
 * @param parent        父节点
 * @param push_info     插入信息
 * @param off_set       偏移点
 * @return
 */
MeetingAddressNote* MeetingAddressModifier::pushLeafToFixedNote(MeetingAddressNote *parent,  PushInfo *push_info, MeetingAddressNote **in_note) throw(PushFinal) {
    if(!(*in_note)){
        //parent的左或右子节点为空,成功插入
        (*in_note) = push_info->note_info;
        push_info->note_info->man_p = parent;
        push_info->transfer_note = false;
    }else{
        if((*in_note)->man_b && (*in_note)->man_off == push_info->next_off){
            //如果parent的左或右子节点的note节点且parent的偏移点是当前偏移点
            //增加parent的字节点数量
            //返回parent的左或右子节点
            return *in_note;
        }

        if(!(*in_note)->man_b){
            //parent的左或右子节点是leaf节点
            //这步骤是整个下沉最后到达点(只会执行一次),设置比较信息
            push_info->complate_addr = (*in_note)->man_key;
        }else{
            //parent的左或右子节点是note节点
            //递归调用,插入到parent的左或右节点里
            try {
                pushAddressInNote0(*in_note, push_info);
            } catch (PushFinal &e) {
                //这里表示调用pushAddressInNote0已经成功插入
                //增加parent的字节点数量
                //重新抛出
                throw;
            }
        }
        //对比插入
        pushLeafToDiffNote(parent, push_info, in_note);
    }

    //插入成功
    if(push_info->note_info->man_p){
        if(push_info->transfer_note) {
            //旋转parent的左或右子节点
            transferMeetingAddressNote(push_info->note_info->man_p, push_info);
        }
        //增加parent的字节点数量
        //onNoteIncrease(parent);
        //抛出异常,让上层栈调用
        throw PushFinal();
    }

    /*
     * 这里返回nullptr说明从parent->man_off + 1到ADDRESS_SIZE_MAX - 1都没有找到合适的插入位置
     */
    return nullptr;
}

/**
 * 将Leaf插入到合适的偏移点里
 * @param parent        父节点
 * @param push_info     插入信息
 */
void MeetingAddressModifier::pushLeafToDiffNote(MeetingAddressNote *parent, PushInfo *push_info, MeetingAddressNote **in_note) throw(PushFinal){
    /*
     * 从parent->man_off +1 <--> ADDRESS_SIZE_MAX - 1里获取push_info->ip_addr和push_info->complate_addr第一个不相同的bit
     */
    int off = getDiffOnFirstTrue(push_info->note_info->man_key, push_info->complate_addr, parent->man_off + 1);
    MeetingAddressNote *fixed_info = push_info->note_info->man_p;

    //是否存在不相同的bit
    if((off >= 0) && (off < ADDRESS_SIZE_MAX)) {
        //初始化Fixed节点
        initNote(fixed_info, off);

        //设置信息
        fixed_info->man_size = static_cast<uint32_t>(insertFixedNote(fixed_info, push_info->note_info, in_note));
        push_info->out_note = PushInfo::getAddrBit(push_info->note_info->man_key, off) ? &fixed_info->man_right : &fixed_info->man_left;
    }
}

/**
 * 转移leaf节点
 * @param parent
 * @param push_info
 */
void MeetingAddressModifier::transferMeetingAddressNote(MeetingAddressNote *parent, PushInfo *push_info) {
    bool link_direction = PushInfo::getAddrBit(push_info->note_info->man_key, parent->man_off);
    int leaf_pos = 0;
    MeetingAddressNote *transfer_note = (parent->man_right == (*push_info->out_note) ? parent->man_left : parent->man_right);

    if(transfer_note->man_b) {
        MeetingAddressNote *transfer_leafs[transfer_note->man_size];
        SearchInfo search_info = SearchInfo();

        std::function<void(MeetingAddressNote *)> leaf_func = [&](MeetingAddressNote *leaf) -> void {
            //判断leaf是否需要转移(左左或右右 => 左^左 = false)
            if (!(link_direction ^ PushInfo::getAddrBit(leaf->man_key, parent->man_off))) {
                transfer_leafs[leaf_pos++] = leaf;
            }
        };
        //遍历parent的所用节点
        ergodicMeetingAddressNote(transfer_note, nullptr, nullptr, leaf_func);

        for (int i = leaf_pos - 1; i >= 0; i--) {
            //获取leaf的parent节点
            transfer_note = (search_info.transfer_leaf = transfer_leafs[i])->man_p;

            //从树中删除leaf节点及其parent,但不会销毁该内存(需要转移)
            deleteMeetingAddressNote(parent, search_info.transfer_leaf, nullptr);

            if (!(*push_info->out_note)->man_b) {
                //该节点为leaf节点,只需要获取与该leaf节点的第一个不同bit的偏移点
                transfer_note->man_off = getDiffOnFirstTrue((*push_info->out_note)->man_key, search_info.transfer_leaf->man_key, parent->man_off + 1);

                search_info.out_note = push_info->out_note;
            } else {
                //该节点为note节点,需要往下(迭代)获取第一个不同bit的偏移点
                transfer_note->man_off = searchFixedPos(initSearchInfo(&search_info, parent));
            }
            //插入转移的note节点和leaf节点(-1是因为push_note的数量还没有增加)
            transfer_note->man_size = static_cast<uint32_t>(insertFixedNote(transfer_note, search_info.transfer_leaf, search_info.out_note) - 1);
            //增加节点数量
            onRise(parent, search_info.transfer_leaf, onNoteIncrease);
        }
    }
}

/**
 * 初始化SearchInfo信息
 * @param search_info
 * @param parent
 * @return
 */
struct SearchInfo* MeetingAddressModifier::initSearchInfo(SearchInfo *search_info, MeetingAddressNote *parent) {
    search_info->start_off = parent->man_off + 1;
    search_info->parent = *search_info->out_note;
    search_info->compare_key = onRight(search_info->parent);
    return search_info;
}

/**
 * 往偏移点插入note节点和leaf节点
 * @param fixed_note    note节点(已保存偏移点)
 * @param leaf          note节点
 * @param out_note      被插入的节点左或右子节
 */
int MeetingAddressModifier::insertFixedNote(MeetingAddressNote *fixed_note, MeetingAddressNote *leaf_note, MeetingAddressNote **out_note) {
    //获取节点信息
    MeetingAddressNote *old_leaf = *out_note;

    //被插入的节点左或右子节重新设置为插入的note节点
    //设置man_p信息
    ((*out_note) = fixed_note)->man_p = old_leaf->man_p;
    leaf_note->man_p = old_leaf->man_p = fixed_note;

    //设置note节点的左右子节点信息
    if(PushInfo::getAddrBit(leaf_note->man_key, fixed_note->man_off)){
        fixed_note->man_right = leaf_note;
        fixed_note->man_left = old_leaf;
    }else{
        fixed_note->man_right = old_leaf;
        fixed_note->man_left = leaf_note;
    };

    return old_leaf->man_b ? old_leaf->man_size : 1;
}

/**
 * 搜索插入note节点的偏移点
 * @param search_info
 * @return
 */
int MeetingAddressModifier::searchFixedPos(SearchInfo *search_info) {
    //获取第一个不同bit的偏移点(从start_off(parent->man_p->man_off + 1)到parent->man_off)
    int off = getDiffOnFirstTrue(search_info->transfer_leaf->man_key, search_info->compare_key->man_key, search_info->start_off, search_info->parent->man_off);
    if(off < 0){
        //没有(所有的bit相同),迭代往下搜索
        //判断往下是左或右子节点
        search_info->out_note = PushInfo::getAddrBit(search_info->transfer_leaf->man_key, search_info->parent->man_off + 1) ?
                                &search_info->parent->man_right : &search_info->parent->man_left;

        if((*search_info->out_note)->man_b){
            //note节点,迭代搜索
            off = searchFixedPos(initSearchInfo(search_info, search_info->parent));
        }else{
            //leaf节点,获取第一个不同bit的偏移点
            off = getDiffOnFirstTrue(search_info->transfer_leaf->man_key, search_info->compare_key->man_key, search_info->parent->man_off + 1);
        }
    }
    return off;
}

/**
 * 初始化note节点信息
 * @param note
 * @param off
 */
void MeetingAddressModifier::initNote(MeetingAddressNote *note_info, int off) {
    note_info->man_b = NOTE_NOTE;
    note_info->man_p = nullptr;
    note_info->man_size = 0;
    note_info->man_off = off;
    note_info->man_right = nullptr;
    note_info->man_left = nullptr;
}

/**
 * 初始化leaf节点信息
 * @param note
 * @param addr
 */
void MeetingAddressModifier::initLeaf(MeetingAddressNote *note_info) {
    note_info->man_b = NOTE_LEAF;
    note_info->man_p = nullptr;
    note_info->man_use = 1;
    note_info->man_global_position = note_position++;
}

//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------MeetingAddressIterator----------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 匹配MeetingAddressNote
 * @param root_note
 * @param addr
 * @return
 */
MeetingAddressNote* MeetingAddressIterator::matchMeetingAddressNote(MeetingAddressNote *root_note, in_addr addr) {
    uint32_t ip_addr = addr.s_addr;

    //先判断ip地址是否为无效地址
    if((ip_addr != INADDR_ANY) && (ip_addr != INADDR_NONE)){
        //由上往下,直到遇到leaf节点
        while(root_note && root_note->man_b){
            if(PushInfo::getAddrBit(ip_addr, root_note->man_off)){
                root_note = root_note->man_right;
            }else{
                root_note = root_note->man_left;
            }
        }

        //比较地址是否已经存在里面
        if(!root_note || compareSockAddr(root_note->man_key,ip_addr)){
            root_note = nullptr;
        }
        //存在,增加使用数量
        if(root_note){
            root_note->man_use++;
        }
    }else{
        root_note = nullptr;
    }

    return root_note;
}

/**
 * 遍历root_note的所用子节点(包含root_note)
 * @param root_note
 * @param note_func
 * @param leaf_func
 */
void MeetingAddressIterator::ergodicMeetingAddressNote(MeetingAddressNote *leave_root, MeetingAddressNote *await_root,
                                                       const ErgodicFunc &note_func, const ErgodicFunc &leaf_func) {
    ergodicMeetingAddressNote0(leave_root, note_func, leaf_func);
    ergodicMeetingAddressNote0(await_root, note_func, leaf_func);
}

void MeetingAddressIterator::ergodicMeetingAddressNote0(MeetingAddressNote *root_note,
                                                        const ErgodicFunc &note_func, const ErgodicFunc &leaf_func) {
    if(root_note->man_b){
        //note节点,先处理左子节点,再处理右子节点,最后回调自身节点
        if(root_note->man_left){
            ergodicMeetingAddressNote0(root_note->man_left, note_func, leaf_func);
        }
        if(root_note->man_right){
            ergodicMeetingAddressNote0(root_note->man_right, note_func, leaf_func);
        }

        if(note_func){ (note_func)(root_note); }
    }else{
        //leaf节点,回调该节点
        if(leaf_func){ (leaf_func)(root_note); }
    }
}

/**
 * 上滤
 * 从note节点往上,直到没有man_p或等于parent节点
 * @param parent    截至节点
 * @param note      开始节点
 * @param rfunc     回调函数
 */
void MeetingAddressIterator::onRise(MeetingAddressNote *parent, MeetingAddressNote *note, void (*rfunc)(MeetingAddressNote*)) {
    if(!note->man_b){
        note = note->man_p;
    }

    for(;; note = note->man_p){
        if(!note){ break; }
        if(rfunc){ rfunc(note); }
        if(note == parent){ break; }
    }
}

/**
 * 更新节点信息
 * @param root_note
 * @param update_note
 * @param update_flags
 */
void MeetingAddressIterator::updateMeetingAddressLeaf(MeetingAddressNote*) {
//    left_leaf = onLeft(root_note);
//    right_leaf = onRight(root_note);
}

void MeetingAddressIterator::initMeetingAddressNote(MeetingAddressNote *root, MeetingAddressNote *init_info) {
    insertMeetingAddressNote(root, init_info);
    increaseMeetingAddress();
}

void MeetingAddressIterator::reuseMeetingAddressNote(MeetingAddressNote *leave_root, MeetingAddressNote *await_root, MeetingAddressNote *reuse_note) {
    deleteMeetingAddressNote(await_root, reuse_note, nullptr);
    insertMeetingAddressNote(leave_root, reuse_note);
    reuseMeetingAddress();
}

void MeetingAddressIterator::unuseMeetingAddressNote(MeetingAddressNote *leave_root, MeetingAddressNote *await_root, MeetingAddressNote *unuse_note) {
    deleteMeetingAddressNote(leave_root, unuse_note, nullptr);
    insertMeetingAddressNote(await_root, unuse_note);
    reduceMeetingAddress();
}

/**
 * 往右节点遍历下去,直到遇到leaf节点为止
 * @param note
 * @return
 */
MeetingAddressNote* MeetingAddressIterator::onRight(MeetingAddressNote *note) {
    while(note && note->man_b){ note = note->man_right; }
    return note;
}

/**
 * 往左节点遍历下去,直到遇到leaf节点为止
 * @param note
 * @return
 */
MeetingAddressNote* MeetingAddressIterator::onLeft(MeetingAddressNote *note) {
    while(note && note->man_b){ note = note->man_left; }
    return note;
}
//--------------------------------------------------------------------------------------------------------------------//
//------------------------------------------MeetingAddressManager-----------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

MeetingAddressManager::MeetingAddressManager(MeetingAddressNote *leave_root, MeetingAddressNote *await_root, const RemoteFunc &func) throw(std::logic_error)
        : MeetingAddressBase(), leave_root_note(leave_root), await_root_note(await_root), note_remove_func(func){
    if(!leave_root_note || !await_root_note || !note_remove_func){
        throw std::logic_error("!root_note,初始化MeetingAddressManager失败！");
    }
}

MeetingAddressManager::~MeetingAddressManager() {
    note_remove_func(nullptr, leave_root_note);
    note_remove_func(nullptr, await_root_note);
}

void MeetingAddressManager::pushNote(MeetingAddressNote *note_info, const FixedFunc &fixed_func) throw(std::bad_alloc) {
    if(!note_info || !(note_info->man_p = fixed_func())){ throw std::bad_alloc(); }
    initMeetingAddressNote(await_root_note, note_info);
}

void MeetingAddressManager::reuseNote(MeetingAddressNote *note_info) {
    if(note_info){
        reuseMeetingAddressNote(leave_root_note, await_root_note, note_info);
    }
}

void MeetingAddressManager::removeNote(MeetingAddressNote *note_info) {
    if(note_info){
        unuseMeetingAddressNote(leave_root_note, await_root_note, note_info);
    }
}

void MeetingAddressManager::ergodicNote(int ergodic_flags, const ErgodicFunc &callback) {
    std::function<void()> ergodic_func;

    if(ergodic_flags & MEETING_ADDRESS_ERGODIC_ALL){
        ergodic_func = [&]() -> void {
            MeetingAddressIterator::ergodicMeetingAddressNote(leave_root_note, await_root_note, nullptr, callback);
        };
    }else if(ergodic_flags & MEETING_ADDRESS_ERGODIC_RUNNING){
        ergodic_func = [&]() -> void {
            MeetingAddressIterator::ergodicMeetingAddressNote(leave_root_note, nullptr, nullptr, callback);
        };
    }else if(ergodic_flags & MEETING_ADDRESS_ERGODIC_AWAIT){
        ergodic_func = [&]() -> void {
            MeetingAddressIterator::ergodicMeetingAddressNote(nullptr, await_root_note, nullptr, callback);
        };
    }else {
        ergodic_func = nullptr;
    }

    if(ergodic_func){
        ergodic_func();
    }
}

MeetingAddressNote* MeetingAddressManager::matchNote(const sockaddr_in &addr) {
    MeetingAddressNote *match_note = nullptr;

    if(!(match_note = matchMeetingAddressNote(leave_root_note, addr.sin_addr))){
        match_note = matchMeetingAddressNote(await_root_note, addr.sin_addr);
    }

    return match_note;
}

sockaddr_in MeetingAddressManager::getSockAddrFromNote(MeetingAddressNote *note) {
    sockaddr_in addr = sockaddr_in();
    memset(&addr, 0, sizeof(sockaddr_in));

    if(note && !note->man_b){
        addr.sin_family = AF_INET;
        addr.sin_port = note->man_port;
        addr.sin_addr.s_addr = note->man_key;
    }

    return addr;
}


