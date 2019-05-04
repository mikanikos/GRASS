#include <grass.h>
#include <message.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <iostream>
#include <string>
#include <sstream>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list>
#include <map>
#include <pthread.h>
using namespace std;

#define NB_COMMANDS 13

#define MAX_THREADS 30

#define IS_DIRECTORY 2
#define IS_FILE 1
#define UNKNOWN_PATH 0

// map socket to user
map<int, struct User> active_Users;

// list of users allowed to login
list<struct User> userList;

char port[7];
char curr_dir[10];

int PORT = 31337;

// Handle threads
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

struct args_getput
{
    int sock;
    int filesize;
    const char *filename;
    int port;
};

int check_authentication(int);
int check_path(string);
int do_login(vector<string>, int);
int do_pass(vector<string>, int);
int do_ping(vector<string>, int);
int do_ls(vector<string>, int);
int do_cd(vector<string>, int);
int do_mkdir(vector<string>, int);
int do_rm(vector<string>, int);
int do_get(vector<string> name, int sock);
int do_put(vector<string> name, int sock);
//int do_grep(vector<string> name, int sock);
int do_date(vector<string>, int);
int do_whoami(vector<string>, int);
int do_w(vector<string>, int);
int do_logout(vector<string>, int);
//int do_exit(vector<string> name, int sock);

typedef int (*shell_fct)(vector<string>, int);

struct shell_map
{
    const char *name;
    shell_fct fct;
    size_t argc;
};

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
    //{"grep", do_grep, 1},
    {"date", do_date, 0},
    {"whoami", do_whoami, 0},
    {"w", do_w, 0},
    {"logout", do_logout, 0},
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
        strcpy(result, "Error: command failed\n");
        write(sock, result, sizeof(result));
        return;
    }
    while (fgets(buff, sizeof(buff), fp) != NULL)
    {
        strcat(result, buff);
    }
    pclose(fp);
    write(sock, result, sizeof(result));
}

int do_login(vector<string> name, int sock)
{
    char res[1024];

    // search for user in paramters of config file
    for (auto const &it : userList)
    {
        if (strcmp(it.uname, name[1].c_str()) == 0)
        {
            // add to map in order to keep track the activity
            active_Users[sock] = it;
            strcpy(res, "User found! Use pass command to access the system\n");
            write(sock, res, sizeof(res));
            return 0;
        }
    }

    // if the user is not in the config file, no access
    strcpy(res, "You are not allowed to access the system\n");
    write(sock, res, sizeof(res));
    return 0;
}

int do_pass(vector<string> name, int sock)
{
    char res[1024];
    map<int, struct User>::iterator it;

    // search for the user who has tried to login
    it = active_Users.find(sock);
    if (it != active_Users.end())
    {
        // password found, authentication is successful
        if (strcmp((it->second).pass, name[1].c_str()) == 0)
        {
            (it->second).isLoggedIn = true;
            strcpy(res, "You're authenticated!\n");
            write(sock, res, sizeof(res));
        }
        // wrong password
        else
        {
            strcpy(res, "Wrong password\n");
            write(sock, res, sizeof(res));
        }
    }
    // the user has to do login before pass
    else
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
    }
    return 0;
}

