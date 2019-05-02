#include <grass.h>
#include <sys/wait.h>
#include <signal.h>
#include <message.h>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <iostream>

using namespace std;

#define PATH_MAX 1024
/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to send
 * d_port: destination port
 */
void send_file(FILE *fp, int d_port, int sock)
{
    char sdbuf[1024]; // Send buffer
    bzero(sdbuf, 1024);
    // printf("sending file put\n");
    // fflush(stdout);
    int f_block_sz;
    while ((f_block_sz = fread(sdbuf, sizeof(char), 1024, fp)) > 0)
    {
        if (send(sock, sdbuf, f_block_sz, 0) < 0)
        {
            printf(ERR_TRANSFER);
            break;
        }
        bzero(sdbuf, 1024);
    }
    // printf("send success put\n");
    // fflush(stdout);
}

/*
 * Recv a file from the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * d_port: destination port
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int d_port, int size)
{
    // TODO
}

/*
 * search all files in the current directory
 * and its subdirectory for the pattern
 *
 * pattern: an extended regular expressions.
 */
void search(char *pattern)
{
    // TODO
}

// catch ctrl c to not stop the program
void signalHandler(int sig_num)
{
}

void do_put(int port, char *path, const char *file, int size)
{
    // printf("start put\n");
    // fflush(stdout);
    // CREATION
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("creation failed\n");
        return;
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(port);

    // BIND
    if ((bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr))) < 0)
    {
        perror("bind failed\n");
        close(sock);
        return;
    }

    // START LISTENING
    if ((listen(sock, 1)) < 0)
    {
        perror("listen failed\n");
        close(sock);
        return;
    }
    // printf("listening put\n");
    // fflush(stdout);

    struct sockaddr_in c_addr;
    int c_addr_len = sizeof(c_addr);
    int sock_new;
    sock_new = accept(sock, (struct sockaddr *)&c_addr, (socklen_t *)&c_addr_len);
    if (sock_new < 0)
    {
        printf("accept failed\n");
        close(sock);
        close(sock_new);
        return;
    }
    char file_path[1024];
    strcpy(file_path, path);
    strcat(file_path, "/");
    strcat(file_path, file);
    char *f_name = file_path;

    FILE *fp = fopen(f_name, "r");
    send_file(fp, port, sock_new);

    close(sock_new);
    close(sock);
}

void handle_file_transfer(char *in, char *cmd, char *path, int sock)
{
    string input(in);
    istringstream buffer_in(input);
    vector<string> tokens_in{istream_iterator<string>(buffer_in), istream_iterator<string>()};

    string command(cmd);
    istringstream buffer_cmd(command);
    vector<string> tokens_cmd{istream_iterator<string>(buffer_cmd), istream_iterator<string>()};

    if (tokens_cmd.size() == 0 || tokens_in.size() == 0)
    {
        return;
    }

    if (tokens_in[0] == "put")
    {
        int port = atoi(tokens_in[2].c_str());
        const char *file = tokens_cmd[1].c_str();
        FILE *fp = fopen(file, "r");
        if (fp == NULL)
        {
            printf(ERR_FILE_NOT_FOUND);
            return;
        }
        int file_sz = atoi(tokens_cmd[2].c_str());
        do_put(port, path, file, file_sz);

        // Check if the server has successfully received the file or not
        char buff[1024];
        bzero(buff, sizeof(buff));
        read(sock, buff, sizeof(buff));
        printf("%s", buff);
    }
}

int check_put_file_exist(char *command)
{
    string input(command);
    istringstream buffer_in(input);
    vector<string> tokens{istream_iterator<string>(buffer_in), istream_iterator<string>()};

    if (tokens.size() == 0)
    {
        return 0;
    }

    if (tokens[0] == "put")
    {
        const char *file = tokens[1].c_str();
        FILE *fp = fopen(file, "r");
        if (fp == NULL)
        {
            printf(ERR_FILE_NOT_FOUND);
            return -1;
        }
    }

    return 0;
}

int main(int argc, char **argv)
{
    // TODO:
    // Make a short REPL to send commands to the server
    // Make sure to also handle the special cases of a get and put command

    signal(SIGINT, signalHandler);

    // current local path where the client was started (used in do_put to send files from there)
    char local_path[PATH_MAX];
    if (getcwd(local_path, sizeof(local_path)) == NULL)
    {
        perror("getcwd() error");
        exit(1);
    }

    int sock;

    if (argc != 3)
        exit(1);

    // CREATION
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("creation failed\n");
        exit(1);
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(argv[1]);
    s_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &s_addr.sin_addr) < 0)
    {
        printf("invalid address\n");
        exit(1);
    }

    // CONNECTION
    if (connect(sock, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        printf("connect failed\n");
        exit(1);
    }

    char in_buff[1024];
    char out_buff[1024];
    int n;
    while (true)
    {
        bzero(in_buff, sizeof(in_buff));
        bzero(out_buff, sizeof(out_buff));
        printf("grass> ");
        n = 0;

        char c = getchar();
        while (c != '\n' && c != EOF)
        {
            out_buff[n] = c;
            n++;
            c = getchar();
        }

        // ctrl d catch
        if (c == EOF)
        {
            strcpy(out_buff, "exit");
            write(sock, out_buff, sizeof(out_buff));
            printf("\nExit successfully\n");
            break;
        }

        int err = check_put_file_exist(out_buff);

        if (err != -1)
        {
            write(sock, out_buff, sizeof(out_buff));

            if ((strcmp(out_buff, "exit")) == 0)
            {
                printf("\nExit successfully\n");
                break;
            }

            read(sock, in_buff, sizeof(in_buff));
            printf("%s", in_buff);

            handle_file_transfer(in_buff, out_buff, local_path, sock);
        }
    }

    close(sock);
    return 0;
}
