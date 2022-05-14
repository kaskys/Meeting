//
// Created by abc on 21-2-2.
//

#include "AddressParameter.h"

//--------------------------------------------------------------------------------------------------------------------//

void RemoteNoteParameter::onNoteJoin() {
    recv_join_size++;
}

void RemoteNoteParameter::onNoteLink() {
    recv_link_size++;
}

void RemoteNoteParameter::onNoteSynchroSend() {
    send_synchro_size++;
}

void RemoteNoteParameter::onNoteSynchroRecv() {
    recv_synchro_size++;
}

void RemoteNoteParameter::onNoteSequence() {
    send_seqeuence_size++;
}

void RemoteNoteParameter::onNoteRtt(uint32_t *value, uint32_t size) {
    for(int i = 0; i < size; i++){
        note_rtt_value.push_back(value[i]);
    }
}

void RemoteNoteParameter::onNoteNormal() {
    note_normal_size++;
}

void RemoteNoteParameter::onNotePassive() {
    note_passive_size++;
}

void RemoteNoteParameter::onNoteInvalid() {
    note_invalid_size++;
}

uint32_t RemoteNoteParameter::onRttSize() {
    return static_cast<uint32_t>(note_rtt_value.size());
}

void RemoteNoteParameter::onData(ParameterRemote *parameter) {
    parameter->recv_join_size = recv_join_size;
    parameter->recv_link_size = recv_link_size;
    parameter->send_synchro_size = send_synchro_size;
    parameter->recv_synchro_size = recv_synchro_size;
    parameter->send_seqeuence_size = send_seqeuence_size;
    parameter->note_normal_size = note_normal_size;
    parameter->note_passive_size = note_passive_size;
    parameter->note_invalid_size = note_invalid_size;

    uint32_t pos = 0;
    for(auto begin = note_rtt_value.begin(), end = note_rtt_value.end(); ((begin != end) && (pos < parameter->value_size)); ++begin, pos++){
        parameter->note_rtt_value[pos] = *begin;
    }
}

void RemoteNoteParameter::onClear() {
    recv_join_size = 0; recv_link_size = 0;
    send_synchro_size = 0; recv_synchro_size = 0;
    send_seqeuence_size = 0;
    note_normal_size = 0; note_passive_size = 0; note_invalid_size = 0;
    note_rtt_value.clear();
}

//--------------------------------------------------------------------------------------------------------------------//

//void HostNoteParameter::onRemoteJoin(uint32_t apply, uint32_t determine) {
//    apply_join_size += apply;
//    determine_join_size.fetch_add(determine, std::memory_order_release);
//}
//
//void HostNoteParameter::onRemoteLink(uint32_t apply, uint32_t determine) {
//    apply_link_size += apply;
//    determine_link_size.fetch_add(determine, std::memory_order_release);
//}
//
//void HostNoteParameter::onRemoteSequence(uint32_t size) {
//    send_sequence_size.fetch_add(size, std::memory_order_release);
//}
//
//void HostNoteParameter::onRemoteExit() {
//    exit_size++;
//}
//
//void HostNoteParameter::onData(ParameterHost *parameter) {
//    parameter->apply_join_size = apply_join_size;
//    parameter->apply_link_size = apply_link_size;
//    parameter->apply_exit_size = apply_exit_size;
//    parameter->determine_join_size = determine_join_size.load(std::memory_order_consume);
//    parameter->determine_link_size = determine_link_size.load(std::memory_order_consume);
//    parameter->send_sequence_size = send_sequence_size.load(std::memory_order_consume);
//}
//
//void HostNoteParameter::onClear() {
//    apply_join_size = 0; apply_link_size = 0; apply_exit_size = 0;
//    determine_join_size.store(0); determine_join_size.store(0); send_sequence_size.store(0);
//}

//--------------------------------------------------------------------------------------------------------------------//

void AddressParameter::onData(ParameterAddress *parameter) {
    parameter->address_size = getSizeAddress();
    parameter->join_size = getJoinAddress();
    parameter->link_size = getLinkAddress();
    parameter->normal_size = getNormalAddress();
    parameter->transfer_size = getTransferAddress();
    parameter->passive_size = getPassiveAddress();
    parameter->exit_size = getExitAddress();
    parameter->unsupport_size = getUnSupportSize();
    parameter->invalid_size = getInvalidtSize();
}

void AddressParameter::onClear() {
    address_size.store(0); join_size.store(0); link_size.store(0);
    normal_size.store(0); transfer_size.store(0); passive_size.store(0);
    exit_size.store(0); unsupport_size.store(0); invalid_size.store(0);
}