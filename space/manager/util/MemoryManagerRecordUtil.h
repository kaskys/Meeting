//
// Created by abc on 19-12-12.
//

#ifndef UNTITLED5_MEMORYMANAGERRECORDUTIL_H
#define UNTITLED5_MEMORYMANAGERRECORDUTIL_H

class MemoryRecorder;

struct RecordNumber{};
struct RecordSize{};
struct RecordCallSize{};
struct RecordSuccessSize{};
struct RecordOccupySize{};
struct RecordUnOccupySize{};

struct RecordBasic{
public:
    virtual uint32_t getRecord(const MemoryRecorder*, const RecordNumber&) const = 0;
    virtual uint32_t getRecord(const MemoryRecorder*, const RecordSize&) const = 0;
    virtual uint32_t getRecord(const MemoryRecorder*, const RecordCallSize&) const = 0;
    virtual uint32_t getRecord(const MemoryRecorder*, const RecordSuccessSize&) const = 0;
    virtual uint32_t getRecord(const MemoryRecorder*, const RecordOccupySize&) const = 0;
    virtual uint32_t getRecord(const MemoryRecorder*, const RecordUnOccupySize&) const = 0;
    virtual void setRecord(int, MemoryRecorder*, const RecordNumber&) = 0;
    virtual void setRecord(int, MemoryRecorder*, const RecordSize&) = 0;
    virtual void setRecord(int, MemoryRecorder*, const RecordCallSize&) = 0;
    virtual void setRecord(int, MemoryRecorder*, const RecordSuccessSize&) = 0;
    virtual void setRecord(int, MemoryRecorder*, const RecordOccupySize&) = 0;
    virtual void setRecord(int, MemoryRecorder*, const RecordUnOccupySize&) = 0;
};

struct RecordFixed : public RecordBasic{
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordNumber&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordOccupySize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize&) const override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize&) override;
};

struct RecordIdle : public RecordBasic{
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordNumber&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordOccupySize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize&) const override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize&) override;
};

struct RecordUse : public RecordBasic{
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordNumber&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordOccupySize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize&) const override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize&) override;
};

struct RecordLoad : public RecordBasic{
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordNumber&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordOccupySize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize&) const override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize&) override;
};

struct RecordExternal : public RecordBasic{
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordNumber&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordOccupySize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize&) const override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize&) override;
};

struct RecordDebris : public RecordBasic{
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordNumber&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordCallSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordSuccessSize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordOccupySize&) const override;
    uint32_t getRecord(const MemoryRecorder *recorder, const RecordUnOccupySize&) const override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordNumber&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordCallSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordSuccessSize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordOccupySize&) override;
    void setRecord(int size, MemoryRecorder *recorder, const RecordUnOccupySize&) override;
};

extern const RecordNumber       record_number;
extern const RecordSize         record_size;
extern const RecordCallSize     record_call_size;
extern const RecordSuccessSize  record_success_size;
extern const RecordOccupySize   record_occupy_size;
extern const RecordUnOccupySize record_unoccupy_size;

extern RecordFixed              record_fixed;
extern RecordIdle               record_idle;
extern RecordUse                record_use;
extern RecordLoad               record_load;
extern RecordExternal           record_external;
extern RecordDebris             record_debris;

#endif //UNTITLED5_MEMORYMANAGERRECORDUTIL_H
