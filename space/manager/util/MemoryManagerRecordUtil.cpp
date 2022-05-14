//
// Created by abc on 19-12-12.
//
#include "../MemoryManager.h"

uint32_t RecordFixed::getRecord(const MemoryRecorder *recorder, const RecordNumber &) const {
    return recorder->record_info.fixed_number.load(std::memory_order_consume);
}

uint32_t RecordFixed::getRecord(const MemoryRecorder *recorder, const RecordSize &) const {
    return 0; //不需要实现
}

uint32_t RecordFixed::getRecord(const MemoryRecorder *recorder, const RecordCallSize &) const  {
    return recorder->record_info.fixed_call_size.load(std::memory_order_consume);
}

uint32_t RecordFixed::getRecord(const MemoryRecorder *recorder, const RecordSuccessSize &) const  {
    return recorder->record_info.fixed_success_size.load(std::memory_order_consume);
}

uint32_t RecordFixed::getRecord(const MemoryRecorder *recorder, const RecordOccupySize &) const {
    return recorder->record_info.fixed_occupy_size.load(std::memory_order_consume);
}

uint32_t RecordFixed::getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize &) const {
    return recorder->record_info.fixed_unoccupy_size.load(std::memory_order_consume);
}

void RecordFixed::setRecord(int size, MemoryRecorder *recorder, const RecordNumber &)  {
    recorder->record_info.fixed_number.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordFixed::setRecord(int, MemoryRecorder*, const RecordSize &) {
    //不需要实现
}

void RecordFixed::setRecord(int size, MemoryRecorder *recorder, const RecordCallSize &)  {
    recorder->record_info.fixed_call_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordFixed::setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize &)  {
    recorder->record_info.fixed_success_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordFixed::setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize &) {
    recorder->record_info.fixed_occupy_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordFixed::setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize &) {
    recorder->record_info.fixed_unoccupy_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

uint32_t RecordIdle::getRecord(const MemoryRecorder *recorder, const RecordNumber&) const {
    return recorder->record_info.idle_number.load(std::memory_order_consume);
}

uint32_t RecordIdle::getRecord(const MemoryRecorder *, const RecordSize &) const {
    return 0;//不需要实现
}

uint32_t RecordIdle::getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const {
    return recorder->record_info.idle_call_size.load(std::memory_order_consume);
}

uint32_t RecordIdle::getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const {
    return recorder->record_info.idle_success_size.load(std::memory_order_consume);
}

uint32_t RecordIdle::getRecord(const MemoryRecorder *, const RecordOccupySize &) const {
    return 0;//不需要实现
}

uint32_t RecordIdle::getRecord(const MemoryRecorder *, const RecordUnOccupySize &) const {
    return 0;//不需要实现
}

void RecordIdle::setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) {
    recorder->record_info.idle_number.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordIdle::setRecord(int, MemoryRecorder*, const RecordSize &) {
    //不需要实现
}

void RecordIdle::setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) {
    recorder->record_info.idle_call_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordIdle::setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) {
    recorder->record_info.idle_success_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordIdle::setRecord(int, MemoryRecorder*, const RecordOccupySize &) {
    //不需要实现
}

void RecordIdle::setRecord(int, MemoryRecorder*, const RecordUnOccupySize &) {
    //不需要实现
}

uint32_t RecordUse::getRecord(const MemoryRecorder *recorder, const RecordNumber&) const {
    return recorder->record_info.use_number.load(std::memory_order_consume);
}

uint32_t RecordUse::getRecord(const MemoryRecorder*, const RecordSize &) const {
    return 0;//不需要实现
}

uint32_t RecordUse::getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const {
    return recorder->record_info.use_call_size.load(std::memory_order_consume);
}

uint32_t RecordUse::getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const {
    return recorder->record_info.use_success_size.load(std::memory_order_consume);
}

uint32_t RecordUse::getRecord(const MemoryRecorder *recorder, const RecordOccupySize &) const {
    return recorder->record_info.use_occupy_size.load(std::memory_order_consume);
}

uint32_t RecordUse::getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize &) const {
    return recorder->record_info.use_unoccupy_size.load(std::memory_order_consume);
}

void RecordUse::setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) {
    recorder->record_info.use_number.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordUse::setRecord(int, MemoryRecorder*, const RecordSize &) {
    //不需要实现
}

void RecordUse::setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) {
    recorder->record_info.use_call_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordUse::setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) {
    recorder->record_info.use_success_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordUse::setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize &) {
    recorder->record_info.use_occupy_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordUse::setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize &) {
    recorder->record_info.use_unoccupy_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

uint32_t RecordLoad::getRecord(const MemoryRecorder *recorder, const RecordNumber&) const {
    return recorder->record_info.load_number.load(std::memory_order_consume);
}

uint32_t RecordLoad::getRecord(const MemoryRecorder*, const RecordSize &) const {
    return 0;//不需要实现
}

uint32_t RecordLoad::getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const {
    return recorder->record_info.load_call_size.load(std::memory_order_consume);
}

uint32_t RecordLoad::getRecord(const MemoryRecorder*, const RecordSuccessSize &) const {
    return 0;//不需要实现
}

uint32_t RecordLoad::getRecord(const MemoryRecorder*, const RecordOccupySize &) const {
    return 0;//不需要实现
}

uint32_t RecordLoad::getRecord(const MemoryRecorder*, const RecordUnOccupySize &) const {
    return 0;//不需要实现
}

void RecordLoad::setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) {
    recorder->record_info.load_number.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordLoad::setRecord(int, MemoryRecorder*, const RecordSize &) {
    //不需要实现
}

void RecordLoad::setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) {
    recorder->record_info.load_call_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordLoad::setRecord(int, MemoryRecorder*, const RecordSuccessSize &) {
    //不需要实现
}

void RecordLoad::setRecord(int, MemoryRecorder*, const RecordOccupySize &) {
    //不需要实现
}

void RecordLoad::setRecord(int, MemoryRecorder*, const RecordUnOccupySize &) {
    //不需要实现
}

uint32_t RecordExternal::getRecord(const MemoryRecorder *recorder, const RecordNumber&) const {
    return recorder->record_info.external_number;
}

uint32_t RecordExternal::getRecord(const MemoryRecorder*, const RecordSize &) const {
    return 0;//不需要实现
}

uint32_t RecordExternal::getRecord(const MemoryRecorder*, const RecordCallSize &) const {
    return 0;//不需要实现
}

uint32_t RecordExternal::getRecord(const MemoryRecorder*, const RecordSuccessSize &) const {
    return 0;//不需要实现
}

uint32_t RecordExternal::getRecord(const MemoryRecorder*, const RecordOccupySize &) const {
    return 0;//不需要实现
}

uint32_t RecordExternal::getRecord(const MemoryRecorder*, const RecordUnOccupySize &) const {
    return 0;//不需要实现
}

void RecordExternal::setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) {
    recorder->record_info.external_number += size;
}

