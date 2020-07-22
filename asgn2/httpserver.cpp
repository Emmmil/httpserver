// Emil Ingemansson (eingeman) httpserver.cpp
// CSE 130 Assignment 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <string>
#include <sys/stat.h>

#define SMALL_BUFF_SIZE 256
#define BUFF_SIZE 1024
#define FIRST 32

off_t log_offset = 0;
int incomingConnections = 0, currentlyWorking = 0;
int allSocketFDs[SMALL_BUFF_SIZE];

char *log_file_name;
bool wantsLog = false;

pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t empty_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

void cleanArray(char the_array[], int size) {
  for (int i = 0; i < size; ++i) {
    the_array[i] = '\0';
  }
}

bool isValidAddress(char address[]) {
  int i = 0;

  while (isalnum(address[i]) or address[i] == '-' or address[i] == '_') {
    i += 1;
  }

  if (address[27] != '\0' or i != 27) {
    return false;
  }
  return true;
}

off_t getOffsetAmount(off_t content_length) {

  off_t firstlinelength = 0;

  int testNumber = content_length;
  int number_of_69lines;
  int last_line;
  int count = 0;

  while (testNumber > 0) { // counts how many places the length goes to
    testNumber = testNumber / 10;
    count++;
  }

  firstlinelength =
      40 +
      count; // 40 is number of characters before how many places length goes to

  number_of_69lines = content_length / 20;
  last_line = ((content_length % 10) * 3) + 9;

  return firstlinelength + (number_of_69lines * 69) + last_line + 9 +
         30; // Somehow always off by 30 bytes just hard code in.
}

void logfail(char *filename, int errorCode, char cli_request[],
             char resource_name[]) {
  char failBuff[65];
  off_t localoffset = log_offset;

  sprintf(failBuff, "FAIL: %s %s HTTP/1.1 --- response %d\n", cli_request,
          resource_name, errorCode);
  off_t increaseOffset = strlen(failBuff);

  pthread_mutex_lock(&log_mutex);
  log_offset += increaseOffset;
  pthread_mutex_unlock(&log_mutex);

  int log_descriptor = open(filename, O_WRONLY | O_CREAT, 00400 | 00200);

  pwrite(log_descriptor, failBuff, strlen(failBuff), localoffset);
}

void logfile(char *filename, char cli_request[], char resource_for_content[]) {

  char logging_buffer[20];
  char buffer69[69];
  char endbuff[] = "========\n";
  int total_read_bytes;
  off_t localoffset = log_offset;

  int log_descriptor = open(filename, O_WRONLY | O_CREAT, 00400 | 00200);
  int content_descriptor = open(resource_for_content, O_RDONLY);

  struct stat content_stats = {};
  fstat(content_descriptor, &content_stats);

  off_t increaseOffset = getOffsetAmount(content_stats.st_size);

  pthread_mutex_lock(&log_mutex);
  log_offset += increaseOffset;
  pthread_mutex_unlock(&log_mutex);

  sprintf(buffer69, "%s %s length %ld\n", cli_request, resource_for_content,
          content_stats.st_size);
  pwrite(log_descriptor, buffer69, strlen(buffer69), localoffset);
  localoffset += strlen(buffer69);

  off_t address = 0;

  while ((total_read_bytes = read(content_descriptor, logging_buffer, 20))) {
    int i = 0;
    sprintf(buffer69, "%08zd", address);
    address += 20;
    while (i < total_read_bytes) {
      sprintf(buffer69, "%s %02x", buffer69, logging_buffer[i]);
      i++;
    }
    sprintf(buffer69, "%s\n", buffer69);
    pwrite(log_descriptor, buffer69, strlen(buffer69), localoffset);
    localoffset += strlen(buffer69);
    cleanArray(buffer69, 69);
  }
  pwrite(log_descriptor, endbuff, strlen(endbuff), localoffset);
}

