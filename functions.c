#include "functions.h"
#include <stdlib.h>
#include <stdio.h>
#include "functionslist.h"

void root_handler(const char *arg, char *result, size_t result_size){
    snprintf(result, result_size, "Root");
}

void square_handler(const char *arg, char *result, size_t result_size){
    char *end;
    long num = strtol(arg, &end, 10);
    if(arg[0] == '\0' || *end != '\0'){
        snprintf(result, result_size, "Invalid number!");
        return;
    }

    snprintf(result, result_size, "%ld", num*num);
}

void cube_handler(const char *arg, char *result, size_t result_size){
    char *end;
    long num = strtol(arg, &end, 10);
    if(arg[0] == '\0' || *end != '\0'){
        snprintf(result, result_size, "Invalid number!");
        return;
    }

    snprintf(result, result_size, "%ld", num*num*num);
}

void helloworld_handler(const char *arg, char *result, size_t result_size){
    snprintf(result, result_size, "Hello %s", arg);
}

void pingpong_handler(const char *arg, char *result, size_t result_size){
    snprintf(result, result_size, "Ping Pong");
}

void prime(const char* arg, char* result, size_t result_size){
    char *end;
    long num = strtol(arg, &end, 10);

    if(arg[0] == '\0' || *end != '\0' || num <2){
        snprintf(result, result_size, "False");
        return;
    }

    for(long i=2 ; i<=num/i ; i++){
        if(num%i == 0){
            snprintf(result, result_size, "False");
            return;
        }
    }

    snprintf(result, result_size, "True");
}

void fibonacci(const char* arg, char* result, size_t result_size){
    char *end;
    long num = strtol(arg, &end, 10);

    if (arg[0] == '\0' || *end != '\0' || num < 0 || num > 90) {
        snprintf(result, result_size, "Invalid number");
        return;
    }

    long a = 0, b = 1;

    for (long i = 0; i < num; i++) {
        long temp = a + b;
        a = b;
        b = temp;
    }

    snprintf(result, result_size, "%ld", a);
}

void (*functions[URL_COUNT])(const char *arg, char *result, size_t result_size) = {
    root_handler,
    square_handler,
    cube_handler,
    helloworld_handler,
    pingpong_handler,
    prime,
    fibonacci
};