//
//  dd_cache_index.hpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//

#ifndef dd_cache_index_hpp
#define dd_cache_index_hpp

#include "dd_internal_def.hpp"

typedef struct dd_index_item{
    int type;
    int capacity;
    int64_t offset_in_stream;
    int64_t offset_in_cache;
    int valid_size;
}dd_index_item;

typedef struct dd_indexs_entry{
    dd_index_item* indexs;
    int64_t file_size;
    int alloc_count;
    int valid_count;
}dd_indexs_entry;

dd_indexs_entry* create_index_entry();

void release_index_entry(dd_indexs_entry** entry);

int add_cache_index(dd_indexs_entry* indexs_entry, int64_t offset_in_stream, int capacity, int64_t offset_in_cache, int valid_size, int type);

int modify_cache_index(dd_indexs_entry* indexs_entry, dd_index_item* item, int read_size, int64_t offset_in_cache);

dd_index_item* find_cache_index(dd_indexs_entry* indexs_entry, int64_t offset_in_stream);

int get_cache_gap(dd_indexs_entry* indexs_entry, int64_t start_offset, int64_t &offset_in_stream, int &size);

#endif /* dd_cache_index_hpp */
