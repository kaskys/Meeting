//
// Created by abc on 21-2-2.
//

#include "../AddressLayer.h"

/**
 * 工具类：远程端的创建
 * @param layer         地址层
 * @param msg           创建信息
 * @param status_util   状态工具
 */
void AddressStatusUtil::onNoteCreate(AddressLayer *layer, MsgHdr *msg, AddressStatusUtil *status_util){
    layer->onNoteCreate(msg, nullptr, status_util);
}

/**
 * 工具类：远程端的初始化
 * @param layer         地址层
 * @param msg           初始化信息
 * @param status_util   状态工具
 */
void AddressStatusUtil::onNoteInit(AddressLayer *layer, MsgHdr *msg, AddressStatusUtil *status_util) {
    layer->onNoteInit(msg, status_util);
}

/**
 * 工具类：远程端的析初始化
 * @param layer         地址层
 * @param msg           析初始化信息
 * @param note_info     远程端
 */
void AddressStatusUtil::onNoteUninit(AddressLayer *layer, MsgHdr *msg, AddressNoteInfo *note_info, AddressStatusUtil*){
    layer->onNoteUninit(msg, note_info, nullptr);
}

bool AddressStatusUtil::onNoteRepeat(AddressLayer *layer, MsgHdr *msg, AddressNoteInfo *note_info, AddressStatusUtil *status_util) {
    return status_util->onInputRepeat(msg, layer, note_info);
}

/**
 * 设置远程端的状态
 * @param info      远程端
 * @param status    状态
 */
void AddressStatusUtil::onStatus(AddressNoteInfo *info, StatusNote status) {
    info->setStatusNote(status);
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 主机端的创建函数
 * @param layer     地址层
 * @param info      主机端
 * @param port      端口
 * @param key       ip地址
 */
void HostStatusUtil::onInputCreate(AddressLayer *layer, AddressNoteInfo *info, uint16_t port, uint32_t key, uint32_t) {
    auto host_info = dynamic_cast<HostNoteInfo*>(info);

    //调用初始化函数
    HostNoteInfo::onNoteInit(host_info, port, key);
    //设置状态
    onStatus(info, status);
    //初始化参数
    layer->layer_parameter.onInitAddress();
}

/**
 * 主机端的初始化函数
 * @param msg       初始化信息
 * @param layer     地址层
 * @param info      主机端
 */
void HostStatusUtil::onInputInit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info, uint16_t) {
    //设置加入参数
    layer->layer_parameter.onJoinAddress();
    //调用主机端初始化函数
    layer->onHostNoteInit(msg, dynamic_cast<HostNoteInfo*>(info));
}

/**
 * 主机端析初始化函数
 * @param msg       析初始化信息
 * @param layer     地址层
 * @param info      主机端
 */
void HostStatusUtil::onInputUninit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    //设置退出参数
    layer->layer_parameter.onExitAddress();
    //调用主机端析初始化函数
    layer->onHostNoteUninit(msg, dynamic_cast<HostNoteInfo*>(info));
}

/**
 * 主机端加入函数
 * @param layer 地址层
 */
void HostStatusUtil::onInputJoin(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //主机端不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_JOIN);
}

/**
 * 主机端重复加入函数
 * @param layer 地址层
 * @return
 */
bool HostStatusUtil::onInputRepeat(MsgHdr *, AddressLayer *layer, AddressNoteInfo *) {
    //主机端不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_JOIN); return false;
}

/**
 * 主机端退出函数
 * @param msg       退出信息
 * @param layer     地址层
 * @param info      主机端
 */
void HostStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    //设置退出状态
    onStatus(info, NOTE_STATUS_EXIT);
    //清空链接信息
    dynamic_cast<HostNoteInfo*>(info)->clearLinkInfo();

    //调用地址层退出函数
    layer->onNoteInputExit0(msg, info);
}

/**
 * 主机端链接函数
 * @param layer     地址层
 */
void HostStatusUtil::onInputLink(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //主机端不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_LINK);
}

/**
 * 主机端同步函数
 * @param msg           同步信息
 * @param layer         地址层
 * @param note_info     主机端
 */
void HostStatusUtil::onInputSynchro(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) {
    //调用地址层同步函数
    layer->onNoteInputSynchro0(msg, note_info);
}

