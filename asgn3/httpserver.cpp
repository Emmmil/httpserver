// Emil Ingemansson (eingeman) httpserver.cpp
// CSE 130 Assignment 2

#include "city.h"
#include "config.h"
#include <algorithm>
#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#define SMALL_BUFF_SIZE 256
#define BUFF_SIZE 1024
#define FIRST 32

//==============START OF CITYHASH CODE=====================================
using namespace std;

static uint32 UNALIGNED_LOAD32(const char *p) {
  uint32 result;
  memcpy(&result, p, sizeof(result));
  return result;
}

#ifdef _MSC_VER

#include <stdlib.h>
#define bswap_32(x) _byteswap_ulong(x)
#define bswap_64(x) _byteswap_uint64(x)

#elif defined(__APPLE__)

// Mac OS X / Darwin features
#include <libkern/OSByteOrder.h>
#define bswap_32(x) OSSwapInt32(x)
#define bswap_64(x) OSSwapInt64(x)

#elif defined(__sun) || defined(sun)

#include <sys/byteorder.h>
#define bswap_32(x) BSWAP_32(x)
#define bswap_64(x) BSWAP_64(x)

#elif defined(__FreeBSD__)

#include <sys/endian.h>
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)

#elif defined(__OpenBSD__)

#include <sys/types.h>
#define bswap_32(x) swap32(x)
#define bswap_64(x) swap64(x)

#elif defined(__NetBSD__)

#include <machine/bswap.h>
#include <sys/types.h>
#if defined(__BSWAP_RENAME) && !defined(__bswap_32)
#define bswap_32(x) bswap32(x)
#define bswap_64(x) bswap64(x)
#endif

#else

#include <byteswap.h>

#endif

#ifdef WORDS_BIGENDIAN
#define uint32_in_expected_order(x) (bswap_32(x))
#define uint64_in_expected_order(x) (bswap_64(x))
#else
#define uint32_in_expected_order(x) (x)
#define uint64_in_expected_order(x) (x)
#endif

#if !defined(LIKELY)
#if HAVE_BUILTIN_EXPECT
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define LIKELY(x) (x)
#endif
#endif

static uint32 Fetch32(const char *p) {
  return uint32_in_expected_order(UNALIGNED_LOAD32(p));
}

// Magic numbers for 32-bit hashing.  Copied from Murmur3.
static const uint32 c1 = 0xcc9e2d51;
static const uint32 c2 = 0x1b873593;

