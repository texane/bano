#ifndef BANO_PERROR_H_INCLUDED
#define BANO_PERROR_H_INCLUDED


#if defined(BANO_CONFIG_PERROR)
#include <stdio.h>
#define BANO_PERROR()				\
do {						\
printf("[!] %s, %u\n", __FILE__, __LINE__);	\
} while (0)
#else
#define BANO_PERROR()
#endif


#endif /* BANO_PERROR_H_INCLUDED */
