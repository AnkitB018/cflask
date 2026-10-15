#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "http-parser.h"
#include "functions.h"
#include "functionslist.h"
/* 
* Implement all the cflask logic here.
* 1. web server initialization 
* 2. accept http requests
* 3. parse http requests
* 4. dispatch http requests
* 5. process, generate and send http responses
*/

typedef struct {
    const char* url;
    int id;
} Route;

static const Route routes[] = {
    {"/", URL_ROOT},
    {"/square", URL_SQUARE},
    {"/cube", URL_CUBE},
    {"/helloworld", URL_HELLOWORLD},
    {"/pingpong", URL_PINGPONG}
};

static int find_route(const char* url){
    for(int i=0 ; i<URL_COUNT ; i++){
        if(strcmp(url, routes[i].url) == 0){
            return routes[i].id;
        }
    }

    return -1;
}

int main(int argc, char *argv[]){
    if(argc != 3 || strcmp(argv[1], "s") != 0){
        fprintf(stderr, "Usage: %s s <port>\n", argv[0]);
        return 1;
    }

    char *end;
    long port = strtol(argv[2], &end, 10);

    if (*argv[2] == '\0' || *end != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "Usage: %s s <port>\n", argv[0]);
        return 1;
    }

    //create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("socket");
        return 1;
    }

    //bind socket to port
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    //Listen for connections
    if(listen(server_fd, 10) < 0){
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("cflask [single-threaded] on [http://0.0.0.0:%ld]\n", port);
    fflush(stdout);

    while(1){
        int client_fd = accept(server_fd,  NULL, NULL);
        if(client_fd < 0){
            continue;
        }

        //receive request
        char request[8192];
        ssize_t n = recv(client_fd, request, sizeof(request)-1, 0);

        if(n<=0){
            close(client_fd);
            continue;
        }

        request[n] = '\0';

        //parse the request
        ParsedRequest req = {0};
        parse_http_request(request, &req);

        const char* status = "200 OK";
        const char* body;

        //Find and call handler
        if(strcmp(req.request_line.method, "GET") != 0){
            status = "404 Not Found";
            body = "Not Found";
        }else{
            int id = find_route(req.request_line.uri);
            if(id == -1){
                status = "404 Not Found";
                body = "Not Found";
            }else{
                body = functions[id]();
            }
        }

        //create http response
        size_t response_size;
        char *response = create_http_response(status, "text/plain", body, strlen(body), &response_size);

        //send response
        if(response != NULL){
            send(client_fd, response, response_size, 0);
            free(response);
        }

        printf("[thread %d] %s %s -> %s\n", (int)getpid(), req.request_line.method, req.request_line.uri, status);
        fflush(stdout);

        //close client socket fd
        close(client_fd);


    }

    close(server_fd);
    return 0;
}

