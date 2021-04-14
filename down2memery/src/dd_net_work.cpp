//
//  dd_net_work.cpp
//  down2memery
//
//  Created by dengchu on 2020/5/28.
//  Copyright © 2020 dengchu. All rights reserved.
//

#include "dd_net_work.hpp"
#include "dd_http_header.hpp"

static void url_split(char *proto, int proto_size,
                  char *authorization, int authorization_size,
                  char *hostname, int hostname_size,
                  int *port_ptr, char *path, int path_size, const char *url)
{
    const char *p, *ls, *ls2, *at, *at2, *col, *brk;
    
    if (port_ptr)
        *port_ptr = -1;
    if (proto_size > 0)
        proto[0] = 0;
    if (authorization_size > 0)
        authorization[0] = 0;
    if (hostname_size > 0)
        hostname[0] = 0;
    if (path_size > 0)
        path[0] = 0;
    
    /* parse protocol */
    if ((p = strchr(url, ':'))) {
        strlcpy(proto, url, min(proto_size, int(p + 1 - url)));
        p++; /* skip ':' */
        if (*p == '/')
            p++;
        if (*p == '/')
            p++;
    } else {
        /* no protocol means plain filename */
        strlcpy(path, url, path_size);
        return;
    }
    
    /* separate path from hostname */
    ls = strchr(p, '/');
    ls2 = strchr(p, '?');
    if (!ls)
        ls = ls2;
    else if (ls && ls2)
        ls = min(ls, ls2);
    if (ls)
        strlcpy(path, ls, path_size);
    else
        ls = &p[strlen(p)];  // XXX
    
    /* the rest is hostname, use that to parse auth/port */
    if (ls != p) {
        /* authorization (user[:pass]@hostname) */
        at2 = p;
        while ((at = strchr(p, '@')) && at < ls) {
            strlcpy(authorization, at2,
                       min(authorization_size, int(at + 1 - at2)));
            p = at + 1; /* skip '@' */
        }
        
        if (*p == '[' && (brk = strchr(p, ']')) && brk < ls) {
            /* [host]:port */
            strlcpy(hostname, p + 1,
                       min(hostname_size, int(brk - p)));
            if (brk[1] == ':' && port_ptr)
                *port_ptr = atoi(brk + 2);
        } else if ((col = strchr(p, ':')) && col < ls) {
            strlcpy(hostname, p,
                       min(int(col + 1 - p), hostname_size));
            if (port_ptr)
                *port_ptr = atoi(col + 1);
        } else
            strlcpy(hostname, p,
                       min(int(ls + 1 - p), hostname_size));
    }
}

static int get_line(int fd, char* buf, int buf_len)
{
    int  count = 0;
    memset(buf, 0, buf_len);
    while (recv(fd, buf + count, 1, 0) == 1)
    {
        if (buf[count] == '\n')
            return count + 1;
        if (count < (buf_len - 2))
            count++;
    }
    return count;
}


dd_net_work::dd_net_work(get_task_cb cb, void* opaque){
    m_cb = cb;
    m_opaque = opaque;
    m_abort = 0;
    pthread_create(&m_thread, NULL, cycle, this);
}

dd_net_work::~dd_net_work(){
    
}

void* dd_net_work::cycle(void* args){
    if (args)
        ((dd_net_work*)args)->do_cycle();
    return NULL;
}

void dd_net_work::do_cycle(){
    struct addrinfo hints, *servinfo, *p;
    int sockfd, rv;

    char hostname[1024],proto[1024],path[1024];
    char portstr[10];
    int port;

    printf("===================start thread(%lld)\n", (int64_t)pthread_self());
    while (!m_abort) {
        int size = 0;
        int64_t range_start = -1, range_end = -1, had_read = 0;
        const dd_task_config* config = NULL;
        dd_share_task task = m_cb(m_opaque);
        config = task.get_config();
        if (!config){
            usleep(5000);
            continue;
        }
            
        int ret = task.get_download_range(range_start, size);
        if (ret <= 0){
            usleep(5000);
            continue;
        }
        if (size > 0)
            range_end = range_start + size - 1;
        
        url_split(proto, sizeof(proto), NULL, 0, hostname, sizeof(hostname),
                  &port, path, sizeof(path), config->m_url.c_str());
        snprintf(portstr, sizeof(portstr), "%d", port);
        
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if((rv = getaddrinfo(hostname, portstr, &hints, &servinfo)) != 0){
            continue;;
        }
        
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                ::close(sockfd);
                continue;
            }
            break;
        }
        freeaddrinfo(servinfo);
        
        string requst_header;
        requst_header.append("GET ").append(path).append(" HTTP/1.1\r\n");
        requst_header.append("User-Agent: dd_down\r\n");
        requst_header.append("Accept: */*\r\n");
        char offset[64] = {0};
        if (range_start >= 0){
             sprintf(offset, "%lld", range_start);
             requst_header.append("Range: bytes=").append(offset).append("-");
        }
        if(range_end >= 0){
            sprintf(offset, "%lld", range_end);
            requst_header.append(offset);
        }
        if (range_start >= 0)
            requst_header.append("\r\n");
        requst_header.append("Connection: close\r\n");
        requst_header.append("Host: ").append(hostname).append("\r\n");
        requst_header.append("\r\n");
        
        const char* buf = requst_header.c_str();
        if(send(sockfd, buf, strlen(buf), 0) == -1){
            ::close(sockfd);
            continue;;
        }
        //printf("===== %s", buf);
        
        //parse header
        char *ptr, *attrName, *attrValue;
        char recv_buf[1024*32];
        HeadData response;

        ret = get_line(sockfd, recv_buf, 1024*32);
        if (ret < 0) continue;
        if (ret == 0) continue;
        //printf("===== %s", recv_buf);

        ptr = recv_buf;
        while (*ptr == ' ') ptr++;
        // skip the http version info
        while (*ptr != '\0' && *ptr != ' ' && *ptr != '\r' && *ptr != '\n') ptr++;
        if (*ptr != ' ') continue;
        while (*ptr == ' ') ptr++;
        int statusCode = atoi(ptr);

        while (1) {
            ret = get_line(sockfd, recv_buf, 1024*32);
            if (ret < 0) continue;
            if (ret == 0) continue;
            //printf("===== %s", recv_buf);

            ptr = recv_buf;
            while (*ptr == ' ') ptr++;
            attrName = ptr;
            if (*ptr == '\r' || *ptr == '\n') break;//end of header
            while (*ptr != '\0' && *ptr != ':' && *ptr != '\r' && *ptr != '\n') ptr++;
            if (*ptr == ':') {
                *ptr = '\0';
                ptr++;
            } else {
                continue;
            }

            while (*ptr == ' ') ptr++;
            attrValue = ptr;
            while (*ptr != '\0' && *ptr != '\r' && *ptr != '\n') ptr++;
            *ptr = '\0';

            response.set_attr(attrName, attrValue);
        }
        //parse header
        
        task.net_response(&response);
        
        int len = 0;
        while(true){
            len = recv(sockfd, recv_buf, 1024*32, 0);
            if (len <= 0)
                break;
            task.net_data(recv_buf, len, range_start + had_read);
            had_read += len;
        }
        ::close(sockfd);
        
        if (len == 0){
            task.net_finish();
        }else{
            task.net_error( len, range_start + had_read);
        }
    }
    printf("===================exit thread(%lld),(m_abort:%d)\n", (int64_t)pthread_self(), m_abort);
}
