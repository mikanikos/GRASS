#include <grass.h>

/*
 * Send a file to the server as its own thread
 *
 * fp: file descriptor of file to send
 * d_port: destination port
 */
void send_file(int fp, int d_port) {
    // TODO
}

/*
 * Recv a file from the server as its own thread
 *
 * fp: file descriptor of file to save to.
 * d_port: destination port
 * size: the size (in bytes) of the file to recv
 */
void recv_file(int fp, int d_port, int size) {
    // TODO
}


/*
 * search all files in the current directory
 * and its subdirectory for the pattern
 *
 * pattern: an extended regular expressions.
 */
void search(char *pattern) {
    // TODO
}

int main(int argc, char **argv) {
    // TODO:
    // Make a short REPL to send commands to the server
    // Make sure to also handle the special cases of a get and put command

    int sock; 

    if (argc != 3)
        exit(0);

    // CREATION 
    sock = socket(AF_INET, SOCK_STREAM, 0); 
    if (sock < 0) {
        perror("creation failed");
        exit(0); 
    }
    
    struct sockaddr_in s_addr;  
    s_addr.sin_family = AF_INET;
    s_addr.sin_addr.s_addr = inet_addr(argv[1]);
    s_addr.sin_port = htons(atoi(argv[2]));
  
    if (inet_pton(AF_INET, argv[1], &s_addr.sin_addr) < 0)  
    { 
        perror("invalid address"); 
        exit(0);
    }

    // CONNECTION 
    if (connect(sock, (struct sockaddr*)&s_addr, sizeof(s_addr)) < 0)  {
        perror("connect failed");   
        exit(0);
    }

    printf("CONNECTED\n");
    
    while(true);
  
    close(sock); 
}
