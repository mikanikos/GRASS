#include <grass.h>
#include <ctype.h>

static struct User **userlist;
static int numUsers;
static struct Command **cmdlist;
static int numCmds;
//char port[7] = "31337";
char port[7];
char curr_dir[10];

// Helper function to run commands in unix.
void run_command(const char *command, int sock)
{
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

// Server side REPL given a socket file descriptor
void *connection_handler(void *sockfd)
{
    printf("Handling connection\n");
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

    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (buffer[0] == '#' || buffer[0] == '\n')
            continue;
        else {
            s = strtok(buffer, " ");
            if (s != NULL) {
                if (strcmp(s, "base") == 0) {
                    t = strtok(NULL, "\n");
                    if (t != NULL) {;
                        strcpy(curr_dir, t);
                        printf("base: %s\n", curr_dir);
                    }
                }
                else if (strcmp(s, "port") == 0) {
                    t = strtok(NULL, "\n");
                    if (t != NULL) {
                        strcpy(port, t);
                        printf("port: %s\n", port);
                    }
                }
                else if (strcmp(s, "user") == 0) {
                    char name[50], pass[50];
                    t = strtok(NULL, " ");
                    if (t != NULL) {
                        strncpy(name, t, 50);
                        t = strtok(NULL, "\n");
                        if (t != NULL) {
                            strncpy(pass, t, 50);
                            printf("User: %s %s\n", name, pass);
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
    printf("Parsing grass.conf file...\n");
    parse_grass();
    
    // Listen to the port and handle each connection

    // CREATION
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("creation failed");
        exit(0);
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr =  inet_addr("127.0.0.1"); // htonl(INADDR_ANY);
    s_addr.sin_port = htons(atoi(port));

    // BIND
    if ((bind(sock, (struct sockadrr_in *)&s_addr, sizeof(s_addr))) < 0) {
        perror("bind failed");
        exit(0);
    }

    // START LISTENING
    if ((listen(sock, 5)) < 0) {
        perror("listen failed");
        exit(0);
    }

    struct sockaddr_in c_addr;
    int c_addr_len = sizeof(c_addr);

    // ACCEPT
    while (true)
    {
        int sock_new;

        sock_new = accept(sock, (struct sockaddr_in *)&c_addr, (socklen_t *)&c_addr_len);
        if (sock_new < 0) {
            perror("accept failed");   
            exit(0);
        }

        connection_handler(&sock_new);

        close(sock_new);
    }
    
    close(sock);
}
