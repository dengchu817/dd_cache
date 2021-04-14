//
//  dd_cache_index.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//
#include "dd_cache_index.hpp"

static int find_entry_index(dd_indexs_entry* indexs_entry, int64_t offset_in_stream, bool backward){
    int a = -1, b = indexs_entry->valid_count, m = -1;
    while (b - a > 1) {
        m = (a + b) >> 1;
        if (indexs_entry->indexs[m].offset_in_stream <= offset_in_stream)
            a = m;
        if (indexs_entry->indexs[m].offset_in_stream >= offset_in_stream)
            b = m;
    }
    if (backward)
        return a;
    else
        return b;
}

//static int merge_index(dd_indexs_entry* indexs_entry, int index){
//    int merege_index = index-1;
//    
//    for (int i = 0; i < 2; i++){
//        if (merege_index >= 0 && merege_index + 1 < indexs_entry->valid_count && indexs_entry->indexs[merege_index].file_size >= indexs_entry->indexs[merege_index].capacity && indexs_entry->indexs[merege_index+1].file_size >= indexs_entry->indexs[merege_index+1].capacity){
//            int64_t stream_drift = indexs_entry->indexs[merege_index+1].offset_in_stream - indexs_entry->indexs[merege_index].offset_in_stream;
//            int64_t cache_drift = indexs_entry->indexs[merege_index+1].file_offset - indexs_entry->indexs[merege_index].file_offset;
//            if (stream_drift == cache_drift && stream_drift <= indexs_entry->indexs[merege_index].capacity){
//                indexs_entry->indexs[merege_index].capacity = indexs_entry->indexs[merege_index].file_size = indexs_entry->indexs[merege_index+1].offset_in_stream +  indexs_entry->indexs[merege_index+1].capacity - indexs_entry->indexs[merege_index].offset_in_stream;
//                if (indexs_entry->valid_count - merege_index - 2 > 0)
//                    memmove(indexs_entry->indexs + merege_index + 1, indexs_entry->indexs + merege_index + 2, sizeof(dd_index_item) * (indexs_entry->valid_count - merege_index - 2));
//                indexs_entry->valid_count--;
//                continue;
//            }
//        }
//        merege_index++;
//    }
//    return 0;
//}

int add_cache_index(dd_indexs_entry* indexs_entry, int64_t offset_in_stream, int capacity, int64_t offset_in_cache, int valid_size, int type){
    printf("===== add index(type:%d,%lld--%lld)\n",type, offset_in_stream, offset_in_stream+capacity-1);
    int a = find_entry_index(indexs_entry, offset_in_stream, true);
    //insert
    int insert_index = a+1;
    if (indexs_entry->alloc_count < indexs_entry->valid_count + 1){
        indexs_entry->alloc_count = FFMAX(indexs_entry->valid_count + 1, indexs_entry->alloc_count*2);
        indexs_entry->indexs = (dd_index_item*)realloc(indexs_entry->indexs, indexs_entry->alloc_count*sizeof(dd_index_item));
    }
    if (indexs_entry->valid_count - insert_index > 0)
        memmove(indexs_entry->indexs + insert_index + 1, indexs_entry->indexs + insert_index, sizeof(dd_index_item) * (indexs_entry->valid_count - insert_index));
    indexs_entry->indexs[insert_index].offset_in_stream = offset_in_stream;
    indexs_entry->indexs[insert_index].capacity = capacity;
    indexs_entry->indexs[insert_index].offset_in_cache = offset_in_cache;
    indexs_entry->indexs[insert_index].valid_size = valid_size;
    indexs_entry->indexs[insert_index].type = type;
    indexs_entry->valid_count++;
    //merge_index(indexs_entry, insert_index);
    return 0;
}

