//
//  dd_memory_cache.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//
#include "dd_memory_cache.hpp"

dd_memory_cache::dd_memory_cache(){
    m_buf = NULL;
    m_buf_size = m_read_index = m_write_index = 0;
}

dd_memory_cache::~dd_memory_cache(){
    
}

int dd_memory_cache::init(int size){
    m_buf_size = size;//10MB
    m_buf = (uint8_t*)calloc(1, m_buf_size);
    m_read_index = m_write_index = 0;
    return 0;
}

void dd_memory_cache::release(){
    if (m_buf)
        free(m_buf);
}

int dd_memory_cache::get_block(int size, int64_t &offset_in_cache){
    int ret = -1, free_size = 0;
    if (m_write_index < m_read_index){
        free_size = m_read_index - m_write_index - 1;
    }else{
        free_size = m_buf_size - (m_write_index - m_read_index) - 1;
    }
    
    if (size < 0)
        size = free_size/2;
    
    if (free_size >= size){
        ret = size;
        offset_in_cache = m_write_index;
        m_write_index = (m_write_index + size) % m_buf_size;
    }
    return ret;
}

int dd_memory_cache::write_to_block(const uint8_t *data, int size, int64_t offset_in_cache){
    int ret = size;
    offset_in_cache = offset_in_cache % m_buf_size;
    if (offset_in_cache + size <= m_buf_size){
        memcpy(m_buf + offset_in_cache, data, size);
    }else{
        memcpy(m_buf + offset_in_cache, data, m_buf_size - offset_in_cache);
        memcpy(m_buf, data + m_buf_size - offset_in_cache, size - m_buf_size + offset_in_cache);
    }
    return ret;
}

int dd_memory_cache::read(uint8_t *data, int size){
    int busy_size = 0;
    if (m_write_index < m_read_index){
        busy_size = m_buf_size - (m_read_index - m_write_index);
    }else{
        busy_size = m_write_index - m_read_index;
    }
    
    if (busy_size < size)
        return -1;
    
    if (m_read_index + size <= m_buf_size){
        memcpy(data, m_buf + m_read_index, size);
    }else{
        memcpy(data, m_buf + m_read_index, m_buf_size - m_read_index);
        memcpy(data + m_buf_size - m_read_index, m_buf, size - m_buf_size + m_read_index);
    }
    m_read_index = (m_read_index + size) % m_buf_size;
    return size;
}
