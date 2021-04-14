//
//  dd_cache_manager.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//

#include "dd_cache_unit.hpp"

dd_cache_unit::dd_cache_unit(){
    m_entry = NULL;
    m_memory_cache = NULL;
    m_file_cache = NULL;
}

dd_cache_unit::~dd_cache_unit(){
    
}

int dd_cache_unit::init(const char* path){
    int ret = -1;
    m_entry = create_index_entry();
    
    m_file_cache = new dd_file_cache();
    ret = m_file_cache->init(path, m_entry);
    if (ret < 0)
        return ret;
    m_memory_cache = new dd_memory_cache();
    ret = m_memory_cache->init(1024*1024*10);//10M
    return ret;
}

void dd_cache_unit::release(){
    if (m_file_cache){
        m_file_cache->release(m_entry);
        delete m_file_cache;
        m_file_cache = NULL;
    }
    if (m_memory_cache){
        m_memory_cache->release();
        delete m_memory_cache;
        m_memory_cache = NULL;
    }
    if (m_entry)
        release_index_entry(&m_entry);
}

int dd_cache_unit::get_download_range(int64_t start_offset, int64_t &offset_in_stream, int &size){
    int64_t gap_start = 0;
    int gap_size = 0 , ret = -1;
    get_cache_gap(m_entry, start_offset, gap_start, gap_size);
    if (gap_size < 0){//file size < 0
        offset_in_stream = gap_start;
        size = FFMIN(gap_size, SEGMENT_SIZE);
        int64_t offset_in_cache = 0;
        if ((ret = m_memory_cache->get_block(size, offset_in_cache)) > 0){
            add_cache_index(m_entry, offset_in_stream, ret, offset_in_cache, 0, DD_MEMORY_CACHE_TYPE);
        }
    }else if (gap_size == 0){//finish
        ret = 0;
    }else{
        offset_in_stream = gap_start;
        size = FFMIN(gap_size, SEGMENT_SIZE);
        int64_t offset_in_cache = 0;
        if ((ret = m_memory_cache->get_block(size, offset_in_cache)) > 0){
            add_cache_index(m_entry, offset_in_stream, ret, offset_in_cache, 0, DD_MEMORY_CACHE_TYPE);
        }
    }
    return ret;
}

int dd_cache_unit::write(uint8_t* data, int size, int64_t offset_in_stream){
    int ret = -1;
    dd_index_item* item = find_cache_index(m_entry, offset_in_stream);
    if (item && m_memory_cache){
        ret = m_memory_cache->write_to_block(data, size, item->offset_in_cache + (offset_in_stream - item->offset_in_stream));
        item->valid_size += ret;
    }
    return ret;
}

int dd_cache_unit::read(uint8_t* data, int size, int64_t offset_in_stream){
    int ret = -1;
    if (m_entry->file_size > 0 && offset_in_stream >= m_entry->file_size)
        return 0;
    dd_index_item* item = find_cache_index(m_entry, offset_in_stream);
    if (!item)
        return ret;
    int valid = item->valid_size - (offset_in_stream - item->offset_in_stream);
    if (valid <= 0)
        return ret;
    int read_size = FFMIN(valid, size);
    if (item->type == DD_MEMORY_CACHE_TYPE){
        ret = m_memory_cache->read(data, read_size);
        if (ret > 0){
            int64_t offset_in_file = 0;
            int len = m_file_cache->write(data, ret, offset_in_file);
            if (len > 0){
                modify_cache_index(m_entry, item, len, offset_in_file);
            }
        }
    }else{
        ret = m_file_cache->read(data, read_size, item->offset_in_cache + (offset_in_stream - item->offset_in_stream));
    }
    
    return ret;
}
