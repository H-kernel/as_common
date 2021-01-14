#include "as_url.h"
void       as_init_url(as_url_t* url)
{
    url->protocol[0] = '\0';
    url->username[0] = '\0';
    url->password[0] = '\0';
    url->host[0]     = '\0';
    url->port        = 0;
    url->path[0]     = '\0';
    url->uri[0]      = '\0';
    url->args[0]     = '\0';
    
}
int32_t    as_parse_url(const char* url,as_url_t* info)
{
    if(NULL == url) {
        return AS_ERROR_CODE_FAIL;
    }
    uint32_t len = 0;
    char* data = (char*)url;
    /* find protocol */
    char* idx = strstr(data,"://");
    if(idx == NULL) {
        strncpy(info->protocol,"unknow",AS_ULR_PROTOCOL_LEN);
    }
    else {
        len = idx - data;
        if(len >= AS_ULR_PROTOCOL_LEN) {
            return AS_ERROR_CODE_FAIL;
        }
        strncpy(info->protocol,data,len);
        data =idx + 3;/* skip the '://' */
    }

    /* find username and password */
    idx = strrchr(data,'@');
    if(NULL != idx) {
        char* username = data;
        data = idx + 1; /* skip the '@' */
        *idx = '\0';
        char* password = strchr(username,':');
        if(NULL == password) {
            len = idx - username;
            strncpy(info->username,username,len);
        }
        else {
            len = password - username;
            strncpy(info->username,username,len);
            password += 1; /* skip the ':' */
            len = idx - password;
            strncpy(info->password,password,len);
        }
        *idx = '@';
    }

    /* find host and port*/
    idx = strchr(data,':');
    if(NULL == idx) {
        /* default port*/
        idx = strchr(data,'/');
        if(NULL == idx) {
            strncpy(info->host,data,AS_URL_MAX_LEN);
            return AS_ERROR_CODE_OK;
        }
        len = idx - data;
        if(len >= AS_URL_MAX_LEN) {
            return AS_ERROR_CODE_FAIL;
        }
        strncpy(info->host,data,len);
        data = idx;
    }
    else {
        len = idx - data;
        if(len >= AS_URL_MAX_LEN) {
            return AS_ERROR_CODE_FAIL;
        }
        strncpy(info->host,data,len);
        data =idx + 1;/* skip the ':' */

        /* parse the port */
        idx = strchr(data,'/');
        if(NULL != idx) {
            *idx = '\0';
        }
        info->port = atoi(data);
        if(NULL == idx) {
            return AS_ERROR_CODE_OK;
        }
        *idx = '/';
        data = idx;
    }
    strncpy(info->uri,data,AS_URL_MAX_LEN);

    /* parse the path */
    idx = strchr(data,'?');
    if(NULL == idx) {
        strncpy(info->path,data,AS_URL_MAX_LEN);
        info->args[0] = '\0';
    }
    else {
        len = idx - data;
        if(len >= AS_URL_MAX_LEN) {
            return AS_ERROR_CODE_FAIL;
        }
        strncpy(info->path,data,len);
        data =idx + 1;/* skip the '?' */
        strncpy(info->args,data,AS_URL_MAX_LEN);
    }
    return AS_ERROR_CODE_OK;
}
int32_t    as_first_arg(as_url_t* url,as_url_arg_t* arg)
{
    if(NULL == url) {
        return AS_ERROR_CODE_FAIL;
    }
    if('\0' == url->args[0]) {
        /* there is no args */
        return AS_ERROR_CODE_FAIL;
    }
    uint32_t len = 0;
    char* data = (char*)&url->args[0];
    char* idx = strchr(data,'&');
    if(NULL == idx) {
        arg->next = NULL;
    }
    else {
        *idx = '\0';
        arg->next = idx + 1;
    }
    char* splite = strchr(data,'=');
    if(NULL == idx) {
        strncpy(arg->name,data,AS_URL_ARG_NAME_LEN);
    }
    else {
        len = splite - data;
        strncpy(arg->name,data,len);
        strncpy(arg->value,splite + 1,AS_URL_ARG_VALUE_LEN);
    }
    if(NULL != idx) {
        *idx = '&';
    }
    return AS_ERROR_CODE_OK;
}
int32_t    as_next_arg(as_url_arg_t* arg,as_url_arg_t* next)
{
    if(NULL == arg) {
        return AS_ERROR_CODE_FAIL;
    }
    if(NULL == arg->next) {
        /* there is no next args */
        return AS_ERROR_CODE_FAIL;
    }
    uint32_t len = 0;
    char* data = arg->next;
    char* idx = strchr(data,'&');
    if(NULL == idx) {
        next->next = NULL;
    }
    else {
        *idx = '\0';
        next->next = idx + 1;
    }
    char* splite = strchr(data,'=');
    if(NULL == idx) {
        strncpy(next->name,data,AS_URL_ARG_NAME_LEN);
    }
    else {
        len = splite - data;
        strncpy(next->name,data,len);
        strncpy(next->value,splite + 1,AS_URL_ARG_VALUE_LEN);
    }
    if(NULL != idx) {
        *idx = '&';
    }
    return AS_ERROR_CODE_OK;
}
int32_t    as_find_arg(as_url_t* url,const char* name,as_url_arg_t* arg)
{
    as_url_arg_t cur;

    if(NULL ==  url) {
        return AS_ERROR_CODE_FAIL;
    }
    int32_t nRet = as_first_arg(url,&cur);

    while(AS_ERROR_CODE_OK == nRet) {
        if(0 != strncmp(name,cur.name,AS_URL_ARG_NAME_LEN)) {
            nRet = as_next_arg(&cur,&cur);
            continue;
        }
        strncpy(arg->name,cur.name,AS_URL_ARG_NAME_LEN);
        strncpy(arg->value,cur.value,AS_URL_ARG_VALUE_LEN);
        arg->next = cur.next;
        return AS_ERROR_CODE_OK;
    }
    return AS_ERROR_CODE_FAIL; /* not find */
}