/**
 * 主机端正常数据函数
 * @param layer     地址层
 */
void HostStatusUtil::onInputNormal(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //主机端不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_NORMAL);
}

/**
 * 主机端被动退出函数
 * @param layer     地址层
 */
void HostStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //主机端不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_EXIT_PASSIVE);
}

/**
 * 主机端不支持请求函数
 * @param layer         地址层
 * @param unstatus      不支持请求
 */
void HostStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo* , StatusNote unstatus) {
    //设置不支持参数
    layer->layer_parameter.onUnSupport();
    //调用地址层响应错误处理
    layer->onReplyError(layer->host_note, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 远程端创建函数
 * @param layer         地址层
 * @param info          远程端
 * @param port          端口号
 * @param key           ip地址
 * @param link_number   同步次数
 */
void RemoteStatusUtil::onInputCreate(AddressLayer *layer, AddressNoteInfo *info, uint16_t port, uint32_t key, uint32_t link_number) {
    auto remote_info = dynamic_cast<RemoteNoteInfo*>(info);
    //调用远程端初始化函数
    RemoteNoteInfo::onNoteInit(remote_info, port, key, link_number);
    //设置状态
    onStatus(info, status);
    //设置初始化参数
    layer->layer_parameter.onInitAddress();
}

/**
 * 远程端初始化函数
 * @param msg       初始化信息
 * @param layer     地址层
 * @param info      远程端
 * @param port      端口号
 */
void RemoteStatusUtil::onInputInit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info, uint16_t port) {
    auto remote_info = dynamic_cast<RemoteNoteInfo*>(info);
    //调用远程端重新加入函数
    remote_info->onNoteReInit(port);
    //设置加入参数
    layer->layer_parameter.onJoinAddress();
    //调用地址层初始化函数
    layer->onRemoteNoteInit(msg, remote_info);
}

/**
 * 远程端析初始化函数
 * @param msg       析初始化信息
 * @param layer     地址层
 * @param info      远程端
 */
void RemoteStatusUtil::onInputUninit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    //设置退出参数
    layer->layer_parameter.onExitAddress();
    //调用地址层析初始化函数
    layer->onRemoteNoteUninit(msg, dynamic_cast<RemoteNoteInfo*>(info));
}

/**
 * 远程端加入函数
 * @param msg       加入信息
 * @param layer     地址层
 * @param info      远程端
 */
void RemoteStatusUtil::onInputJoin(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    //设置加入状态
    onStatus(info, NOTE_STATUS_JOIN);
    //调用地址层加入函数
    layer->onNoteInputJoin0(msg, info);
}

/**
 * 远程端重覆加入函数
 * @param layer     地址层
 * @return
 */
bool RemoteStatusUtil::onInputRepeat(MsgHdr *, AddressLayer *layer, AddressNoteInfo *) {
    //远程端不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_JOIN); return false;
}

/**
 * 远程端退出函数
 * @param msg       退出信息
 * @param layer     地址层
 * @param info      远程端
 */
void RemoteStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    //设置退出状态
    onStatus(info, NOTE_STATUS_EXIT);

    {
        auto remote_info = dynamic_cast<RemoteNoteInfo*>(info);
        //清空黑名单信息
        remote_info->clrBlackNote();
        //清空类型过滤信息
        remote_info->clear();
        //清空链接信息
        remote_info->clearLinkInfo();
        //清空定时器信息
        remote_info->clearTimerInfo();
    }

    //调用地址层退出函数
    layer->onNoteInputExit0(msg, info);
}

/**
 * 远程端链接函数
 * @param layer 地址层
 */
void RemoteStatusUtil::onInputLink(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //远程不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_LINK);
}

/**
 * 远程端同步函数
 * @param layer 地址层
 */
void RemoteStatusUtil::onInputSynchro(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //远程不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_LINK);
}

/**
 * 远程端正常数据函数
 * @param layer 地址层
 */
void RemoteStatusUtil::onInputNormal(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //远程不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_NORMAL);
}

/**
 * 远程端被动退出函数
 * @param layer 地址层
 */
void RemoteStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    //远程不支持请求
    onInputUnSupport(layer, nullptr, NOTE_STATUS_EXIT_PASSIVE);
}

