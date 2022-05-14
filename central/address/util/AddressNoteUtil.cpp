//
// Created by abc on 21-3-9.
//

#include "../../BasicControl.h"

const int prime_value[PRIME_SIZE] = {13, 47, 61, 89, 107, 149, 191, 223};

/**
 * 构造原始密钥
 * @return  密钥
 */
PasswordKey KeyUtil::generateSourceKey() {
    //设置源数值
    int64_t source_value = prime_value[std::chrono::steady_clock::now().time_since_epoch().count()];
    //构造空密钥
    PasswordKey source_key{0, 0, 0, 0};

    //设置匹配区
    source_key.match_area = *reinterpret_cast<unsigned char*>(&source_key);
    //设置补码区
    source_key.decode_area = static_cast<unsigned char>(source_value >> (source_key.match_area & (PRIME_SIZE - 1)));
    //设置解密区
    source_key.decrypt_area = (~source_key.match_area) ^ source_key.decode_area;

    //返回原始密钥
    return source_key;
}

/**
 * 编码密钥
 * @param source_key    原始密钥
 * @param code_size     编码数量
 * @return              编码密钥
 */
PasswordKey KeyUtil::codeKey(const PasswordKey &source_key, uint32_t code_size) {
    //构造编码密钥
    PasswordKey code_key{source_key.match_area, 0, 0,
                         static_cast<unsigned char>(prime_value[std::chrono::steady_clock::now().time_since_epoch().count() & (PRIME_SIZE - 1)])};

    //设置补码区
    code_key.decode_area = code_key.amount_area ^ source_key.decode_area;
    //设置解密区
    code_key.decrypt_area = keyAlgorithm(code_key.amount_area, source_key.decrypt_area);
    //设置数量
    code_key.amount_area = static_cast<unsigned char>(code_size);

    return code_key;
}

/**
 * 解码密钥
 * @param code_key      原始密钥
 * @param source_key    解码密钥
 * @return
 */
bool KeyUtil::decodeKey(const PasswordKey &code_key, const PasswordKey &source_key) {
    //匹配区是否相同
    if(code_key.match_area != source_key.match_area){
        return false;
    }
    //是否超过数量
    if(code_key.amount_area > source_key.amount_area){
        return false;
    }

    //解码解密区和补码区
    return (keyAlgorithm(code_key.decode_area ^ source_key.decode_area, source_key.decrypt_area) == code_key.decrypt_area);
}

unsigned char KeyUtil::keyAlgorithm(unsigned char key_value, unsigned char decrypt_value) {
    unsigned char high_bit = (key_value >> 4), lower_bit = (key_value & static_cast<unsigned char>(((1 << 4) - 1)));
    return decrypt_value & (high_bit + lower_bit);
}

//--------------------------------------------------------------------------------------------------------------------//

void AddressRunInfo::onLinkRunNote(RemoteNoteInfo *note_info) {
    note_info->next_remote_note = run_note_info; run_note_info->prev_remote_note = note_info;
    run_note_info = note_info; run_note_size++;
}

void AddressRunInfo::unLinkRunNote(RemoteNoteInfo *note_info) {
    if(note_info->next_remote_note){
        note_info->next_remote_note->prev_remote_note = note_info->prev_remote_note;
    }

    if(note_info->prev_remote_note){
        note_info->prev_remote_note->next_remote_note = note_info->next_remote_note;
    }else{
        run_note_info = note_info->next_remote_note;
    }

    run_note_size--;
}

void AddressRunInfo::onRunNoteInfo(const std::function<void(MeetingAddressNote *, uint32_t)> &note_func) {
    RemoteNoteInfo *note_info = run_note_info;
    for(uint32_t pos = 0;;){
        if(!note_info || (pos >= run_note_size)){
            break;
        }
        note_func(note_info, pos++);
        note_info = note_info->next_remote_note;
    }
}

//--------------------------------------------------------------------------------------------------------------------//

std::atomic<uint32_t> AddressNoteInfo::note_sequence_generator{1};

