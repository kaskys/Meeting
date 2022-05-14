//
// Created by abc on 20-4-21.
//

#ifndef UNTITLED5_WORDTHREAD_H
#define UNTITLED5_WORDTHREAD_H

#include "ManagerThread.h"

#define COMPLETE_VALUE    1

class BasicExecutor;
class WordThread;
class ManagerThread;

class Word {
public:
    explicit Word(WordThread *wthread) : word_thread(wthread) {}
    virtual ~Word() = default;

    virtual ThreadType type() const = 0;
    virtual void exe() = 0;
    virtual void interrupt() = 0;
    virtual void onPush(std::shared_ptr<ExecutorTask>) = 0;
    virtual void onExecutor(std::shared_ptr<ExecutorNote>) = 0;
    virtual void onInterrupt(SInstruct) throw(finish_error) = 0;
    virtual void onLoop() = 0;
protected:
    ThreadStatus onStatus();
    void onStartComplete(std::shared_ptr<StartInstruct>&);
    void onStopComplete(std::shared_ptr<StopSocketInstruct>&);
    void onExecutor();
    void onStop();
    void onFinish(void(*)(BasicThread*)) throw(finish_error);
    void response(SInstruct);

    Word* originalWordThread(const WordThread*) const;
    ManagerThread* threadManager() const;

    WordThread *word_thread;    //指向WordThread指针
};

class WordThread : public BasicThread{
    friend class Word;
public:
    WordThread() = default;
    ~WordThread() override = default;

    virtual void onLoop(std::promise<void>&);
    std::function<void(std::promise<void>&)> onStart() override { return std::bind(&WordThread::onLoop, this, std::placeholders::_1); }
    ThreadType type() const override { return word->type(); }
    void receiveTask(std::shared_ptr<ExecutorNote>) override;
    void receiveTask(std::shared_ptr<ExecutorTask>) override;
    void receiveInstruct(SInstruct) override;

    void correlateThreadManager(ManagerThread *thread) { thread_manager = thread; }
    void setWord(Word *w) { word = w; }
    Word* getWord() const { return word; }
protected:
    void handleInterrupt() throw(finish_error) override;
    void handleLoop() override;
private:
    void sendInstruct(SInstruct);

    Word *word;                     //指向word的指针
    ManagerThread *thread_manager;  //ManagerThread线程
    SInstruct *local_instruct;      //本地接收指令
};

#endif //UNTITLED5_WORDTHREAD_H
