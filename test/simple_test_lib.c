#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>


/* function from an other .c, not compiled
    with -finstrument-functions */
void _f4();



/* just spend 1 sec */
void s1(void)
{
  sleep(1);
}

/* simple functions */
void f1(int a)
{
  int i, j=0;

  for(i=0; i<a; i++)
    {
    j += 1;
    }
}
void f2(int b)
{
  int i, j=0;

  for(i=0; i<b; i++)
    {
    j -= 1;
    }
}
void f3()
{
  f1(10);
  f1(20);
  f1(30);
}

/* simple recursive function */
void recurs(int n)
{
  if (n <= 1)
    return;
  recurs(n-1);
}

/* recursive function which takes 1s */
void recurs_1s(int n)
{
  sleep(1);
  s1();
  if (n <= 1)
    return;
  recurs_1s(n-1);
}


/* cross-recursive function */
void recurs_b(int n);
void recurs_a(int n)
{
int i,j=0;
for(i=0; i<10000000; i++)
{
j = j + 1;
j = j * 2;
j = j / 3;
}
  //sleep(1);
  //s1();
  recurs_b(n);
}

void recurs_b(int n)
{
int i,j=0;
for(i=0; i<10000000; i++)
{
j = j + 1;
j = j * 2;
j = j / 3;
}
  //sleep(1);
  //s1();
  if (n <= 1)
    return;
  recurs_a(n-1);
}

/* heavy function */
void f4()
{
  int i;
  double j=0., w;

  for(i=0; i<100; i++)
    {
    j += sin(cos(sin(cos((double)i))));
    w = pow(j, cos(j));
    j = cos(cos(j));
    }
}

/* small function */
void small()
{
  int i;

  i = 0;
}

/* call to a not-profiled function */
void test()
{
  _f4();
}


void alloc4()
{
  malloc(32);
}
void alloc3()
{
  alloc4();
}
void alloc2()
{
  alloc3();
}
void alloc1()
{
  alloc2();
}

/* calls to functions */
int main(int argc, char *argv[])
{
  char *p1, *p2, *p3;

  p1 = malloc(sizeof(char)*32);
  p2 = malloc(sizeof(char)*32);
  p3 = malloc(sizeof(char)*32);

  recurs(32);

  recurs_a(4);

  recurs_1s(4);

  p2 = realloc(p2, sizeof(char)*64);
  p3 = realloc(p3, sizeof(char)*64);

  //recurs_1s(4);

  test();
  f3();
  small();

  p2 = realloc(p2, sizeof(char)*128);

  free(p2);

  alloc1();

  return(0);
}