// check if the user is allowed to execute/access, returns true if yes
int check_authentication(int sock)
{
    map<int, struct User>::iterator it;

    // search the user in the map, if not present the user is not authnticated yet (no login command), otherwise check if he completed the authentication with pass
    it = active_Users.find(sock);
    if (it != active_Users.end())
    {
        if ((it->second).isLoggedIn)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

int do_logout(vector<string> name, int sock)
{
    char res[1024] = "";

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // find the user accoding to the socket fd and set the loggedin status to false
    map<int, struct User>::iterator it;
    it = active_Users.find(sock);
    if (it != active_Users.end())
    {
        (it->second).isLoggedIn = false;
        active_Users.erase(sock);
        strcpy(res, "You've logged out\n");
        write(sock, res, sizeof(res));
    }
    // no user found, there's a problem
    else
    {
        strcpy(res, "No user found: did you hack the system?\n");
        write(sock, res, sizeof(res));
    }

    return 0;
}

// UNAUTHENTICATED USERS ALLOWED
int do_ping(vector<string> name, int sock)
{
    string command;
    command = name[0] + " " + name[1] + " -c 1";
    run_command(command.c_str(), sock);
    return 0;
}

// TO REVIEW, WORKS BUT NOT CORRECTLY
int do_ls(vector<string> name, int sock)
{
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // NOT CORRECT -> it must be based on system users
    string command;
    command = name[0] + " -l";
    run_command(command.c_str(), sock);
    return 0;
}

int do_date(vector<string> name, int sock)
{
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    run_command(name[0].c_str(), sock);
    return 0;
}

int do_cd(vector<string> name, int sock)
{
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check if the path exists and it's a directory
    if (check_path(name[1]) == IS_DIRECTORY)
    {
        chdir(name[1].c_str());
        write(sock, "", sizeof(""));
    }
    else
    {
        write(sock, ERR_UNKNOWN_PATH, sizeof(ERR_UNKNOWN_PATH));
    }
    return 0;
}

int do_mkdir(vector<string> name, int sock)
{
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    string command;
    int check = check_path(name[1]);

    // check if directory already exists
    if (check == IS_DIRECTORY)
    {
        write(sock, ERR_PATH_EXISTS, sizeof(ERR_PATH_EXISTS));
    }
    else
    {
        command = name[0] + " " + name[1];
        run_command(command.c_str(), sock);
    }
    return 0;
}

int do_rm(vector<string> name, int sock)
{
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    string command;
    int check = check_path(name[1]);

    // if file, remove file
    if (check == IS_FILE)
    {
        command = name[0] + " " + name[1];
        run_command(command.c_str(), sock);
    }
    else
    {
        // if directory, change command and remove directory
        if (check == IS_DIRECTORY)
        {
            name[0] = "rmdir";
            command = name[0] + " " + name[1];
            run_command(command.c_str(), sock);
        }
        else
        {
            write(sock, ERR_UNKNOWN_PATH, sizeof(ERR_UNKNOWN_PATH));
        }
    }
    return 0;
}

// check validity of the path issued, returns code to identify whether it is a folder/file or unknown path
int check_path(string path)
{
    struct stat s;
    if (stat(path.c_str(), &s) == 0)
    {
        if (s.st_mode & S_IFDIR)
        {
            return IS_DIRECTORY;
        }
        else if (s.st_mode & S_IFREG)
        {
            return IS_FILE;
        }
    }
    return UNKNOWN_PATH;
}

int do_w(vector<string> name, int sock)
{
    char res[1024] = "";

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // go through all the active users in the system and check who's loggedin
    for (auto const &it : active_Users)
    {
        if ((it.second).isLoggedIn)
        {
            strcat(res, (it.second).uname);
            strcat(res, " ");
        }
    }
    strcat(res, "\n");

    write(sock, res, sizeof(res));
    return 0;
}

int do_whoami(vector<string> name, int sock)
{
    char res[1024] = "";

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // retrun username correspeonding to the socket fd
    map<int, struct User>::iterator it;
    it = active_Users.find(sock);
    if (it != active_Users.end())
    {
        strcpy(res, (it->second).uname);
        strcat(res, "\n");
        write(sock, res, sizeof(res));
    }
    // if user not found, it means that the user is not authenticated but it still accessed the command execution statement (there's a problem)
    else
    {
        strcpy(res, "We didn't find you: did you hack the system?\n");
        write(sock, res, sizeof(res));
    }

    return 0;
}

/*
 * Receive a file from the client as its own thread
 *
 */
void *recv_file(void *args)
{
    int sock = ((struct args_getput *)args)->sock;         // socket that has already been created.
    int filesize = ((struct args_getput *)args)->filesize; // the size (in bytes) of the file to recv
    const char *filename = ((struct args_getput *)args)->filename;
    int port = ((struct args_getput *)args)->port;

    char res[1024];

    strcpy(res, "put port: ");
    strcat(res, std::to_string(port).c_str());
    strcat(res, "\n");
    write(sock, res, sizeof(res));
    bzero(res, 1024);

    int sock_new;

    // CREATION
    sock_new = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_new < 0)
    {
        printf("creation failed\n");
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        return NULL;
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &s_addr.sin_addr) < 0)
    {
        printf("invalid address\n");
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        return NULL;
    }
    // CONNECTION
    // try to connect until the client is listening to the port
    clock_t timeStart = clock();
    while (connect(sock_new, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        if ((clock() - timeStart) / CLOCKS_PER_SEC >= 30) // time in seconds
        {
            printf("timeout (30 seconds)\n");
            return NULL;
        }
    }

    char revbuf[1024];
    FILE *fp = fopen(filename, "w+");
    if (fp == NULL)
    {
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        close(sock_new);
        return NULL;
    }
    else
    {
        bzero(revbuf, 1024);
        int f_block_sz = 0;
        int total_size_recv = 0;
        int to_be_transferred = filesize;
        while (f_block_sz = recv(sock_new, revbuf, 1024, 0))
        {
            if (f_block_sz < 0)
            {
                strcpy(res, ERR_TRANSFER);
                write(sock, res, sizeof(res));
                fclose(fp);
                close(sock_new);
                return NULL;
            }
            total_size_recv += f_block_sz;
            if (to_be_transferred > f_block_sz)
            {
                int write_sz = fwrite(revbuf, sizeof(char), f_block_sz, fp);
                if (write_sz < f_block_sz)
                {
                    strcpy(res, ERR_TRANSFER);
                    write(sock, res, sizeof(res));
                    fclose(fp);
                    close(sock_new);
                    return NULL;
                }
                to_be_transferred -= f_block_sz;
            }
            else
            {
                int write_sz = fwrite(revbuf, sizeof(char), to_be_transferred, fp);
                if (write_sz < to_be_transferred)
                {
                    strcpy(res, ERR_TRANSFER);
                    write(sock, res, sizeof(res));
                    fclose(fp);
                    close(sock_new);
                    return NULL;
                }
                to_be_transferred = 0;
            }

            bzero(revbuf, 1024);
        }
        fclose(fp);

        // if the transferred stream’s size doesn’t match with the specified size
        if (total_size_recv != filesize)
        {
            strcpy(res, ERR_TRANSFER);
            write(sock, res, sizeof(res));
            close(sock_new);
            return NULL;
        }
    }
    // the transfer was a success
    write(sock, "", sizeof(""));
    close(sock_new);
}

int do_put(vector<string> name, int sock)
{
    const char *filename = name[1].c_str();
    int filesize = atoi(name[2].c_str());
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check if the filename is not too long
    if (strlen(filename) > 128)
    {
        strcpy(res, ERR_PATH_TOO_LONG);
        write(sock, res, sizeof(res));
        return 1;
    }

    pthread_t t;

    int port = PORT++;
    struct args_getput *args = (struct args_getput *)malloc(sizeof(struct args_getput));
    args->sock = sock;
    args->filesize = filesize;
    args->filename = filename;
    args->port = port;

    if (pthread_create(&t, NULL, recv_file, (void *)args) != 0)
    {
        perror("thread creation failed");
    }

    return 0;
}

/*
 * Send a file to the client as its own thread
 *
 */
void *send_file(void *args)
{
    int sock = ((struct args_getput *)args)->sock; // socket that has already been created.
    const char *filename = ((struct args_getput *)args)->filename;
    int port = ((struct args_getput *)args)->port;

    char res[1024];

    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        strcpy(res, ERR_FILE_NOT_FOUND);
        write(sock, res, sizeof(res));
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long int filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // CREATION
    int sock_get;

    sock_get = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_get < 0)
    {
        perror("creation failed\n");
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        return NULL;
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(port);

    // BIND
    if ((bind(sock_get, (struct sockaddr *)&s_addr, sizeof(s_addr))) < 0)
    {
        perror("bind failed\n");
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        close(sock_get);
        return NULL;
    }

    // START LISTENING
    if ((listen(sock_get, 1)) < 0)
    {
        perror("listen failed\n");
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        close(sock_get);
        return NULL;
    }

    strcpy(res, "get port: ");
    strcat(res, std::to_string(port).c_str());
    strcat(res, " size: ");
    strcat(res, std::to_string(filesize).c_str());
    strcat(res, "\n");
    write(sock, res, sizeof(res));

    struct sockaddr_in c_addr;
    int c_addr_len = sizeof(c_addr);
    int sock_new;
    sock_new = accept(sock_get, (struct sockaddr *)&c_addr, (socklen_t *)&c_addr_len);
    if (sock_new < 0)
    {
        perror("accept failed\n");
        strcpy(res, ERR_TRANSFER);
        write(sock, res, sizeof(res));
        close(sock_get);
        close(sock_new);
        return NULL;
    }

    char sdbuf[1024]; // send buffer
    bzero(sdbuf, 1024);
    int f_block_sz;

    while ((f_block_sz = fread(sdbuf, sizeof(char), 1024, fp)) > 0)
    {
        if (send(sock_new, sdbuf, f_block_sz, 0) < 0)
        {
            strcpy(res, ERR_TRANSFER);
            write(sock, res, sizeof(res));
            break;
        }
        bzero(sdbuf, 1024);
    }
    // the transfer is a success
    write(sock, "", sizeof(""));
    close(sock_new);
    close(sock_get);
}

int do_get(vector<string> name, int sock)
{
    const char *filename = name[1].c_str();
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock))
    {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check if the filename is not too long
    if (strlen(filename) > 128)
    {
        strcpy(res, ERR_PATH_TOO_LONG);
        write(sock, res, sizeof(res));
        return 1;
    }

    int port = PORT++;

    pthread_t t;

    struct args_getput *args = (struct args_getput *)malloc(sizeof(struct args_getput));
    args->sock = sock;
    args->filename = filename;
    args->port = port;

    if (pthread_create(&t, NULL, send_file, (void *)args) != 0)
    {
        perror("thread creation failed");
    }

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
    int sock = *((int *)sockfd);

    char buff[1024] = "";

    while (true)
    {
        bzero(buff, 1024);

        // read message from client
        read(sock, buff, sizeof(buff));

        // take the lock to execute operations
        pthread_mutex_lock(&lock);

        // if exit, simply close the connection and remove user data from the active user map
        if (strcmp(buff, "exit") == 0)
        {
            active_Users.erase(sock);
            pthread_mutex_unlock(&lock);
            break;
        }

        // handle command issued
        handle_input(buff, sock);

        // release lock
        pthread_mutex_unlock(&lock);
    }
    close(sock);
    pthread_exit(0);
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
    char file[] = "../grass.conf";
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
                // parse base directory
                if (strcmp(s, "base") == 0)
                {
                    t = strtok(NULL, "\n");
                    if (t != NULL)
                    {
                        strcpy(curr_dir, t);
                    }
                }

                // parse port
                else if (strcmp(s, "port") == 0)
                {
                    t = strtok(NULL, "\n");
                    if (t != NULL)
                    {
                        strcpy(port, t);
                    }
                }

                // parse user
                else if (strcmp(s, "user") == 0)
                {
                    t = strtok(NULL, " ");
                    if (t != NULL)
                    {
                        char *name = t;
                        t = strtok(NULL, "\n");
                        if (t != NULL)
                        {
                            char *pass = t;

                            // store users in usersList
                            struct User u;
                            u.uname = (char *)malloc(strlen(name) + 1);
                            u.pass = (char *)malloc(strlen(pass) + 1);
                            strcpy(u.uname, name);
                            strcpy(u.pass, pass);
                            u.isLoggedIn = false;
                            userList.push_back(u);
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
        exit(1);
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
    s_addr.sin_port = htons(atoi(port));

    // BIND
    if ((bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr))) < 0)
    {
        perror("bind failed");
        exit(1);
    }

    // START LISTENING
    if ((listen(sock, MAX_THREADS)) < 0)
    {
        perror("listen failed");
        exit(1);
    }

    struct sockaddr_in c_addr;
    int c_addr_len = sizeof(c_addr);
    pthread_t t;

    // ACCEPT CLIENTS
    while (true)
    {
        int sock_new;

        sock_new = accept(sock, (struct sockaddr *)&c_addr, (socklen_t *)&c_addr_len);
        if (sock_new < 0)
        {
            printf("accept failed");
            exit(1);
        }

        // Create thread and handle connection for each client
        if (pthread_create(&t, NULL, connection_handler, &sock_new) != 0)
        {
            perror("thread creation failed");
            exit(1);
        }
    }

    close(sock);
    return 0;
}
