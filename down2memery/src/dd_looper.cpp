//
//  dd_command_looper.cpp
//  down2memery
//
//  Created by dengchu on 2021/2/9.
//  Copyright © 2021 dengchu. All rights reserved.
//

#include "dd_looper.hpp"
command_queue::command_queue(){
    pthread_mutex_init(&m_deque_lock, NULL);
    pthread_cond_init(&m_deque_cond, NULL);
    m_exit = false;
}

command_queue::~command_queue(){
    pthread_mutex_destroy(&m_deque_lock);
    pthread_cond_destroy(&m_deque_cond);
}

bool command_queue::push_command(share_command cmd){
    pthread_mutex_lock(&m_deque_lock);
    m_command_queue.push_back(cmd);
    pthread_cond_signal(&m_deque_cond);
    pthread_mutex_unlock(&m_deque_lock);
    return true;
}

bool command_queue::pop_command(share_command& cmd){
    bool ret = false;
    pthread_mutex_lock(&m_deque_lock);
    while(m_command_queue.size() <= 0 && !m_exit){
        pthread_cond_wait(&m_deque_cond, &m_deque_lock);
    }
    if (m_command_queue.size() > 0){
        cmd = m_command_queue.front();
        m_command_queue.pop_front();
        ret = true;
    }
    pthread_mutex_unlock(&m_deque_lock);
    return ret;
}

void command_queue::abort(){
    pthread_mutex_lock(&m_deque_lock);
    m_exit = true;
    pthread_cond_signal(&m_deque_cond);
    pthread_mutex_unlock(&m_deque_lock);
}

dd_looper::dd_looper(){
    pthread_create(&m_thread_id, NULL, cycle, this);
}

dd_looper::~dd_looper(){
    abort();
    if (m_thread_id)
        pthread_join(m_thread_id, NULL);
}

void* dd_looper::cycle(void* args){
    dd_looper* obj = (dd_looper*)args;
    if (obj){
        obj->do_cycle();
    }
    return NULL;
}

void dd_looper::do_cycle(){
    while (true) {
        share_command scmd;
        bool ret = pop_command(scmd);
        if (!ret)
            break;
        ref_command* cmd = scmd.get_looper_command();
        if (!cmd)
            continue;
        pthread_mutex_lock(&cmd->command_mutex);
        if (cmd->cmd_func){
            cmd->cmd_func(cmd->user_data, cmd->params);
        }
        cmd->finish = 1;
        pthread_cond_signal(&cmd->command_cond);
        pthread_mutex_unlock(&cmd->command_mutex);
    }
}

void dd_looper::post_sync_command(command_func _cmd_func, void* _user_data, const char* args, ...){
    va_list args_ptr;
    va_start(args_ptr, args);
    share_command scmd(_cmd_func, _user_data, args_ptr);
    push_command(scmd);
    ref_command* cmd = scmd.get_looper_command();
    pthread_mutex_lock(&cmd->command_mutex);
    if (!cmd->finish)
        pthread_cond_wait(&cmd->command_cond, &cmd->command_mutex);
    pthread_mutex_unlock(&cmd->command_mutex);
}

void dd_looper::post_async_command(command_func _cmd_func, void* _user_data, const char* args, ...){
    va_list args_ptr;
    va_start(args_ptr, args);
    share_command scmd(_cmd_func, _user_data, args_ptr);
    push_command(scmd);
}
