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

//#define buffer_size 1024

void cleanArray(char the_array[]){
    for(int i = 0; i < 1024; ++i){
        the_array[i] = '\0';
    }
}

off_t findContent(int fd, char cli_request[], char resource_name[], char content_count_offset[]){
    char buffer[1024], buffer2[31], check3[1024];
    off_t offset = 0;
    if( !(read(fd, &buffer2, 32) > 0) ){
        return -1;
    }
    //write(1, &buffer2, strlen(buffer2));
    sscanf(buffer2, "%s %s", cli_request, resource_name);
    if(resource_name[0] == '\''){
        memmove(resource_name, resource_name+1, strlen(resource_name));
    }

    while(read(fd, &buffer, 1024) > 0){
        for(size_t j = 14; j < 1024; ++j){
            if(buffer[j-14] == 'C' and buffer[j-13] == 'o' and buffer[j-12] == 'n' and buffer[j-11] == 't' and buffer[j-10] == 'e' and buffer[j-9] == 'n' and buffer[j-8] == 't' 
            and buffer[j-7] == '-' and buffer[j-6] == 'L' and buffer[j-5] == 'e' and buffer[j-4] == 'n' and buffer[j-3] == 'g' and buffer[j-2] == 't' and buffer[j-1] == 'h' and buffer[j] == ':'){
                int k = j+2;
                int start = 0;
                cleanArray(content_count_offset);
                while(isdigit(buffer[k]) > 0){
                   content_count_offset[start] = buffer[k];
                   k++;
                   start++;
                }
                //write(1, check3, strlen(check3));
                //*content_count_offset = (off_t)j + offset;
                //*content_count_offset = check3;
            }
        }
        for(size_t i = 3; i < 1024; ++i){
            if(buffer[i-3] == '\r' and buffer[i-2] == '\n' and buffer[i-1] == '\r' and buffer[i] == '\n'){
               return (off_t)i + offset;
            }
        }
        offset += 1024;
    }
    return -1;
}

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

void performRequest(int newsockfd){

    while(1){
        int read_num_bytes, file_des, buffer_size = 1024;
        char buffer[1024], cli_request[1024], resource_name[1024], second_buffer[1024], response[1024], check[1024], check2[12];//
        int content_length;
        char newline[]  = "\n";
    
        struct stat fd_stats = {};

        off_t offset_for_content;
        char offset[32];
        cleanArray(offset);

        if( (offset_for_content = findContent(newsockfd, cli_request, resource_name, offset)) == -1){
            //sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"); 
            //write(newsockfd, response, strlen(response));  
            break;
        }

        write(1, newline, strlen(newline));
        write(1, cli_request, strlen(cli_request));
        write(1, newline, strlen(newline));
        write(1, resource_name, strlen(resource_name));
        write(1, newline, strlen(newline));
        //write(1, offset, strlen(offset));
        //write(1, newline, strlen(newline));
        //write(1, newline, strlen(newline));

        if(!isValidAddress(resource_name)){
            sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"); 
            write(newsockfd, response, strlen(response));  
            continue; 
        }

        if(strcmp(cli_request, "GET") == 0){ //SEND BACK HEADER, THEN SEND BACK CONTENT BECAUSE CONTENT LENGTH > 0 assuming no err

            if((file_des = open(resource_name, O_RDONLY)) == -1){ //error
                sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n"); 
                write(newsockfd, response, strlen(response));  

                read_num_bytes = 0;
                cleanArray(buffer);
                cleanArray(offset);
                cleanArray(second_buffer);
                cleanArray(response);

                close(newsockfd);
                continue; 
            }
            else{ //We can open the file
                fstat(file_des, &fd_stats);
                off_t file_size = fd_stats.st_size;
                sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n", ((int)file_size)); 
                write(newsockfd, response, strlen(response));  

                while((read_num_bytes = read(file_des, &second_buffer, file_size)) > 0) {
                    if(read_num_bytes == -1){
                        sprintf(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n"); 
                        write(newsockfd, response, strlen(response));  
                        continue; 
                    }
                    write(newsockfd, &second_buffer, read_num_bytes);
                }
                close(file_des);
                read_num_bytes = 0;
                cleanArray(buffer);
                cleanArray(offset);
                cleanArray(second_buffer);
                cleanArray(response); 
                close(newsockfd);   
                continue; 
            }
        }
        else if(strcmp(cli_request, "PUT") == 0){ //PUT DATA IN FILE, SEND BACK HEADER
            bool primero = true;
            content_length = std::stoi(offset, nullptr);
            sprintf(check2, "%d", content_length);
            write(1, check2, strlen(check2));
            write(1, newline, strlen(newline));

            if( (file_des = open(resource_name, O_WRONLY | O_CREAT | O_TRUNC, 00400 | 00200)) < 0){ //file opeing already in use
                sprintf(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n"); 
                write(newsockfd, response, strlen(response));

                read_num_bytes = 0;
                cleanArray(buffer);
                cleanArray(offset);
                cleanArray(second_buffer);
                cleanArray(response); 

                close(newsockfd); 
                continue;
            }
            int count = 0;
            while( (read_num_bytes = read(newsockfd, &second_buffer, 1)) > 0){ 
                if(primero){
                    sprintf(response, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
                    write(newsockfd, response, strlen(response)); 
                    primero = false;
                }
                write(file_des, &second_buffer, read_num_bytes);
                count++;
                if(count == content_length){
                    break;
                }
                //write(1, &second_buffer, read_num_bytes);
                    //sprintf(check, "%d", read_num_bytes);
                    //write(1, &check, strlen(check));
                    //write(1, newline, strlen(newline));
            }
            close(file_des);

            read_num_bytes = 0;
            cleanArray(buffer);
            cleanArray(offset);
            cleanArray(second_buffer);
            cleanArray(response);

            close(newsockfd);
            continue; 
        } 
        else{
            sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n"); 
            write(newsockfd, response, strlen(response));  

            read_num_bytes = 0;
            cleanArray(buffer);
            cleanArray(offset);
            cleanArray(second_buffer);
            cleanArray(response);

            close(newsockfd);
            continue; 
        }
    }
    //close(newsockfd);
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
    /*int read_num_bytes, newsockfd, file_des, buffer_size = 1024;
    char buffer[1024], cli_request[1024], resource_name[1024], second_buffer[1024], response[1024], check[1024];//
    */

    //struct stat fd_stats = {};

    char newline[]  = "\n";

    //off_t offset_for_content;
    //off_t offset;
    struct sockaddr client_addr;
    socklen_t client_len;

    while(1){
        
        int acceptfd = accept(main_socket, &client_addr, &client_len);
        if(acceptfd < 0){
            warn("accept %s", argv[1]);
            return EXIT_FAILURE;
        }
        //write(1, newline, strlen(newline));
        performRequest(acceptfd); 
        //write(1, newline, strlen(newline));
    }
    
    return EXIT_SUCCESS;

}
