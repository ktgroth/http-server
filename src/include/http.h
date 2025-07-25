
#ifndef HTTP_H
#define HTTP_H

typedef struct
{
    char *method;
    char *path;
    char *version;
} http_request;

typedef struct
{
    char *version;
    size_t status;
    char *reason;

    char *body;
    size_t content_length;
} http_response;


http_response *parse_request(char *input);

http_response *get(http_request *request);
http_response *post(http_request *request);

#endif

