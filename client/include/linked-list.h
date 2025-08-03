#ifndef __LINKED_LIST_STR
#define __LINKED_LIST_STR

#define MAX_COMMAND_LENGTH 256

// the linked list has a pseudo head or whatever it's called. stores the value zero. 
// 0-indexed positioning, not counting the head. 
// acts like a string 

struct link {
    char *d; 
    struct link *next; 
    struct link *prev;
};

int printl(struct link *list); 

/* Create a linked list and return the head. */
struct link *createList();

/* Free memory stored by the linked list. */
int delete (struct link *list); 

/* Insert element to beginning of list. */
int insert (struct link *list, char *d);

/* Remove element from end of list. */
char *pop (struct link *list);

/* Return length of list. */
int linklen (struct link *list);

/* Return first occurence of D. Count is backwards. */
int findFirstOccurence (char *d, struct link *list);

/* BOOL value telling if D occurs in LIST */
int occursIn (char *d, struct link *list);

#endif