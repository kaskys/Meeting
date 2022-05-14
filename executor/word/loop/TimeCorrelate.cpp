//
// Created by abc on 20-4-17.
//
#include "../LoopWordThread.h"

/**
 * 插入时间任务
 * @param task 任务
 * @return
 */
TaskInfo* CorrelateBase::pushTask(std::shared_ptr<LoopExecutorTask> &&task) throw(std::bad_alloc) {
    //将任务插入内存中（非设置二叉堆信息）
    std::pair<int, int> task_pos = bucket_link.push(std::move(task));
    //返回任务信息
    return dynamic_cast<TaskInfo*>(bucket_link.getBinaryInfo(task_pos));
}

/**
 * 任务触发
 * @param trigger_info  任务信息
 * @param new_time
 */
void CorrelateBase::onTimeTrigger(TaskInfo *trigger_info) {
    std::shared_ptr<LoopExecutorTask> trigger_task;

    //获取任务信息
    if((trigger_task = getTaskTime(trigger_info))){
        //是否触发完成
        if(!onTimeTrigger0(trigger_task)){
            //完成,根据下一次触发时间复位任务
            onResetLevel(trigger_info);
        }else{
            //任务完结
            onTimeComplete(trigger_info);
        }
    }
}

/**
 * 任务触发的实现函数
 * @param time_task     触发任务
 * @param new_time      触发时间点
 * @return  true:移除任务（1：取消,2：超出最大时间）,false：触发完成
 */
bool CorrelateBase::onTimeTrigger0(std::shared_ptr<LoopExecutorTask> &time_task) {
    //任务是否移除
    bool task_erase = false;

    //判断任务是否取消
    if (!time_task->isCancel()) {
        //获取触发任务执行note信息
        std::shared_ptr<LoopBaseNote> executor_note = time_task->onTimeTrigger(correlate_time_point,
                                                                               [&](bool timeout) -> void {
                                                                                   //超出任务最大时间,完成触发后需要移除任务
                                                                                   task_erase = timeout;
                                                                               });
        //关联task
        executor_note->correlateLoopTask(time_task);
        //LoopThread线程接收任务的执行note
        loop_thread->onExecutor(std::static_pointer_cast<ExecutorNote>(executor_note));
    } else {
        //任务已经被取消,从时间关联器内移除任务
        task_erase = true;
    }

    return task_erase;
}

/**
 * 任务完结,从内存中移除
 * @param complete_info 任务
 */
void CorrelateBase::onTimeComplete(TaskInfo *complete_info) {
    onClear(complete_info);
}

void CorrelateBase::onClear(TaskInfo *info) {
    bucket_link.erase(info->getInfoPosition());
}

/**
 * 递增遍历时间间隔
 * @param start         开始点
 * @param end           结束点
 * @param ergodic_func  遍历函数(true:结束遍历函数,false:继续遍历)
 * @param end_func      结束函数
 */
void CorrelateBase::increaseErgodic(int start, int end, const std::function<bool(int)> &ergodic_func, const std::function<void()> &end_func) {
    for(; start < end; start++){
        if(ergodic_func(start)){
            return;
        }
    }

    if(end_func != nullptr){
        end_func();
    }
}

/**
 * 递减遍历时间间隔
 * @param start         开始点
 * @param end           结束点
 * @param ergodic_func  遍历函数
 * @param end_func      结束函数
 */
void CorrelateBase::reduceErgodic(int start, int end, const std::function<bool(int)> &ergodic_func, const std::function<void()> &end_func) {
    for(; start >= end; start--){
        if(ergodic_func(start)){
            return;
        }
    }

    if(end_func != nullptr){
        end_func();
    }
}

