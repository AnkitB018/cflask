#include<functions.h>
#include<functionslist.h>

const char* root_handler(void){
    return "Root";
}

const char* square_handler(void){
    return "1";
}

const char* cube_handler(void){
    return "1";
}

const char* helloworld_handler(void){
    return "Hello world";
}

const char* pingpong_handler(void){
    return "ping pong";
}


const char *(*functions[URL_COUNT])(void) = {
    root_handler,
    square_handler,
    cube_handler,
    helloworld_handler,
    pingpong_handler
};