#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

typedef struct client {
    char    msg[110000];
    int     id;
} t_client;

t_client    clients[1024];
fd_set      fds, w_fds, r_fds;
int         max_fd = 0, next_id = 0;
char        w_buf[42 * 4096], r_buf[42 * 4096];

void    fatal_error()
{
    write(2,"Fatal error\n", 12);
    exit (1);
}

void    send_all(int sender_fd)
{
    for (int fd = 0; fd <= max_fd; fd++)
    {
        if (FD_ISSET(fd, &r_fds) && fd != sender_fd)
            send(fd, w_buf, strlen(w_buf), 0);
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        exit (1);
    }
    bzero(&clients, sizeof(clients));
    FD_ZERO(&fds);
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == -1)
        fatal_error();
    max_fd = socket_fd;
    FD_SET(socket_fd, &fds);
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1]));
    int addr_len = sizeof(servaddr);
    if (bind(socket_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
        fatal_error();
    if (listen(socket_fd, 128) == -1)
        fatal_error();
    while(1)
    {
        r_fds = w_fds = fds;
        if (select(max_fd + 1, &w_fds, &r_fds, NULL, NULL) == -1)
            continue ;
        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &w_fds) && fd == socket_fd)
            {
                int client_fd = accept(socket_fd, (struct sockaddr *)&servaddr, (socklen_t *)&addr_len);
                if (client_fd == -1)
                    continue ;
                clients[client_fd].id = next_id++;
                if (client_fd > max_fd)
                    max_fd = client_fd;
                FD_SET(client_fd, &fds);
                sprintf(w_buf, "server: client %d just arrived\n", clients[client_fd].id);
                send_all(client_fd);
                break;
            }
            if (FD_ISSET(fd, &w_fds) && fd != socket_fd)
            {
                int len_msg = recv(fd, r_buf, 42 * 4096, 0);
                if (len_msg <= 0)
                {
                    sprintf(w_buf, "server: client %d just left\n", clients[fd].id);
                    send_all(fd);
                    FD_CLR(fd, &fds);
                    close(fd);
                    break;
                }
                else
                {
                    for (int i = strlen(clients[fd].msg), j = 0; j < len_msg; i++, j++)
                    {
                        clients[fd].msg[i] = r_buf[j];
                        if (clients[fd].msg[i] == '\n')
                        {
                            clients[fd].msg[i] = '\0';
                            sprintf(w_buf, "client %d: %s\n", clients[fd].id, clients[fd].msg);
                            send_all(fd);
                            bzero(clients[fd].msg, sizeof(clients[fd].msg));
                            i = -1;
                        }
                    }
                    break;
                }
            }
        }
    }
    return (0);
}
