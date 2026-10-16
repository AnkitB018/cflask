#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

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


//adding a requests queue
#define QUEUE_SIZE 10
int queue[QUEUE_SIZE];
int front = 0, rear = 0, count =0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

void enqueue(int client_fd){
    pthread_mutex_lock(&mutex);

    while(count == QUEUE_SIZE){
        pthread_cond_wait(&not_full, &mutex);
    }

    queue[rear] = client_fd;
    rear = (rear+1) % QUEUE_SIZE;
    count++;
    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
}

int dequeue(void){
    pthread_mutex_lock(&mutex);

    while(count ==0){
        pthread_cond_wait(&not_empty, &mutex);
    }

    int client_fd = queue[front];
    front = (front +1) % QUEUE_SIZE;
    count--;

    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);

    return client_fd;
}

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

#define ROUTE_COUNT (sizeof(routes) / sizeof(routes[0]))


static int find_route(const char* url){
    for(int i=0 ; i<URL_COUNT ; i++){
        if(strcmp(url, routes[i].url) == 0){
            return routes[i].id;
        }
    }

    return -1;
}

int get_arguement(const char *query, const char *name, char *value, size_t value_size){
    const char* p =query;
    size_t name_len = strlen(name);

    while(*p){
        const char *equal = strchr(p, "=");
        if(equal == NULL){
            break;
        }
        const char *amp = strchr(p, "&");
        if(amp == NULL){
            amp = p + strlen(p);
        }

        if(equal < amp && (size_t)(equal-p) == name_len && strncmp(p, name, name_len) == 0){
            size_t len = (size_t)(amp-equal-1);

            if(len > value_size){
                len = value_size-1;
            }

            memcpy(value, equal+1, len);
            value[len] = '\0';
            return 1;
        }
        if(*amp == '\0'){
            break;
        }
        p = amp+1;
    }

    value[0] = '\0';
    return 0;
}


static void handle_client(int client_fd){
    char request[8192];
        ssize_t n = recv(client_fd, request, sizeof(request)-1, 0);

        if(n<=0){
            close(client_fd);
            return;
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

        printf("[thread %lu] %s %s -> %s\n", (unsigned long)pthread_self(), req.request_line.method, req.request_line.uri, status);
        fflush(stdout);

        //close client socket fd
        close(client_fd);
}


static void *pool_worker(void *args){
    (void)args;

    while(1){
        int client_fd = dequeue();
        handle_client(client_fd);
    }

    return NULL;
}

static void *worker(void *args){
    int client_fd = *(int *)args;
    free(args);

    handle_client(client_fd);

    return NULL;
}

int main(int argc, char *argv[]){
    int fail = 0;

    if (argc == 3) {
        if (strcmp(argv[1], "s") != 0 && strcmp(argv[1], "m") != 0) {
            fail = 1;
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "t") != 0) {
            fail = 1;
        }
    } else {
        fail = 1;
    }

    if (fail) {
        fprintf(stderr, "Usage: %s s <port> | %s m <port> | %s t <port> <num_threads>\n", argv[0], argv[0], argv[0]);
        return 1;
    }

    int num_threads = 0;
    if(strcmp(argv[1], "t") == 0){
        num_threads = atoi(argv[3]);
        if(num_threads > 256 || num_threads <1){
            fprintf(stderr, "Number of threads must be between 1 to 256");
            return 1;
        }
    }
    
    char *end;
    long port = strtol(argv[2], &end, 10);

    if (*argv[2] == '\0' || *end != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "Usage: %s s <port>\n", argv[0]);
        return 1;
    }


    // ----------------------------- START of base creation (socket, bind, create) -----------------------------

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

    // ----------------------------- END of base creation (socket, bind, create) -----------------------------



    if(strcmp(argv[1], "s") == 0){
        printf("cflask [single-threaded] on http://0.0.0.0:%ld\n", port);
    }else if(strcmp(argv[1], "m") ==0){
        printf("cflask [multi-threaded] on http://0.0.0.0:%ld\n", port);
    }else{
        for(int i=0 ; i<num_threads; i++){
            pthread_t thread;
            if(pthread_create(&thread, NULL, pool_worker, NULL) != 0){
                perror("pthread create");
                close(server_fd);
                return 1;
            }
        }
        printf("cflask [thread-pool, %d threads] on http://0.0.0.0:%ld\n", num_threads, port);
    }
    
    fflush(stdout);

    while(1){
        int client_fd = accept(server_fd,  NULL, NULL);
        if(client_fd < 0){
            continue;
        }

        if(strcmp(argv[1], "s") == 0){
            handle_client(client_fd);
        }else if(strcmp(argv[1], "m") == 0){
            //multi-threading, give this connection to a new thread
            int *arg = malloc(sizeof(int));
            if(arg == NULL){
                close(client_fd);
                continue;
            }
            *arg = client_fd;

            pthread_t thread;
            if(pthread_create(&thread, NULL, worker, arg) != 0){
                close(client_fd);
                free(arg);
                continue;
            }

            pthread_detach(thread);
        }else{
            enqueue(client_fd);
        }
        
    }

    close(server_fd);
    return 0;
}

