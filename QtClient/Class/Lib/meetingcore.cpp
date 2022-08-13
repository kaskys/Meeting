#include "meetingcore.h"


bool MeetingCoreHolder::isLaunch() const
{
    return core->isLaunch();
}

MeetingCoreMode MeetingCoreHolder::onCoreMode() const
{
    return core->onControlMode();
}

void MeetingCoreHolder::onCoreMode(MeetingCoreMode mode)
{
    core->updateControlMode(mode);
}

MsgHdr* MeetingCoreHolder::onInitLaunch(const AddressInitInfo &ainfo, const TimeInitInfo &tinfo, const DisplayInitInfo &dinfo)
{
    return dynamic_cast<CentralizationControl*>(core->onMeetingCoreControl())->onCompileLaunchMsg(ainfo, tinfo, dinfo);
}

void MeetingCoreHolder::launchControl(MsgHdr *launch_msg)
{
    BasicControl *control = core->onMeetingCoreControl();
    if(launch_msg && control){ control->launchControl(launch_msg); }
}

void MeetingCoreHolder::stopControl(MsgHdr *stop_msg)
{
    BasicControl *control = core->onMeetingCoreControl();
    if(control){ control->stopControl(stop_msg); }
}

void MeetingCoreHolder::agreeNoteIndicator(uint32_t pos)
{
    CentralizationControl::onExternalInput(CONTROL_LAYER_DISPLAY,
                                           [&](MsgHdr *indicator_msg, const int flag) -> void {
                                                core->onMeetingCoreControl()->onCommonInput(indicator_msg, flag);
                                           }, pos);
}

void MeetingCoreHolder::completeMediaBuffer(const char *buffer, uint32_t len)
{
    CentralizationControl::onExternalInput(CONTROL_LAYER_DISPLAY,
                                           [&](MsgHdr *buffer_msg, const int flag) -> void {
                                                core->onMeetingCoreControl()->onCommonInput(buffer_msg, flag);
                                           }, buffer, len);
}

void MeetingCoreHolder::inputVideoFrame(uint32_t len, unsigned char *buffer, const std::function<void ()> &callback)
{
    CentralizationControl::onExternalInput(CONTROL_LAYER_DISPLAY,
                                           [&](MsgHdr *frame_buffer, const int flag) -> void {
                                                core->onMeetingCoreControl()->onCommonInput(frame_buffer, flag);
                                           }, len, buffer, callback);
}

void MeetingCoreHolder::inputAudioFrame(uint32_t len, char *buffer)
{
    CentralizationControl::onExternalInput(CONTROL_LAYER_DISPLAY,
                                           [&](MsgHdr *frame_buffer, const int flag) -> void {
                                                core->onMeetingCoreControl()->onCommonInput(frame_buffer, flag);
                                           }, len, buffer);
}

//-----------------------------------------------------------------------------------------------------------------//

bool MeetingCore::isLaunch() const
{
    ControlStatus status = CONTROL_STATUS_INIT;
    if(core_mode == CLIENT_MODE) { status = servant_control.getControlStatus(); }
    else if(core_mode == SERVER_MODE) { status = master_control.getControlStatus(); }

    if((status == CONTROL_STATUS_LAUNCH) || (status == CONTROL_STATUS_LAUNCH)) { return true; }
    else { return false; }
}
