#ifndef SERVER_H
#define SERVER_H

#include <grass.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string>
using namespace std;


#define IS_DIRECTORY 2
#define IS_FILE 1
#define UNKNOWN_PATH 0

const string server_ip = "127.0.0.1";

int TRANSFER_PORT = 31337;

struct args_getput
{
    int sock;
    int filesize;
    const char *filename;
    int port;
};

struct User
{
    std::string uname;
    std::string pass;
    std::string cwd;
    bool isLoggedIn;
};

typedef int (*shell_fct)(const string&, const int);

struct shell_map
{
    const char *name;
    shell_fct fct;
    size_t argc;
};

int check_authentication(const int);
int check_path(const string&);
int do_login(const string&, const int);
int do_pass(const string&, const int);
int do_ping(const string&, const int);
int do_ls(const string&, const int);
int do_cd(const string&, const int);
int do_mkdir(const string&, const int);
int do_rm(const string&, const int);
int do_get(const string&, const int);
int do_put(const string&, const int);
int do_grep(const string&, const int);
int do_date(const string&, const int);
int do_whoami(const string&, const int);
int do_w(const string&, const int);
int do_logout(const string&, const int);


struct shell_map shell_cmds[NB_COMMANDS] = {
    {"login", do_login, 1},
    {"pass", do_pass, 1},
    {"ping", do_ping, 1},
    {"ls", do_ls, 0},
    {"cd", do_cd, 1},
    {"mkdir", do_mkdir, 1},
    {"rm", do_rm, 1},
    {"get", do_get, 1},
    {"put", do_put, 2},
    {"grep", do_grep, 1},
    {"date", do_date, 0},
    {"whoami", do_whoami, 0},
    {"w", do_w, 0},
    {"logout", do_logout, 0},
};

#endif
