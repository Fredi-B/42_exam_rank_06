#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>


typedef struct client {
    int id;
    char msg_buf[4096 * 42];
} t_client;

t_client    clients[1024];

int         max_fd = 0, next_id = 0;
fd_set      fds, read_fds, write_fds;
char        buf_write[4096 * 42], buf_read[4096 * 42];

void fatal_error(int i)
{
    printf("Fatal error: %d\n", i);
    exit(1);
}

void send_all(int client_fd)
{
    for (int i = 0; i <= max_fd; i++) {
        if (FD_ISSET(i, &read_fds) && i != client_fd)
        {
            send(i, buf_write, strlen(buf_write), 0);
        }
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
    if (server_fd < 0)
        fatal_error(0);
    max_fd = server_fd;
    FD_SET(server_fd, &fds);

    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(2130706433);
    addr.sin_port = htons(atoi(argv[1]));
    if (bind(server_fd, (const struct sockaddr*)&addr, sizeof(addr)) < 0)
        fatal_error(1);
    if (listen(server_fd, 128) < 0)
        fatal_error(2);

    while (1)
    {
        read_fds = write_fds = fds;
        if (select(max_fd + 1, &write_fds, &read_fds, NULL, NULL) < 0)
            continue;
        for (int i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &write_fds) && i == server_fd)
            {
                int client_fd = accept(server_fd, (struct sockaddr*)&addr, &addr_len);
                if (client_fd < 0)
                    continue;
                FD_SET(client_fd, &fds);
                if (client_fd > max_fd)
                    max_fd = client_fd;
                clients[client_fd].id = next_id++;
                sprintf(buf_write, "server: client %d just arrived\n", clients[client_fd].id);
                send_all(client_fd);
            }
            if (FD_ISSET(i, &write_fds) && i != server_fd)
            {
                int msg_len = recv(i, buf_read, 11000, 0);
                if (msg_len <= 0)
                {
                    sprintf(buf_write, "server: client %d just left\n", clients[i].id);
                    send_all(i);
                    FD_CLR(i, &fds);
                    close(i);
                    break;
                }
                else
                {
                    for (int j = 0, k = strlen(clients[i].msg_buf); j < strlen(buf_read); j++, k++)
                    {
                        clients[i].msg_buf[k] = buf_read[j];
                        if (buf_read[j] == '\n')
                        {
                            clients[i].msg_buf[k] = '\0';
                            sprintf(buf_write, "client %d: %s\n", clients[i].id, clients[i].msg_buf);
                            send_all(i);
                            bzero(clients[i].msg_buf, strlen(clients[i].msg_buf));
                            k = -1;
                        }
                    }
                break;
                }
            }
        }
    }
    return (0);
}