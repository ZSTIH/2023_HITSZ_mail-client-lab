#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095

#define swap16(x) ((((x)&0xFF) << 8) | (((x) >> 8) & 0xFF))

char buf[MAX_SIZE+1];

void recv_from_server(int s_fd, void* buf, int flags, char* err_msg)
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

char* get_content_from_file(const char *path)
{
    FILE *fp = fopen(path, "r");

    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        long file_size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        char *content = (char *) malloc(file_size);
        fread(content, 1, file_size, fp);
        fclose(fp);
        return content;
    }

    return NULL;
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.163.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "example@163.com"; // TODO: Specify the user
    const char* pass = "**********"; // TODO: Specify the password
    const char* from = "example@163.com"; // TODO: Specify the mail address of the sender
    char dest_ip[16]; // Mail server IP address
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;

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

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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

    // Send EHLO command and print server response
    const char* EHLO = "EHLO 163.com\r\n"; // TODO: Enter EHLO command here
    send_to_server(s_fd, (void *) EHLO, strlen(EHLO), 0, "send EHLO");
    // TODO: Print server response to EHLO command
    recv_from_server(s_fd, (void *) buf, 0, "recv EHLO");
    
    // TODO: Authentication. Server response should be printed out.
    const char* AUTH = "AUTH login\r\n";
    send_to_server(s_fd, (void *) AUTH, strlen(AUTH), 0, "send AUTH");
    recv_from_server(s_fd, (void *) buf, 0, "recv AUTH");

    char* user64 = encode_str(user);
    send_to_server(s_fd, (void *) user64, strlen(user64), 0, "send user64");
    recv_from_server(s_fd, (void *) buf, 0, "recv user64");

    char* pass64 = encode_str(pass);
    send_to_server(s_fd, (void *) pass64, strlen(pass64), 0, "send pass64");
    recv_from_server(s_fd, (void *) buf, 0, "recv pass64");

    // TODO: Send MAIL FROM command and print server response
    sprintf(buf, "MAIL FROM:<%s>\r\n", from);
    send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send MAIL FROM");
    recv_from_server(s_fd, (void *) buf, 0, "recv MAIL FROM");
    
    // TODO: Send RCPT TO command and print server response
    sprintf(buf, "RCPT TO:<%s>\r\n", receiver);
    send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send RCPT TO");
    recv_from_server(s_fd, (void *) buf, 0, "recv RCPT TO");
    
    // TODO: Send DATA command and print server response
    const char* DATA = "DATA\r\n";
    send_to_server(s_fd, (void *) DATA, strlen(DATA), 0, "send DATA");
    recv_from_server(s_fd, (void *) buf, 0, "recv DATA");

    // TODO: Send message data
    const char* boundary = "qwertyuiopasdfghjklzxcvbnm";
    // send header
    sprintf(buf,
    "FROM: %s\r\nTo: %s\r\nContent-Type: multipart/mixed; boundary=%s\r\n",
    from, receiver, boundary);
    if (subject != NULL)
    {
        strcat(buf, "Subject: ");
        strcat(buf, subject);
        strcat(buf, "\r\n\r\n");
    }
    send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send DATA header");

    // send message
    if (msg)
    {
        sprintf(buf, "--%s\r\nContent-Type:text/plain\r\n\r\n", boundary);
        send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send DATA msg header");
        if (!access(msg, F_OK))
        {
            char *content = get_content_from_file(msg);
            strcat(content, "\r\n");
            send_to_server(s_fd, (void *) content, strlen(content), 0, "send DATA msg content");
            free(content);
        }
        else
        {
            char *content_end = "\r\n";
            send_to_server(s_fd, (void *) msg, strlen(msg), 0, "send DATA msg content");
            send_to_server(s_fd, (void *) content_end, strlen(content_end), 0, "send DATA msg content");
        }
    }

    // send attachment
    if (att_path)
    {

        FILE *fp = fopen(att_path, "r");
        if (!fp)
        {
            perror("file not found");
            exit(EXIT_FAILURE);
        }

        sprintf(buf,
        "--%s\r\nContent-Type:application/octet-stream\r\nContent-Disposition:attachment; name=%s\r\nContent-Transfer-Encoding: base64\r\n\r\n",
        boundary, att_path);
        send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send DATA att header");

        FILE *fp_temp = fopen("temp", "w");
        encode_file(fp, fp_temp);
        fclose(fp);
        fclose(fp_temp);
        char *content = get_content_from_file("temp");
        send_to_server(s_fd, (void *) content, strlen(content), 0, "send DATA att content");
        remove("temp");        
        free(content);
    }
    sprintf(buf, "--%s--\r\n", boundary);
    send_to_server(s_fd, (void *) buf, strlen(buf), 0, "send DATA end");

    // TODO: Message ends with a single period
    send_to_server(s_fd, (void *) end_msg, strlen(end_msg), 0, "send END");
    recv_from_server(s_fd, (void *) buf, 0, "recv END");

    // TODO: Send QUIT command and print server response
    const char* QUIT = "QUIT\r\n";
    send_to_server(s_fd, (void *) QUIT, strlen(QUIT), 0, "send QUIT");
    recv_from_server(s_fd, (void *) buf, 0, "recv QUIT");

    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
