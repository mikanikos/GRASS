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

#define NB_COMMANDS 11

#define MAX_THREADS 30

#define PATH_MAX 128

#define IS_DIRECTORY 2
#define IS_FILE 1
#define UNKNOWN_PATH 0

// map socket to user
map<int, struct User> active_Users;

// list of users allowed to login
list<struct User> userList;

char port[7];
char curr_dir[PATH_MAX];

// Handle threads
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int check_authentication(int);
int check_path(string);
int do_login(vector<string>, int);
int do_pass(vector<string>, int);
int do_ping(vector<string>, int);
int do_ls(vector<string>, int);
int do_cd(vector<string>, int);
int do_mkdir(vector<string>, int);
int do_rm(vector<string>, int);
//int do_get(vector<string> name, int sock);
//int do_put(vector<string> name, int sock);
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
    // {"get", do_get, 1},
    // {"put", do_put, 2},
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
    char res[1024];

    // search for user in paramters of config file 
    for (auto const& it : userList) {
        if (strcmp(it.uname, name[1].c_str()) == 0) {
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
    if (it != active_Users.end()) {
        // password found, authentication is successful
        if (strcmp((it->second).pass, name[1].c_str()) == 0) {
            (it->second).isLoggedIn = true;
            strcpy(res, "You're authenticated!\n");
            write(sock, res, sizeof(res));
        }
        // wrong password
        else {
            strcpy(res, "Wrong password\n");
            write(sock, res, sizeof(res));
        }
    }
    // the user has to do login before pass
    else {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
    }
    return 0;
}

// check if the user is allowed to execute/access, returns true if yes 
int check_authentication(int sock) {
    map<int, struct User>::iterator it;
     
    // search the user in the map, if not present the user is not authnticated yet (no login command), otherwise check if he completed the authentication with pass 
    it = active_Users.find(sock);
    if (it != active_Users.end()) {
        if ((it->second).isLoggedIn) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

int do_logout(vector<string> name, int sock)
{
    char res[1024] = "";

    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // find the user accoding to the socket fd and set the loggedin status to false
    map<int, struct User>::iterator it;
    it = active_Users.find(sock);
    if (it != active_Users.end()) {
        (it->second).isLoggedIn = false;
        active_Users.erase(sock);
        strcpy(res, "You've logged out\n");
        write(sock, res, sizeof(res));
    }
    // no user found, there's a problem
    else {
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
    if (!check_authentication(sock)) {
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
    if (!check_authentication(sock)) {
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
    if (!check_authentication(sock)) {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check path length
    if (name[1].length() > PATH_MAX) {
        strcpy(res, ERR_PATH_LONG);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check if the path exists and it's a directory
    if (check_path(name[1]) == IS_DIRECTORY)
    {
        // check permission of path
        if (strstr(realpath(name[1].c_str(), NULL), curr_dir) == NULL) {
            strcpy(res, ERR_ACCESS_DENIED);
            write(sock, res, sizeof(res));
            return 1;
        }

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
    if (!check_authentication(sock)) {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check path length
    if (name[1].length() > PATH_MAX) {
        strcpy(res, ERR_PATH_LONG);
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

        string parent_path = name[1];

        while (parent_path.back() == '\\' || parent_path.back() == '/') {
            parent_path = parent_path.substr(0, parent_path.size()-1);
        }

        size_t pos = parent_path.find_last_of("/\\");
        
        if (pos != string::npos) {
            parent_path = parent_path.substr(0, pos).c_str();
            
            // check permission of path
            if (strstr(realpath(parent_path.c_str(), NULL), curr_dir) == NULL) {
                strcpy(res, ERR_ACCESS_DENIED);
                write(sock, res, sizeof(res));
                return 1;
            }
        }

        command = name[0] + " " + name[1];
        run_command(command.c_str(), sock);
    }
    return 0;
}

int do_rm(vector<string> name, int sock)
{
    char res[1024];

    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // check path length
    if (name[1].length() > PATH_MAX) {
        strcpy(res, ERR_PATH_LONG);
        write(sock, res, sizeof(res));
        return 1;
    }

    string command;
    int check = check_path(name[1]);

    // if file, remove file
    if (check == IS_FILE || check == IS_DIRECTORY)
    {
        // check permission of path
        if (strstr(realpath(name[1].c_str(), NULL), curr_dir) == NULL) {
            strcpy(res, ERR_ACCESS_DENIED);
            write(sock, res, sizeof(res));
            return 1;
        }

        command = name[0] + " -r " + name[1];
        run_command(command.c_str(), sock);
    }
    else
    {
        write(sock, ERR_UNKNOWN_PATH, sizeof(ERR_UNKNOWN_PATH));
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
    if (!check_authentication(sock)) {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // go through all the active users in the system and check who's loggedin
    for (auto const& it : active_Users) {
        if ((it.second).isLoggedIn) {
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
    if (!check_authentication(sock)) {
        strcpy(res, ISSUE_LOGIN_MES);
        write(sock, res, sizeof(res));
        return 1;
    }

    // retrun username correspeonding to the socket fd 
    map<int, struct User>::iterator it;
    it = active_Users.find(sock);
    if (it != active_Users.end()) {
        strcpy(res, (it->second).uname);
        strcat(res, "\n");
        write(sock, res, sizeof(res));
    }
    // if user not found, it means that the user is not authenticated but it still accessed the command execution statement (there's a problem) 
    else {
        strcpy(res, "We didn't find you: did you hack the system?\n");
        write(sock, res, sizeof(res));
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
        if (strcmp(buff, "exit") == 0) {
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
                        strcpy(curr_dir, realpath(t, NULL));
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
                        char* name = t;
                        t = strtok(NULL, "\n");
                        if (t != NULL)
                        {
                            char *pass = t;
                            
                            // store users in usersList
                            struct User u;
                            u.uname = (char*) malloc(strlen(name)+1);
                            u.pass = (char*) malloc(strlen(pass)+1);
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
        if (pthread_create(&t, NULL, connection_handler, &sock_new) != 0) {
            perror("thread creation failed");
            exit(1);
        }
    }

    close(sock);
    return 0;
}
