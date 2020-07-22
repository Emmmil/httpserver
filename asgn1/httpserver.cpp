//Emil Ingemansson (eingeman) htmlserver.cpp
//CSE 130 Assignment 1 HTTP SERVER


#include <sys/sendfile.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <err.h>
#include <unistd.h>
#include <string>

#define SMALL_BUFF_SIZE 256
#define BUFF_SIZE 1024
#define FIRST 32


bool isValidAddress(char address[]){
    int i = 0;

    while(isalnum(address[i]) or address[i] == '-' or address[i] == '_'){
        i += 1; 
    }

    if(address[27] != '\0' or i != 27){
        return false;
    }
    return true;
}

bool parseHeader(int file_descriptor, char cli_request[], char resource_name[], char content_length[]){
    char requestAndAddress[SMALL_BUFF_SIZE], buffer[1];

    //char newline[] = "\n";

    read(file_descriptor, requestAndAddress, FIRST);
    sscanf(requestAndAddress, "%s %s", cli_request, resource_name);

    int end_count = 0; 
    int content_count = 0;
    int start = 0;

    while(read(file_descriptor, &buffer, 1) > 0){ //Read from the file byte by byte
        //write(1, buffer, 1);

        //At beginning so we read the next byte once we know we reached the "number" for content length
        if(content_count == 16){ //Make it all the way through IF statements for content-length
            if(isdigit(buffer[0])){ //For each consecative number we read, add to content-length string
                content_length[start] = buffer[0];
                start++;
            }
            else{
                content_count++;
            }

        }        

        if(buffer[0] == '\r'){ 
            if(end_count == 0 or end_count == 2){
                end_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        } 
        else if((buffer[0]) == '\n'){
            if(end_count == 1 or end_count == 3){
                end_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == 'C'){ 
            if(content_count == 0){
                //write(1, newline, 1);
                ++content_count;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        } 
        else if(buffer[0] == 'o'){ 
            if(content_count == 1){
                //write(1, newline, 1);
                ++content_count;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }         
        else if(buffer[0] == 'n'){ 
            if(content_count == 2 or content_count == 5 or content_count == 10){
                //write(1, newline, 1);
                ++content_count;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == 't'){ 
            if(content_count == 3 or content_count == 6 or content_count == 12){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == 'e'){ 
            if(content_count == 4 or content_count == 9){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == '-'){ 
            if(content_count == 7){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }        
        else if(buffer[0] == 'L'){ 
            if(content_count == 8){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == 'g'){ 
            if(content_count == 11){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == 'h'){ 
            if(content_count == 13){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == ':'){ 
            if(content_count == 14){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else if(buffer[0] == ' '){ 
            if(content_count == 15){
                //write(1, newline, 1);
                content_count++;
            }
            else{
                content_count = 0;
                end_count = 0;
            }
        }
        else{
            if(content_count != 16){
                content_count = 0;
            }
            end_count = 0; 
        }        

        if(end_count == 4){  //Break out before we read next byte
            break;
        }

    }

    if(cli_request[0] != '\0' and resource_name[0] != '\0' and content_length[0] != '\0'){
        return true;
    }
    else{
        return false;
    }

}

void performRequest(int file_descriptor, char cli_request[], char resource_name[], char content_length[]){
    //Firstly, check if the address is valid. If not then write back saying 
    char response[SMALL_BUFF_SIZE], buffer[BUFF_SIZE]; //check[SMALL_BUFF_SIZE];
    int opened_file_des, read_num_bytes, length_as_int;
    struct stat fd_stats = {};

    //char newline[] = "\n";

    if(!isValidAddress(resource_name)){ 
        sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"); 
        write(file_descriptor, response, strlen(response));  
        return; 
    }

    if(strcmp(cli_request, "GET") == 0){
        if((opened_file_des = open(resource_name, O_RDONLY)) == -1){ //Can't open
            sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"); 
            write(file_descriptor, response, strlen(response));  
            return;
        }
        else{ //Can open
            fstat(opened_file_des, &fd_stats);
            off_t file_size = fd_stats.st_size;
            sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", ((int)file_size)); 
            write(file_descriptor, response, strlen(response));

            while((read_num_bytes = read(opened_file_des, &buffer, file_size)) > 0) {
                if(read_num_bytes == -1){
                    sprintf(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n"); 
                    write(file_descriptor, response, strlen(response));  
                    return; 
                }
                write(file_descriptor, &buffer, read_num_bytes);
            }
            close(opened_file_des);
            return;
        }
    }
    else if(strcmp(cli_request, "PUT") == 0){
        bool primero = true;
        int counter = 0;
        length_as_int = std::stoi(content_length, nullptr);

        if((opened_file_des = open(resource_name, O_WRONLY | O_CREAT | O_TRUNC, 00400 | 00200)) < 0){
            sprintf(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n"); 
            write(file_descriptor, response, strlen(response));
            return;
        }

        while( (read_num_bytes = read(file_descriptor, &buffer, 1)) > 0){ //Read one byte at a time
            if(primero){ //First time in send back header before writing opened file
                sprintf(response, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
                write(file_descriptor, response, strlen(response)); 
                primero = false;
            }
            write(opened_file_des, &buffer, read_num_bytes);
            write(1, buffer, strlen(buffer));
            counter++;
            if(counter == length_as_int){
                break;
            }
        }
        close(opened_file_des);
        return;
    }
    else{
        sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"); 
        write(file_descriptor, response, strlen(response));
        return;
    }
}


int main(int argc, char const *argv[]) {
    
    char const * hostname = argv[1];
    char const * port;

    if(argc == 3){
        port = argv[2];
    }
    else{
        port = "80";  
    }

    struct addrinfo *addrs, hints= {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    
    if(getaddrinfo(hostname, port, &hints, &addrs) != 0){
        warn("getaddrinfo %s", argv[1]);
        return EXIT_FAILURE;
    }
    
    int main_socket = socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
    if(main_socket < 0){
        warn("socket %s", argv[1]);
        return EXIT_FAILURE;
    }
    
    int enable = 1;
    setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if(bind(main_socket, addrs->ai_addr, addrs->ai_addrlen) < 0){
        warn("bind %s", argv[1]);
        return EXIT_FAILURE;
    }
    
    // N is the maximum number of "waiting" connections on the socket.
    // We suggest something like 16.
    if(listen (main_socket, 16) < 0){
        warn("listen %s", argv[1]);
        return EXIT_FAILURE;
    }
    

    ///////////////////////////////////////////////////////////////////////
    // Your code, starting with accept(), goes here
    ///////////////////////////////////////////////////////////////////////

    //char newline[]  = "\n";

    struct sockaddr client_addr;
    socklen_t client_len;


    char cli_request[BUFF_SIZE], resource_name[BUFF_SIZE], content_length[BUFF_SIZE];

    while(1){
        
        int acceptfd = accept(main_socket, &client_addr, &client_len);
        if(acceptfd < 0){
            warn("accept %s", argv[1]);
            return EXIT_FAILURE;
        }

        while(parseHeader(acceptfd, cli_request, resource_name, content_length)){
            /*
            write(1, cli_request, strlen(cli_request));
            write(1, newline, strlen(newline));
            write(1, resource_name, strlen(resource_name));
            write(1, newline, strlen(newline));
            write(1, content_length, strlen(content_length));
            write(1, newline, strlen(newline));*/

            if(! isdigit(content_length[strlen(content_length) - 1] ) ){ //Some reason on netcat I read an extra symbol.
                content_length[strlen(content_length) - 1] = '\0';
            }
            if (resource_name[0] == '/') {
                memmove(resource_name, resource_name+1, strlen(resource_name));
            }
            performRequest(acceptfd, cli_request, resource_name, content_length);
        } 

        /* //PRINT TESTS FOR PARSE HEADER
        write(1, newline, strlen(newline));
        write(1, cli_request, strlen(cli_request));
        write(1, newline, strlen(newline));
        write(1, resource_name, strlen(resource_name));
        write(1, newline, strlen(newline));
        write(1, content_length, strlen(content_length));
        write(1, newline, strlen(newline));*/

    }
    
    return EXIT_SUCCESS;

}