double CorrelateBase::initContainTime(TaskInfo *info) {
    return static_cast<double>(getTaskTime(info)->triggerTime(correlate_time_point));
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

MicrosecondSection::MicrosecondSection(LoopWordThread *lthread) : CorrelateBase(lthread) {
    initInterval();
}

/**
 * 初始化微秒级时间间隔
 */
void MicrosecondSection::initInterval() {
    increaseErgodic(0, CORRELATE_MICROSECOND_INTERVAL_SIZE,
                    [&](int pos) -> bool {
                        microsecond_interval[pos] = TimeInterval(pos * 500, (pos + 1) * 500, this,
                                                                 (ContainDynamicFunc)&MicrosecondSection::onContain, &bucket_link);
                        return false;
                  }, nullptr);
}

/**
 * 判断任务触发时间是否在时间间隔里(微妙级)
 * @param info          任务信息
 * @param new_time      当前时间点
 * @param contain_start 时间间隔起始的开区间
 * @param contain_end   时间间隔结束的闭区间
 * @return
 */
bool MicrosecondSection::onContain(TaskInfo *info, uint32_t contain_start, uint32_t contain_end) throw(std::logic_error) {
    std::shared_ptr<LoopExecutorTask> contain_task = getTaskTime(info);

    if(contain_task){
        if(contain_task->isCancel()){
            //任务取消
            return false;
        }else{
            //返回任务是否在当前时间间隔内
            return onContainStatic(initContainTime(info), contain_start, contain_end);
        }
    }else{
        throw std::logic_error("TimeCorrelate on Stop!");
    }
}

void MicrosecondSection::ergodicTimeInfo() {
    reduceErgodic(CORRELATE_MICROSECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool {
                      microsecond_interval[pos].ergodicTaskInfo(correlate_time_point);
                      return false;
                }, nullptr);
}

/**
 * 获取时间间隔里的任务最小触发时间(微秒级)
 * @return  任务信息
 */
TaskInfo* MicrosecondSection::onMinTime() {
    TaskInfo *min_info = nullptr;

    /*
     * 递增遍历微秒级所有时间间隔,判断是否有任务
     * 有：返回任务信息
     * 否：nullptr
     */
    increaseErgodic(0, CORRELATE_MICROSECOND_INTERVAL_SIZE,
                    [&](int pos) -> bool {
                        return static_cast<bool>((min_info = microsecond_interval[pos].minTaskInfo()));
                    }, nullptr);

    return min_info;
}

/**
 * 插入任务到时间间隔（微秒级）
 * @param info          任务信息
 * @param new_time      当前时间点
 */
void MicrosecondSection::onInsert(TaskInfo *info) {
    //格式化任务触发时间
    double trigger_time = initContainTime(info);

    //递减遍历所有时间间隔
    reduceErgodic(CORRELATE_MICROSECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                      //判断当前时间间隔是否能接收任务(任务触发时间是否在当前时间间隔里)
                      if(microsecond_interval[pos].onReceive(info, trigger_time, correlate_time_point)){
                          //接收任务,设置任务的时间间隔信息
                          info->interval_pos = static_cast<uint16_t>(pos);
                          info->interval_type = TIME_INTERVAL_TYPE_MICROSECOND;
                          return true;
                      }else{
                          return false;
                      }
               }, [&]() -> void {
                      //微秒级时间间隔不能接收(微妙级,直接触发任务)
                      onTimeTrigger(info);
               });
}

/**
 * 更新时间间隔里的所有任务(微秒级)
 * @param new_time  当前时间点
 */
void MicrosecondSection::onUpdate() {
    TaskInfo *surmount_info = nullptr;
    std::shared_ptr<LoopExecutorTask> update_task;

    reduceErgodic(CORRELATE_MICROSECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool {
                      try {
                          while((surmount_info = microsecond_interval[pos].onSurmount(correlate_time_point))){
                              if(!(update_task = getTaskTime(surmount_info))){
                                  throw std::logic_error("TimeCorrelate on Stop!");
                              }

                              if(update_task->isCancel()){
                                  onTimeComplete(surmount_info);
                              }else{
                                  if(pos > 0){
                                      microsecond_interval[pos - 1].forceReceive(surmount_info, correlate_time_point);
                                  }else{
                                      onTimeTrigger(surmount_info);
                                  }
                              }
                          }
                      }catch (std::logic_error &e){
                          std::cout << "MicrosecondSection::onUpdate->" << e.what() << std::endl;
                      }
                      return false;
                  }, nullptr);
}

/**
 * 毫秒级的任务升级到微秒级
 * @param skip_info 升级任务
 * @param new_time  当前时间点
 */

