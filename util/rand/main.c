#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

int main(int ac, char** av)
{
  uint32_t mod = 0;
  const char* fmt = NULL;
  size_t n = 0;
  size_t i;

  if ((ac & 1) == 0)
  {
    return -1;
  }

  for (i = 1; i != ac; i += 2)
  {
    if (strcmp(av[i], "-f") == 0)
    {
      if (strcmp(av[i + 1], "uint8") == 0)
      {
	fmt = "0x%02x";
	mod = 0xff;
      }
      else if (strcmp(av[i + 1], "uint32") == 0)
      {
	fmt = "0x%08x";
	mod = 0xffffffff;
      }
    }
    else if (strcmp(av[i], "-n") == 0)
    {
      n = (size_t)atoi(av[i + 1]);
    }
  }

  if ((fmt == NULL) || (n == 0))
  {
    return -1;
  }

  srand((unsigned int)(getpid() * time(NULL)));

  for (i = 0; i != n; ++i)
  {
    if (i) printf(",");
    printf(fmt, (uint32_t)rand() & mod);
  }

  return 0;
}
