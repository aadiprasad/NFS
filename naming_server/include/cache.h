#include <string.h>
#ifndef MACRO
#include <stdio.h>
#endif
#include "uthash.h"

// this is an example of how to do a LRU cache in C using uthash
// http://uthash.sourceforge.net/
// by Jehiah Czebotar 2011 - jehiah@gmail.com
// this code is in the public domain http://unlicense.org/

#define MAX_CACHE_SIZE 100000

struct CacheEntry {
    char *key;
    uint64_t value;
    UT_hash_handle hh;
};

uint64_t find_in_cache(char *key);
void remove_from_cache(char *key);
void add_to_cache(char *key, uint64_t value);