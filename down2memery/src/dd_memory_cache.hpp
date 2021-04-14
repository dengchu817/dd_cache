//
//  dd_memory_cache.hpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_memory_cache_hpp
#define dd_memory_cache_hpp

#include "dd_internal_def.hpp"
#include "dd_cache_index.hpp"
class dd_memory_cache{
public:
    dd_memory_cache();
    ~dd_memory_cache();
public:
    int init(int size);
    void release();
    int get_block(int size, int64_t &offset_in_cache);
    int write_to_block(const uint8_t* data, int size, int64_t offset_in_cache);
    int read(uint8_t* data, int size);
private:
    uint8_t* m_buf;
    int m_buf_size;
    int m_read_index;
    int m_write_index;
};

#endif /* dd_memory_cache_hpp */
