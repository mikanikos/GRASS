#ifndef GRASS_H
#define GRASS_H

#define DEBUG true

#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

#define MAX_BUFF_SIZE 1024
#define NB_COMMANDS 12
#define MAX_THREADS 100
#define PATH_MAX 128

const string config_file = "grass.conf";

void hijack_flow();

#endif
