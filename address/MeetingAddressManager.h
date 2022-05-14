//
// Created by abc on 20-4-5.
//

#ifndef UNTITLED8_MEETINGADDRESSMANAGER_H
#define UNTITLED8_MEETINGADDRESSMANAGER_H

#include "MeetingAddressNote.h"

#define MEETING_ADDRESS_ERGODIC_ALL         0x03
#define MEETING_ADDRESS_ERGODIC_RUNNING     0x02
#define MEETING_ADDRESS_ERGODIC_AWAIT       0x01

using FixedFunc   = std::function<MeetingAddressNote*()>;
using RemoteFunc  = std::function<void(MeetingAddressNote*, MeetingAddressNote*)>;
using ErgodicFunc = std::function<void(MeetingAddressNote*)>;

class MeetingAddressBase{
public:
    MeetingAddressBase() = default;
    virtual ~MeetingAddressBase() = default;

    virtual void insertMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*) = 0;
    virtual void deleteMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, const RemoteFunc&) = 0;
    virtual void updateMeetingAddressLeaf(MeetingAddressNote*) = 0;
    virtual void initMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*) = 0;
    virtual void reuseMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, MeetingAddressNote*) = 0;
    virtual void unuseMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, MeetingAddressNote*) = 0;
    virtual void onRise(MeetingAddressNote*, MeetingAddressNote*, void(*)(MeetingAddressNote*)) = 0;
    virtual MeetingAddressNote* onRight(MeetingAddressNote*) = 0;
    virtual MeetingAddressNote* onLeft(MeetingAddressNote*) = 0;

    uint32_t getRunMeetingAddressSize() const { return note_size; }
    uint32_t getAwaitMeetingAddressSize() const { return (total_size - note_size); }
    uint32_t getAllMeetingAddressSize() const { return total_size; }

    static int getDiffOnFirstTrue(uint32_t, uint32_t, int, int end_off = ADDRESS_SIZE_MAX);
    static bool compareSockAddr(uint32_t addr1, uint32_t addr2) { return static_cast<bool>(addr1 ^ addr2); }
protected:
    virtual MeetingAddressNote* matchMeetingAddressNote(MeetingAddressNote*, in_addr) = 0;
    virtual void ergodicMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, const ErgodicFunc&, const ErgodicFunc&) = 0;
    void increaseMeetingAddress() { total_size++; }
    void reduceMeetingAddress() { note_size--; }
    void reuseMeetingAddress() { note_size++; }

    uint32_t note_size;
    uint32_t total_size;
};

class MeetingAddressModifier : virtual public MeetingAddressBase{
public:
    MeetingAddressModifier() = default;
    ~MeetingAddressModifier() override = default;

    void insertMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*) override;
    void deleteMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, const RemoteFunc&) override;
protected:
    void initNote(MeetingAddressNote*, int);
    void initLeaf(MeetingAddressNote*);
private:
    void pushAddressInNote(MeetingAddressNote*, MeetingAddressNote*) throw(PushFinal);
    void pushAddressInNote0(MeetingAddressNote*, PushInfo*) throw(PushFinal);
    void pushLeafToDiffNote(MeetingAddressNote*, PushInfo*, MeetingAddressNote**) throw(PushFinal);
    MeetingAddressNote* pushLeafToFixedNote(MeetingAddressNote*, PushInfo*, MeetingAddressNote**) throw(PushFinal);

    void transferMeetingAddressNote(MeetingAddressNote*, PushInfo*);
    int searchFixedPos(SearchInfo*);
    SearchInfo* initSearchInfo(SearchInfo*, MeetingAddressNote*);
    static int insertFixedNote(MeetingAddressNote*, MeetingAddressNote*, MeetingAddressNote**);

    static void onNoteIncrease(MeetingAddressNote *parent) { parent->man_size++; }
    static void onNoteReduce(MeetingAddressNote *parent) { parent->man_size--; }

    uint32_t note_position;
};

class MeetingAddressIterator : virtual public MeetingAddressBase{
public:
    MeetingAddressIterator() = default;
    ~MeetingAddressIterator() override = default;

    void updateMeetingAddressLeaf(MeetingAddressNote*) override;
    void initMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*) override;
    void reuseMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, MeetingAddressNote*) override;
    void unuseMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, MeetingAddressNote*) override;
    void onRise(MeetingAddressNote*, MeetingAddressNote*, void(*)(MeetingAddressNote*)) override;
    MeetingAddressNote* onRight(MeetingAddressNote*) override;
    MeetingAddressNote* onLeft(MeetingAddressNote*) override;
protected:
    MeetingAddressNote* matchMeetingAddressNote(MeetingAddressNote*, in_addr) override;
    void ergodicMeetingAddressNote(MeetingAddressNote*, MeetingAddressNote*, const ErgodicFunc&, const ErgodicFunc&) override;
private:
    void ergodicMeetingAddressNote0(MeetingAddressNote*, const ErgodicFunc&, const ErgodicFunc&);
};

class MeetingAddressManager final : private MeetingAddressModifier, private MeetingAddressIterator{
public:
    explicit MeetingAddressManager(MeetingAddressNote*, MeetingAddressNote*, const RemoteFunc&) throw(std::logic_error);
    ~MeetingAddressManager() override;

    MeetingAddressManager(const MeetingAddressManager&) = delete;
    MeetingAddressManager(MeetingAddressManager&&) = delete;

    MeetingAddressManager& operator=(const MeetingAddressManager&) = delete;
    MeetingAddressManager& operator=(MeetingAddressManager&&) = delete;

    void pushNote(MeetingAddressNote*, const FixedFunc&) throw(std::bad_alloc);
    void reuseNote(MeetingAddressNote*);
    void removeNote(MeetingAddressNote*);
    void ergodicNote(int, const ErgodicFunc&);
    MeetingAddressNote* matchNote(const sockaddr_in&);

    static sockaddr_in getSockAddrFromNote(MeetingAddressNote*);

    static void setNoteTransferPos(MeetingAddressNote *note, uint32_t transfer_pos) { note->man_transfer_position = transfer_pos; }
    static uint32_t getNoteTransferPos(MeetingAddressNote *note) { return note->man_transfer_position; }
    static void setNotePermitPos(MeetingAddressNote *note, uint32_t permit_pos) { note->man_permit = permit_pos; }
    static uint32_t getNotePermitPos(MeetingAddressNote *note) { return note->man_permit; }
    static uint32_t getNoteGlobalPos(MeetingAddressNote *note) { return note->man_global_position; }
    static void onNoteStatusInfo(MeetingAddressNote *note, const std::function<void(uint32_t, uint32_t)> &callback){
        callback(getNoteGlobalPos(note), getSockAddrFromNote(note).sin_addr.s_addr);
    }
private:
    MeetingAddressNote *leave_root_note;
    MeetingAddressNote *await_root_note;
    RemoteFunc note_remove_func;
};

#endif //UNTITLED8_MEETINGADDRESSMANAGER_H
