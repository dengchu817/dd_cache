//
//  dd_file_cache.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//
#include "json.h"
#include "dd_file_cache.hpp"

dd_file_cache::dd_file_cache(){
    m_media_file = NULL;
}

dd_file_cache::~dd_file_cache(){
    
}

int dd_file_cache::init(const char* path, dd_indexs_entry* entry){
    if (access(path, F_OK)) {//目录不存在
        if (mkdir(path, 0777)) {//创建目录
            return -1;
        }
    }
    char info_path[1024] = {0};
    sprintf(info_path, "%s/cacheinfo", path);
    
    if (!access(info_path, F_OK)){
        FILE* infofd = fopen(info_path, "rb+");
        if (infofd){
            fseek(infofd, 0L, SEEK_END);
            long infofd_size=ftell(infofd);
            fseek(infofd, 0L, SEEK_SET);
            uint8_t* temp_buf = (uint8_t*)calloc(1, infofd_size);
            fread(temp_buf, 1, infofd_size, infofd);
            
            cJSON* root = cJSON_Parse((char *)temp_buf);
            if (root){
                cJSON* file_size_json = cJSON_GetObjectItem(root, "file_size");
                if (file_size_json){
                    entry->file_size = file_size_json->valuedouble;
                }
                
                cJSON* valid_seg = cJSON_GetObjectItem(root, "valid_seg");
                if (valid_seg){
                    int arr_size = cJSON_GetArraySize(valid_seg);
                    for(int i = 0; i < arr_size; i++){
                        cJSON* elements = cJSON_GetArrayItem(valid_seg, i);
                        if (elements) {
                            cJSON* offset_in_stream = cJSON_GetObjectItem(elements, "offset_in_stream");
                            cJSON* offset_in_cache = cJSON_GetObjectItem(elements, "offset_in_cache");
                            cJSON* size = cJSON_GetObjectItem(elements, "size");
                            if (offset_in_stream && offset_in_stream && size &&
                                offset_in_stream->valuedouble >= 0 && offset_in_stream->valuedouble >= 0 && size->valuedouble > 0){
                                add_cache_index(entry, offset_in_stream->valuedouble, size->valuedouble, offset_in_cache->valuedouble, size->valuedouble, DD_FILE_CACHE_TYPE);
                            }
                        }
                    }
                }
                cJSON_Delete(root);
            }
            free(temp_buf);
            fclose(infofd);
        }
    }
    //
    char data_path[104] = {0};
    sprintf(data_path, "%s/segment", path);
    if (entry->valid_count <= 0){//没有cache
        m_media_file = fopen(data_path, "wb+");//wb+覆盖写 rb+保留上次的，追加写，但文件必须存在
    }else{
        m_media_file = fopen(data_path, "rb+");
    }
    m_path = path;
    return 0;
}

void dd_file_cache::release(dd_indexs_entry* entry){
    char info_path[1024] = {0};
    sprintf(info_path, "%s/cacheinfo", m_path.c_str());
    cJSON * root = cJSON_CreateObject();
    if (root){
        cJSON* file_js = cJSON_CreateNumber(entry->file_size);
        cJSON_AddItemToObject(root, "file_size",  file_js);
        
        cJSON * array = cJSON_CreateArray();
        for (int i = 0; i < entry->valid_count; i++){
            if (entry->indexs[i].type == DD_FILE_CACHE_TYPE){
                cJSON * item = cJSON_CreateObject();
                
                cJSON* offset_in_stream = cJSON_CreateNumber(entry->indexs[i].offset_in_stream);
                cJSON_AddItemToObject(item, "offset_in_stream", offset_in_stream);
                cJSON* offset_in_cache = cJSON_CreateNumber(entry->indexs[i].offset_in_cache);
                cJSON_AddItemToObject(item, "offset_in_cache", offset_in_cache);
                cJSON* size = cJSON_CreateNumber(entry->indexs[i].capacity);
                cJSON_AddItemToObject(item, "size", size);
                cJSON_AddItemToArray(array, item);
            }
        }
        
        cJSON_AddItemToObject(root, "valid_seg",  array);
        FILE* infofd = fopen(info_path, "wb+");
        if (infofd){
            char* json_text = cJSON_Print(root);
            fwrite(json_text, 1, strlen(json_text), infofd);
            fclose(infofd);
        }
        cJSON_Delete(root);
    }
    //
    if (m_media_file)
        fclose(m_media_file);
    m_media_file = NULL;
}

int dd_file_cache::write(const uint8_t* data, int size, int64_t &offset_in_cache){
    int ret = -1;
    if (!m_media_file)
        return ret;
    fseek(m_media_file, 0L, SEEK_END);
    offset_in_cache = ftell(m_media_file);
    ret = fwrite(data, 1, size, m_media_file);
    return ret;
}

int dd_file_cache::read(uint8_t* data, int size, int64_t offset_in_cache){
    int ret = -1;
    if (!m_media_file)
        return ret;
    fseek(m_media_file, offset_in_cache, SEEK_SET);
    ret = fread(data, 1, size, m_media_file);
    return ret;
}