void MicrosecondSection::onSkipLevel(TaskInfo *skip_info) {
    double trigger_time = initContainTime(skip_info);

    reduceErgodic(CORRELATE_MICROSECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool {
                      return microsecond_interval[pos].onReceive(skip_info, trigger_time, correlate_time_point);
               }, [&]() -> void {
                      onTimeTrigger(skip_info);
               });
}

/**
 * 任务触发完成,复位任务
 * @param reset_info    任务信息
 * @param new_time      当前时间点
 */
void MicrosecondSection::onResetLevel(TaskInfo *reset_info) {
    double trigger_time = initContainTime(reset_info);

    if(trigger_time <= 0){
        onTimeTrigger(reset_info);
    }else{
        increaseErgodic((reset_info->interval_pos <= 0) ? reset_info->interval_pos : reset_info->interval_pos - 1, CORRELATE_MICROSECOND_INTERVAL_SIZE,
                        [&](int pos) -> bool{
                            if(microsecond_interval[pos].onReceive(reset_info, trigger_time, correlate_time_point)){
                                reset_info->interval_pos = static_cast<uint16_t>(pos);
                                reset_info->interval_type = TIME_INTERVAL_TYPE_MICROSECOND;
                                return true;
                            } else {
                                return false;
                            }
                     }, [&]() -> void{
                            onResetMicrosecond(reset_info);
                        });
    }
}

/**
 * 清理时间间隔（微秒级）
 */
void MicrosecondSection::clearInterval() {
    reduceErgodic(CORRELATE_MICROSECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool {
                      microsecond_interval[pos].onClear();
                      return false;
                 }, nullptr);
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
int MillisecondSection::millisecond_init_value[CORRELATE_MILLISECOND_INTERVAL_SIZE + 1] = {1, 200, 500 , 1000};

MillisecondSection::MillisecondSection(LoopWordThread *lthread) : MicrosecondSection(lthread) {
    initInterval();
}

/**
 * 初始化毫秒级时间间隔
 */
void MillisecondSection::initInterval() {
    increaseErgodic(0, CORRELATE_MILLISECOND_INTERVAL_SIZE,
                    [&](int pos) -> bool {
                        millisecond_interval[pos] = TimeInterval(millisecond_init_value[pos], millisecond_init_value[pos + 1],
                                                                 this, (ContainDynamicFunc)&MillisecondSection::onContain, &bucket_link);
                        return false;
                   }, nullptr);
}

/**
 * 判断任务触发时间是否在时间间隔里(毫秒级)
 * @param info          任务
 * @param new_time      时间点
 * @param contain_start 时间间隔开区间
 * @return
 */
bool MillisecondSection::onContain(TaskInfo *info, uint32_t contain_start, uint32_t contain_end) throw(std::logic_error) {
    std::shared_ptr<LoopExecutorTask> contain_task = getTaskTime(info);

    if(contain_task){
        if(contain_task->isCancel()){
            return false;
        }else{
            return onContainStatic(initContainTime<CORRELATE_MILLISECOND_DEN_VALUE>(info, correlate_time_point), contain_start, contain_end);
        }
    }else{
        throw std::logic_error("TimeCorrelate on Stop!");
    }
}

void MillisecondSection::ergodicTimeInfo() {
    reduceErgodic(CORRELATE_MILLISECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool {
                      millisecond_interval[pos].ergodicTaskInfo(correlate_time_point);
                      return false;
               }, [&]() -> void {
                MicrosecondSection::ergodicTimeInfo();
               });
}

/**
 * 获取时间间隔里的任务最小触发时间(毫秒级)
 * @return
 */
TaskInfo* MillisecondSection::onMinTime() {
    TaskInfo *min_info = MicrosecondSection::onMinTime();

    if(!min_info){
        increaseErgodic(0, CORRELATE_MILLISECOND_INTERVAL_SIZE,
                        [&](int pos) -> bool {
                            return static_cast<bool>((min_info = millisecond_interval[pos].minTaskInfo()));
                        }, nullptr);
    }

    return min_info;
}

/**
 * 插入任务到时间间隔（毫秒级）
 * @param section_info  任务
 * @param new_time      时间点
 */
void MillisecondSection::onInsert(TaskInfo *task_info) {
    double contain_time = initContainTime<CORRELATE_MILLISECOND_DEN_VALUE>(task_info, correlate_time_point);

    if(contain_time < millisecond_init_value[0]){
        MicrosecondSection::onInsert(task_info);
    }else {
        reduceErgodic(CORRELATE_MILLISECOND_INTERVAL_SIZE - 1, 0,
                      [&](int pos) -> bool {
                          if (millisecond_interval[pos].onReceive(task_info, contain_time, correlate_time_point)) {
                              task_info->interval_pos = static_cast<uint16_t>(pos);
                              task_info->interval_type = TIME_INTERVAL_TYPE_MILLISECOND;
                              return true;
                          } else {
                              return false;
                          }
                   }, [&]() -> void {
                          MicrosecondSection::onInsert(task_info);
                   });
    }
}

void MillisecondSection::onUpdate() {
    TaskInfo *surmount_info = nullptr;
    std::shared_ptr<LoopExecutorTask> update_task;

    reduceErgodic(CORRELATE_MILLISECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                    try{
                        while((surmount_info = millisecond_interval[pos].onSurmount(correlate_time_point))){
                            if(!(update_task = getTaskTime(surmount_info))){
                                throw std::logic_error("TimeCorrelate on Stop!");
                            }

                            if(update_task->isCancel()){
                                onTimeComplete(surmount_info);
                            }else{
                                if(pos > 0){
                                    millisecond_interval[pos - 1].forceReceive(surmount_info, correlate_time_point);
                                }else{
                                    MicrosecondSection::onSkipLevel(surmount_info);
                                }
                            }
                        }
                    } catch(std::logic_error &e){
                        std::cout << "MillisecondSection::onUpdate->" << e.what() << std::endl;
                    }
                    return false;
               }, [&]() -> void {
                    MicrosecondSection::onUpdate();
               });
}