// A 32-bit to 32-bit integer hash copied from Murmur3.
static uint32 fmix(uint32 h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

static uint32 Rotate32(uint32 val, int shift) {
  // Avoid shifting by 32: doing so yields an undefined result.
  return shift == 0 ? val : ((val >> shift) | (val << (32 - shift)));
}

#undef PERMUTE3
#define PERMUTE3(a, b, c)                                                      \
  do {                                                                         \
    std::swap(a, b);                                                           \
    std::swap(a, c);                                                           \
  } while (0)

static uint32 Mur(uint32 a, uint32 h) {
  // Helper from Murmur3 for combining two 32-bit values.
  a *= c1;
  a = Rotate32(a, 17);
  a *= c2;
  h ^= a;
  h = Rotate32(h, 19);
  return h * 5 + 0xe6546b64;
}

static uint32 Hash32Len13to24(const char *s, size_t len) {
  uint32 a = Fetch32(s - 4 + (len >> 1));
  uint32 b = Fetch32(s + 4);
  uint32 c = Fetch32(s + len - 8);
  uint32 d = Fetch32(s + (len >> 1));
  uint32 e = Fetch32(s);
  uint32 f = Fetch32(s + len - 4);
  uint32 h = len;

  return fmix(Mur(f, Mur(e, Mur(d, Mur(c, Mur(b, Mur(a, h)))))));
}

static uint32 Hash32Len0to4(const char *s, size_t len) {
  uint32 b = 0;
  uint32 c = 9;
  for (size_t i = 0; i < len; i++) {
    signed char v = s[i];
    b = b * c1 + v;
    c ^= b;
  }
  return fmix(Mur(b, Mur(len, c)));
}

static uint32 Hash32Len5to12(const char *s, size_t len) {
  uint32 a = len, b = len * 5, c = 9, d = b;
  a += Fetch32(s);
  b += Fetch32(s + len - 4);
  c += Fetch32(s + ((len >> 1) & 4));
  return fmix(Mur(c, Mur(b, Mur(a, d))));
}

uint32 CityHash32(const char *s, size_t len) {
  if (len <= 24) {
    return len <= 12
               ? (len <= 4 ? Hash32Len0to4(s, len) : Hash32Len5to12(s, len))
               : Hash32Len13to24(s, len);
  }

  // len > 24
  uint32 h = len, g = c1 * len, f = g;
  uint32 a0 = Rotate32(Fetch32(s + len - 4) * c1, 17) * c2;
  uint32 a1 = Rotate32(Fetch32(s + len - 8) * c1, 17) * c2;
  uint32 a2 = Rotate32(Fetch32(s + len - 16) * c1, 17) * c2;
  uint32 a3 = Rotate32(Fetch32(s + len - 12) * c1, 17) * c2;
  uint32 a4 = Rotate32(Fetch32(s + len - 20) * c1, 17) * c2;
  h ^= a0;
  h = Rotate32(h, 19);
  h = h * 5 + 0xe6546b64;
  h ^= a2;
  h = Rotate32(h, 19);
  h = h * 5 + 0xe6546b64;
  g ^= a1;
  g = Rotate32(g, 19);
  g = g * 5 + 0xe6546b64;
  g ^= a3;
  g = Rotate32(g, 19);
  g = g * 5 + 0xe6546b64;
  f += a4;
  f = Rotate32(f, 19);
  f = f * 5 + 0xe6546b64;
  size_t iters = (len - 1) / 20;
  do {
    a0 = Rotate32(Fetch32(s) * c1, 17) * c2;
    a1 = Fetch32(s + 4);
    a2 = Rotate32(Fetch32(s + 8) * c1, 17) * c2;
    a3 = Rotate32(Fetch32(s + 12) * c1, 17) * c2;
    a4 = Fetch32(s + 16);
    h ^= a0;
    h = Rotate32(h, 18);
    h = h * 5 + 0xe6546b64;
    f += a1;
    f = Rotate32(f, 19);
    f = f * c1;
    g += a2;
    g = Rotate32(g, 18);
    g = g * 5 + 0xe6546b64;
    h ^= a3 + a1;
    h = Rotate32(h, 19);
    h = h * 5 + 0xe6546b64;
    g ^= a4;
    g = bswap_32(g) * 5;
    h += a4 * 5;
    h = bswap_32(h);
    f += a0;
    PERMUTE3(f, h, g);
    s += 20;
  } while (--iters != 0);
  g = Rotate32(g, 11) * c1;
  g = Rotate32(g, 17) * c1;
  f = Rotate32(f, 11) * c1;
  f = Rotate32(f, 17) * c1;
  h = Rotate32(h + g, 19);
  h = h * 5 + 0xe6546b64;
  h = Rotate32(h, 17) * c1;
  h = Rotate32(h + f, 19);
  h = h * 5 + 0xe6546b64;
  h = Rotate32(h, 17) * c1;
  return h;
}
//=======================END OF CITYHASH
//CODE=====================================

off_t log_offset = 0;
int incomingConnections = 0, currentlyWorking = 0;
int allSocketFDs[SMALL_BUFF_SIZE];

char *log_file_name;
char *hash_file_name;
bool wantsLog = false;
bool wantsHash = false;

pthread_mutex_t full_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t empty_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t hash_mutex = PTHREAD_MUTEX_INITIALIZER;

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
  char failBuff[SMALL_BUFF_SIZE];
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

void loghash(char *filename, char cli_request[], char resource_for_content[],
             char actual_content[]) {
  char loghashBuff[SMALL_BUFF_SIZE];
  off_t localoffset = log_offset;

  sprintf(loghashBuff, "%s %s length %ld\n%s========\n", cli_request,
          resource_for_content, strlen(actual_content), actual_content);

  off_t increaseOffset = strlen(loghashBuff);

  pthread_mutex_lock(&log_mutex);
  log_offset += increaseOffset;
  pthread_mutex_unlock(&log_mutex);

  int log_descriptor = open(filename, O_WRONLY | O_CREAT, 00400 | 00200);

  pwrite(log_descriptor, loghashBuff, strlen(loghashBuff), localoffset);
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
  char first_buffer[1];
  int countee = 0;
  int charcount = 0;
  int cleanbody = 0;
  cleanArray(requestAndAddress, SMALL_BUFF_SIZE);

  // if (read(file_descriptor, requestAndAddress, FIRST) <= 0) {
  // return false;
  //}

  while (read(file_descriptor, first_buffer, 1) > 0) {
    if (first_buffer[0] == '\r') {
      if (cleanbody == 0 or cleanbody == 2) {
        cleanbody += 1;
      } else {
        cleanbody = 0;
      }
    } else if (first_buffer[0] == '\n') {
      if (cleanbody == 1 or cleanbody == 3) {
        cleanbody += 1;
      } else {
        cleanbody = 0;
      }
    } else {
      cleanbody = 0;
    }

    if (cleanbody == 4) {
      break;
    }
    requestAndAddress[charcount] = first_buffer[0];
    charcount++;

    if (first_buffer[0] == ' ') {
      countee += 1;
      // write(1, "countee!\n", 9);
    }
    if (countee == 2) {
      // write(1, "breakee!\n", 9);
      break;
    }
  }

  sscanf(requestAndAddress, "%s %s", cli_request, resource_name);

  if (strlen(cli_request) == 0 or strlen(resource_name) == 0) {
    return false;
  }

  if (cleanbody == 4) {
    return true;
  }

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

  if (strcmp(cli_request, "GET") == 0) {
    int hashfd = open(hash_file_name, O_RDONLY);
    int i = 0;
    char hashBuffer[SMALL_BUFF_SIZE];
    char existing_name[SMALL_BUFF_SIZE];
    char new_name[SMALL_BUFF_SIZE];
    int increment = 0;

    // write(1, "slop\n", 5);

    while (!isValidAddress(resource_name)) {
      // write(1, "loop\n", 5);
      if (i >= 10) {
        sprintf(response,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        write(file_descriptor, response, strlen(response));

        logfail(log_file_name, 400, cli_request, resource_name);
        return;
      }
      while (1) {

        pread(hashfd, hashBuffer, 128,
              (((CityHash32(resource_name, strlen(resource_name)) % 8000) +
                increment) *
               128) +
                  10);
        sscanf(hashBuffer, "%s %s", new_name, existing_name);
        // write(1, hashBuffer, strlen(hashBuffer));
        if (strcmp(existing_name, resource_name) == 0 or
            strlen(existing_name) ==
                0) { // If values match or we reach an empty bucket
          break;
        }
        increment++;
      }

      // write(1, existing_name, strlen(new_name));

      if (strcmp(existing_name, resource_name) == 0) {
        resource_name = new_name;
        break;
      }

      if (strlen(existing_name) == 0) {
        sprintf(response,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        write(file_descriptor, response, strlen(response));

        logfail(log_file_name, 400, cli_request, resource_name);
        return;
      }
      i += 1;
    }

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

    int hashfd = open(hash_file_name, O_RDONLY);
    int i = 0;
    char hashBuffer[SMALL_BUFF_SIZE];
    char existing_name[SMALL_BUFF_SIZE];
    char new_name[SMALL_BUFF_SIZE];
    int increment = 0;

    while (!isValidAddress(resource_name)) {
      // write(1, "loop\n", 5);
      if (i >= 10) {
        sprintf(response,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        write(file_descriptor, response, strlen(response));

        logfail(log_file_name, 400, cli_request, resource_name);
        return;
      }
      while (1) {

        pread(hashfd, hashBuffer, 128,
              (((CityHash32(resource_name, strlen(resource_name)) % 8000) +
                increment) *
               128) +
                  10);
        sscanf(hashBuffer, "%s %s", existing_name, new_name);
        if (strcmp(new_name, resource_name) == 0 or
            strlen(existing_name) ==
                0) { // If values match or we reach an empty bucket
          break;
        }
        increment++;
      }

      if (strcmp(new_name, resource_name) == 0) {
        resource_name = existing_name;
        break;
      }

      if (strlen(existing_name) == 0) {
        sprintf(response,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        write(file_descriptor, response, strlen(response));

        logfail(log_file_name, 400, cli_request, resource_name);
        return;
      }
      i++;
    }

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

  } else if (strcmp(cli_request, "PATCH") == 0) {
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
    char bodyBuffer[SMALL_BUFF_SIZE];
    char keyValue[SMALL_BUFF_SIZE];

    while ((read_num_bytes = read(file_descriptor, &put_buffer, 1)) >
           0) { // Read one byte at a time
      bodyBuffer[counter] = put_buffer[0];
      if (++counter == length_as_int) {
        break;
      }
    }

    memmove(keyValue, bodyBuffer + 6, strlen(bodyBuffer));

    pthread_mutex_lock(&hash_mutex);
    char checkbucket[5];
    char existing_name[SMALL_BUFF_SIZE];
    char new_name[SMALL_BUFF_SIZE];
    int hashfd = open(hash_file_name, O_WRONLY);
    bool bucketInUse;
    int increment = 0;

    sscanf(keyValue, "%s %s", existing_name, new_name);

    if (open(existing_name, O_RDONLY) == -1) {
      sprintf(response, "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
      write(file_descriptor, response, strlen(response));
      if (wantsLog)
        logfail(log_file_name, 404, cli_request, resource_name);
      return;
    }

    for (size_t i = 0; i < strlen(new_name); i++) {
      if (!isprint(new_name[i])) {
        sprintf(response,
                "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
        write(file_descriptor, response, strlen(response));
        if (wantsLog)
          logfail(log_file_name, 400, cli_request, resource_name);
        return;
      }
    }

    while (true) {
      bucketInUse = false;
      pread(hashfd, checkbucket, 5,
            (((CityHash32(new_name, strlen(new_name)) % 8000) + increment) *
             128) +
                10);
      for (int i = 0; i < 5; i++) {
        if (checkbucket[i] != '\0') {
          bucketInUse = true;
        }
      }
      if (bucketInUse == false) {
        break;
      }
      increment += 1;
    }

    pwrite(
        hashfd, keyValue, strlen(keyValue),
        (((CityHash32(new_name, strlen(new_name)) % 8000) + increment) * 128) +
            10);
    close(hashfd);

    pthread_mutex_unlock(&hash_mutex);

    sprintf(response, "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n");
    write(file_descriptor, response, strlen(response));
    if (wantsLog) {
      loghash(log_file_name, cli_request, resource_name, bodyBuffer);
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
    while (incomingConnections == 0) {
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

  while ((opt = getopt(argc, argv, "N:l:a:")) != -1) {
    switch (opt) {
    case 'N':
      number_of_threads = atoi(optarg);
      break;
    case 'l':
      log_file_name = optarg;
      wantsLog = true;
      break;
    case 'a':
      hash_file_name = optarg;
      wantsHash = true;
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

  pthread_t worker_threads[SMALL_BUFF_SIZE];
  for (int i = 0; i < number_of_threads; ++i) {
    pthread_create(&worker_threads[i], NULL, do_work, NULL);
  }

  int x = open(log_file_name, O_WRONLY | O_CREAT | O_TRUNC, 00400 | 00200);
  close(x); // Reset file if it already exits.

  int index = 0;

  for (int i = 0; i < SMALL_BUFF_SIZE; ++i) {
    allSocketFDs[i] = 0;
  }

  char checkMagic[10];
  char magicNumber[] = "6942069420";
  char emptyBucket[] =
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"
      "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";

  int hashfd;
  if (wantsHash) {
    if ((hashfd = open(hash_file_name, O_RDONLY)) !=
        -1) { // hash file already exists
      read(hashfd, checkMagic, 10);
      if (strcmp(checkMagic, magicNumber) != 0) {
        write(2, "Error: Magic Number invalid\n", 28);
        return EXIT_FAILURE;
      }
    } else { // hash file doens't exist
      hashfd = open(hash_file_name, O_RDWR | O_CREAT, 00400 | 00200);
      write(hashfd, magicNumber, strlen(magicNumber));
      for (int i = 0; i < 8001; i++) {
        pwrite(hashfd, emptyBucket, sizeof(emptyBucket) - 1, (i * 128) + 10);
      }
    }
    close(hashfd);
  } else { // The -a option was not specified
    write(2, "Error: No hash file specified\n", 30);
    return EXIT_FAILURE;
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
  int acceptfd;
  struct sockaddr client_addr;
  socklen_t client_len;

  while (1) {

    acceptfd = accept(main_socket, &client_addr, &client_len);
    if (acceptfd < 0) {
      warn("accept");
      return EXIT_FAILURE;
    }

    pthread_mutex_lock(&mutex);
    incomingConnections++;
    allSocketFDs[index] = acceptfd;
    index = (index + 1) % SMALL_BUFF_SIZE;
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&full_mutex);
    while (currentlyWorking == number_of_threads) {
      pthread_cond_wait(&full, &full_mutex);
    }
    pthread_mutex_unlock(&full_mutex);

    pthread_cond_signal(&empty);
  }

  return EXIT_SUCCESS;
}