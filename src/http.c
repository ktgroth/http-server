
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "include/http.h"

#define BUFFER_SIZE 0x1000


char *get_file_extension(char *file_name)
{
    char *dot = strrchr(file_name, '.');
    if (!dot || dot == file_name)
        return "";

    return dot + 1;
}

char *get_mime_type(char *file_ext)
{
    if (!strcasecmp(file_ext, "html") || !strcasecmp(file_ext, "htm"))
        return "text/html";
    if (!strcasecmp(file_ext, "txt"))
        return "text/plain";
    if (!strcasecmp(file_ext, "jpg"))
        return "image/jpeg";
    if (!strcasecmp(file_ext, "png"))
        return "image/png";

    return "application/octet-stream";
}

char *url_decode(char *src)
{
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);
    size_t decoded_len = 0;

    for (size_t i = 0; i < src_len; ++i)
    {
        if (src[i] == '%' && i + 2 < src_len)
        {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        } else
            decoded[decoded_len++] = src[i];
    }

    decoded[decoded_len] = '\0';
    return decoded;
}

void parse_request(int client_fd, char *request)
{
    regex_t rx;
    regcomp(&rx, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
    regmatch_t matches[2];

    if (!regexec(&rx, request, 2, matches, 0))
    {
        request[matches[1].rm_eo] = '\0';
        char *url_encoded_file_name = request + matches[1].rm_so;
        char *file_name = url_decode(url_encoded_file_name);

        char file_ext[32];
        strcpy(file_ext, get_file_extension(file_name));

        char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));
        size_t response_len;
        build_http_response(file_name, file_ext, response, &response_len);

        send(client_fd, response, response_len, 0);

        free(response);
        free(file_name);
    }

    regfree(&rx);
}

void build_http_response(char *file_name, char *file_ext, char *response, size_t *response_len)
{
    char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "\r\n",
             mime_type);

    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        snprintf(response, BUFFER_SIZE,
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "404 Not Found");
        *response_len = strlen(response);

        return;
    }

    struct stat file_stat;
    fstat(file_fd, &file_stat);
    off_t file_size = file_stat.st_size;

    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd,
                              response + *response_len,
                              BUFFER_SIZE - *response_len)) > 0)
        *response_len += bytes_read;

    free(header);
    close(file_fd);
}

