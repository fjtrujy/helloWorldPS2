#include <kernel.h>

int printf(const char *format, ...);

int main()
{
   while (1)
   {
      printf("Hello, world!\n");
   }
   
   return 0;
}

#if defined(DUMMY_TIMEZONE)
   void _libcglue_timezone_update() {}
#endif

#if defined(DUMMY_LIBC_INIT)
   void _libcglue_init() {}
   void _libcglue_deinit() {}
   void _libcglue_args_parse() {}
#endif

#if defined(KERNEL_NOPATCH)
    DISABLE_PATCHED_FUNCTIONS();
#endif