// Minimal hiredis stub — allows cmake to find Hiredis when libhiredis-dev is
// not installed but libhiredis1.1.0 runtime is present.
// Install libhiredis-dev to replace with the real headers.
#pragma once
#ifndef HIREDIS_H
#define HIREDIS_H
#include <stdint.h>
#include <stddef.h>
#define HIREDIS_MAJOR 1
#define HIREDIS_MINOR 1
#define HIREDIS_PATCH 0
typedef struct redisContext { int err; char errstr[128]; int fd; } redisContext;
typedef struct redisReply {
    int type;
    long long integer;
    double dval;
    size_t len;
    char *str;
    size_t elements;
    struct redisReply **element;
} redisReply;
#define REDIS_REPLY_STRING  1
#define REDIS_REPLY_ARRAY   2
#define REDIS_REPLY_INTEGER 3
#define REDIS_REPLY_NIL     4
#define REDIS_REPLY_STATUS  5
#define REDIS_REPLY_ERROR   6
#endif /* HIREDIS_H */
