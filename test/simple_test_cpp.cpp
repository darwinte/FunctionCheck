#include <stdio.h>
#include <stdlib.h>


/* this file is a small C++ program with a global object.
   it is used to test problems related to constructors/destructors
   in C++ programs, which are called resp. before and after
   main() (it is enoying for me, because I need to detected
   the REAL end of the program.                                    */


class Yannick
{
  int a,b,c;
  char *t;

public:
  Yannick(void);
  ~Yannick();
}global_object;

int i=0;

Yannick::Yannick(void)
{
  printf("cnst\n");
  a = 0;
  b = 1;
  c = 2;

  t = (char *) malloc(100);
}

Yannick::~Yannick(void)
{
  free(t);
  printf("~\n");
}


int main(void)
{
  printf("hello world!\n");
  return(0);
}
