
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
   void _ps2sdk_timezone_update() {}
#endif

#if defined(DUMMY_LIBC_INIT)
   void _ps2sdk_libc_init() {}
   void _ps2sdk_libc_deinit() {}
#endif