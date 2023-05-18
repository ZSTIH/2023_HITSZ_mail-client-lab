#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

#define swap16(x) ((((x)&0xFF) << 8) | (((x) >> 8) & 0xFF))

char buf[MAX_SIZE+1];

int recv_from_server(int s_fd, void* buf, int flags, char* err_msg)
{
    int r_size = -1;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror(err_msg);
        exit(EXIT_FAILURE);
    }
    char* buf_temp = (char *) buf;
    buf_temp[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf_temp);
    return r_size;
}

void send_to_server(int s_fd, void* buf, int len, int flags, char* err_msg)
{
    int s_size = -1;
    if ((s_size = send(s_fd, buf, len, 0)) == -1)
    {
        perror(err_msg);
        exit(EXIT_FAILURE);
    }
    char* buf_temp = (char *) buf;
    printf("%s", buf_temp);
}

void recv_mail()
{
    const char* host_name = "pop.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "example@qq.com"; // TODO: Specify the user
    const char* pass = "**********"; // TODO: Specify the password
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = swap16(port);
    servaddr.sin_addr = (struct in_addr) {inet_addr(dest_ip)};
    bzero(servaddr.sin_zero, 8);
    connect(s_fd, (struct sockaddr *) (&servaddr), sizeof(struct sockaddr_in));

    // Print welcome message
    recv_from_server(s_fd, (void *) buf, 0, "recv welcome");

    // TODO: Send user and password and print server response
    sprintf(buf, "USER %s\r\n", user);
    send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send USER");
    recv_from_server(s_fd, (void *) buf, 0, "recv USER");

    sprintf(buf, "PASS %s\r\n", pass);
    send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send PASS");
    recv_from_server(s_fd, (void *) buf, 0, "recv PASS");

    // TODO: Send STAT command and print server response
    const char* STAT = "STAT\r\n";
    send_to_server(s_fd, (void *) STAT, strlen(STAT), 0, "send STAT");
    recv_from_server(s_fd, (void *) buf, 0, "recv STAT");

    // TODO: Send LIST command and print server response
    const char* LIST = "LIST\r\n";
    send_to_server(s_fd, (void *) LIST, strlen(LIST), 0, "send LIST");
    recv_from_server(s_fd, (void *) buf, 0, "recv LIST");

    // TODO: Retrieve the first mail and print its content
    int size;
    const char* RETR1 = "RETR 1\r\n";
    send_to_server(s_fd, (void *) RETR1, strlen(RETR1), 0, "send RETR 1");
    r_size = recv_from_server(s_fd, (void *) buf, 0, "recv RETR 1");
    sscanf(buf, "+OK %d octets", &size);
    size -= r_size;
    while (size > 0)
    {
        r_size = recv_from_server(s_fd, (void *) buf, 0, "recv RETR 1");
        size -= r_size;           
    }

    // TODO: Send QUIT command and print server response
    const char* QUIT = "QUIT\r\n";
    send_to_server(s_fd, (void *) QUIT, strlen(QUIT), 0, "send QUIT");
    recv_from_server(s_fd, (void *) buf, 0, "recv QUIT");

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
