#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>

typedef struct client {
    char    msg[11000];
    int     id;
}   t_client;

t_client    clients[1024];
fd_set      fds, w_fds, r_fds;
int         max_fd = 0, next_id = 0;
char        w_buf[42 * 4096], r_buf[42 * 4096];

void    fatal_error()
{
    write(2, "Fatal error\n", 13);
    exit(1);
}

void    send_all(int sender_fd)
{
    for (int i = 0; i <= max_fd; i++)
    {
        if (FD_ISSET(i, &r_fds) && i != sender_fd)
            send(i, w_buf, sizeof(w_buf), 0);
    }
}


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", 27);
        exit(1);
    }
    bzero(&clients, sizeof(clients));
    FD_ZERO(&fds);
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
        fatal_error();
    max_fd = server_fd;
    FD_SET(server_fd, &fds);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(2130706433); // 127.0.0.1
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(server_fd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) != 0)
        fatal_error();
    if (listen(server_fd, 128))
        fatal_error();
    int len_servaddr = sizeof(servaddr);
    while (1)
    {
        w_fds = r_fds = fds;
        if (select(max_fd + 1, &w_fds, &r_fds, NULL, NULL) < 0)
            continue;
        for (int fd = 0; fd <= max_fd; fd++)
        {
            if (FD_ISSET(fd, &w_fds) && fd == server_fd)
            {
                int client_fd = accept(server_fd, (struct sockaddr *)&servaddr, (socklen_t *)&len_servaddr);
                if (client_fd == -1)
                    continue;
                if (client_fd > max_fd)
                    max_fd = client_fd;
                FD_SET(client_fd, &fds);
                clients[client_fd].id = next_id++;
                sprintf(w_buf, "server: client %d just arrived\n", clients[client_fd].id);
                send_all(client_fd);
                break;
            }
            if (FD_ISSET(fd, &w_fds) && fd != server_fd)
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
                            bzero(&clients[fd].msg, sizeof(clients[fd].msg));
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