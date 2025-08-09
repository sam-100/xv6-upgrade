#ifndef UTIL_H
#define UTIL_H

void error(const char *msg);            // exits the program with the given error message
void println(int fd);                   // print a new line to file pointed by fd


#define PAGESIZE 4096           // page size in bytes


#endif