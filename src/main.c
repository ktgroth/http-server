
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "include/server.h"


int main(int argc, char *argv[])
{
    server_t *server = init_server("127.0.0.1", 3000);
    if (!server)
    {
        fprintf(stderr, "Failed to initialize server");
        return 1;
    }

    printf(
        "Server Online\n"
        "Awaiting Requests ...\n"
    );

    pthread_t listener;
    if (pthread_create(&listener, NULL, client_listener, &server))
    {
        perror("Failed to create \"Listener\" thread.");
        return 1;
    }

    pthread_t joiner;
    if (pthread_create(&joiner, NULL, client_joiner, NULL))
    {
        perror("Failed to create\"Joiner\" thread.");
        return 1;
    }

    if (pthread_join(listener, NULL))
    {
        perror("Failed to join \"Listener\" thread.");
        return 1;
    }

    if (pthread_join(joiner, NULL))
    {
        perror("Failed to join \"Joiner\" thread.");
        return 1;
    }

    return 0;
}

