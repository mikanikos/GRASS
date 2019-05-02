#ifndef GRASS_H
#define GRASS_H

#define DEBUG true

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string>
 

#define MAX_BUFF_SIZE 1024
#define NB_COMMANDS 12
#define MAX_THREADS 30
#define PATH_MAX 128


char config_file[] = "grass.conf";
char server_ip[] = "127.0.0.1";

struct User
{
    std::string uname;
    std::string pass;
    char cwd[MAX_BUFF_SIZE];
    bool isLoggedIn;
};

struct Command
{
    const char *cname;
    const char *cmd;
    const char *params;
};

void hijack_flow();

#endif