void MillisecondSection::onSkipLevel(TaskInfo *skip_info) {
    double contain_time = initContainTime<CORRELATE_MILLISECOND_DEN_VALUE>(skip_info, correlate_time_point);

    reduceErgodic(CORRELATE_MILLISECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                    return millisecond_interval[pos].onReceive(skip_info, contain_time, correlate_time_point);
               }, [&]() -> void{
                    MicrosecondSection::onSkipLevel(skip_info);
               });
}

void MillisecondSection::onResetLevel(TaskInfo *reset_info) {
    double contain_time = initContainTime<CORRELATE_MILLISECOND_DEN_VALUE>(reset_info, correlate_time_point);

    if(contain_time < millisecond_init_value[0]){
        reset_info->interval_pos = 0;
        MicrosecondSection::onResetLevel(reset_info);
    }else{
        increaseErgodic((reset_info->interval_pos <= 0) ? reset_info->interval_pos : reset_info->interval_pos - 1, CORRELATE_MILLISECOND_INTERVAL_SIZE,
                        [&](int pos) -> bool{
                            if(millisecond_interval[pos].onReceive(reset_info, contain_time, correlate_time_point)){
                                reset_info->interval_pos = static_cast<uint16_t>(pos);
                                reset_info->interval_type = TIME_INTERVAL_TYPE_MILLISECOND;
                                return true;
                            }else{
                                return false;
                            }
                     }, [&]() -> void{
                            onResetMillisecond(reset_info);
                     });
    }
}

void MillisecondSection::onResetMicrosecond(TaskInfo *reset_info) {
    double contain_time = initContainTime<CORRELATE_MILLISECOND_DEN_VALUE>(reset_info, correlate_time_point);

    increaseErgodic(0, CORRELATE_MILLISECOND_INTERVAL_SIZE,
                    [&](int pos) -> bool{
                        if(millisecond_interval[pos].onReceive(reset_info, contain_time, correlate_time_point)){
                            reset_info->interval_pos = static_cast<uint16_t>(pos);
                            reset_info->interval_type = TIME_INTERVAL_TYPE_MILLISECOND;
                            return true;
                        }else{
                            return false;
                        }
                 }, [&]() -> void{
                        onResetMillisecond(reset_info);
                 });
}

void MillisecondSection::clearInterval() {
    reduceErgodic(CORRELATE_MILLISECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                      millisecond_interval[pos].onClear();
                      return false;
               }, [&]() -> void{
                      MicrosecondSection::clearInterval();
               });
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//
int SecondSection::second_init_value[CORRELATE_SECOND_INTERVAL_SIZE + 1] = {1, 5, 15, 30 ,60 ,-1};

