// Dog.cpp by Emil Ingemansson (eingeman)
// CSE 130 asgn 0

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

void print_func(int in_file_des, int out_file_des) {
  size_t buffer_size = 1024, read_bytes;
  char buffer[1024];

  while ((read_bytes = read(in_file_des, &buffer, buffer_size)) > 0) {
    write(out_file_des, &buffer, read_bytes);
  }
}

int main(int argc, const char *argv[]) {
  int exit_status = EXIT_SUCCESS;
  size_t file_des;
  int STDIN = 0, STDOUT = 1;

  if (argc < 2) {
    print_func(STDIN,
               STDOUT); // If no args, then read from STDIN and print STDOUT
  } else {
    for (int i = 1; i < argc; ++i) {
      if (*argv[i] == '-') {
        print_func(
            STDIN,
            STDOUT); // If read a '-', then read from STDIN and print to STDOUT
      } else {
        if ((file_des = open(argv[i], O_RDWR)) == -1) {
          warn("%s", argv[i]);
          exit_status = EXIT_FAILURE;
        } else {
          print_func(file_des, STDOUT);
          close(file_des);
        }
      }
    }
  }
  return exit_status;
}
