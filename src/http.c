
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/http.h"

http_response *parse_request(char *input)
{
    http_request request;

    char *method = strtok(input, " ");
    char *path = strtok(NULL, " ");
    char *version = strtok(NULL, "\r\n");

    size_t size = strlen(path) + 2;
    char *full_path = (char *)calloc(size, sizeof(char));
    snprintf(full_path, size, ".%s", path);

    request.method = method;
    request.path = full_path;
    request.version = version ? version : "HTTP/0.9";

    if (!request.method || !request.path)
    {
        http_response *response = (http_response *)calloc(1, sizeof(http_response));
        response->version = "HTTP/0.9";
        response->status = 404;
        response->reason = "Malformed request\r\n";
    }

    if (!strcmp(request.method, "GET"))
        return get(&request);
    if (!strcmp(request.method, "POST"))
        return post(&request);

    http_response *response = (http_response *)calloc(1, sizeof(http_response));
    response->version = "HTTP/0.9";
    response->status = 404;
    response->reason = "Unsupported method\r\n";

    return response;
}

http_response *get(http_request *request)
{
    http_response *response = (http_response *)calloc(1, sizeof(http_response));
    if (!response)
    {
        perror("Could not allocate response");
        return NULL;
    }

    FILE *fp = fopen(request->path, "r");
    if (!fp)
    {
        perror("File doesn't exist");
        response->status = 404;
        response->reason = "FIle doesn't exist";

        return response;
    }


    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    char *body = (char *)calloc(size + 1, sizeof(char));
    if (!body)
    {
        perror("Failed to allocate memory for file content");
        fclose(fp);
        free(response);
        return NULL;
    }

    fread(body, 1, size, fp);
    body[size] = '\0';
    fclose(fp);

    response->version = "HTTP/1.0";
    response->status = 200;
    response->reason = "OK";
    response->body = body;
    response->content_length = size;

    return response;
}

http_response *post(http_request *request)
{
    return NULL;
}

