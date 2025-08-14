#ifndef UTIL_H
#define UTIL_H
#include "types.h"

void error(const char *msg);                // exits the program with the given error message
void println(int fd);                       // print a new line to file pointed by fd
void assert(int condn, const char *msg);   // if condition is false, exit the program with 'msg' error message.

#define PAGESIZE 4096           // page size in bytes


#endif