//
// Created by abc on 20-4-24.
//

#ifndef UNTITLED5_LOOPWORDINSTRUCT_H
#define UNTITLED5_LOOPWORDINSTRUCT_H

#include "Instruct.h"

struct TransmitNoteInstruct final : public Instruct{
    explicit TransmitNoteInstruct(InstructType instruct_type, BasicThread *thread, uint32_t size)
            : Instruct(instruct_type, thread), socket_size(size), note_msg(nullptr), release_func(nullptr), transmit_func(nullptr) {}
    ~TransmitNoteInstruct() override = default;

    uint32_t classSize() const override { return sizeof(TransmitNoteInstruct); };

    std::atomic<uint32_t> socket_size;
    MsgHdr *note_msg;
    std::function<void()> release_func;
    std::function<void(TransmitThreadUtil*, MeetingAddressNote*, MsgHdr*, uint32_t)> transmit_func;
};

struct AddressJoinInstruct final : public ThreadInstruct{
    explicit AddressJoinInstruct(InstructType instruct_type, BasicThread *thread, ThreadType thread_type, MeetingAddressNote *note = nullptr)
            : ThreadInstruct(instruct_type, thread, thread_type), join_note(note), join_func(nullptr), note_func(nullptr) {}
    ~AddressJoinInstruct() override = default;

    uint32_t classSize() const override { return sizeof(AddressJoinInstruct); }

    MeetingAddressNote *join_note;
    std::promise<TransmitThreadUtil*> join_promise;
    std::function<void(TransmitThreadUtil*, uint32_t)> join_func;
    std::function<void(MeetingAddressNote*)> note_func;
};

struct AddressExitInstruct final : public Instruct{
    using Instruct::Instruct;
    explicit AddressExitInstruct(InstructType instruct_type, BasicThread *thread, MeetingAddressNote *note)
                           : Instruct(instruct_type, thread), exit_note(note), exit_func(nullptr), note_func(nullptr) {}
    ~AddressExitInstruct() override = default;

    uint32_t classSize() const override { return sizeof(AddressExitInstruct); }

    MeetingAddressNote *exit_note;
    std::function<void(uint32_t)> exit_func;
    std::function<void(MeetingAddressNote*)> note_func;
};

struct AddressTransferInstruct final : public Instruct{
    explicit AddressTransferInstruct(InstructType instruct_type, BasicThread *thread) : Instruct(instruct_type, thread) {
        transfer_iterator = transfer_address.end();
    }
    ~AddressTransferInstruct() override = default;

    uint32_t classSize() const override { return sizeof(AddressTransferInstruct); }

    std::list<MeetingAddressNote*> transfer_address;
    std::list<MeetingAddressNote*>::iterator transfer_iterator;
};

struct TransferCompleteInstruct final : public Instruct{
    explicit TransferCompleteInstruct(BasicThread *thread) : Instruct(INSTRUCT_ADDRESS_COMPLETE, thread) {}
    ~TransferCompleteInstruct() override = default;

    uint32_t classSize() const override { return sizeof(TransferCompleteInstruct); }
};

#endif //UNTITLED5_LOOPWORDINSTRUCT_H
