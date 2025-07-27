
#ifndef SERVER_H_
#define SERVER_H_

#include <stdint.h>

typedef struct
{
    char *addr;
    uint16_t port;
} server_t;

typedef struct 
{
    pthread_t thread;
    int is_done;
} client_thread_t;

typedef struct
{
    int client_fd;
    client_thread_t *cthread;
} client_args_t;


uint32_t parse_address(char *address);
server_t *init_server(char *address, uint16_t port);

void *client_listener(void *server);
void *client_joiner(void *);

#endif
