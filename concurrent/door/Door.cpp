//
// Created by abc on 19-12-6.
//
#include "Door.h"

/**
 * 构造函数
 * @param status    门状态（默认开门）
 */
Door::Door(bool status) : door_status(status), wait_size(0), door_promise() {
    if(status){
        //开门状态,设置future
        door_future = door_promise.get_future();
    }else{
        //关门状态,调用关门函数
        close();
    }
}

/**
 * 开门实现函数
 */
void Door::open0() {
    //设置开门状态
    door_status.store(true, std::memory_order_release);
    //减少等待通过数量
    wait_size.fetch_sub(1, std::memory_order_release);
    //设置唤醒阻塞等待
    door_promise.set_value();
}

/**
 * 关门函数
 */
void Door::close() {
    //等待通过数量为0
    while(wait_size.load(std::memory_order_consume));

    //重置promise
    door_promise = std::promise<void>();
    //设置future
    door_future = door_promise.get_future();

    //设置关门状态
    door_status.store(false, std::memory_order_release);
    //增加等待通过数量
    wait_size.fetch_add(1, std::memory_order_release);
}

void Door::pass() {
    //判断门状态
    if (!door_status.load(std::memory_order_acquire)) {
        //关门状态
        try {
            //增加等待通过数量
            wait_size.fetch_add(1, std::memory_order_release);
            //阻塞等待开门
            door_future.get();
        } catch (std::future_error &e) {
            //多线程冲突
            std::cout << "Door.cpp( destroy the door(门被摧毁了)！" << e.what() << ")" << std::endl;
        }
        //已经通过,减少等待通过数量
        wait_size.fetch_sub(1, std::memory_order_release);
    }
}

UniqueDoor::UniqueDoor(Door *door) : door_(door){
    if(door_ && door_->is_open()){
        door_->close();
    }
}

UniqueDoor::~UniqueDoor() {
    if(door_ && door_->is_close()){
        door_->open0();
    }
}