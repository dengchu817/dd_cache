//
//  dd_file_cache.hpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_file_cache_hpp
#define dd_file_cache_hpp

#include "dd_internal_def.hpp"
#include "dd_cache_index.hpp"
class dd_file_cache{
public:
    dd_file_cache();
    ~dd_file_cache();
public:
    int init(const char* path, dd_indexs_entry* entry);
    void release(dd_indexs_entry* entry);
    int write(const uint8_t* data, int size, int64_t &offset_in_cache);
    int read(uint8_t* data, int size, int64_t offset_in_cache);
private:
    string m_path;
    FILE* m_media_file;
};
#endif /* dd_file_cache_hpp */
