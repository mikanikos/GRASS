#include <grass.h>
#include <error.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>

using namespace std;

static struct User **userlist;
static int numUsers;
static struct Command **cmdlist;
static int numCmds;
//char port[7] = "31337";
char port[7];
char curr_dir[10];

int do_login(vector<string> name, int sock);
int do_pass(vector<string> name, int sock);
int do_ping(vector<string> name, int sock);
int do_ls(vector<string> name, int sock);
int do_cd(vector<string> name, int sock);
int do_mkdir(vector<string> name, int sock);
int do_rm(vector<string> name, int sock);
int do_get(vector<string> name, int sock);
int do_put(vector<string> name, int sock);
int do_grep(vector<string> name, int sock);
int do_date(vector<string> name, int sock);
int do_whoami(vector<string> name, int sock);
int do_w(vector<string> name, int sock);
int do_logout(vector<string> name, int sock);
int do_exit(vector<string> name, int sock);

typedef int (*shell_fct)(vector<string>, int);

struct shell_map
{
    const char *name;
    shell_fct fct;
    size_t argc;
};

#define NB_COMMANDS 4

struct shell_map shell_cmds[NB_COMMANDS] = {
    {"login", do_login, 1},
    {"pass", do_pass, 1},
    // {"ping", do_ping, 1},
    // {"ls", do_ls, 0},
    // {"cd", do_cd, 1},
    // {"mkdir", do_mkdir, 1},
    // {"rm", do_rm, 1},
    // {"get", do_get, 1},
    // {"put", do_put, 2},
    // {"grep", do_grep, 1},
    {"date", do_date, 0},
    {"whoami", do_whoami, 0},
    // {"w", do_w, 0},
    // {"logout", do_logout, 0},
    // {"exit", do_exit, 0},
};

// Helper function to run commands in unix.
void run_command(const char *command, int sock)
{
    char buff[1024], result[1024] = "";
    FILE *fp;
    fp = popen(command, "r");
    if (fp == NULL)
    {
        perror("command failed");
        exit(0);
    }
    while (fgets(buff, sizeof(buff), fp) != NULL)
    {
        strcat(result, buff);
    }
    pclose(fp);
    write(sock, result, sizeof(result));
}

/*
 * Send a file to the client as its own thread
 *
 * fp: file descriptor of file to send
 * sock: socket that has already been created.
 */
void send_file(int fp, int sock)
{
}

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * sock: socket that has already been created.
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int sock, int size)
{
}

int do_login(vector<string> name, int sock)
{
    write(sock, "welcome!\n", 10);
    return 1;
}

int do_pass(vector<string> name, int sock)
{
    write(sock, "failed to authenticate\n", 24);
    return 1;
}

int do_whoami(vector<string> name, int sock)
{
    run_command(name[0].c_str(), sock);
    return 0;
}

int do_date(vector<string> name, int sock)
{
    run_command(name[0].c_str(), sock);
    return 0;
}

void handle_input(char *command, int sock)
{
    string input(command);
    istringstream buffer(input);

    vector<string> tokens{istream_iterator<string>(buffer), istream_iterator<string>()};

    if (tokens.size() == 0)
    {
        write(sock, ERR_NULL_CMD, sizeof(ERR_NULL_CMD));
        return;
    }

    for (int i = 0; i < NB_COMMANDS; i++)
    {
        if (shell_cmds[i].name == tokens[0])
        {
            if (shell_cmds[i].argc != tokens.size() - 1)
            {
                write(sock, ERR_NBR_PARAMETERS, sizeof(ERR_NBR_PARAMETERS));
                return;
            }
            shell_cmds[i].fct(tokens, sock);
            return;
        }
    }
    write(sock, ERR_UNKNOWN_CMD, sizeof(ERR_UNKNOWN_CMD));
    return;
}

// Server side REPL given a socket file descriptor
void *connection_handler(void *sockfd)
{
    int sock = *(int *)sockfd;

    char buff[1024] = "";

    while (true)
    {
        bzero(buff, 1024);

        read(sock, buff, sizeof(buff));

        if (strncmp("exit", buff, 4) == 0)
        {
            break;
        }

        handle_input(buff, sock);
    }
}

/*
 * search all files in the current directory
 * and its subdirectory for the pattern
 *
 * pattern: an extended regular expressions.
 * Output: A line seperated list of matching files' addresses
 */
void search(char *pattern)
{
    // TODO
}

// Parse the grass.conf file and fill in the global variables
void parse_grass()
{

    char *s, *t;
    char *file = "../grass.conf";
    FILE *fp = fopen(file, "r");

    char buffer[256];

    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (buffer[0] == '#' || buffer[0] == '\n')
            continue;
        else
        {
            s = strtok(buffer, " ");
            if (s != NULL)
            {
                if (strcmp(s, "base") == 0)
                {
                    t = strtok(NULL, "\n");
                    if (t != NULL)
                    {
                        ;
                        strcpy(curr_dir, t);
                        //printf("base: %s\n", curr_dir);
                    }
                }
                else if (strcmp(s, "port") == 0)
                {
                    t = strtok(NULL, "\n");
                    if (t != NULL)
                    {
                        strcpy(port, t);
                        //printf("port: %s\n", port);
                    }
                }
                else if (strcmp(s, "user") == 0)
                {
                    char name[50], pass[50];
                    t = strtok(NULL, " ");
                    if (t != NULL)
                    {
                        strncpy(name, t, 50);
                        t = strtok(NULL, "\n");
                        if (t != NULL)
                        {
                            strncpy(pass, t, 50);
                            //printf("User: %s %s\n", name, pass);
                        }
                    }
                }
            }
        }
    }

    fclose(fp);
}

int main()
{
    // Parse the grass.conf file
    parse_grass();

    // Listen to the port and handle each connection

    // CREATION
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("creation failed");
        exit(0);
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
    s_addr.sin_port = htons(atoi(port));

    // BIND
    if ((bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr))) < 0)
    {
        perror("bind failed");
        exit(0);
    }

    // START LISTENING
    if ((listen(sock, 5)) < 0)
    {
        perror("listen failed");
        exit(0);
    }

    struct sockaddr_in c_addr;
    int c_addr_len = sizeof(c_addr);

    // ACCEPT
    while (true)
    {
        int sock_new;

        sock_new = accept(sock, (struct sockaddr *)&c_addr, (socklen_t *)&c_addr_len);
        if (sock_new < 0)
        {
            perror("accept failed");
            exit(0);
        }

        connection_handler(&sock_new);

        close(sock_new);
    }

    close(sock);
}
