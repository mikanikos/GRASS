#include <server.h>
#include <errmsg.h>
#include <ctype.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <list> 
#include <map> 
#include <set>
#include <pthread.h>
#include <wordexp.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
using namespace std;

// map socket to user
map<int, struct User> active_Users;

// list of users allowed to login
list<struct User> userList;

char port[7];
char curr_dir[PATH_MAX];

// initialize lock
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;


// function to write message to the client and server console
void write_message(const int sock, const char *message) 
{
    write(sock, message, 1024);
    printf("%s", message);
}

// Helper function to run commands in unix.
void run_command(const char *command, const int sock)
{
    char buff[MAX_BUFF_SIZE], result[MAX_BUFF_SIZE] = "";
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
    // search for user in paramters of config file 
    for (auto const& it : userList) {
        if (it.uname == name) {
            // add to map in order to keep track the activity
            active_Users[sock] = it;
            write(sock, "", sizeof(""));
            return 0;
        }
    }
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
        if ((it->second).pass == name) {
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

    run_command(("ping " + name + " -c 1").c_str(), sock);
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

    run_command((string("ls -l ") + active_Users[sock].cwd).c_str(), sock);
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

    run_command(("grep -rl " + name + " | sort -t: -n -k2").c_str(), sock);
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
    wordexp_t p;
    wordexp(name.c_str(), &p, 0);

    string folderName = string(p.we_wordv[0]);

    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // check path length
    if (folderName.length() > PATH_MAX) {
        write_message(sock, ERR_PATH_LONG);
        return 1;
    }

    // using user cwd
    string nname = string(active_Users[sock].cwd) + "/" + folderName;

    // check if the path exists and it's a directory
    if (check_path(nname) == IS_DIRECTORY)
    {
        // check permission of path
        if (strstr(realpath(nname.c_str(), NULL), curr_dir) == NULL) {
            write_message(sock, ERR_ACCESS_DENIED);
            return 1;
        }

        //chdir(nname.c_str());
        strcpy(active_Users[sock].cwd, realpath(nname.c_str(), NULL));
        write(sock, "", sizeof(""));
    }
    else
    {
        if (check_path(nname) == IS_FILE)
        {
            write_message(sock, ("cd: " + name + ": Not a directory\n").c_str());
        }
        else {
            write_message(sock, ("cd: " + name + ": No such file or directory\n").c_str());
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

        run_command(("mkdir " + nname).c_str(), sock);
    }
    else {
        write_message(sock, ("mkdir: cannot create '" + name + "': File exists\n").c_str());
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

        run_command(("rm -r " + nname).c_str(), sock);
    }
    else {
        write_message(sock, ("rm: cannot remove '" + name + "': No such file or directory\n").c_str());
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
    // check if the user is allowed to execute the command
    if (!check_authentication(sock)) {
        active_Users.erase(sock);
        write_message(sock, ERR_ACCESS_DENIED);
        return 1;
    }

    // go through all the active users in the system and check who's loggedin
    std::set<std::string> sortedUsers;
    for (auto const& it : active_Users) {
        if ((it.second).isLoggedIn) {
            sortedUsers.insert((it.second).uname);
        }
    }

    // sort users names
    ostringstream stream;
    copy(sortedUsers.begin(), sortedUsers.end(), std::ostream_iterator<std::string>(stream, " "));

    write_message(sock, (stream.str() + "\n").c_str());
    return 0;
}

int do_whoami(const string& name, const int sock)
{
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
        write_message(sock, ((it->second).uname + "\n").c_str());
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
    else if (p.we_wordc == 0) {
        write_message(sock, ERR_UNDEFINED_CHARS);
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
    FILE *fp = fopen(config_file.c_str(), "r");

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
                            u.uname = string(name);
                            u.pass = string(pass);
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
    s_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
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
    else {
        string message = "Listening on address " + server_ip + " and port " + string(port);
        perror(message.c_str());
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
