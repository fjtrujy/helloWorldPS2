#include <stdio.h>

__thread int tls_var;

int main()
{
   tls_var = 10;
   printf("tls_var = %d\n", tls_var);

   while (1);
   
   
   return 0;
}
