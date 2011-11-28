#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>




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

void *start_control(void *data)
{
  f2(10);

  return(NULL);
}


int main(int argc, char*argv[])
{
  pthread_t tid;
  int data=10;
  void *ret=NULL;

  printf("Simple test program for functioncheck...\n");
  rec1();
  
  combo(20);

  pthread_create(&tid, NULL, start_control, (void*)(&data));

  f1(5);
  
  pthread_join(tid, &ret);

  printf("byebye.\n");

  return(0);
}
