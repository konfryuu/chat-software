#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>
#define MAX_USER_LIMIT 200
#define MAX_BUFFER_SIZE 128
#define MAX_CLIENT_DATA_NUMBER 200
typedef struct ClientData
{
    int client_fd;
    struct sockaddr_in address;
    char *write_buffer_pt;
    char buffer[MAX_BUFFER_SIZE];
    // int buffer_current_point;
    /* data */
} client_data;
typedef struct ClientArray
{
    int client_array[MAX_USER_LIMIT];
    int current_user_number;
    /* data */
} client_array_struct;
void client_array_initialize(client_array_struct *client_array_buffer_pt)
{
    memset(client_array_buffer_pt->client_array, 0, sizeof(client_array_buffer_pt->client_array));
    client_array_buffer_pt->current_user_number = 0;
}
int client_array_add(int client_fd, client_array_struct *client_array_buffer_pt)
{
    if (client_array_buffer_pt->current_user_number < MAX_USER_LIMIT)
    {
        client_array_buffer_pt->client_array[client_array_buffer_pt->current_user_number] = client_fd;
        client_array_buffer_pt->current_user_number++;
        return 0;
    }
    else
    {
        printf("The number of people excessed limit.\n");
        return -1;
    }
}
int client_array_find(int client_fd, client_array_struct *client_array_buffer_pt)
{
    for (size_t i = 0; i < client_array_buffer_pt->current_user_number; i++)
    {
        if (client_fd == client_array_buffer_pt->client_array[i])
            return i;
    }
    return -1;
}
int client_array_delete(int client_fd, client_array_struct *client_array_buffer_pt)
{
    int i;
    if (client_array_buffer_pt->current_user_number > 0)
    {
        i = client_array_find(client_fd, client_array_buffer_pt);
        client_array_buffer_pt->client_array[i] = client_array_buffer_pt->client_array[client_array_buffer_pt->current_user_number - 1];
        client_array_buffer_pt->current_user_number--;
        return 0;
    }
    else
    {
        printf("The client array is empty.\n");
        return -1;
    }
}
int client_array_find_max_fd(client_array_struct *client_array_buffer_pt)
{
    if (client_array_buffer_pt->current_user_number <= 0)
    {
        printf("The array is empty.\n");
        return -1;
    }
    int temp_fd = 0;
    for (size_t i = 0; i < client_array_buffer_pt->current_user_number; i++)
    {
        if (temp_fd < client_array_buffer_pt->client_array[i])
            temp_fd = client_array_buffer_pt->client_array[i];
    }
    return temp_fd;
}
int setnonblock(int fd) // Set nonblock io mode.
{
    int old_flag, new_flag;
    old_flag = fcntl(fd, F_GETFL);
    new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return fd;
}
int fun_listen(char *argv[]) // Listen socket and bind local address, return listen file descriptor.
{
    const char *ip_addr = argv[1];
    int port_number = atoi(argv[2]);
    int function_flag = 0;
    struct sockaddr_in inet_address;
    memset(&inet_address, 0, sizeof(inet_address));
    inet_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip_addr, &inet_address.sin_addr);
    inet_address.sin_port = htons(port_number);
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("fun_listen listen_fd %d.\n", listenfd);
    if (listenfd < 0)
    {
        printf("The socket connect failure.\n");
        return -1;
    }
    function_flag = bind(listenfd, (struct sockaddr *)&inet_address, sizeof(inet_address));
    if (function_flag == -1)
    {
        printf("Bind failure.\n");
        printf("Bind failure ,error(%s).\n", strerror(errno));
        return -1;
    }
    function_flag = listen(listenfd, 5);
    if (function_flag == -1)
    {
        printf("Listen failure.\n");
        printf("Listen failure ,error(%s).\n", strerror(errno));
        return -1;
    }
    printf("Start listen.\n");
    return listenfd;
}
int fun_accept(int listen_fd, struct sockaddr_in *client_address_pt, socklen_t *client_address_len_pt) // Connect client.
{
    int accept_fd = accept(listen_fd, client_address_pt, client_address_len_pt);
    if (accept_fd < 0)
    {
        printf("Accept error:%s.\n", strerror(errno));
        return -1;
    }
    printf("Accept file descriptor:%d.\n", accept_fd);
    accept_fd = setnonblock(accept_fd);
    return accept_fd;
}
int add_event(int epfd, int fd) // Add file descriptor to epoll interest list.
{
    int function_flag;
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP;
    event.data.fd = fd;
    function_flag = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
    if (function_flag < 0)
    {
        printf("Epoll_ctl error: %s.\n", strerror(errno));
        return -1;
    }
    return 0;
}
int send_error_to_client(int client_fd, char *buffer_pt, int buffer_len)
{
    int flag;
    flag = write(client_fd, buffer_pt, buffer_len);
    if (flag != buffer_len)
    {
        printf("Server send error information error.\n");
    }
    return 0;
}
void add_client(int client_fd, client_data *client_data_buffer, struct sockaddr_in *client_address_pt)
{
    client_data_buffer[client_fd].client_fd = client_fd;
    client_data_buffer[client_fd].address = *client_address_pt;
    client_data_buffer[client_fd].write_buffer_pt = NULL;
    memset(client_data_buffer[client_fd].buffer, '\0', sizeof(client_data_buffer[client_fd].buffer));
    // client_data_buffer[client_fd].buffer_current_point = 0;
}
void del_client(int client_fd, client_data *client_data_buffer, client_array_struct *client_array_buffer_pt)
{
    client_data_buffer[client_fd].client_fd = -1;
    client_data_buffer[client_fd].write_buffer_pt = NULL;
    client_array_delete(client_fd, client_array_buffer_pt);
}
//{
// int temp_fd;
// temp_fd = client_fd;
// close(client_fd);
// client_data_buffer[*max_client_fd].client_fd = dup2(client_data_buffer[*max_client_fd].client_fd, temp_fd);
// memcpy(&client_data_buffer[temp_fd], &client_data_buffer[*max_client_fd], sizeof(client_data_buffer[*max_client_fd]));
// temp_fd = *max_client_fd;
// close(*max_client_fd);
// *max_client_fd = temp_fd - 1;
//}
void change_event(int epoll_fd, struct epoll_event *epoll_event_pt, client_array_struct *client_array_buffer_pt)
{
    int temp_fd, function_flag;
    for (size_t i = 0; i < client_array_buffer_pt->current_user_number; i++)
    {
        temp_fd = client_array_buffer_pt->client_array[i];
        function_flag = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, temp_fd, epoll_event_pt);
        if (function_flag == -1)
        {
            printf("Change_event epoll_ctl error, file descriptor: %d.\n", i);
            printf("Error information: %s.\n", strerror(errno));
        }
    }
}
//{
// int function_flag;

