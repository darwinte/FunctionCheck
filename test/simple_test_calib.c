#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* simple test with calibrated time spend in each function */

void lo1()
{
  sleep(1);
}


void lo2()
{
  sleep(2);
}

void lo3()
{
  sleep(3);
}


void lo1to2()
{
  sleep(1);
  lo1();
}

void lo3to6()
{
  sleep(3);
  lo3();
}

void rec(int n)
{
  sleep(1);
  if (n > 0)
    rec(n-1);
}


int main(int argc, char *argv[])
{
  lo1();

  lo2();


  lo1to2();
  
  lo3to6();
  
  rec(5);

  return(0);
}
