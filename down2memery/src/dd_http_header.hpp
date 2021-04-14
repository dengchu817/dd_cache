//
//  dd_http_header.hpp
//  down2memery
//
//  Created by dengchu on 2021/4/6.
//  Copyright © 2021 dengchu. All rights reserved.
//

#ifndef dd_http_header_hpp
#define dd_http_header_hpp

#include <stdio.h>
class HeadDataNode {
    public:
        HeadDataNode():attrName(NULL), attrValue(NULL), next(NULL) {}
        HeadDataNode(const HeadDataNode &that);
        ~HeadDataNode();
        HeadDataNode& operator = (const HeadDataNode &that);

        const char *attrName;
        const char *attrValue;
        HeadDataNode *next;
};

class HeadData {
    public:
        HeadData():head(NULL) {}
        HeadData(const HeadDataNode &that);
        ~HeadData();
        HeadData& operator = (const HeadData&that);

        // set a attrib property if not exist, add one
        int set_attr(const char *attrName, const char *attrValue);
        // get the attribValue identified by attrName;
        const char* get_attr(const char *attrName);
        // remote the attr identified by attrName;
        int remove_attr(const char *attrName);
        // traversal the data use trav_fun
        int traversal(int(*trav_fun)(HeadDataNode*));
        // remote all the atts
        void remove_all();

        HeadDataNode *head;
};
#endif /* dd_http_header_hpp */
