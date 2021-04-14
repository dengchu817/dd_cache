//
//  dd_task_manager.hpp
//  down2memery
//
//  Created by dengchu on 2020/9/11.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_task_manager_hpp
#define dd_task_manager_hpp

#include "dd_share_task.hpp"
#include "dd_net_work.hpp"
#include "dd_looper.hpp"
class dd_task_manager{
public:
    dd_task_manager();
    ~dd_task_manager();
    
public:
    int create_task(download_task_type type, const char* url, const char* key, const char* cache_path, int64_t start_range, int64_t end_range);
    void destory_task(const char* key);
    int read_data(const char* key, uint8_t* buf, int size);
private:
    static dd_share_task get_task_callback(void* opaque);
    dd_share_task do_get_task_callback();
private:
    map<string, dd_share_task*> m_tasks;
    vector<dd_net_work*> m_workers;
    pthread_mutex_t m_mutex;
};

#endif /* dd_task_manager_hpp */
