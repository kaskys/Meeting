//
// Created by abc on 20-4-23.
//

#ifndef UNTITLED5_SOCKETWORDINSRUCT_H
#define UNTITLED5_SOCKETWORDINSRUCT_H

#include "Instruct.h"

#define SOCKET_INSTRUCT_DEFAULT_SIZE    8   //关联SocketThread线程的SOCKET_PERMIT_DEFAULT_SIZE

struct AddressDoneInstruct final : public Instruct{
    using Instruct::Instruct;
    ~AddressDoneInstruct() override = default;

    uint32_t classSize() const override { return sizeof(AddressDoneInstruct); }
};

struct TransferSocketInstruct final : public Instruct{
    explicit TransferSocketInstruct(InstructType type, BasicThread *thread) : Instruct(type, thread), transfer_size(0) {
        std::fill(std::begin(transfer_note), std::end(transfer_note), nullptr);
    }
    ~TransferSocketInstruct() override = default;

    uint32_t classSize() const override { return sizeof(TransferSocketInstruct); }

    uint32_t transfer_size;
    MeetingAddressNote *transfer_note[SOCKET_INSTRUCT_DEFAULT_SIZE];
};

struct ExceptionSocketInstruct final : public Instruct{
    explicit ExceptionSocketInstruct(BasicThread *thread) : Instruct(EXCEPTION_SOCKET_RELEASE, thread), exception_promise() {}
    ~ExceptionSocketInstruct() override = default;

    uint32_t classSize() const override { return sizeof(ExceptionSocketInstruct); }

    std::promise<void> exception_promise;
};

struct ReleaseSocketInstruct final : public Instruct{
    using Instruct::Instruct;
    explicit ReleaseSocketInstruct(InstructType type, BasicThread *thread, int thread_size)
            : Instruct(type, thread), transfer_thread_size(thread_size), release_notes() {}
    ~ReleaseSocketInstruct() override = default;

    uint32_t classSize() const override { return sizeof(ReleaseSocketInstruct); }

    int transfer_thread_size;
    std::list<MeetingAddressNote*> release_notes;
};

struct StopSocketInstruct final : public Instruct{
    explicit StopSocketInstruct(InstructType type, BasicThread *thread, int size) : Instruct(type, thread), stop_size(size) {}
    ~StopSocketInstruct() override = default;

    uint32_t classSize() const override { return sizeof(StopSocketInstruct); }

    std::atomic_int stop_size;
    std::promise<void> stop_promise;
};

#endif //UNTITLED5_SOCKETWORDINSRUCT_H
