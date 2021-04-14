//
//  main.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/22.
//  Copyright © 2020 dengchu. All rights reserved.
//

#include <iostream>
#include "dd_cache_index.hpp"
#include "dd_task_manager.hpp"


int main(int argc, const char * argv[]) {
    // insert code here...
    dd_task_manager* manager = new dd_task_manager();
    manager->create_task(download_task_type_play, "http:10.222.69.33:8087/360test.mp4", "12345678", "/Users/dengchu/Downloads", 0, -1);
    
    char buf[1024] = {0};
    while (1) {
        
        int ret = manager->read_data("12345678", (uint8_t*) buf, 1024);
        if (ret == 0){
            manager->destory_task("12345678");
            break;
        }
        usleep(10000);
    }
    
    while (1) {
        usleep(10000);
    }
}