void RecordExternal::setRecord(int, MemoryRecorder*, const RecordSize &) {
    //不需要实现
}

void RecordExternal::setRecord(int, MemoryRecorder*, const RecordCallSize &) {
    //不需要实现
}

void RecordExternal::setRecord(int, MemoryRecorder*, const RecordSuccessSize &) {
    //不需要实现
}

void RecordExternal::setRecord(int, MemoryRecorder*, const RecordOccupySize &) {
    //不需要实现
}

void RecordExternal::setRecord(int, MemoryRecorder*, const RecordUnOccupySize &) {
    //不需要实现
}

uint32_t RecordDebris::getRecord(const MemoryRecorder*, const RecordNumber &) const {
    return 0;//不需要实现
}

uint32_t RecordDebris::getRecord(const MemoryRecorder *recorder, const RecordSize&) const {
    return recorder->record_info.debris_size;
}

uint32_t RecordDebris::getRecord(const MemoryRecorder*, const RecordCallSize &) const {
    return 0;//不需要实现
}

uint32_t RecordDebris::getRecord(const MemoryRecorder*, const RecordSuccessSize &) const {
    return 0;//不需要实现
}

uint32_t RecordDebris::getRecord(const MemoryRecorder*, const RecordOccupySize &) const {
    return 0;//不需要实现
}

uint32_t RecordDebris::getRecord(const MemoryRecorder*, const RecordUnOccupySize &) const {
    return 0;//不需要实现
}

void RecordDebris::setRecord(int, MemoryRecorder*, const RecordNumber &) {
    //不需要实现
}

void RecordDebris::setRecord(int size, MemoryRecorder *recorder, const RecordSize&) {
    recorder->record_info.debris_size.fetch_add(static_cast<uint32_t>(size), std::memory_order_release);
}

void RecordDebris::setRecord(int, MemoryRecorder*, const RecordCallSize &) {
    //不需要实现
}

void RecordDebris::setRecord(int, MemoryRecorder*, const RecordSuccessSize &) {
    //不需要实现
}

void RecordDebris::setRecord(int, MemoryRecorder*, const RecordOccupySize &) {
    //不需要实现
}

void RecordDebris::setRecord(int, MemoryRecorder*, const RecordUnOccupySize &) {
    //不需要实现
}

const RecordNumber       record_number           = RecordNumber();
const RecordSize         record_size             = RecordSize();
const RecordCallSize     record_call_size        = RecordCallSize();
const RecordSuccessSize  record_success_size     = RecordSuccessSize();
const RecordOccupySize   record_occupy_size      = RecordOccupySize();
const RecordUnOccupySize record_unoccupy_size    = RecordUnOccupySize();

RecordFixed    record_fixed                      = RecordFixed();
RecordIdle     record_idle                       = RecordIdle();
RecordUse      record_use                        = RecordUse();
RecordLoad     record_load                       = RecordLoad();
RecordExternal record_external                   = RecordExternal();
RecordDebris   record_debris                     = RecordDebris();