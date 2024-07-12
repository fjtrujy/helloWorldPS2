#include <stdio.h>

#include <kernel.h>
#include <ps2prof.h>

int calculateNPrime(int n) {
   int i, j, count = 0;
   for (i = 2; i <= n; i++) {
      for (j = 2; j <= i; j++) {
         if (i % j == 0) {
            break;
         }
      }
      if (i == j) {
         count++;
      }
   }
   return count;

}

int dummy_function()
{
   int i;
   for (i = 0; i < 10000; i++)
   {
      printf(".");
   }
   printf("\n");
   return 0;
}

int main()
{
   printf("Hello, world!\n");
   dummy_function();  
   calculateNPrime(1000);
   printf("Goodbye, world!\n");

   gprof_cleanup();
   SleepThread();
   return 0;
}