#ifndef MEETINGCORE_H
#define MEETINGCORE_H

#include <central/centralization/MasterCentralization.h>
#include <central/centralization/ServantCentralization.h>

class MasterCentralization;
class ServantCentralization;
struct MsgHdr;
struct AddressInitInfo;
struct TimeInitInfo;
struct DisplayInitInfo;

class MeetingCore;

enum MeetingCoreMode{
    CLIENT_MODE,
    SERVER_MODE,
    FAILI_MODE
};

class MeetingCoreHolder{
    friend class MeetingCore;
public:
    bool isLaunch() const;
    MeetingCoreMode onCoreMode() const;
    void onCoreMode(MeetingCoreMode);

    MsgHdr* onInitLaunch(const AddressInitInfo&, const TimeInitInfo&, const DisplayInitInfo&);
    void launchControl(MsgHdr*);
    void stopControl(MsgHdr*);

    void agreeNoteIndicator(uint32_t);
    void completeMediaBuffer(const char*, uint32_t);

    void inputVideoFrame(uint32_t, unsigned char*, const std::function<void()>&);
    void inputAudioFrame(uint32_t, char*);
private:
    explicit MeetingCoreHolder(MeetingCore *c) : core(c) {}
    ~MeetingCoreHolder() = default;

    MeetingCore *core;
};

class MeetingCore
{
    friend class MeetingCoreHolder;
public:
    MeetingCore() : launch_mode(CONTROL_LAUNCH_USE_START), core_mode(SERVER_MODE), master_control(CONTROL_LAUNCH_USE_START), core_holder(this) {}
    ~MeetingCore() {
        if(core_mode == SERVER_MODE) { master_control.~MasterCentralization(); }
        else if(core_mode == CLIENT_MODE) { servant_control.~ServantCentralization(); }
    }
    MeetingCoreHolder* onControlHolder() { return &core_holder; }
private:
    bool isLaunch() const;
    void updateControlMode(MeetingCoreMode mode, ControlLaunchMode lmode = CONTROL_LAUNCH_USE_START){
        if(core_mode == mode) return;

        core_mode = mode; launch_mode = lmode;
        if(core_mode == SERVER_MODE){
            servant_control.~ServantCentralization();
            new (&master_control) MasterCentralization(launch_mode);
        }else if(core_mode == CLIENT_MODE){
            master_control.~MasterCentralization();
            new (&servant_control) ServantCentralization(launch_mode);
        }
    }
    MeetingCoreMode onControlMode() const { return core_mode; }
    BasicControl* onMeetingCoreControl() {
        if(core_mode == CLIENT_MODE) { return &servant_control; }
        else if(core_mode == SERVER_MODE) { return &master_control; }
        else { return nullptr; }
    }

    ControlLaunchMode launch_mode;
    MeetingCoreMode core_mode;
    union {
        MasterCentralization    master_control;
        ServantCentralization   servant_control;
    };

    MeetingCoreHolder core_holder;
};

#endif // MEETINGCORE_H
