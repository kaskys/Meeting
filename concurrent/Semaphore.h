//
// Created by abc on 19-8-17.
//

#ifndef UNTITLED8_SEMAPHORE_H
#define UNTITLED8_SEMAPHORE_H

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <functional>

#define SEMAPHORE_OPERATE_VALUE     1

template <typename Master, typename Servant> class Permit{
public:
    Permit(Master *m, Servant *s, const std::function<void(Permit<Master, Servant>*)> &func) throw(std::logic_error)
            :status(true), master(m), servant(s), release_func(func) {
        if(!release_func){
            throw std::logic_error("Permit::Permit() release_func is null!");
        }
    }
    virtual ~Permit() = default;

    Permit(const Permit &permit) noexcept : status(permit.status.load()), master(permit.master), servant(permit.servant), release_func(permit.release_func) {}
    Permit(Permit &&permit) noexcept : status(permit.status.load()), master(permit.master), servant(permit.servant), release_func(permit.release_func) {
        permit.status.store(false); permit.master = nullptr; permit.servant = nullptr; permit.release_func = nullptr;
    }

    Permit& operator=(const Permit &permit) noexcept {
        status = permit.status.load(); master = permit.master; servant = permit.servant; release_func = permit.release_func;
        return *this;
    }
    Permit& operator=(Permit &&permit) noexcept {
        status = permit.status.load(); master = permit.master; servant = permit.servant; release_func = permit.release_func;
        permit.status.store(false); permit.master = nullptr; permit.servant = nullptr; permit.release_func = nullptr;
        return *this;
    }

    void masterRelease() {
        if(master){
            onInspectAndRelease(); master = nullptr;
        }
    }
    void servantRelease() {
        if(servant){
            onInspectAndRelease(); servant = nullptr;
        }
    }
    bool checkPermit(){ return status.load(); }
protected:
    std::atomic_bool status;
    Master *master;
    Servant *servant;
private:
    void onInspectAndRelease(){
        if(!status.load()){
            release_func(this);
        }else{
            status.store(false);
        }
    }
    std::function<void(Permit<Master, Servant>*)> release_func;
};

template <typename Permit_, typename Factory_> class Semaphore final{
public:
    Semaphore(int size, const Factory_ &factory) : semaphore_size(size), remain_size(size), callback(nullptr), permit_factory(factory){}
    ~Semaphore() = default;

    Permit_* acquire(const Permit_ &permit) {
        Permit_ *acquire_permit = nullptr;
        if(remain_size.load() > 0){
            if(remain_size.fetch_sub(SEMAPHORE_OPERATE_VALUE) > 0){
                acquire_permit = permit_factory.createPermit(permit);
            }else{
                remain_size.fetch_add(SEMAPHORE_OPERATE_VALUE);
            };
        }
        return acquire_permit;
    }
    void release(Permit_ *permit){
        if(permit){
            permit_factory.destroyPermit(permit);
            if(remain_size.fetch_add(SEMAPHORE_OPERATE_VALUE) >= (semaphore_size - 1)){
                if(callback != nullptr){
                    callback();
                }
            }
        }
    }

    void setCallback(const std::function<void()> &func) { callback = func; }

    bool emptySemaphore() const { return (remain_size.load() <= 0); }
    bool fullSemaphore() const { return (remain_size.load() >= semaphore_size); }
private:
    int semaphore_size;
    std::atomic_int remain_size;
    std::function<void()> callback;
    Factory_ permit_factory;
};

#endif //UNTITLED8_SEMAPHORE_H
