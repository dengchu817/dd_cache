//
//  dd_http_header.cpp
//  down2memery
//
//  Created by dengchu on 2021/4/6.
//  Copyright © 2021 dengchu. All rights reserved.
//

#include "dd_http_header.hpp"
#include <cstring>
#include <cassert>

#include "dd_http_header.hpp"

HeadDataNode::~HeadDataNode() {
    delete[] attrName;
    delete[] attrValue;
};


HeadData::~HeadData() {
    HeadDataNode *it;
    HeadDataNode *tmp;
    for (it = head; it != NULL;) {
        tmp = it->next;
        delete it;
        it = tmp;
    }
    head = NULL;
};

const char* HeadData::get_attr(const char *attrName) {
    HeadDataNode *it;

    assert(attrName != NULL);

    for (it = head; it != NULL; it = it->next) {
        if (strcasecmp(attrName, it->attrName) == 0)
            return it->attrValue;
    }

    return NULL;
};

int HeadData::set_attr(const char *attrName, const char *attrValue) {
    HeadDataNode *it;

    assert(attrName != NULL && attrValue != NULL);

    for (it = head; it != NULL; it = it->next) {
        if (strcasecmp(attrName, it->attrName) == 0)
            break;
    }

    if (it != NULL) {
        delete[] it->attrValue;
        it->attrValue = strdup(attrValue);
        return 0;
    } else {
        it = new HeadDataNode();
        it->attrName = strdup(attrName);
        it->attrValue = strdup(attrValue);
        it->next = head;
        head = it;
        return 1;
    }
};

int HeadData::remove_attr(const char *attrName) {
    HeadDataNode *it;

    assert(attrName != NULL);

    for (it = head; it != NULL; it = it->next) {
        if (strcasecmp(attrName, it->attrName) == 0)
            break;
    }

    if (it != NULL) {
        if (it == head) {
            head = it->next;
            delete it;
        } else {
            HeadDataNode *pre;
            for (pre = head; pre->next != it; it = it->next) ;
            pre->next = it->next;
            delete it;
        }
        return 0;
    } else {
        return 1;
    }
}

void HeadData::remove_all() {
    HeadDataNode *it;
    HeadDataNode *tmp;
    for (it = head; it != NULL;) {
        tmp = it->next;
        delete it;
        it = tmp;
    }
    head = NULL;
}

int HeadData::traversal(int(*trav_fun)(HeadDataNode*)) {
    HeadDataNode *it;
    int ret;

    for (it = head; it != NULL; it = it->next) {
        if ((ret=trav_fun(it)) != 0) return ret;
    }

    return 0;
}