/**
 * 远程端不支持函数
 * @param layer     地址层
 * @param unstatus  不支持请求
 */
void RemoteStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo*, StatusNote unstatus) {
    //设置不支持参数
    layer->layer_parameter.onUnSupport();
    //调用地址层响应错误处理
    layer->onReplyError(layer->host_note, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

/*
 * MasterAddressLayer(被动)  -> 重复、延迟或远程端重新加入请求(控制层已处理：onNoteVerify)
 * ServantAddressLayer(主动) -> 重复加入响应 -> 抛弃、无效
 */
void JoinStatusUtil::onInputJoin(MsgHdr *, AddressLayer *, AddressNoteInfo *) {
    //不需要处理
}

/*
 * MasterAddressLayer(被动)  -> 有效的的链接请求（控制层已经过滤无效的链接请求）
 * ServantAddressLayer(主动) -> 重复、延迟的响应（控制层已处理）
 */
void JoinStatusUtil::onInputLink(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_LINK);
    dynamic_cast<RemoteNoteInfo*>(info)->clearTimerInfo();

    layer->layer_parameter.onLinkAddress();
    layer->onNoteInputLink0(msg, info);
}

void JoinStatusUtil::onInputSynchro(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

/*
 * MasterAddressLayer(被动)  -> 重新加入后延迟的到达正常数据 -> 抛弃、无效
 * ServantAddressLayer(主动) -> 主机响应重新加入请求后的延迟到达正常数据 -> 抛弃、无效
 */
void JoinStatusUtil::onInputNormal(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

/*
 * 不支持操作
 * MasterAddressLayer(主动)  -> 不用调用该函数(除非出错)
 * ServantAddressLayer(主动) -> 不用调用该函数(除非出错)
 */
void JoinStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, layer->host_note, NOTE_STATUS_EXIT_PASSIVE);
}

void JoinStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_EXIT);
    layer->onNoteInputExit0(msg, info);
}

void JoinStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo *info, StatusNote unstatus) {
    layer->layer_parameter.onUnSupport();
    layer->onReplyError(info, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

/*
 * MasterAddressLayer(被动)  -> 判断是否有效的加入请求(远程端重新加入) -> 控制层已处理：有效输入
 * ServantAddressLayer(主动) -> 重复的延迟加入响应(因为已经响应了链接) -> 无效输入
 */
void LinkStatusUtil::onInputJoin(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    if(layer->isNoteRepeatJoin(msg, info)) {
        info->clearTimerInfo();
        info->clearLinkInfo();

        onStatus(info, NOTE_STATUS_JOIN);

        layer->layer_parameter.onJoinAddress();
        layer->onNoteInputJoin0(msg, info);
    }else{
        layer->onNoteInvalid(info);
    }
}

/*
 * 不需要处理
 * MasterAddressLayer(被动)  -> 重复或延迟链接请求 -> 抛弃、无效数据
 * ServantAddressLayer(主动) -> 重复或延迟链接响应 -> 抛弃
 */
void LinkStatusUtil::onInputLink(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

void LinkStatusUtil::onInputSynchro(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInputSynchro0(msg, info);
}

void LinkStatusUtil::onInputSequence(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_SEQUENCE);
    layer->onNoteInputSequence0(msg, info);
}

/*
 * 不需要处理
 * MasterAddressLayer(被动)  -> 重新加入后延迟的到达正常数据或提前发送的正常数据 -> 抛弃、无效
 * ServantAddressLayer(主动) -> 主机响应重新加入请求及链接请求后的延迟到达正常数据或提取发送的正常数据 -> 抛弃、无效
 */
void LinkStatusUtil::onInputNormal(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info){
    layer->onNoteInvalid(info);
}

/*
 * 不支持操作
 * MasterAddressLayer(主动)  -> 不用调用该函数(除非出错)
 * ServantAddressLayer(主动) -> 不用调用该函数(除非出错)
 */
void LinkStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, layer->host_note, NOTE_STATUS_EXIT_PASSIVE);
}

void LinkStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_EXIT);
    layer->onNoteInputExit0(msg, info);
}

void LinkStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo *info , StatusNote unstatus) {
    layer->layer_parameter.onUnSupport();
    layer->onReplyError(info, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

/*
 * MasterAddressLayer(被动)  -> 判断是否有效的加入请求(远程端重新加入) -> 控制层已处理：有效输入
 * ServantAddressLayer(主动) -> 延迟的请求响应 -> 无效输入
 */
void SequenceStatusUtil::onInputJoin(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    if(layer->isNoteRepeatJoin(msg, info)) {
        info->clearTimerInfo();
        info->clearLinkInfo();

        onStatus(info, NOTE_STATUS_JOIN);
        layer->layer_parameter.onJoinAddress();
        layer->onNoteInputJoin0(msg, info);
    }else{
        layer->onNoteInvalid(info);
    }
}

/*
 * 不需要处理
 * MasterAddressLayer(被动)  -> 延迟的链接请求 -> 抛弃、无效数据
 * ServantAddressLayer(主动) -> 延迟的链接响应 -> 抛弃、无效数据
 */
void SequenceStatusUtil::onInputLink(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

void SequenceStatusUtil::onInputSynchro(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

void SequenceStatusUtil::onInputNormal(MsgHdr*, AddressLayer *layer, AddressNoteInfo *info) {
    dynamic_cast<RemoteNoteInfo*>(info)->clearTimerInfo();
    onStatus(info, NOTE_STATUS_NORMAL);
    layer->layer_parameter.onNormalAddress();
}

/*
 * 不支持操作
 * MasterAddressLayer(主动)  -> 不用调用该函数(除非出错)
 * ServantAddressLayer(主动) -> 不用调用该函数(除非出错)
 */
void SequenceStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, layer->host_note, NOTE_STATUS_EXIT_PASSIVE);
}

void SequenceStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_EXIT);
    layer->onNoteInputExit0(msg, info);
}

void SequenceStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo *info , StatusNote unstatus) {
    layer->layer_parameter.onUnSupport();
    layer->onReplyError(info, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

/*
 * MasterAddressLayer(被动)  -> 判断是否有效的加入请求(远程端重新加入) -> 控制层已处理：有效输入
 * ServantAddressLayer(主动) -> 延迟的加入响应 -> 无效输入
 */
void NormalStatusUtil::onInputJoin(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    if(layer->isNoteRepeatJoin(msg, info)){
        info->clearLinkInfo();

        onStatus(info, NOTE_STATUS_JOIN);
        layer->layer_parameter.onJoinAddress();
        layer->onNoteInputJoin0(msg, info);
    }else {
        layer->onNoteInvalid(info);
    }
}

/*
 * MasterAddressLayer(被动)  -> 判断是否重新链接请求（远程端对本机被动处理） -> 控制层已处理：有效输入
 * ServantAddressLayer(主动) -> 延迟的链接响应 -> 控制层已处理：无效输入（输入不会调用该函数,但程序本身可能会执行该函数,但会执行onNoteInvalid函数）
 */
void NormalStatusUtil::onInputLink(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    if(layer->isNoteRepeatLink(msg, info)) {
        //跳过加入请求,不需要更新sequence
        info->clearLinkInfo();
        layer->layer_parameter.onLinkAddress();
        onStatus(info, NOTE_STATUS_LINK);
        layer->onNoteInputLink0(msg, info);
    }else{
        layer->onNoteInvalid(info);
    }
}

void NormalStatusUtil::onInputSynchro(MsgHdr *, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

/*
 * 不需要处理
 * MasterAddressLayer(被动)  -> 控制层已经在解析master、shared、response前已经更新sequence
 * ServantAddressLayer(主动) -> 控制层已经在解析master、shared、response前已经更新sequence
 */
void NormalStatusUtil::onInputNormal(MsgHdr*, AddressLayer*, AddressNoteInfo*) {}

void NormalStatusUtil::onInputPassive(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    //由自己调用被动处理,不需要更新sequence
    onStatus(info, NOTE_STATUS_EXIT_PASSIVE);
    layer->layer_parameter.onPassiveAddress();
    layer->onNoteInputPassive0(msg, info);
}

void NormalStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_EXIT);
    layer->onNoteInputExit0(msg, info);
}

void NormalStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo *info , StatusNote unstatus) {
    layer->layer_parameter.onUnSupport();
    layer->onReplyError(info, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

/*
 * MasterAddressLayer(被动)  -> 判断是否有效的加入请求(远程端重新加入) -> 是：处理加入请求，否：无效请求
 * ServantAddressLayer(主动) -> 被动处理（重新链接）或 延迟响应加入 -> 延迟由控制从处理：无效输入，被动处理：有效出入
 */
void PassiveStatusUtil::onInputJoin(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    info->clearTimerInfo();
    info->clearLinkInfo();

    onStatus(info, NOTE_STATUS_JOIN);
    layer->layer_parameter.onJoinAddress();
    layer->onNoteInputJoin0(msg, info);
}

/*
 * MasterAddressLayer(被动)  -> 判断是否新链接请求（远程端对本机被动处理） -> 是:处理链接请求，否:无效请求
 * ServantAddressLayer(主动) -> 只会处于被动状态调用onInputJoin函数 -> 输入不会调用该函数,但程序本身可能会执行该函数,但会执行onNoteInvalid函数
 */
void PassiveStatusUtil::onInputLink(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    if(layer->isNoteRepeatLink(msg, info)) {
        //跳过加入请求,不需要更新sequence
        info->clearTimerInfo();
        info->clearLinkInfo();

        onStatus(info, NOTE_STATUS_LINK);
        layer->layer_parameter.onLinkAddress();
        layer->onNoteInputLink0(msg, info);
    }else{
        layer->onNoteInvalid(info);
    }
}

void PassiveStatusUtil::onInputSynchro(MsgHdr *, AddressLayer *layer, AddressNoteInfo *info) {
    layer->onNoteInvalid(info);
}

/*
 * MasterAddressLayer(被动)  -> 恢复正常状态
 * ServantAddressLayer(主动) -> 只会处于被动状态调用onInputJoin函数
 */
void PassiveStatusUtil::onInputNormal(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    dynamic_cast<RemoteNoteInfo*>(info)->clearTimerInfo();
    onStatus(info, NOTE_STATUS_NORMAL);
    layer->layer_parameter.onNormalAddress();
    layer->onNoteInputNormal0(msg, info);
}

/*
 * 不支持操作
 * MasterAddressLayer(主动)  -> 不用调用该函数(除非出错)
 * ServantAddressLayer(主动) -> 不用调用该函数(除非出错)
 */
void PassiveStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, layer->host_note, NOTE_STATUS_EXIT_PASSIVE);
}

void PassiveStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *info) {
    onStatus(info, NOTE_STATUS_EXIT);
    layer->onNoteInputExit0(msg, info);
}

void PassiveStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo *info, StatusNote unstatus) {
    layer->layer_parameter.onUnSupport();
    layer->onReplyError(info, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}

//--------------------------------------------------------------------------------------------------------------------//

void ExitStatusUtil::onInputExit(MsgHdr *msg, AddressLayer *layer, AddressNoteInfo *note_info) {
    onStatus(note_info, note_info->onNoteAttributeStatus());

    onNoteUninit(layer, msg, note_info, nullptr);
}

void ExitStatusUtil::onInputJoin(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, nullptr, NOTE_STATUS_JOIN);
}

bool ExitStatusUtil::onInputRepeat(MsgHdr*, AddressLayer*, AddressNoteInfo*) {
    return true;
}

void ExitStatusUtil::onInputLink(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, nullptr, NOTE_STATUS_LINK);
}

void ExitStatusUtil::onInputSynchro(MsgHdr *, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, nullptr, NOTE_STATUS_LINK);
}

void ExitStatusUtil::onInputNormal(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, nullptr, NOTE_STATUS_NORMAL);
}

void ExitStatusUtil::onInputPassive(MsgHdr*, AddressLayer *layer, AddressNoteInfo*) {
    onInputUnSupport(layer, nullptr, NOTE_STATUS_EXIT_PASSIVE);
}

void ExitStatusUtil::onInputUnSupport(AddressLayer *layer, AddressNoteInfo*, StatusNote unstatus) {
    layer->layer_parameter.onUnSupport();
    layer->onReplyError(layer->host_note, NOTE_ERROR_TYPE_UNSUPPORT, unstatus);
}