#ifndef __FILE_STRUCTURE__
#define __FILE_STRUCTURE__

#include <pthread.h>

typedef struct {
    pthread_rwlock_t lock; 
    char *path;     
    char *metadata; 
    int type; // file or directory
} FileData; 

#endif