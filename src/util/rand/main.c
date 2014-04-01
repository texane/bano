#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

int main(int ac, char** av)
{
  srand((unsigned int)(getpid() * time(NULL)));
  printf("0x%08x", (uint32_t)(rand() % 0xffffffff));
  return 0;
}
