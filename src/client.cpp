#include <client.h>

#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <vector>
#include <iterator>
#include <iostream>
using namespace std;


/*
 * Send a file to the server as its own thread
 *
 */
void send_file(FILE *fp, int sock)
{
    char sdbuf[1024]; // Send buffer
    bzero(sdbuf, 1024);
    int f_block_sz;
    while ((f_block_sz = fread(sdbuf, sizeof(char), 1024, fp)) > 0)
    {
        if (send(sock, sdbuf, f_block_sz, 0) < 0)
        {
            break;
        }
        bzero(sdbuf, 1024);
    }
}

/*
 * Recv a file from the server as its own thread
 *
 */
void recv_file(FILE *fp, int size, int sock)
{
    char revbuf[1024];
    bzero(revbuf, 1024);
    int f_block_sz = 0;
    int total_size_recv = 0;
    int to_be_transferred = size;
    while ((f_block_sz = recv(sock, revbuf, 1024, 0)))
    {
        if (f_block_sz < 0)
        {
            printf(ERR_TRANSFER);
            fclose(fp);
            close(sock);
            return;
        }
        total_size_recv += f_block_sz;
        if (to_be_transferred > f_block_sz)
        {
            int write_sz = fwrite(revbuf, sizeof(char), f_block_sz, fp);
            if (write_sz < f_block_sz)
            {
                printf(ERR_TRANSFER);
                fclose(fp);
                close(sock);
                return;
            }
            to_be_transferred -= f_block_sz;
        }
        else
        {
            int write_sz = fwrite(revbuf, sizeof(char), to_be_transferred, fp);
            if (write_sz < to_be_transferred)
            {
                printf(ERR_TRANSFER);
                fclose(fp);
                close(sock);
                return;
            }
            to_be_transferred = 0;
        }

        bzero(revbuf, 1024);
    }
    fclose(fp);

    // if the transferred stream’s size doesn’t match with the specified size
    if (total_size_recv != size)
    {
        printf(ERR_TRANSFER);
        close(sock);
        return;
    }
}

// catch ctrl c to not stop the program
void signalHandler(int)
{
}

void *do_put(void *args)
{
    int port = ((struct args_put *)args)->port;
    char *path = ((struct args_put *)args)->path;
    const char *file = ((struct args_put *)args)->file;

    // CREATION
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        pthread_exit(0);
        return NULL;
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(port);

    // BIND
    if ((bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr))) < 0)
    {
        close(sock);
        pthread_exit(0);
        return NULL;
    }

    // START LISTENING
    if ((listen(sock, 1)) < 0)
    {
        close(sock);
        pthread_exit(0);
        return NULL;
    }

    struct sockaddr_in c_addr;
    int c_addr_len = sizeof(c_addr);
    int sock_new;
    sock_new = accept(sock, (struct sockaddr *)&c_addr, (socklen_t *)&c_addr_len);
    if (sock_new < 0)
    {
        close(sock);
        close(sock_new);
        pthread_exit(0);
        return NULL;
    }
    char file_path[1024];
    strcpy(file_path, path);
    strcat(file_path, "/");
    strcat(file_path, file);
    char *f_name = file_path;

    FILE *fp = fopen(f_name, "r");

    send_file(fp, sock_new);

    close(sock_new);
    close(sock);

    pthread_exit(0);
    return NULL;
}

void *do_get(void *args)
{
    int port = ((struct args_get *)args)->port;
    char *path = ((struct args_get *)args)->path;
    const char *file = ((struct args_get *)args)->file;
    int filesize = ((struct args_get *)args)->filesize;

    int sock;

    // CREATION
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf(ERR_TRANSFER);
        pthread_exit(0);
        return NULL;
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    s_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, "127.0.0.1", &s_addr.sin_addr) < 0)
    {
        printf(ERR_TRANSFER);
        pthread_exit(0);
        return NULL;
    }

    // CONNECTION
    if (connect(sock, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        printf(ERR_TRANSFER);
        pthread_exit(0);
        return NULL;
    }

    char file_path[1024];
    strcpy(file_path, path);
    strcat(file_path, "/");
    strcat(file_path, file);
    char *f_name = file_path;
    FILE *fp = fopen(f_name, "w+");
    if (fp == NULL)
    {
        printf(ERR_TRANSFER);
        close(sock);
        pthread_exit(0);
        return NULL;
    }
    else
    {
        recv_file(fp, filesize, sock);
        pthread_exit(0);
        return NULL;
    }
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
        struct args_put *args = (struct args_put *)malloc(sizeof(struct args_put));
        args->port = atoi(tokens_in[2].c_str());
        args->file = tokens_cmd[1].c_str();
        args->path = path;

        pthread_t t;

        if (pthread_create(&t, NULL, do_put, (void *)args) != 0)
        {
            perror("thread creation failed");
        }

        // Check if the server has successfully received the file or not
        char buff[1024];
        bzero(buff, sizeof(buff));
        read(sock, buff, sizeof(buff));
        printf("%s", buff);
    }

    if (tokens_in[0] == "get")
    {
        struct args_get *args = (struct args_get *)malloc(sizeof(struct args_get));
        args->port = atoi(tokens_in[2].c_str());
        args->file = tokens_cmd[1].c_str();
        args->filesize = atoi(tokens_in[4].c_str());
        args->path = path;

        pthread_t t;

        if (pthread_create(&t, NULL, do_get, (void *)args) != 0)
        {
            perror("thread creation failed");
        }

        // Check if the server has successfully transferred the file or not
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
    char local_path[MAX_BUFF_SIZE];
    if (getcwd(local_path, sizeof(local_path)) == NULL)
    {
        perror("getcwd() error");
        exit(1);
    }

    int sock;

    if (argc == 5)
    {
        automated_mode = true;
    }
    else if (argc != 3)
    {
        printf("Error: wrong number of arguments passed\n");
        exit(1);
    }

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

    char in_buff[MAX_BUFF_SIZE];
    char out_buff[MAX_BUFF_SIZE];
    int n;

    if (automated_mode)
    {
        string line;

        ifstream infile(argv[3]);
        ofstream outfile(argv[4]);
        
        if (!infile.is_open() || !outfile.is_open()) {
            printf("Error in opening files");
            exit(1);
        }

        while (true)
        {
            if (!getline(infile, line)) {
                //line = "exit";
                infile.close();
                outfile.close();
                // redirect to default mode if no exit command is issued
                goto DEFAULT;
            }

            bzero(out_buff, sizeof(out_buff));
            bzero(in_buff, sizeof(in_buff));

            strcpy(out_buff, line.c_str());
            
            int err = check_put_file_exist(out_buff);

            if (err != -1)
            {
                write(sock, out_buff, sizeof(out_buff));

                if ((strcmp(out_buff, "exit")) == 0)
                {
                    break;
                }

                read(sock, in_buff, sizeof(in_buff));
                outfile << in_buff;

                handle_file_transfer(in_buff, out_buff, local_path, sock);
            }
        }
        
        infile.close();
        outfile.close();
    }
    else
    {
        while (true)
        {
            DEFAULT:

            printf("> ");

            bzero(out_buff, sizeof(out_buff));
            bzero(in_buff, sizeof(in_buff));
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
            }

            int err = check_put_file_exist(out_buff);

            if (err != -1)
            {
                write(sock, out_buff, sizeof(out_buff));

                if ((strcmp(out_buff, "exit")) == 0)
                {
                    break;
                }

                read(sock, in_buff, sizeof(in_buff));
                printf("%s", in_buff);

                handle_file_transfer(in_buff, out_buff, local_path, sock);
            }
        }
    }

    close(sock);
    return 0;
}
