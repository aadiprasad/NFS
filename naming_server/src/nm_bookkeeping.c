#include"nm_bookkeeping.h"


char* writing_path;
FILE *fptr;

// Function to initialize Naming Server logging
void init_nm_logging(const char* log_file_path)
{
    writing_path = log_file_path;

    fptr = fopen(writing_path, "w");
    if(fptr == NULL)
    {
        printf("Error!");
        exit(1);
    }
    fprintf(fptr, "Naming Server Log\n");
}

// Function to log Naming Server operations
void log_nm_operation(const char* operation_details)
{
    fprintf(fptr, "%s\n", operation_details);
    fflush(fptr);        
    printf("%s\n", operation_details);
}