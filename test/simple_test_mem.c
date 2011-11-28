#include <stdio.h>
#include <stdlib.h>




void * my_malloc(int size)
{
  return(malloc(size));
}


void * _my_realloc(void *ptr, int size)
{
  return(realloc(ptr, size));
}


void * my_realloc(void *ptr, int size)
{
  return(_my_realloc(ptr, size));
}


int main(int argc, char*argv[])
{
  void *p1, *p2, *p3;

  printf("starting...\n");

  p1 = my_malloc(64);
  printf("p1=%p (64)\n", p1);
  p2 = my_malloc(128);
  printf("p2=%p (128)\n", p2);

  p1 = my_realloc(p1, 32);
  printf("p1 -> %p (32)\n", p1);
  p2 = my_realloc(p2, 256);
  printf("p2 -> %p (256)\n", p2);

  p3 = my_malloc(16);
  printf("p3=%p (16)\n", p3);

  free(p2);
  printf("free p2\n");

  printf("the end.\n");

  return(0);
}
