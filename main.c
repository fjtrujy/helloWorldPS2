#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <sbv_patches.h>
#include <iopcontrol.h>
#include <stdio.h>

int main()
{
   SifInitRpc(0);
   /* Comment this line if you don't wanna debug the output */
   while(!SifIopReset(NULL, 0)){};

   while(!SifIopSync()){};
   SifInitRpc(0);
   sbv_patch_enable_lmb();

   printf("Hello, world!\n");
   float hello = 1.0f;

   printf("Hello, world! %f\n", hello);

   while (1)
   {
      /* code */
   }
   

   return 0;
}