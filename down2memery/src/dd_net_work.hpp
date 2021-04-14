//
//  dd_net_work.hpp
//  down2memery
//
//  Created by dengchu on 2020/5/28.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_net_work_hpp
#define dd_net_work_hpp

#include "dd_share_task.hpp"

typedef dd_share_task (*get_task_cb)(void* opaque);
class dd_net_work{
public:
    dd_net_work(get_task_cb cb, void* opaque);
    virtual ~dd_net_work();
private:
    static void* cycle(void* args);
    void do_cycle();
private:
    pthread_t m_thread;
    bool m_abort;
    get_task_cb m_cb;
    void* m_opaque;
};

#endif /* dd_net_work_hpp */