void AddressNoteInfo::onInit() {
#define NOTE_INFO_INIT_VALUE    1
    note_position = note_sequence_generator.fetch_add(NOTE_INFO_INIT_VALUE);
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 远程端处理加入
 * @param link_time     加入定时器触发时间
 * @param timeout       加入定时器超时时间
 */
void RemoteNoteInfo::onNoteJoin(uint32_t link_time, uint32_t timeout) {
    //设置时间参数
    remote_link_info.onJoin(link_time, timeout);
    //设置加入参数
    remote_parameter.onNoteJoin();
}

/**
 * 远程端处理链接
 * @param passive_time  链接定时器超时时间
 * @param sequence      链接序号
 */
void RemoteNoteInfo::onNoteLink(uint32_t passive_time, uint32_t sequence){
    //设置时间参数
    remote_link_info.onLink(passive_time);
    //设置序号参数
    remote_link_info.onNormal(sequence);
    //设置链接参数
    remote_parameter.onNoteLink();
}

/**
 * 远程源处理同步请求
 */
void RemoteNoteInfo::onNoteSynchro() {
    //设置同步发送参数
    remote_parameter.onNoteSynchroSend();
}

/**
 * 远程端处理同步响应
 * @param sequence          同步序号
 * @param sequence_data     同步数据
 * @param layer             地址层
 * @param synchro_func      链接回调函数
 */
void RemoteNoteInfo::onNoteSynchro(uint32_t sequence, SequenceData *sequence_data, AddressLayer *layer, LinkCallbackFunc synchro_func) {
    //设置序号参数
    remote_link_info.onNormal(sequence);
    //设置同步接收参数
    remote_parameter.onNoteSynchroRecv();

    //判断远端端的延迟时间（RTT）是否符号要求
    if(((remote_link_info.linked_number <= 0) || ((2 * onNoteRtt()) > sequence_data->link_rtt))
       && NetWordMaxSinglePing(sequence_data->link_rtt) && remote_link_info.onSynchro(sequence_data->link_rtt)
       && NetWordMaxAveragePing(onNoteRtt())){
        //远程端多次RTT符号要求,调用回调函数
        (*synchro_func)(layer, this);
    }
}

/**
 * 远程端处理序号
 * @param data  序号数据
 */
void RemoteNoteInfo::onNoteSequence(const SequenceData &data) {
    //设置序号数据参数（RTT、帧数量、延迟时间、序号）
    remote_link_info.onSequence(data.link_rtt, data.link_frame, data.link_delay,  data.serial_number);
    //设置序号参数
    remote_parameter.onNoteSequence();
}

/**
 * 远程端处理正常数据
 * @param sequence  序号
 */
void RemoteNoteInfo::onNoteNormal(uint32_t sequence) {
    //设置序号参数
    remote_link_info.onNormal(sequence);
    //设置正常参数
    remote_parameter.onNoteNormal();
}

/**
 * 远程端处理被动退出
 */
void RemoteNoteInfo::onNotePassive() {
    //设置被动退出参数
    remote_parameter.onNotePassive();
}

/**
 * 远程端处理退出
 */
void RemoteNoteInfo::onNoteExit() {
    //设置退出时间参数
    remote_link_info.onExit();
}

/**
 * 拷贝远程参数
 * @param parameter
 */
void RemoteNoteInfo::onData(ParameterRemote *parameter) {
    //拷贝ip地址和端口号
    onNoteAddress([&](uint32_t port, uint32_t key) -> void {
        parameter->note_port = port; parameter->note_addr = key;
    });
    //拷贝参数
    remote_link_info.onData(parameter);
    //拷贝参数
    remote_parameter.onData(parameter);
}

/**
 * 远程端处理无效请求
 */
void RemoteNoteInfo::onNoteInvalid() {
    //设置无效参数
    remote_parameter.onNoteInvalid();
}

/**
 * 获取远程端链接数据（该数据远程端不会存储,由主机响应加入请求时发送给远程端）
 * @return  空链接数据
 */
LinkData RemoteNoteInfo::onNoteLinkData() {
    return LinkData();
}

/**
 * 获取远程端序号数据
 * @return  远程端的rtt数据
 */
SequenceData RemoteNoteInfo::onNoteSequenceData() {
    return SequenceData{onNoteRtt(), 0, 0, 0};
}

/**
 * 判断远程端是否关联线程工具类（线程工具类可以单方面取消关联远程端）
 * @return
 */
bool RemoteNoteInfo::isCorrelateThread() {
    return correlate_thread && correlate_thread->isThreadCorrelateRemote(this);
}

/**
 * 重新初始化远程端数据（端口号）（退出后重新加入）
 * @param port
 */
void RemoteNoteInfo::onNoteReInit(uint16_t port) {
    //设置端口号
    this->man_port = port;
    //清空参数
    this->remote_parameter.onClear();
    //清空运行参数
    this->remote_link_info.onClear();
}

/**
 * 初始化远程端数据（第一次加入）
 * @param note_info     远程端
 * @param port          端口号
 * @param key           ip地址
 * @param link_number   需要同步次数
 */
void RemoteNoteInfo::onNoteInit(RemoteNoteInfo *note_info, uint16_t port, uint32_t key, uint32_t link_number) {
    //构造远程端
    new (reinterpret_cast<void*>(note_info)) RemoteNoteInfo(link_number);
    //设置ip地址
    note_info->man_key = key;
    //设置端口号
    note_info->man_port = port;
    //调用远程端初始化函数
    note_info->onInit();
}

/**
 * 链接远程端（运行）
 * @param run_info      运行远程端信息
 * @param note_info     运行远程端
 */
void RemoteNoteInfo::onLinkRunNote(AddressRunInfo *run_info, RemoteNoteInfo *note_info) {
    run_info->onLinkRunNote(note_info);
}

/**
 * 链接远程端（退出）
 * @param run_info      远程远程端信息
 * @param note_info     退出远程端
 */
void RemoteNoteInfo::unLinkRunNote(AddressRunInfo *run_info, RemoteNoteInfo *note_info) {
    run_info->unLinkRunNote(note_info);
}

//--------------------------------------------------------------------------------------------------------------------//

/**
 * 主机端处理加入
 * @param link_time 链接触发时间
 * @param timeout   链接超时时间
 */
void HostNoteInfo::onNoteJoin(uint32_t link_time, uint32_t timeout) {
    //设置加入参数
    host_link_info.onJoin(link_time, timeout);
}

/**
 * 主机端处理连续
 * @param passive_time  超时时间
 * @param sequence      序号
 */
void HostNoteInfo::onNoteLink(uint32_t passive_time, uint32_t) {
    //设置链接参数
    host_link_info.onLink(passive_time);
}

/**
 * 主机端处理同步
 * @param sequence  序号
 */
void HostNoteInfo::onNoteSynchro(uint32_t) {
    //不需要处理
}

/**
 * 主机端处理序号
 * @param data  序号数据
 */
void HostNoteInfo::onNoteSequence(const SequenceData &data) {
    //设置序号参数（rtt、帧数量、延迟时间、序号）
    host_link_info.onSequence(data.link_rtt, data.link_frame, data.link_delay, data.serial_number);
}

/**
 * 主机端处理正常数据
 * @param sequence  序号
 */
void HostNoteInfo::onNoteNormal(uint32_t) {
    //不需要处理
}

/**
 * 主机端处理被动退出
 */
void HostNoteInfo::onNotePassive() {
    //不需要处理
}

/**
 * 主机端处理退出（用户点击退出）
 */
void HostNoteInfo::onNoteExit() {
    //设置退出参数
    host_link_info.onExit();
}

/**
 * 拷贝主机端参数
 * @param parameter
 */
void HostNoteInfo::onData(ParameterHost *parameter) {
    //主机端ip地址和端口号
    onNoteAddress([&](uint32_t port, uint32_t key) -> void {
        parameter->note_port = port;parameter->note_addr = key;
    });
    //拷贝参数
    host_link_info.onData(parameter);
}

/**
 * 主机端无效请求
 */
void HostNoteInfo::onNoteInvalid() {
    //不需要处理
}

/**
 * 获取链接数据
 * @return
 */
LinkData HostNoteInfo::onNoteLinkData() {
    //链接触发时间、链接超时时间
    return LinkData{host_link_info.link_time, host_link_info.link_timeout};
}

/**
 * 获取序号数据
 * @return
 */
SequenceData HostNoteInfo::onNoteSequenceData() {
    //rtt、帧数量、延迟时间
    return SequenceData{host_link_info.link_rtt, host_link_info.link_frame, host_link_info.link_delay, 0};
}

/**
 * 初始化主机端
 * @param note_info 主机端
 * @param port      端口号
 * @param key       ip地址
 */
void HostNoteInfo::onNoteInit(HostNoteInfo *note_info, uint16_t port, uint32_t key) {
    //构造主机端
    new (reinterpret_cast<void*>(note_info)) HostNoteInfo();
    //设置ip地址
    note_info->man_key = key;
    //设置端口号
    note_info->man_port = port;
    //调用主机端初始化函数
    note_info->onInit();
    //清空参数
    note_info->host_link_info.onClear();
}