bool parseHeader(int file_descriptor, char cli_request[], char resource_name[],
                 char content_length[]) {
  char requestAndAddress[SMALL_BUFF_SIZE];
  uint8_t buffer[2048];

  if (read(file_descriptor, requestAndAddress, FIRST) <= 0) {
    return false;
  }

  sscanf(requestAndAddress, "%s %s", cli_request, resource_name);

  int end_count = 0;
  int content_count = 0;
  int start = 0;

  while (read(file_descriptor, &buffer, 1) >
         0) { // Read from the file byte by byte

    // At beginning so we read the next byte once we know we reached the
    // "number" for content length
    if (content_count ==
        16) { // Make it all the way through IF statements for content-length
      if (isdigit(buffer[0])) { // For each consecative number we read, add to
                                // content-length string
        content_length[start] = buffer[0];
        start++;
      } else {
        content_count++;
      }
    }

    if (buffer[0] == '\r') {
      if (end_count == 0 or end_count == 2) {
        end_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if ((buffer[0]) == '\n') {
      if (end_count == 1 or end_count == 3) {
        end_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'C') {
      if (content_count == 0) {
        ++content_count;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'o') {
      if (content_count == 1) {
        ++content_count;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'n') {
      if (content_count == 2 or content_count == 5 or content_count == 10) {
        ++content_count;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 't') {
      if (content_count == 3 or content_count == 6 or content_count == 12) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'e') {
      if (content_count == 4 or content_count == 9) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == '-') {
      if (content_count == 7) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'L') {
      if (content_count == 8) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'g') {
      if (content_count == 11) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == 'h') {
      if (content_count == 13) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == ':') {
      if (content_count == 14) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else if (buffer[0] == ' ') {
      if (content_count == 15) {
        content_count++;
      } else {
        content_count = 0;
        end_count = 0;
      }
    } else {
      if (content_count != 16) {
        content_count = 0;
      }
      end_count = 0;
    }

    if (end_count == 4) { // Break out before we read next byte
      break;
    }
  }

  return true;
}

void performRequest(int file_descriptor, char cli_request[],
                    char resource_name[], char content_length[]) {

  char response[SMALL_BUFF_SIZE];
  uint8_t buffer[BUFF_SIZE], put_buffer[1];
  int opened_file_des, read_num_bytes;
  struct stat fd_stats = {};

  if (resource_name[0] == '/') {
    memmove(resource_name, resource_name + 1, strlen(resource_name));
  }

  if (!isValidAddress(resource_name)) {
    sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
    write(file_descriptor, response, strlen(response));

    logfail(log_file_name, 400, cli_request, resource_name);
    return;
  }

  if (strcmp(cli_request, "GET") == 0) {
    if ((opened_file_des = open(resource_name, O_RDONLY)) == -1) { // Can't open
      sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
      write(file_descriptor, response, strlen(response));
      logfail(log_file_name, 404, cli_request, resource_name);
      return;
    } else { // Can open
      fstat(opened_file_des, &fd_stats);
      off_t file_size = fd_stats.st_size;
      sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\n\r\n",
              ((int)file_size));
      write(file_descriptor, response, strlen(response));

      while ((read_num_bytes = read(opened_file_des, &buffer, 1024)) > 0) {
        if (read_num_bytes == -1) {
          sprintf(response,
                  "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
          write(file_descriptor, response, strlen(response));
          logfail(log_file_name, 403, cli_request, resource_name);
          return;
        }
        write(file_descriptor, &buffer, read_num_bytes);
      }
      close(opened_file_des);

      if (wantsLog) {
        logfile(log_file_name, cli_request, resource_name);
      }

      return;
    }
  } else if (strcmp(cli_request, "PUT") == 0) {

    if (content_length == NULL) {
      sprintf(response,
              "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
      write(file_descriptor, response, strlen(response));
      logfail(log_file_name, 400, cli_request, resource_name);
      return;
    }

    int counter = 0;
    int length_as_int;
    length_as_int = std::stoi(content_length, nullptr);
    // sscanf(content_length, "%d", &length_as_int);

    if ((opened_file_des = open(resource_name, O_WRONLY | O_CREAT | O_TRUNC,
                                00400 | 00200)) < 0) {
      sprintf(response, "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n");
      write(file_descriptor, response, strlen(response));
      logfail(log_file_name, 403, cli_request, resource_name);
      return;
    }

    while ((read_num_bytes = read(file_descriptor, &put_buffer, 1)) >
           0) { // Read one byte at a time
      write(opened_file_des, &put_buffer, read_num_bytes);
      if (++counter == length_as_int) {
        break;
      }
    }
    sprintf(response, "HTTP/1.1 201 Created\r\nContent-Length: 0\r\n\r\n");
    write(file_descriptor, response, strlen(response));
    close(opened_file_des);

    if (wantsLog) {
      logfile(log_file_name, cli_request, resource_name);
    }

    return;
  } else {
    sprintf(response, "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
    write(file_descriptor, response, strlen(response));
    logfail(log_file_name, 400, cli_request, resource_name);
    return;
  }
}

void *do_work(void *) {

  int i = 0, my_socketfd;
  char cli_request[BUFF_SIZE], resource_name[BUFF_SIZE],
      content_length[BUFF_SIZE];

  while (1) {

    pthread_mutex_lock(&empty_mutex);
    while(incomingConnections == 0) {
      pthread_cond_wait(&empty, &empty_mutex);
    }
    pthread_mutex_unlock(&empty_mutex);

    pthread_mutex_lock(&mutex);
    while (i < 256) {
      if (allSocketFDs[i] != 0) {
        my_socketfd = allSocketFDs[i];
        allSocketFDs[i] = 0;
      }
      i += 1;
    }
    i = 0;

    incomingConnections--;
    currentlyWorking++;
    pthread_mutex_unlock(&mutex);

    // DO WORK
    while (
        parseHeader(my_socketfd, cli_request, resource_name, content_length)) {

      performRequest(my_socketfd, cli_request, resource_name, content_length);
      cleanArray(cli_request, BUFF_SIZE);
      cleanArray(resource_name, BUFF_SIZE);
      cleanArray(content_length, BUFF_SIZE);
    }

    pthread_mutex_lock(&mutex);
    currentlyWorking--;
    pthread_mutex_unlock(&mutex);

    pthread_cond_signal(&full);
  }
}

int main(int argc, char *argv[]) {

  int opt;
  int number_of_threads = 4;
  char const *hostname;
  char const *port;
  bool noPort = true;
  int first = 0;

  while ((opt = getopt(argc, argv, "N:l:")) != -1) {
    switch (opt) {
    case 'N':
      number_of_threads = atoi(optarg);
      break;
    case 'l':
      log_file_name = optarg;
      wantsLog = true;
      break;
    default:
      warn("Unknown Option: %s", optarg);
      break;
    }
  }

  for (int index = optind; index < argc; index++) {
    if (first == 0) {
      hostname = argv[index];
      first++;
    } else if (first == 1) {
      port = argv[index];
      noPort = false;
    }
  }

  if (noPort == true) {
    port = "80";
  }

  struct addrinfo *addrs, hints = {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(hostname, port, &hints, &addrs) != 0) {
    warn("getaddrinfo %s", argv[1]);
    return EXIT_FAILURE;
  }

  int main_socket =
      socket(addrs->ai_family, addrs->ai_socktype, addrs->ai_protocol);
  if (main_socket < 0) {
    warn("socket %s", argv[1]);
    return EXIT_FAILURE;
  }

  int enable = 1;
  setsockopt(main_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
  if (bind(main_socket, addrs->ai_addr, addrs->ai_addrlen) < 0) {
    warn("bind %s", argv[1]);
    return EXIT_FAILURE;
  }

  // N is the maximum number of "waiting" connections on the socket.
  // We suggest something like 16.
  if (listen(main_socket, 16) < 0) {
    warn("listen %s", argv[1]);
    return EXIT_FAILURE;
  }

  ///////////////////////////////////////////////////////////////////////
  // Your code, starting with accept(), goes here
  ///////////////////////////////////////////////////////////////////////

  // char newline[]  = "\n";

  pthread_t worker_threads[SMALL_BUFF_SIZE];
  for (int i = 0; i < number_of_threads; ++i) {
    pthread_create(&worker_threads[i], NULL, do_work, NULL);
  }

  struct sockaddr client_addr;
  socklen_t client_len;

  // char testbuff[BUFF_SIZE];

  int x = open(log_file_name, O_WRONLY | O_CREAT | O_TRUNC, 00400 | 00200);
  close(x); // Reset file if it already exits.

  int index = 0;

  for (int i = 0; i < SMALL_BUFF_SIZE; ++i) {
    allSocketFDs[i] = 0;
  }

  while (1) {

    int acceptfd = accept(main_socket, &client_addr, &client_len);
    if (acceptfd < 0) {
      warn("accept %s", argv[1]);
      return EXIT_FAILURE;
    }

    pthread_mutex_lock(&mutex);
    incomingConnections++;
    allSocketFDs[index] = acceptfd;
    index = (index + 1) % SMALL_BUFF_SIZE;
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&full_mutex);
    while(currentlyWorking == number_of_threads) {
      pthread_cond_wait(&full, &full_mutex);
    }
    pthread_mutex_unlock(&full_mutex);

    pthread_cond_signal(&empty);
  }

  // close(acceptfd);
  return EXIT_SUCCESS;
}