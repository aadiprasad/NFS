#ifndef NM_BOOKKEEPING_H
#define NM_BOOKKEEPING_H

#include "communication_protocols.h"
#include<stdlib.h>
#include<stdio.h>
// Function to initialize Naming Server logging
void init_nm_logging(const char* log_file_path);

// Function to log Naming Server operations
void log_nm_operation(const char* operation_details);

#endif