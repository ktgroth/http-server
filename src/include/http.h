
#ifndef HTTP_H
#define HTTP_H


void parse_request(int client_fd, char *request);
void build_http_response(char *file_name, char *file_ext, char *response, size_t *response_len);

#endif