int modify_cache_index(dd_indexs_entry* indexs_entry, dd_index_item* item, int read_size, int64_t offset_in_cache){
    int a = item - (dd_index_item*)indexs_entry->indexs;
    bool merge_index = false;
    if (a-1 >= 0 && indexs_entry->indexs[a-1].type == DD_FILE_CACHE_TYPE){
        int64_t stream_drift = item->offset_in_stream - indexs_entry->indexs[a-1].offset_in_stream;
        int64_t cache_drift = offset_in_cache - indexs_entry->indexs[a-1].offset_in_cache;
        if (stream_drift == cache_drift && stream_drift <= indexs_entry->indexs[a-1].capacity){
            indexs_entry->indexs[a-1].valid_size = indexs_entry->indexs[a-1].capacity = item->offset_in_stream + read_size - indexs_entry->indexs[a-1].offset_in_stream;
            merge_index = true;
        }
    }
    
    if (!merge_index){
        if (indexs_entry->alloc_count < indexs_entry->valid_count + 1){
            indexs_entry->alloc_count = FFMAX(indexs_entry->valid_count + 1, indexs_entry->alloc_count*2);
            indexs_entry->indexs = (dd_index_item*)realloc(indexs_entry->indexs, indexs_entry->alloc_count*sizeof(dd_index_item));
        }
        if (indexs_entry->valid_count - a > 0)
            memmove(indexs_entry->indexs + a + 1, indexs_entry->indexs + a, sizeof(dd_index_item) * (indexs_entry->valid_count - a));
        indexs_entry->indexs[a].offset_in_stream = indexs_entry->indexs[a+1].offset_in_stream;
        indexs_entry->indexs[a].valid_size = indexs_entry->indexs[a].capacity = read_size;
        indexs_entry->indexs[a].offset_in_cache = offset_in_cache;
        indexs_entry->indexs[a].type = DD_FILE_CACHE_TYPE;
        indexs_entry->valid_count++;
        item = &indexs_entry->indexs[a + 1];
        a++;
    }
    
    item->offset_in_stream += read_size;
    item->offset_in_cache   = item->offset_in_cache + read_size;
    item->capacity         -= read_size;
    item->valid_size       -= read_size;
    if (item->capacity <= 0){
        if (indexs_entry->valid_count - a - 1 > 0)
            memmove(item, item + 1, sizeof(dd_index_item)*(indexs_entry->valid_count - a - 1));
        indexs_entry->valid_count--;
    }

    return 0;
}

dd_index_item* find_cache_index(dd_indexs_entry* indexs_entry, int64_t offset_in_stream){
    int a = find_entry_index(indexs_entry, offset_in_stream, true);
    
    if (a >= 0 && a < indexs_entry->valid_count && indexs_entry->indexs[a].offset_in_stream <= offset_in_stream && offset_in_stream <= indexs_entry->indexs[a].offset_in_stream + indexs_entry->indexs[a].capacity - 1)
        return &indexs_entry->indexs[a];
    return NULL;
}

int get_cache_gap(dd_indexs_entry* indexs_entry, int64_t start_offset, int64_t &offset_in_stream, int &size){
    int a = find_entry_index(indexs_entry, offset_in_stream, true);
    
    while(a >= 0 && a < indexs_entry->valid_count - 1){
        int64_t offset_end_a = indexs_entry->indexs[a].offset_in_stream + indexs_entry->indexs[a].capacity;
        int64_t offset_end_a1 = indexs_entry->indexs[a+1].offset_in_stream;
        if (offset_end_a >= offset_end_a1)
            a++;
        else
            break;
    }
    
    int64_t offset_in_stream_start = start_offset;
    int64_t offset_in_stream_end = indexs_entry->file_size - 1;
    
    if (a >= 0 && a < indexs_entry->valid_count){
        offset_in_stream_start = FFMAX(start_offset, indexs_entry->indexs[a].offset_in_stream + indexs_entry->indexs[a].capacity);
    }
    if (a >= 0 && a+1 < indexs_entry->valid_count){
        offset_in_stream_end = indexs_entry->indexs[a+1].offset_in_stream - 1;
    }
    offset_in_stream = offset_in_stream_start;
    size = (int)(offset_in_stream_end - offset_in_stream_start + 1);
    return 0;
}

dd_indexs_entry* create_index_entry(){
    dd_indexs_entry* entry = (dd_indexs_entry*)calloc(1, sizeof(dd_indexs_entry));
    entry->file_size = 0x7fffffffffffffff;
    return entry;;
}

void release_index_entry(dd_indexs_entry** entry){
    if (!entry || !*entry)
        return;
    if ((*entry)->indexs)
        free((*entry)->indexs);
    free(*entry);
    *entry = NULL;
}
