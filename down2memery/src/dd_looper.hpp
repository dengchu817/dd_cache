//
//  dd_command_looper.hpp
//  down2memery
//
//  Created by dengchu on 2021/2/9.
//  Copyright © 2021 dengchu. All rights reserved.
//

#ifndef dd_command_looper_hpp
#define dd_command_looper_hpp
#include "dd_internal_def.hpp"
class command_queue{
public:
    command_queue();
    virtual ~command_queue();
protected:
    bool push_command(share_command cmd);
    bool pop_command(share_command& cmd);
    void abort();
private:
    deque<share_command> m_command_queue;
    pthread_mutex_t m_deque_lock;
    pthread_cond_t m_deque_cond;
    bool m_exit;
};

class dd_looper : public command_queue{
public:
    dd_looper();
    ~dd_looper();
public:
    void post_sync_command(command_func _cmd_func, void* _user_data, const char* args, ...);
    void post_async_command(command_func _cmd_func, void* _user_data, const char* args, ...);
private:
    static void* cycle(void* args);
    void do_cycle();
private:
    pthread_t m_thread_id;
};

#endif /* dd_command_looper_hpp */
