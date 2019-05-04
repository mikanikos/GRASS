#ifndef CLIENT_H
#define CLIENT_H

#include <grass.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string>
using namespace std;

struct args_put
{
    int port;
    char *path;
    const char *file;
};

struct args_get
{
    const char *file;
    int filesize;
    int port;
    char *path;
};

static bool automated_mode = false;

#endif
