//
//  dd_net_cache.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/28.
//  Copyright © 2020 dengchu. All rights reserved.
//

#include "dd_share_task.hpp"

dd_source_task::dd_source_task(download_task_type type, const char* url, const char* key, const char* cache_path,int64_t range_start, int64_t range_end):m_config(type, url, key, cache_path, range_start, range_end){
    pthread_mutex_init(&m_mutex, NULL);
    m_status = task_status_init;
    m_read_offset = range_start;
    m_cache = new dd_cache_unit();
    m_cache->init(cache_path);
}

dd_source_task::~dd_source_task(){
    printf("=====dc release %s task\n", m_config.m_key.c_str());
    if (m_cache){
        m_cache->release();
        delete m_cache;
    }
    pthread_mutex_destroy(&m_mutex);
}

void dd_source_task::stop(){
    pthread_mutex_lock(&m_mutex);
    m_status = task_status_stop;
    pthread_mutex_unlock(&m_mutex);
}

void dd_source_task::net_response(HeadData* response){
    pthread_mutex_lock(&m_mutex);
    
    // get fileSize in bytes
    int fileSize = 0;
    char* ptr = (char*)response->get_attr("Content-Range");
    if (ptr != NULL) {
        while (ptr && *ptr != '/')
            ptr++;
        ptr++;
        for (fileSize = 0; *ptr >= '0' && *ptr <= '9'; ptr++) {
            fileSize = fileSize * 10 + (*ptr - '0');
        }
    } else {
        fileSize = -1;
    }

    // get Transfer-Encoding
    ptr = (char*)response->get_attr("Transfer-Encoding");
    if (ptr != NULL && strcasecmp(ptr, "chunked") == 0) {
//        isChunked = true;
//        chunkedSize = 0;
        m_cache->set_file_size(-1);
    } else if (fileSize >= 0) {
//        isChunked = false;
//        chunkedSize = fileSize;
        m_cache->set_file_size(fileSize);
    } else {  // some http proxyer dose not return the content-size when the file is large
//        isChunked = false;
//        chunkedSize = -1;
    }
    pthread_mutex_unlock(&m_mutex);
}

void dd_source_task::net_data(void* data, int size, int64_t offset){
    pthread_mutex_lock(&m_mutex);
    m_cache->write((uint8_t*)data, size, offset);
    pthread_mutex_unlock(&m_mutex);
}

void dd_source_task::net_finish(){
    pthread_mutex_lock(&m_mutex);
    pthread_mutex_unlock(&m_mutex);
}

void dd_source_task::net_error(int error, int64_t offset){
    pthread_mutex_lock(&m_mutex);
    pthread_mutex_unlock(&m_mutex);
}

int dd_source_task::read_data(void* data, int size){
    if (!m_cache)
        return -1;
    pthread_mutex_lock(&m_mutex);
    int ret = m_cache->read((uint8_t*)data, size, m_read_offset);
    if (ret > 0)
        m_read_offset += ret;
    pthread_mutex_unlock(&m_mutex);
    return ret;
}

int dd_source_task::get_download_range(int64_t& offset_in_stream, int& size){
    int ret = -1;
    if (!m_cache)
        return ret;
    pthread_mutex_lock(&m_mutex);
    if (m_cache->get_file_size() == 0x7fffffffffffffff){
        if (m_status == task_status_probe){
            pthread_mutex_unlock(&m_mutex);
            return -1;
        }else{
            m_status = task_status_probe;
            offset_in_stream = 0;
            size = 1;
            pthread_mutex_unlock(&m_mutex);
            return size;
        }
    }
    
    ret = m_cache->get_download_range(m_read_offset, offset_in_stream, size);
    pthread_mutex_unlock(&m_mutex);
    return ret;
}

const dd_task_config* dd_source_task::get_config(){
    return &m_config;
}

task_status dd_source_task::get_status(){
    return m_status;
}
