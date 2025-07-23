#pragma once

#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <psphttp.h>

char* net_useragent = (char*) "PlayStoreStation/" ATTR_VERSION_STR;
const char* net_baseurl = (char*) "http://storestation.projects.regdev.me";

typedef struct {
    char buffer[1024];
    int code;
    uint32_t size;
} net_SimpleResponse;

typedef struct {
    char *buffer;
    int code;
    uint32_t size;
} net_Response;

net_SimpleResponse net_simpleget(const char* endpoint, int timeout = 10 * 1000000) {
    char endpointBuffer[256];
    snprintf(endpointBuffer, 255, "%s%s", net_baseurl, endpoint);
    int tmpl = sceHttpCreateTemplate(net_useragent, PSP_HTTP_VERSION_1_1, 1);
    if (tmpl < 0) TraceLog(LOG_ERROR, "Template < 0");
    sceHttpSetConnectTimeOut(tmpl, timeout);
    int conn = sceHttpCreateConnectionWithURL(tmpl, endpointBuffer, 0);
    if (conn < 0) TraceLog(LOG_ERROR, "Connection < 0");
    int res, req = sceHttpCreateRequestWithURL(conn, PSP_HTTP_METHOD_GET, endpointBuffer, 0);
    if (req < 0) TraceLog(LOG_ERROR, "Request < 0");
    res = sceHttpSendRequest(req, nullptr, 0);
    if (res < 0) TraceLog(LOG_ERROR, "sceHttpSendRequest error %x", res);
    net_SimpleResponse resp;
    memset(&resp, 0, sizeof(resp));
    uint64_t total_size;
    int total_read = 0;
    res = sceHttpGetStatusCode(req, &resp.code);
    if (res < 0) TraceLog(LOG_ERROR, "sceHttpGetStatusCode error %x", res);
    res = sceHttpGetContentLength(req, &total_size);
    if (res < 0 || total_size > sizeof(resp.buffer)) {
        TraceLog(LOG_ERROR, "sceHttpGetContentLength error %x", res);
        resp.code = -1;
        goto err;
    }
    TraceLog(LOG_INFO, "HTTP[%d] (%d): %s", resp.code, (int) total_size, endpointBuffer);
    while (1) {
        sceKernelDelayThreadCB(1);
        int read = sceHttpReadData(req, resp.buffer + total_read, sizeof(resp.buffer) - total_read - 1);
        if (read <= 0) break;
        total_read += read;
    }
    resp.size = total_read;
    err:
    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tmpl);
    return resp;
}

net_Response net_get(const char* endpoint, int timeout = 10 * 1000000) {
    char endpointBuffer[256];
    snprintf(endpointBuffer, 255, "%s%s", net_baseurl, endpoint);
    int tmpl = sceHttpCreateTemplate(net_useragent, PSP_HTTP_VERSION_1_1, 1);
    if (tmpl < 0) TraceLog(LOG_ERROR, "Template < 0");
    sceHttpSetConnectTimeOut(tmpl, timeout);
    int conn = sceHttpCreateConnectionWithURL(tmpl, endpointBuffer, 0);
    if (conn < 0) TraceLog(LOG_ERROR, "Connection < 0");
    int res, req = sceHttpCreateRequestWithURL(conn, PSP_HTTP_METHOD_GET, endpointBuffer, 0);
    if (req < 0) TraceLog(LOG_ERROR, "Request < 0");
    res = sceHttpSendRequest(req, nullptr, 0);
    if (res < 0) TraceLog(LOG_ERROR, "sceHttpSendRequest error %x", res);
    net_Response resp;
    memset(&resp, 0, sizeof(resp));
    uint64_t total_size;
    int total_read = 0;
    res = sceHttpGetStatusCode(req, &resp.code);
    if (res < 0) TraceLog(LOG_ERROR, "sceHttpGetStatusCode error %x", res);
    res = sceHttpGetContentLength(req, &total_size);
    if (res < 0) {
        TraceLog(LOG_ERROR, "sceHttpGetContentLength error %x", res);
        resp.code = -1;
        goto err;
    }
    resp.buffer = (char *) malloc(total_size+1);
    memset(resp.buffer, 0, total_size+1);
    TraceLog(LOG_INFO, "HTTP[%d] (%d): %s", resp.code, (int) total_size, endpointBuffer);
    while (1) {
        sceKernelDelayThreadCB(1);
        int read = sceHttpReadData(req, resp.buffer + total_read, total_size - total_read);
        if (read <= 0) break;
        total_read += read;
    }
    resp.size = total_read;
    err:
    sceHttpDeleteRequest(req);
    sceHttpDeleteConnection(conn);
    sceHttpDeleteTemplate(tmpl);
    return resp;
}

void net_closeget(net_Response *resp) {
    if (resp->buffer != nullptr) free(resp->buffer);
    resp->buffer = nullptr;
}

constexpr inline int net_timeouts(const int seconds) {
    return seconds * 1000000;
}