SecondSection::SecondSection(LoopWordThread *thread) : MillisecondSection(thread) {
    initInterval();
}

void SecondSection::initInterval() {
    increaseErgodic(0, CORRELATE_SECOND_INTERVAL_SIZE,
                    [&](int pos) -> bool {
                        second_interval[pos] = TimeInterval(second_init_value[pos], second_init_value[pos + 1],
                                                            this, (ContainDynamicFunc)&SecondSection::onContain, &bucket_link);
                        return false;
                   }, nullptr);
}

bool SecondSection::onContain(TaskInfo *info, uint32_t contain_start, uint32_t contain_end) throw(std::logic_error) {
    std::shared_ptr<LoopExecutorTask> contain_task = getTaskTime(info);

    if(contain_task){
        if(contain_task->isCancel()){
            return false;
        }else{
            return onContainStatic(initContainTime<CORRELATE_SECOND_DEN_VALUE>(info, correlate_time_point), contain_start, contain_end);
        }
    }else{
        throw std::logic_error("TimeCorrelate on Stop!");
    }
}

void SecondSection::ergodicTimeInfo() {
    reduceErgodic(CORRELATE_SECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                      second_interval[pos].ergodicTaskInfo(correlate_time_point);
                      return false;
               }, [&]() -> void{
                      MillisecondSection::ergodicTimeInfo();
               });
}

TaskInfo* SecondSection::onMinTime() {
    TaskInfo *min_info = MillisecondSection::onMinTime();

    if(!min_info){
        increaseErgodic(0, CORRELATE_SECOND_INTERVAL_SIZE,
                        [&](int pos) -> bool{
                            return static_cast<bool>((min_info = second_interval[pos].minTaskInfo()));
                        }, nullptr);
    }

    return min_info;
}

void SecondSection::onInsert(TaskInfo *info) {
    double contain_time = initContainTime<CORRELATE_SECOND_DEN_VALUE>(info, correlate_time_point);

    if(contain_time < second_init_value[0]){
        MillisecondSection::onInsert(info);
    }else {
        reduceErgodic(CORRELATE_SECOND_INTERVAL_SIZE - 1, 0,
                      [&](int pos) -> bool {
                          if (second_interval[pos].onReceive(info, contain_time, correlate_time_point)) {
                              info->interval_pos = static_cast<uint16_t>(pos);
                              info->interval_type = TIME_INTERVAL_TYPE_SECOND;
                              return true;
                          } else {
                              return false;
                          }
                   }, [&]() -> void {
                        MillisecondSection::onInsert(info);
                   });
    }
}

void SecondSection::onUpdate() {
    TaskInfo *surmount_info = nullptr;
    std::shared_ptr<LoopExecutorTask> update_task;

    reduceErgodic(CORRELATE_SECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                      try {
                          while((surmount_info = second_interval[pos].onSurmount(correlate_time_point))){
                              if(!(update_task = getTaskTime(surmount_info))){
                                  throw std::logic_error("TimeCorrelate on Stop!");
                              }

                              if(update_task->isCancel()){
                                  onTimeComplete(surmount_info);
                              }else{
                                  if(pos > 0){
                                      second_interval[pos - 1].forceReceive(surmount_info, correlate_time_point);
                                  }else{
                                      MillisecondSection::onSkipLevel(surmount_info);
                                  }
                              }
                          }
                      }catch (std::logic_error &e){
                          std::cout << "SecondSection::onUpdate->" << e.what() << std::endl;
                      }
                      return false;
                  }, [&]() -> void {
                    MillisecondSection::onUpdate();
                  });
}



void SecondSection::onSkipLevel(TaskInfo *) {
    //不需要实现,只能向上跳级
}

