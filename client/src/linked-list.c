#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linked-list.h"

// the linked list has a pseudo head or whatever it's called. stores the value zero. 
// 0-indexed positioning, not counting the head. 

// debugging function. prints the list
int printl (struct link *list) {
    struct link *iterator = list->next; 
    while (iterator != NULL) {
        printf("[%s]", iterator->d); 
        iterator = iterator->next;
    }
    printf("\n");
    return 0; // we can probably make this function void return and remove this    
}

struct link *createList () {
    struct link *head = (struct link *) malloc (sizeof(struct link));
    head->d = malloc(MAX_COMMAND_LENGTH); 
    head->d[0] = 0; head->next = NULL; head->prev = NULL;
    
    return head; 
}

int insert (struct link *list, char *d) {
    struct link *node = (struct link *) malloc (sizeof(struct link));
    node->d = d; 
    node->next = list->next; node->prev = list; 
    list->next = node; 

    return 0; 
}

// int insertAtTail (struct link *list, char *d) {
//     struct link *node = (struct link *) malloc (sizeof(struct link));
//     node->d = d; node->next = NULL; 
//     while (list->next != NULL) {
//         list = list->next; 
//     }
//     node->prev = list;  
//     list->next = node; 

//     return 1; 
// }

int linklen (struct link *list) {
    struct link *iterator = list->next; 
    int size; 
    for (size = 0; iterator != NULL; size++) {
        iterator = iterator->next; 
    }
    return size;     
}

int findFirstOccurence (char *d, struct link *list) {
    struct link *iterator = list->next; 
    int pos; 
    for (pos = 0; iterator != NULL; pos++) {
        if (strcmp(iterator->d, d) == 0) {
            return linklen(list) - pos - 1; 
        }
        iterator = iterator->next; 
    }
    return -1; 
}

int occursIn (char *d, struct link *list) {
    int flag = findFirstOccurence(d, list);
    if (flag == -1) {
        return 0;
    } else {
        return 1; 
    }
}

// returns string from the end of linked list
char *pop (struct link *list) { 
    struct link *iterator = list; 
    while (iterator->next->next != NULL) {
        iterator = iterator->next; 
    }

    char *return_val = malloc(strlen(iterator->next->d) + 1); 
    strcpy(return_val, iterator->next->d);
    
    free(iterator->next);
    iterator->next = NULL; 
    
    return return_val; 
}

int delete (struct link *list) {
    int len = linklen(list); 
    for (int i = 0; i < len; i++) {
        pop(list); 
    }
    free(list); 
    return 0; 
}
