#include <grass.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>

using namespace std;


static bool automated_mode = false;

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to send
 * d_port: destination port
 */
void send_file(int fp, int d_port)
{
    // TODO
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

int main(int argc, char **argv)
{
    // TODO:
    // Make a short REPL to send commands to the server
    // Make sure to also handle the special cases of a get and put command

    signal(SIGINT, signalHandler);

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
        printf("creation failed");
        exit(1);
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(argv[1]);
    s_addr.sin_port = htons(atoi(argv[2]));

    if (inet_pton(AF_INET, argv[1], &s_addr.sin_addr) < 0)
    {
        printf("invalid address");
        exit(1);
    }

    // CONNECTION
    if (connect(sock, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)
    {
        printf("connect failed");
        exit(1);
    }

    char buff[1024];
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

            bzero(buff, sizeof(buff));

            strcpy(buff, line.c_str());
            
            write(sock, buff, sizeof(buff));

            if ((strcmp(buff, "exit")) == 0)
            {
                //outfile << "Exit successfully\n";
                break;
            }

            bzero(buff, sizeof(buff));
            read(sock, buff, sizeof(buff));

            outfile << buff;
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

            bzero(buff, sizeof(buff));
            n = 0;

            char c = getchar();
            while (c != '\n' && c != EOF)
            {
                buff[n] = c;
                n++;
                c = getchar();
            }

            // ctrl d catch
            if (c == EOF)
            {
                strcpy(buff, "exit");
            }

            write(sock, buff, sizeof(buff));

            if ((strcmp(buff, "exit")) == 0)
            {
                break;
            }

            bzero(buff, sizeof(buff));
            read(sock, buff, sizeof(buff));
            printf("%s", buff);
        }
    }

    close(sock);
    return 0;
}