// for (size_t i = listen_fd + 1; i <= max_client_fd; i++)
//{
//     (*epoll_event_pt).data.fd = i;

//}
//}
void send_message(int client_fd, int listen_fd, client_data *client_data_buffer, int max_user_number)
{
    for (size_t i = 0; i < max_user_number; i++)
    {
        int j = listen_fd + 1;
        if (client_data_buffer[j].client_fd == client_fd)
        {
            j++;
            continue;
        }
        else
        {
            client_data_buffer[j].write_buffer_pt = client_data_buffer[client_fd].buffer;
            j++;
        }
        /* code */
    }
}
int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage:%s ip_address port_number.\n", basename(argv[0]));
        return 0;
    }
    printf("Chat server start.\n");
    int listen_fd = fun_listen(argv);
    int epoll_flag = 0;
    int max_user_number = 0;
    // int max_client_fd = -1;
    int epoll_fd;
    client_array_struct client_array_buffer;
    client_array_initialize(&client_array_buffer);
    struct epoll_event events[MAX_USER_LIMIT + 1]; // Epoll event array, the array is used for epoll_wait function to return ready events............................................
    // char buffer[BUFFER_MAX_SIZE];
    client_data *client_data_buffer = (client_data *)malloc(MAX_CLIENT_DATA_NUMBER * sizeof(client_data)); // Allocat memory for client data buffer..........................................
    if (client_data_buffer == NULL)
    {
        printf("Allocate memory failure, server exit.\n");
        return -1;
    }
    for (size_t i = 0; i < MAX_CLIENT_DATA_NUMBER; i++) // Initialize client data buffer file descriptor..........................
    {
        client_data_buffer[i].client_fd = -1;
        /* code */
    }

    struct sockaddr_in client_addr; // Client data temp buffer, the buffer is used for fun_accept and add_client function to deliver necessary data................................
    socklen_t client_addr_len = sizeof(client_addr);
    epoll_fd = epoll_create(5); // Create epoll evevt.........................
    int function_flag;
    struct epoll_event event; // Epoll event buffer................
    event.data.fd = listen_fd;
    event.events = EPOLLIN | EPOLLET;
    function_flag = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &event);
    if (function_flag == -1)
    {
        printf("Epoll_ctl add listen file descriptor occurred error.\n");
        printf("Error information: %s.\n", strerror(errno));
        return -1;
    }
    while (1)
    {
        epoll_flag = epoll_wait(epoll_fd, events, MAX_USER_LIMIT + 1, -1); // Epoll_wait function wait socket event.................................
        printf("Epoll_flag: %d.\n", epoll_flag);
        for (size_t i = 0; i < epoll_flag; i++)
        {
            printf("i=%d.\n", i);
            if (events[i].data.fd == listen_fd) //
            {
                int accept_fd;
                memset(&client_addr, 0, client_addr_len);
                accept_fd = fun_accept(listen_fd, &client_addr, &client_addr_len);
                if (accept_fd == -1)
                {
                    continue;
                }
                if (max_user_number > MAX_USER_LIMIT)
                {
                    char *char_flag;
                    // int flag;
                    char error_buffer;
                    error_buffer = (char *)malloc(64 * sizeof(char));
                    char presentation_client_ip[16];
                    char_flag = inet_ntop(AF_INET, &client_addr.sin_addr, presentation_client_ip, sizeof(presentation_client_ip));
                    if (char_flag == NULL)
                    {
                        printf("Could't convet client address to string.\n");
                        printf("Error :%s.\n", strerror(errno));
                        printf("Connect failure.\n");
                        continue;
                    }
                    printf("The client exceed the maximum number,connect failure.\n ");
                    snprintf(error_buffer, sizeof(error_buffer), "The client exceed the maximum number,connect failure.\n ");
                    printf("Client address: %s.\n", presentation_client_ip);
                    send_error_to_client(accept_fd, error_buffer, sizeof(error_buffer));
                    close(accept);
                    continue;
                    /* code */
                }
                client_array_add(accept_fd, &client_array_buffer);
                add_client(accept_fd, client_data_buffer, &client_addr);
                add_event(epoll_fd, accept_fd);
                // max_client_fd = accept_fd;
                max_user_number++;
                printf("Add client success.\n");
            }
            else if (events[i].events && EPOLLIN)
            {
                int flag = 0;
                char *char_flag;
                char presentation_client_ip[16];
                int port_number;
                char client_receive_buffer[MAX_BUFFER_SIZE];
                int client_fd = events[i].data.fd;
                char_flag = inet_ntop(AF_INET, &client_data_buffer[client_fd].address.sin_addr, presentation_client_ip, sizeof(presentation_client_ip));
                if (char_flag == NULL)
                {
                    printf("Could't convet client address to string.\n");
                    printf("Error :%s.\n", strerror(errno));
                    memset(presentation_client_ip, '\0', sizeof(presentation_client_ip));
                }
                port_number = ntohs(client_data_buffer[client_fd].address.sin_port);
                if(port_number==0)
                {
                    break;
                }
                memset(client_data_buffer[client_fd].buffer, '\0', sizeof(client_data_buffer[client_fd].buffer));
                // client_data_buffer[client_fd].buffer_current_point = 0;
                while (1)
                {
                    int function_flag = 0;
                    memset(client_receive_buffer, '\0', MAX_BUFFER_SIZE);
                    function_flag = recv(client_fd, client_receive_buffer, MAX_BUFFER_SIZE - flag - 1, 0);
                    printf("Accept %d message from %s:%d.\n", function_flag, presentation_client_ip, port_number);
                    printf("The message :%s.\n", client_receive_buffer);
                    if (function_flag > 0)
                    {
                        strncpy(client_data_buffer[client_fd].buffer + flag, client_receive_buffer, sizeof(client_data_buffer[client_fd].buffer + flag));
                        flag += function_flag;
                        // client_data_buffer[client_fd].buffer_current_point += function_flag;
                        if (flag >= MAX_BUFFER_SIZE - 1)
                        {
                            if (strrchr(!client_data_buffer[client_fd].buffer, '\n'))
                            {
                                client_data_buffer[client_fd].buffer[sizeof(client_data_buffer[client_fd].buffer) - 2] = '\n';
                            }
                            break;
                        }
                    }
                    else if (function_flag == 0)
                    {
                        int temp_fd;
                        temp_fd = client_fd;
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        del_client(client_fd, client_data_buffer, &client_array_buffer);
                        max_user_number--;
                        break;
                        /* code */
                    }
                    else
                    {
                        if (errno == EAGAIN | errno == EWOULDBLOCK)
                            break;
                        else
                        {
                            printf("An unknowed error occurred.\n");
                            del_client(client_fd, client_data_buffer, &client_array_buffer);
                            max_user_number--;
                            break;
                        }
                    }
                    /* code */
                }
                if (sizeof(client_data_buffer[client_fd].buffer)) // if the buffer has data, ready to send data to other client.
                {
                    struct epoll_event event;
                    event.events = EPOLLOUT | EPOLLET | EPOLLERR | EPOLLHUP;
                    change_event(epoll_fd, &event, &client_array_buffer);
                    send_message(client_fd, listen_fd, client_data_buffer, max_user_number);
                }
            }
            else if (events[i].events & EPOLLOUT)
            {
                int client_fd = events[i].data.fd;
                if (client_data_buffer[client_fd].write_buffer_pt == NULL)
                    continue;
                else
                {
                    int flag;
                    send(client_fd, client_data_buffer[client_fd].write_buffer_pt, sizeof(client_data_buffer[client_fd].write_buffer_pt), 0);
                    client_data_buffer[client_fd].write_buffer_pt = NULL;
                    struct epoll_event event;
                    event.data.fd = client_fd;
                    event.events = EPOLLIN | EPOLLERR | EPOLLET | EPOLLHUP;
                    flag = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event);
                    if (flag == -1)
                    {
                        printf("Epoll_ctl error, file descriptor: %d.\n", i);
                        printf("Error information: %s.\n", strerror(errno));
                    }
                }

                /* code */
            }
            else if (events[i].events & EPOLLHUP)
            {
                int client_fd = events[i].data.fd;
                int temp_fd;
                temp_fd = client_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                del_client(client_fd, client_data_buffer, &client_array_buffer);
                max_user_number--;
                /* code */
            }
            else
            {
                continue;
            }

            /* code */
        }

        /* code */
    }
    free(client_data_buffer);
    close(listen_fd);
    return 0;
}