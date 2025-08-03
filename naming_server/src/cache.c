#include "cache.h"

struct CacheEntry *cache = NULL;
uint64_t find_in_cache(char *key)
{
    struct CacheEntry *entry; 
    HASH_FIND_STR(cache, key, entry);
    if (entry) {
        // remove it (so the subsequent add will throw it on the front of the list)
        HASH_DELETE(hh, cache, entry);
        HASH_ADD_KEYPTR(hh, cache, entry->key, strlen(entry->key), entry);
        return entry->value;
    }
    return UINT64_MAX;
}

void remove_from_cache(char *key)
{
    // full disclosure I copied all of this code - rijul
    struct CacheEntry *entry;
    HASH_FIND_STR(cache, key, entry);
    if (entry)
        HASH_DELETE(hh, cache, entry);
}

void add_to_cache(char *key, uint64_t value)
{
    struct CacheEntry *entry, *tmp_entry;
    entry = malloc(sizeof(struct CacheEntry));
    entry->key = strdup(key);
    entry->value = value;
    HASH_ADD_KEYPTR(hh, cache, entry->key, strlen(entry->key), entry);
    
    // prune the cache to MAX_CACHE_SIZE
    if (HASH_COUNT(cache) >= MAX_CACHE_SIZE) {
        HASH_ITER(hh, cache, entry, tmp_entry) {
            // prune the first entry (loop is based on insertion order so this deletes the oldest item)
            HASH_DELETE(hh, cache, entry);
            free(entry->key);
            free(entry);
            break;
        }
    }    
}