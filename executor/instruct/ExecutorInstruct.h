//
// Created by abc on 20-5-2.
//

#ifndef UNTITLED5_EXECUTORINSTRUCT_H
#define UNTITLED5_EXECUTORINSTRUCT_H

#include "Instruct.h"

struct StealInstruct final : public Instruct{
    using Instruct::Instruct;
    explicit StealInstruct(InstructType instruct_type, BasicThread *thread, uint32_t steal_size, const std::thread::id &steal_id)
            : Instruct(instruct_type, thread), request_steal_size(steal_size), request_steal_id(steal_id), steal_util() {}
    ~StealInstruct() override = default;

    uint32_t classSize() const override { return sizeof(StealInstruct); }

    uint32_t request_steal_size;
    std::thread::id request_steal_id;
    UnLockQueue<std::shared_ptr<ExecutorNote>>::UnLockQueueUtil steal_util;
};

struct ScopeInstruct final : public Instruct{
    using Instruct::Instruct;
    ~ScopeInstruct() override = default;

    uint32_t classSize() const override { return sizeof(ScopeInstruct); }

    union {
        BasicThread     *manager;
        BasicExecutor   *pool;
    };
};

struct InterruptPolicyInstruct final : public Instruct{
    explicit InterruptPolicyInstruct(InstructType instruct_type, BasicThread *thread, InterruptPolicy interrupt_policy)
                                                          : Instruct(instruct_type, thread), policy(interrupt_policy) {}
    ~InterruptPolicyInstruct() override = default;

    uint32_t classSize() const override { return sizeof(InterruptPolicyInstruct); }

    InterruptPolicy policy;
};

struct UpdateNoteSizeInstruct : public Instruct{
    using Instruct::Instruct;
    ~UpdateNoteSizeInstruct() override = default;

    uint32_t classSize() const override { return sizeof(UpdateNoteSizeInstruct); }
};

struct TransferInstruct : public Instruct{
    using Instruct::Instruct;
    ~TransferInstruct() override = default;

    uint32_t classSize() const override { return sizeof(TransferInstruct); }

    UnLockQueue<std::shared_ptr<ExecutorNote>>::UnLockQueueUtil transfer_util;
};

struct ReleaseInstruct : public Instruct{
    using Instruct::Instruct;
    ~ReleaseInstruct() override = default;

    uint32_t classSize() const override { return sizeof(ReleaseInstruct); }
};

#endif //UNTITLED5_EXECUTORINSTRUCT_H
