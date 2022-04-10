#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#define MAX_BUFFER_SIZE 128
int function_connect(char *argv[])
{
    const char *ip_address;
    int port_number, socket_fd;
    int function_flag = 0;
    ip_address = argv[1];
    port_number = atoi(argv[2]);
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &server_address.sin_addr);
    server_address.sin_port = htons(port_number);
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket_fd=%d.\n", socket_fd);
    if (socket_fd < 0)
    {
        printf("Create socket failure,error(%s).\n", strerror(errno));
        return -1;
    }
    function_flag = connect(socket_fd, (struct sockaddr *)&server_address, sizeof(server_address));
    printf("Connect return value %d.\n", function_flag);
    if (function_flag < 0)
    {
        printf("Connect failure ,error(%s).\n", strerror(errno));
        close(socket_fd);
        return -1;
    }
    return socket_fd;
}
int main(int argc, int *argv[])
{
    int function_flag;
    if (argc < 3)
    {
        printf("Usage :%s ip_address port_number.\n", basename(argv[0]));
        return;
    }
    printf("Chat client start.\n");
    int server_fd;
    server_fd = function_connect(argv);
    printf("Server file descriptor: %d\n", server_fd);
    struct pollfd poll_event[2];
    poll_event[0].fd = 0;
    poll_event[0].events = POLLIN;
    poll_event[0].revents = 0;
    poll_event[1].fd = server_fd;
    poll_event[1].events = POLLIN | POLLRDHUP|POLLOUT;
    poll_event[1].revents = 0;

    char read_buffer[MAX_BUFFER_SIZE];
    char write_buffer[MAX_BUFFER_SIZE];
    //int pipefd[2];
    //function_flag = pipe(pipefd);
    //if (function_flag < 0)
    //{
       // printf("Create pipe failure.\n");
        //return 1;
   // }

    while (1)
    {
        function_flag = poll(poll_event, 2, -1);
        if (function_flag < 0)
        {
            printf("Poll error(%s).\n", strerror(errno));
            break;
        }
        if (poll_event[1].revents & POLLRDHUP)
        {
            printf("Server close the connection.\n");
            break;
        }
        else if (poll_event[1].revents & POLLIN)
        {
            memset(read_buffer, '\0', sizeof(read_buffer));
            recv(poll_event[1].fd, read_buffer, MAX_BUFFER_SIZE - 1, 0);
            printf("%s.\n", read_buffer);
        }
        else if (poll_event[1].revents & POLLOUT)
        {
            memset(write_buffer,'\0',MAX_BUFFER_SIZE-1);
            printf("$:");
            fgets(write_buffer, MAX_BUFFER_SIZE - 1, stdin);
            if (!strrchr(write_buffer, '\n'))
            {
                write_buffer[strlen(write_buffer)-1]='\n';
            }
                function_flag=send(poll_event[1].fd,write_buffer,strlen(write_buffer)+1,0);
                printf("Send %d message.\n",function_flag);

            // splice(0,NULL,pipefd[1],NULL,1024,SPLICE_F_MORE|SPLICE_F_MOVE);
            // splice(pipefd[0],NULL,server_fd,NULL,1024,SPLICE_F_MORE|SPLICE_F_MOVE);
        }
    }
    close(server_fd);
    return 0;
}