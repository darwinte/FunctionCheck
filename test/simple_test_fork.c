#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>




int f1(int n)
{
  int i, j=0;


  for(i=0; i<n; i++)
    {
    j++;
    }
  return(j);
}


int f2(int n)
{
  int i, j=0;


  for(i=0; i<n; i++)
    {
    j += 2;
    }
  return(j);
}

int rec4()
{
  return(-1);
}

int rec3()
{
  return(rec4());
}

int rec2()
{
  return(rec3());
}

int rec1()
{
  return(rec2());
}

int combo(int n)
{
  return(f1(n) + f2(n));
}


int main(int argc, char*argv[])
{
  printf("Simple test program for functioncheck...\n");

  rec1();
  
  combo(20);

  if (fork() == 0)
    {
    f1(5);
    }
  else
    {
    f2(10);
    }

  printf("byebye.\n");

  return(0);
}
