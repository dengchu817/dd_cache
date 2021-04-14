//
//  dd_task_manager.cpp
//  down2memery
//
//  Created by dengchu on 2020/9/11.
//  Copyright © 2020 dengchu. All rights reserved.
//

#include "dd_task_manager.hpp"
dd_task_manager::dd_task_manager(){
    pthread_mutex_init(&m_mutex, NULL);
    for (int i = 0; i < 3; i++){
        dd_net_work* worker = new dd_net_work(get_task_callback, this);
        m_workers.push_back(worker);
    }
}

dd_task_manager::~dd_task_manager(){
}

int dd_task_manager::create_task(download_task_type type, const char* url, const char* key, const char* cache_path, int64_t start_range, int64_t end_range){
    if (!url || !key || !cache_path)
        return NULL;
    pthread_mutex_lock(&m_mutex);
    map<string, dd_share_task*>::iterator it = m_tasks.find(key);
    if (it == m_tasks.end()){
        printf("=====%s, new task url=%s, key=%s\n",__FUNCTION__, url, key);
        dd_share_task* task = new dd_share_task(type, url, key, cache_path, start_range, end_range);
        m_tasks.insert(pair<string, dd_share_task*>(key, task));
    }
    pthread_mutex_unlock(&m_mutex);
    return 0;
}

void dd_task_manager::destory_task(const char* key){
    pthread_mutex_lock(&m_mutex);
    map<string, dd_share_task*>::iterator it = m_tasks.find(key);
    if (it != m_tasks.end()){
        //close all connection
        dd_share_task* task = it->second;
        m_tasks.erase(it);
        task->stop();
        delete task;
    }
    pthread_mutex_unlock(&m_mutex);
}

dd_share_task dd_task_manager::get_task_callback(void* opaque){
    dd_share_task task;
    if (opaque)
        return ((dd_task_manager*)opaque)->do_get_task_callback();
    return task;
}

dd_share_task dd_task_manager::do_get_task_callback(){
    dd_share_task task;
    pthread_mutex_lock(&m_mutex);
    for (map<string, dd_share_task*>::iterator itor = m_tasks.begin(); itor != m_tasks.end(); itor++){
        dd_share_task* task_ = itor->second;
        if (task_->get_config() && task_->get_config()->m_type == download_task_type_play && task.get_status() != task_status_stop){
            task = *task_;
            break;
        }
    }
    pthread_mutex_unlock(&m_mutex);
    return task;
}

int dd_task_manager::read_data(const char* key, uint8_t* buf, int size){
    int ret = -1;
    pthread_mutex_lock(&m_mutex);
    dd_share_task* task = get_by_key(key);
    if (task){
        ret = task->read_data(buf, size);
    }
    pthread_mutex_unlock(&m_mutex);
    return ret;
}
