//
//  dd_net_cache.hpp
//  down2memery
//
//  Created by dengchu on 2020/5/28.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_net_cache_hpp
#define dd_net_cache_hpp

#include "dd_internal_def.hpp"
#include "dd_cache_unit.hpp"
#include "dd_http_header.hpp"

class dd_task_config{
public:
    dd_task_config(download_task_type type, const char* url, const char* key, const char* cache_path, int64_t range_start, int64_t range_end):m_type(type), m_url(url), m_key(key), m_cache_path(cache_path), m_range_start(range_start), m_range_end(range_end){
        
    }
    ~dd_task_config(){
        
    }
public:
    const download_task_type m_type;
    const string m_url;
    const string m_key;
    const string m_cache_path;
    const int64_t m_range_start;
    const int64_t m_range_end;
};

class dd_source_task : public ins_ref{
public:
    dd_source_task(download_task_type type, const char* url, const char* key, const char* cache_path, int64_t range_start, int64_t range_end);
    ~dd_source_task();
public:
    //net callback
    void net_response(HeadData* response);
    void net_data(void* data, int size, int64_t offset);
    void net_finish();
    void net_error(int error, int64_t offset);
    //
    void stop();
    int read_data(void* data, int size);
    int get_download_range(int64_t& offset_in_stream, int& size);
    const dd_task_config* get_config();
    task_status get_status();
private:
    pthread_mutex_t m_mutex;
    const dd_task_config m_config;
    int64_t m_read_offset;
    task_status m_status;
    dd_cache_unit* m_cache;
};

class dd_share_task{
public:
    dd_share_task(){
        m_task = NULL;
    }
    
    dd_share_task(download_task_type type, const char* url, const char* key, const char* cache_path, int64_t range_start, int64_t range_end){
        m_task = new dd_source_task(type, url, key, cache_path, range_start, range_end);
    }
    
    dd_share_task(const dd_share_task& share_task){
        if (share_task.m_task){
            share_task.m_task->increase_reference();
            m_task = share_task.m_task;
        }
    }
    
    dd_share_task& operator=(const dd_share_task& share_task){
        if (m_task && m_task->decrease_reference() == 0){
            delete m_task;
            m_task = NULL;
        }
        if (share_task.m_task){
            share_task.m_task->increase_reference();
            m_task = share_task.m_task;
        }
        return *this;
    }
    
    ~dd_share_task(){
        if (m_task && m_task->decrease_reference() == 0){
            delete m_task;
            m_task = NULL;
        }
    };
public:
    //worker callback
    void net_response(HeadData* response){
        if (m_task)
            m_task->net_response(response);
    }
    void net_data(void* data, int size, int64_t offset){
        if (m_task)
            m_task->net_data(data, size, offset);
    }
    void net_finish(){
        if (m_task)
            m_task->net_finish();
    }
    void net_error(int error, int64_t offset){
        if (m_task)
            m_task->net_error(error, offset);
    }
    //manager
    void stop(){
        if (m_task)
            m_task->stop();
    }
    int read_data(void* data, int size){
        if (m_task)
            return m_task->read_data(data, size);
        return -1;
    }
    int get_download_range(int64_t& offset_in_stream, int& size){
        if (m_task)
            return m_task->get_download_range(offset_in_stream, size);
        return -1;
    }
    
    const dd_task_config* get_config(){
        if (m_task)
            return m_task->get_config();
        return NULL;
    }
    
    task_status get_status(){
        if (m_task)
            return m_task->get_status();
        return task_status_unknow;
    };
    
private:
    dd_source_task* m_task;
};
#endif /* dd_net_cache_hpp */
