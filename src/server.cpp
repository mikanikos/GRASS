#include <grass.h>
#include <error.h>
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
#include <wordexp.h>
using namespace std;

#define NB_COMMANDS 11

#define MAX_THREADS 30

#define PATH_MAX 128

#define IS_DIRECTORY 2
#define IS_FILE 1
#define UNKNOWN_PATH 0

char config_file[] = "grass.conf";

// map socket to user
map<int, struct User> active_Users;

// list of users allowed to login
list<struct User> userList;

char port[7];
char curr_dir[PATH_MAX];

// Handle threads
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int check_authentication(const int);
int check_path(const string&);
int do_login(const string&, const int);
int do_pass(const string&, const int);
int do_ping(const string&, const int);
int do_ls(const string&, const int);
int do_cd(const string&, const int);
int do_mkdir(const string&, const int);
int do_rm(const string&, const int);
//int do_get(const string&, const int);
//int do_put(const string&, const int);
int do_grep(const string&, const int);
int do_date(const string&, const int);
int do_whoami(const string&, const int);
int do_w(const string&, const int);
int do_logout(const string&, const int);
//int do_exit(const string&, const int);

typedef int (*shell_fct)(const string&, const int);

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


// function to write message to the client and server console
void write_message(const int sock, const char *message) 
{
    write(sock, message, 1024);
    printf("%s", message);
}

// Helper function to run commands in unix.
void run_command(const char *command, const int sock)
{
    char buff[1024], result[1024] = "";
    FILE *fp;

    string com(command);
    
    // get stderr too
    com = com + " 2>&1";

    fp = popen(com.c_str(), "r");
    if (fp == NULL)
    {
        write_message(sock, ERR_COMMAND_FAIL);
        return;
    }
    while (fgets(buff, sizeof(buff), fp) != NULL)
    {
        strcat(result, buff);
    }
    pclose(fp);
    write_message(sock, result);
}

/*
 * Send a file to the client as its own thread
 *
 * fp: file descriptor of file to send
 * sock: socket that has already been created.
 */
void send_file(const int fp, const int sock)
{
}

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * sock: socket that has already been created.
 * size: the size (in bytes) of the file to recv
 */
void recv_file(const int fp, const int sock, const int size)
{
}

int do_login(const string& name, const int sock)
{
    // user alredy logged in has to logout for re-issuing this command
    // map<int, struct User>::iterator it;
    // it = active_Users.find(sock);
    // if (it != active_Users.end() && (it->second).isLoggedIn) {
    //     //strcpy(res, "You're alreaddy logged in\n");
    //     write(sock, res, sizeof(res));
    //     return 1;
    // }

    // search for user in paramters of config file 
    for (auto const& it : userList) {
        if (strcmp(it.uname, name.c_str()) == 0) {
            // add to map in order to keep track the activity
            active_Users[sock] = it;
            //strcpy(res, "User found! Use pass command to access the system\n");
            write(sock, "", sizeof(""));
            return 0;
        }
    }

    // if the user is not in the config file, no access
    //strcpy(res, ERR_ACCESS_DENIED);
    //write(sock, res, sizeof(res));
    //printf("%s\n", res);
    write_message(sock, ERR_ACCESS_DENIED);
    return 1;
}

int do_pass(const string& name, const int sock)
{
    map<int, struct User>::iterator it;
    
    // search for the user who has tried to login 
    it = active_Users.find(sock);
    if (it != active_Users.end()) {
        // password found, authentication is successful
        if (strcmp((it->second).pass, name.c_str()) == 0) {
            (it->second).isLoggedIn = true;
            write(sock, "", sizeof(""));
        }
        // wrong password
        else {
            write_message(sock, ERR_AUTH_FAIL);
        }
    }
    // the user has to do login before pass
    else {
        write_message(sock, ERR_ACCESS_DENIED);
    }
    return 0;
}

