#include <cstdio>
#include <cstdlib>
#include <libc/pc9800.h>

int main() {
  printf("Example program built and running successfully!\n");

  int m = __crt0_mtype;
  if (ISPCAT(m)) {
    printf("Running on a PC-AT compatible\n");
  } else if (ISPC98(m)) {
    printf("Running on a PC-98\n");
  } else {
    printf("Running on something unknown\n");
  }
  return EXIT_SUCCESS;
}
