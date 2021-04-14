//
//  dd_cache_manager.hpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_cache_manager_hpp
#define dd_cache_manager_hpp
#include "dd_internal_def.hpp"
#include "dd_cache_index.hpp"
#include "dd_file_cache.hpp"
#include "dd_memory_cache.hpp"
class dd_cache_unit{
public:
    dd_cache_unit();
    ~dd_cache_unit();
    
public:
    int init(const char* path);
    void release();
    int get_download_range(int64_t start_offset, int64_t &offset_in_stream, int &size);
    int write(uint8_t* data, int size, int64_t offset_in_stream);//net callback
    int read(uint8_t* data, int size, int64_t offset_in_stream);//play read
    inline void set_file_size(int file_size){
        if (m_entry){
            m_entry->file_size = file_size;
        }
    }
    inline int64_t get_file_size(){
        if (m_entry)
            return m_entry->file_size;
        return -1;
    }
private:
    dd_indexs_entry *m_entry;
    dd_memory_cache *m_memory_cache;
    dd_file_cache *m_file_cache;
};

#endif /* dd_cache_manager_hpp */
