//
//  dd_internel_def.hpp
//  down2memery
//
//  Created by dengchu on 2021/2/9.
//  Copyright © 2021 dengchu. All rights reserved.
//

#ifndef dd_internal_def_hpp
#define dd_internal_def_hpp

#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <queue>
#include <istream>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>
#include <algorithm>
#include <sys/stat.h>

using namespace std;

#define MEMORY_CACHE_SIZE 4*1024*1024
#define THREAD_COUNT 10

#define DD_MEMORY_CACHE_TYPE 0
#define DD_FILE_CACHE_TYPE 1
#define SEGMENT_SIZE 1024*1024

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
/*
io return code
 */
#define DD_FAILED -1
#define DD_EAGAIN -2
#define DD_INVALID -3
#define DD_EOF -4
#define DD_NO_MEMORY -5

typedef enum download_task_type{
    download_task_type_preload,
    download_task_type_play,
    download_task_type_storage
}download_task_type;

typedef enum task_status{
    task_status_unknow,
    task_status_init,
    task_status_probe,
    task_status_response,
    task_status_finish,
    task_status_error,
    task_status_stop
}task_status;

///params
class params_array{
public:
    params_array(){};
    ~params_array(){};
    
public:
    void param_set(const char* key, const char* value);
    const char* param_get(const char* key);
private:
    map<string, string> m_params;
};

static inline int64_t get_time(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

///command//
typedef void (*command_func)(void* user_data, const params_array* params);

class ins_ref{
public:
    ins_ref(){
        ref_count = 1;
    }
    
    ~ins_ref(){
        ref_count = 0;
    }
private:
    int ref_count;
public:
    int decrease_reference(){
        return __sync_sub_and_fetch(&ref_count, 1);
    }

    int increase_reference(){
        return __sync_add_and_fetch(&ref_count, 1);
    }
};

class ref_command : public ins_ref{

public:
    ref_command() {
        pthread_mutex_init(&command_mutex, NULL);
        pthread_cond_init(&command_cond, NULL);
        params = new params_array();
        cmd_func = NULL;
        user_data = NULL;
        finish = false;
    }
    
    ~ref_command() {
        cmd_func = NULL;
        user_data = NULL;
        params = NULL;
        pthread_cond_destroy(&command_cond);
        pthread_mutex_destroy(&command_mutex);
    }
    
public:
    bool finish;
    command_func cmd_func;
    void* user_data;
    params_array* params;
    
    pthread_mutex_t command_mutex;
    pthread_cond_t command_cond;
};

//ref_command池
template <class T>
class singleton{
public:
    static T* get_instance(){
        if (m_instance == NULL){
            pthread_mutex_lock(&m_instance_lock);
            if (m_instance == NULL){
                T* t = new T();
                m_instance = t;
            }
            pthread_mutex_unlock(&m_instance_lock);
        }
        return m_instance;
    };
    
    static void destroy_instance(){
        pthread_mutex_lock(&m_instance_lock);
        if (m_instance){
            delete m_instance;
            m_instance = NULL;
        }
        pthread_mutex_unlock(&m_instance_lock);
    };
private:
    singleton(){};
    ~singleton(){};
    singleton(const singleton& obj){};
    singleton& operator=(const singleton& obj){return *this;};
private:
    static T* m_instance;
    static pthread_mutex_t m_instance_lock;
};

template <class T>
T* singleton<T>::m_instance = NULL;

template <class T>
pthread_mutex_t singleton<T>::m_instance_lock = PTHREAD_MUTEX_INITIALIZER;

class ref_command_queue{
    friend class singleton<ref_command_queue>;
    
public:
    ref_command* alloc_command(){
        ref_command* obj = NULL;
        if (m_command_queue.size() <= 0){
            obj = new ref_command();
        }else{
            obj = m_command_queue.front();
            m_command_queue.pop_front();
        }
        
        return obj;
    };
    void free_command(ref_command* command){
        m_command_queue.push_back(command);
    };
private:
    ref_command_queue(){};
    ~ref_command_queue(){
        while(m_command_queue.size() > 0){
            ref_command *cmd = m_command_queue.front();
            m_command_queue.pop_front();
            if (cmd)
                delete cmd;
        }
    };
    ref_command_queue(const ref_command_queue& obj){};
    ref_command_queue& operator=(const ref_command_queue& obj){return *this;};
private:
    deque<ref_command*> m_command_queue;
};

///share_looper_command 的接口不是线程安全的,多线程同时操作同一个对象会有问题，模块或线程需要自己引用该对象（维护引用计数）
class share_command{
public:
    share_command(){
        ptr = NULL;
    }
    
    share_command(command_func _cmd_func, void* _user_data, va_list args){
        ptr = singleton<ref_command_queue>::get_instance()->alloc_command();
        ptr->cmd_func = _cmd_func;
        ptr->user_data = _user_data;
        
        if(ptr)
            ptr->increase_reference();
    }
    
    share_command& operator=(const share_command& obj){
        if (ptr && ptr->decrease_reference() <= 0){
            singleton<ref_command_queue>::get_instance()->free_command(ptr);
        }
        ptr = NULL;
        
        if (obj.ptr){
            ptr = obj.ptr;
            ptr->increase_reference();
        }
        return *this;
    }
    
    share_command(const share_command& obj){
        if (obj.ptr){
            ptr = obj.ptr;
            ptr->increase_reference();
        }
    }
    
    ~share_command(){
        if (ptr && ptr->decrease_reference() <= 0){
            singleton<ref_command_queue>::get_instance()->free_command(ptr);
        }
        ptr = NULL;
    }
public:
    ref_command* get_looper_command(){
        return ptr;
    }
    
private:
    ref_command* ptr;
};


#endif /* dd_internal_def_hpp */
