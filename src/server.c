
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "include/server.h"
#include "include/http.h"

uint32_t parse_address(char *address)
{
    uint8_t ap[4];
    if (sscanf(address, "%hhd.%hhd.%hhd.%hhd", &ap[0], &ap[1], &ap[2], &ap[3]) == EOF)
    {
        perror("sscanf(address, \"%c.%c.%c.%c\", ap)");
        return 0;
    }

    uint32_t addr =
        ((uint32_t)ap[0] << 24) + 
        ((uint32_t)ap[1] << 16) + 
        ((uint32_t)ap[2] << 8) + 
        ap[3];
    return addr;
}

server_t *init_server(char *address, uint16_t port)
{
    uint32_t addr = parse_address(address);
    if (!addr)
        return NULL;

    server_t *server = (server_t *)calloc(1, sizeof(server));
    if (!server)
    {
        perror("calloc(1, sizeof(server))");
        return NULL;
    }

    server->addr = addr;
    server->port = port;

    return server;
}


volatile size_t clients = 0;
volatile client_thread_t *open_clients = NULL;
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;


void start_server(int *server_fd)
{
    int opt = 1;

    if ((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket(AF_INET, SOCK_STREAM, 0)");
        exit(-1);
    }

    if (setsockopt(*server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt)))
    {
        perror("setsockopt");
        exit(-1);
    }
}

void bind_server(int *server_fd, struct sockaddr_in *address)
{
    if (bind(*server_fd, (struct sockaddr *)address, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind failed");
        exit(-1);
    }
}

void listen_for_client(int *server_fd)
{
    if (listen(*server_fd, 3) < 0)
    {
        perror("listen");
        exit(-1);
    }
}

#define CHUNK_SIZE 1024
char *read_from_client(int client_fd)
{
    char *buffer = NULL;
    size_t total_size = 0;
    size_t capacity = 0;
    char temp[CHUNK_SIZE];
    ssize_t bytes_read;

    while ((bytes_read = recv(client_fd, temp, CHUNK_SIZE, 0)) > 0)
    {
        if (total_size + bytes_read >= capacity)
        {
            capacity = (capacity == 0) ? bytes_read * 2 : capacity * 2;
            buffer = realloc(buffer, capacity + 1);
            if (!buffer)
            {
                perror("Failed to realloc buffer");
                free(buffer);
                return NULL;
            }
        }

        memcpy(buffer + total_size, temp, bytes_read);
        total_size += bytes_read;

        if (strstr(buffer, "\r\n\r\n") || strstr(buffer, "\n\n"))
            break;
    }

    if (bytes_read < 0)
    {
        perror("revc failed");
        free(buffer);
        return NULL;
    }

    if (buffer)
        buffer[total_size] = '\0';

    return buffer;
}

void *handle_client(void *arg)
{
    client_args_t *args = (client_args_t *)arg;
    int client_fd = args->client_fd;
    client_thread_t *cthread = args->cthread;

    char *buffer = read_from_client(client_fd);
    printf("%s\n", buffer);

    http_response *response = parse_request(buffer);
    dprintf(client_fd,
            "%s %ld %s\r\n"
            "Content-Length: %zu\r\n"
            "Content-Type: text/html\r\n"
            "\r\n"
            "%s",
            response->version,
            response->status,
            response->reason,
            response->content_length,
            response->body);

    close(client_fd);
    cthread->is_done = 1;
    printf("{ %d } Client response sent", client_fd);

    free(args);
    return NULL;
}

void sanatize_open_clients()
{
    pthread_mutex_lock(&client_lock);

    size_t count = 0;
    for (size_t i = 0; i < clients; ++i)
    {
        if (open_clients[i].is_done != -1)
            open_clients[count++] = open_clients[i];
    }

    if (count < clients)
    {
        open_clients = realloc((void *)open_clients, count * sizeof(client_thread_t));
        clients = count;
    }

    pthread_mutex_unlock(&client_lock);
}

void *client_joiner(void *)
{
    while (1)
    {
        pthread_mutex_lock(&client_lock);

        for (size_t i = 0; i < clients; ++i)
        {
            if (open_clients[i].is_done)
            {
                pthread_join(open_clients[i].thread, NULL);
                open_clients[i].is_done = -1;
            }
        }

        pthread_mutex_unlock(&client_lock);
        sanatize_open_clients();
        usleep(100000);
    }

    return NULL;
}

void *client_listener(void *server)
{
    int server_fd;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    start_server(&server_fd);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(3000);
    bind_server(&server_fd, &address);

    int running = 1;
    while (running)
    {
        listen_for_client(&server_fd);

        int new_client;
        if ((new_client = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("accept");
            exit(-1);
        }

        client_thread_t new_cthread = { 0 };
        client_args_t *args = (client_args_t *)malloc(sizeof(client_args_t));
        args->client_fd = new_client;
        args->cthread = &new_cthread;

        pthread_mutex_lock(&client_lock);
        open_clients = (client_thread_t *)realloc((void *)open_clients, (clients + 1) * sizeof(client_thread_t));
        open_clients[clients] = new_cthread;
        pthread_mutex_unlock(&client_lock);

        if (pthread_create((pthread_t *)&open_clients[clients].thread, NULL, handle_client, args))
        {
            perror("Failed to create \"Client Handler\" thread.");
            exit(1);
        }

        ++clients;
    }

    return NULL;
}