void SecondSection::onResetLevel(TaskInfo *reset_info) {
    double contain_time = initContainTime<CORRELATE_SECOND_DEN_VALUE>(reset_info, correlate_time_point);

    if(contain_time < second_init_value[0]){
        reset_info->interval_pos = 0;
        MillisecondSection::onResetLevel(reset_info);
    }else{
        increaseErgodic((reset_info->interval_pos <= 0) ? reset_info->interval_pos : reset_info->interval_pos - 1, CORRELATE_SECOND_INTERVAL_SIZE,
                        [&](int pos) -> bool {
                            if(second_interval[pos].onReceive(reset_info, contain_time, correlate_time_point)){
                                reset_info->interval_pos = static_cast<uint16_t>(pos);
                                reset_info->interval_type = TIME_INTERVAL_TYPE_SECOND;
                                return true;
                            }else{
                                return false;
                            };
                        }, nullptr);
    }
}

void SecondSection::onResetMillisecond(TaskInfo *millisecond_info) {
    double contain_time = initContainTime<CORRELATE_SECOND_DEN_VALUE>(millisecond_info, correlate_time_point);

    increaseErgodic(0, CORRELATE_SECOND_INTERVAL_SIZE,
                    [&](int pos) -> bool{
                        if(second_interval[pos].onReceive(millisecond_info, contain_time, correlate_time_point)){
                            millisecond_info->interval_pos = static_cast<uint16_t>(pos);
                            millisecond_info->interval_type = TIME_INTERVAL_TYPE_SECOND;
                            return true;
                        }else{
                            return false;
                        }
                    }, nullptr);
}

void SecondSection::clearInterval() {
    reduceErgodic(CORRELATE_SECOND_INTERVAL_SIZE - 1, 0,
                  [&](int pos) -> bool{
                      second_interval[pos].onClear();
                      return false;
               }, [&]() -> void{
                      MillisecondSection::clearInterval();
               });
}

//--------------------------------------------------------------------------------------------------------------------//
//--------------------------------------------------------------------------------------------------------------------//

/**
 * 获取所有任务里的最小触发时间
 * @param new_time 当前时间点
 * @return  微秒时间
 */
int64_t TimeCorrelate::minTime() throw(task_trigger_error) {
    int64_t min_time = 0;
    TaskInfo *min_info = nullptr;

    if((min_info = onMinTime())){
        if((min_time = getTaskTime(min_info)->triggerTime(correlate_time_point)) <= 0){
            throw task_trigger_error();
        }
    }

    return min_time;
}


/**
 * 获取指定任务的触发时间
 * @param info      指定任务
 * @param new_time  当前时间点
 * @return  微秒时间
 */

int64_t TimeCorrelate::minTime(TaskInfo *info) throw(task_trigger_error) {
    int64_t min_time = 0;

    if((min_time = getTaskTime(info)->triggerTime(correlate_time_point)) <= 0){
        throw task_trigger_error();
    }

    return min_time;
}

/**
 * 插入任务
 * @param task      任务
 * @param new_time  当前时间点
 */
void TimeCorrelate::insertTimeTask(std::shared_ptr<LoopExecutorTask> task) throw(std::bad_alloc) {
    SecondSection::onInsert(pushTask(std::move(task)));
}

/**
 * 更新任务时间
 * @param new_time  当前时间点
 */
void TimeCorrelate::updateTimeTask() {
    SecondSection::onUpdate();
}

/**
 * 更新指定任务时间（序号任务）,非所有任务
 * @param update_info   指定任务
 * @param new_time      当前时间
 */
void TimeCorrelate::timeTaskUpdate(TaskInfo *update_info) {
    std::shared_ptr<LoopExecutorTask> update_task{nullptr};

    if((update_task = getTaskTime(update_info))){
        CorrelateBase::onTimeTrigger0(update_task);
    }
}

/**
 * 清空所有任务
 */
void TimeCorrelate::clearAll() {
    SecondSection::clearInterval();
}

/**
 * 低位时间间隔向高位时间间隔传递任务
 * @param reset_info
 * @param new_time
 */
void TimeCorrelate::onResetLevel(TaskInfo *reset_info) {
    switch (reset_info->interval_type){
        case TIME_INTERVAL_TYPE_MICROSECOND:
            MicrosecondSection::onResetLevel(reset_info);
            break;
        case TIME_INTERVAL_TYPE_MILLISECOND:
            MillisecondSection::onResetLevel(reset_info);
            break;
        case TIME_INTERVAL_TYPE_SECOND:
            SecondSection::onResetLevel(reset_info);
            break;
        default:
            break;
    }
}
