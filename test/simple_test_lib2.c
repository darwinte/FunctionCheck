#include <stdio.h>
#include <stdlib.h>
#include <math.h>



void _f4()
{
  int i;
  double j=0., w;

  for(i=0; i<1000000; i++)
    {
    j += sin(cos(sin(cos((double)i))));
    w = pow(j, cos(j));
    j = cos(cos(j));
    }
}