// check if the user is allowed to execute/access, returns true if yes 
int check_authentication(const int sock) {
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

int do_logout(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // find the user accoding to the socket fd and set the loggedin status to false
    map<int, struct User>::iterator it;
    it = active_Users.find(sock);
    if (it != active_Users.end()) {
        (it->second).isLoggedIn = false;
        active_Users.erase(sock);
        write(sock, "", sizeof(""));
    }
    // no user found, there's a problem
    else {
        write_message(sock, ERR_NO_USER_FOUND);
    }

    return 0;
}

// UNAUTHENTICATED USERS ALLOWED
int do_ping(const string& name, const int sock)
{
    // validate argument
    if (name.find_first_not_of("0123456789qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM.- ") != std::string::npos) { 
        write_message(sock, ERR_WRONG_PARAM);
        return 1;
    }

    string command;
    command = "ping " + name + " -c 1";
    run_command(command.c_str(), sock);
    return 0;
}

int do_ls(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    string command;
    command = string("ls -l ") + active_Users[sock].cwd;
    run_command(command.c_str(), sock);
    return 0;
}

int do_grep(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    run_command(("grep -rl " + name).c_str(), sock);
    return 0;
}

int do_date(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    run_command("date", sock);
    return 0;
}

int do_cd(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // check path length
    if (name.length() > PATH_MAX) {
        write_message(sock, ERR_PATH_LONG);
        return 1;
    }

    // using user cwd
    string nname = string(active_Users[sock].cwd) + "/" + name;

    // check if the path exists and it's a directory
    if (check_path(nname) == IS_DIRECTORY)
    {
        // check permission of path
        if (strstr(realpath(nname.c_str(), NULL), curr_dir) == NULL) {
            write_message(sock, ERR_ACCESS_DENIED);
            return 1;
        }

        //chdir(nname.c_str());
        strcpy(active_Users[sock].cwd, nname.c_str());
        write(sock, "", sizeof(""));
    }
    else
    {
        size_t pos = nname.find_last_of("/\\");
        string dir = nname.substr(pos+1).c_str();

        if (check_path(nname) == IS_FILE)
        {
            write_message(sock, ("cd: " + dir + ": Not a directory\n").c_str());
        }
        else {
            write_message(sock, ("cd: " + dir + ": No such file or directory\n").c_str());
        }
    }
    return 0;
}

int do_mkdir(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // check path length
    if (name.length() > PATH_MAX) {
        write_message(sock, ERR_PATH_LONG);
        return 1;
    }

    string command;

    // using user cwd
    string nname = string(active_Users[sock].cwd) + "/" + name;
    
    int check = check_path(nname);
    
    // check if directory already exists
    if (check != IS_DIRECTORY)
    {
        string parent_path = nname;

        while (parent_path.back() == '\\' || parent_path.back() == '/') {
            parent_path = parent_path.substr(0, parent_path.size()-1);
        }

        size_t pos = parent_path.find_last_of("/\\");
        
        if (pos != string::npos) {
            parent_path = parent_path.substr(0, pos).c_str();
            
            // check permission of path
            if (strstr(realpath(parent_path.c_str(), NULL), curr_dir) == NULL) {
                write_message(sock, ERR_ACCESS_DENIED);
                return 1;
            }
        }

        command = "mkdir " + nname;
        run_command(command.c_str(), sock);
    }
    else {
        size_t pos = nname.find_last_of("/\\");
        string dir = nname.substr(pos+1).c_str();
        write_message(sock, ("mkdir: cannot create '" + dir + "': File exists\n").c_str());
    }
    
    return 0;
}

int do_rm(const string& name, const int sock)
{
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // check path length
    if (name.length() > PATH_MAX) {
        write_message(sock, ERR_PATH_LONG);
        return 1;
    }

    string command;

    // using user cwd
    string nname = string(active_Users[sock].cwd) + "/" + name;

    int check = check_path(nname);

    // if file, remove file
    if (check == IS_FILE || check == IS_DIRECTORY)
    {
        // check permission of path
        if (strstr(realpath(nname.c_str(), NULL), curr_dir) == NULL) {
            write_message(sock, ERR_ACCESS_DENIED);
            return 1;
        }

        command = "rm -r " + nname;
        run_command(command.c_str(), sock);
    }
    else {
        size_t pos = nname.find_last_of("/\\");
        string dir = nname.substr(pos+1).c_str();
        write_message(sock, ("rm: cannot remove '" + dir + "': No such file or directory\n").c_str());
    }
    
    return 0;
}

// check validity of the path issued, returns code to identify whether it is a folder/file or unknown path
int check_path(const string& path)
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

int do_w(const string& name, const int sock)
{
    char res[1024] = "";

    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
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

    write_message(sock, res);
    return 0;
}

int do_whoami(const string& name, const int sock)
{
    char res[1024] = "";

    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // retrun username correspeonding to the socket fd 
    map<int, struct User>::iterator it;
    it = active_Users.find(sock);
    if (it != active_Users.end()) {
        strcpy(res, (it->second).uname);
        strcat(res, "\n");
        write_message(sock, res);
    }
    // if user not found, it means that the user is not authenticated but it still accessed the command execution statement (there's a problem) 
    else {
        write_message(sock, ERR_NO_USER_FOUND);
    }

    return 0;
}

void handle_input(const char *command, const int sock)
{
    string input(command);
    istringstream buffer(input);

    vector<string> tokens{istream_iterator<string>(buffer), istream_iterator<string>()};

    wordexp_t p;
    p.we_wordc = 0;
    wordexp(command, &p, 0);

    if (tokens.size() == 0)
    {
        write_message(sock, ERR_NULL_CMD);
        return;
    }

    for (int i = 0; i < NB_COMMANDS; i++)
    {
        if (shell_cmds[i].name == tokens[0])
        {
            if (shell_cmds[i].argc != p.we_wordc - 1)
            {
                write_message(sock, ERR_NBR_PARAMETERS);
                wordfree(&p);
                return;
            }
            string args = string(command + (strlen(shell_cmds[i].name) + 1));
            shell_cmds[i].fct(args, sock);
            wordfree(&p);
            return;
        }
    }
    write_message(sock, ERR_UNKNOWN_CMD);
    wordfree(&p);
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
    //char file[PATH_MAX];

    // taking absolute path of the binary, works on Linux only
    //readlink("/proc/self/exe", file, sizeof(file));

    // string path(file);
    // size_t pos = path.find_last_of("/\\");    
    // path = path.substr(0, pos+1).c_str();

    // strcpy(file, path.c_str());
    // strcat(file, config_file);

    // using absolute path for opening config file so the server can be called from any directory
    FILE *fp = fopen(config_file, "r");

    if (fp == NULL) {
        perror("No configuration file found");
        exit(1);
    }

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
                        if (check_path(t) != IS_DIRECTORY) {
                            perror("directory in config file doesn't exist");
                            exit(1);
                        }
                        strcpy(curr_dir, realpath(t, NULL));
                        chdir(curr_dir);
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
                            strcpy(u.cwd, curr_dir);
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
            perror("accept failed");
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
