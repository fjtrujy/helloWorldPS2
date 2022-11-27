# HELLO WORLD & Binary Sizes for PS2

## Summary
This repo was created with the objective of comparing binary sizes of a super dummy `elf` for `PS2`, as usual there is no better example than `Hello World!`

The `ps2dev` community has been existing a lot of years, but just in the latest one (from 2019) the toolchain has been updated (`binutils, gcc and newlib`). The toolchain's upgrade bring a lot of new functionality & features, but on the other hands could bring as well bugs or unexpected behaviour.

One of the thing that some PS2 Devs complained was about the binary size increased.

## Action
How I was considering this size increase, something that shouldn't happen, I have been a couple of weeks investigating the root of this issue.

## Information
Ideally when a binary is generated, the information that contains inside "should" be **JUST** the necessary functionality, which means the funcionality that the developer wrote in his program.

Well, that sentency is partially true, let me explain it.
Our program is just a hello world:
```c
int main() {
    printf("Hello world!\n");

    return 0;
}
```

So, in theory the program should just contains required things to do a single `printf`, however we can't forget, that before this `main` function is called, there are tons of thing that need to be prepared in order to make this `printf` to work. Usually this "pre needed" things are know as "standard libs". In the case of the `PS2` what we have:

```
- newlib -> libc.a
- ps2sdkc -> libps2sdkc.a
- ps2 kernel -> libkernel.a
```

These 3 libraries, are the "guilties" of the exceed of the binary size.

## Why?
These libraries contains the basic functionality to make to work every single PS2 app, and by default there are some functions that are called during initializacion of the program, in order to prepare the environment and basic functionality.

Working in this way, it will work for majority of developers, they will have a environment to work with a lot of functionality ready to be used, but in cons... some devs are affected increasing binary sizes.

## How can we reduce it?
So there are some ways of trying it.

### Nano newlib
First of all is try to reduce the size of the libraries it self. The most critical one is `newlib`. In order to achieve this I have created a nano version of newlib.

### Override weak functions
Secondly during the initalization of the `ps2sdkc` there are some `weaks` methods required by `newlib` they are:

```c
void _libcglue_init();
void _libcglue_deinit();

....
void _libcglue_timezone_update(); // Called by _libcglue_init in the weak implementation
```

These functions prepare the app to use properly all the timing functionality (sleep, nanosleep, cpu_ticks, getlocaltimezone).

If your app, is not going to use any functionality realted to this, you can "stop" the including of these code onto your main app. In order to do this, you can just create empty methods.

If your app uses time function but is ignoring `timezones`:
```c
void _libcglue_timezone_update() {}
```

If your app is not using time functions at all (this one also ignore `timezones`)):
```c
void _libcglue_init() {}
void _libcglue_deinit() {}
```

Adding these piece of code to your main app, will avoid to include unncesary initialization to your project.

You can take a look about where these `weak functions` are here:
https://github.com/ps2dev/ps2sdk/blob/master/ee/libc/src/init.c

### Kernel library
The current PS2 toolchain expose 2 versions of the kerner library:
```
libkernel.a
libkernel-nopatch.a
```
I'm not going to enter in details about what are they differences (honestly I did't take a look deeply), however I can tell you that if you are looking for reduce the binary size, you should give a try and use `libkernel-nopatch`

## Process
In the CI (`Github Actions`), we have used the different toolchains versions, for generating the hello world example in order to compare sizes.
https://github.com/fjtrujy/helloWorldPS2/actions

Using the `matrix` option of `GitHub Actions` we have created all the different possibities. Here you have the different flags that we are passing to the hello world, to check sizes. The matrix also covers combinations between them.
```
DUMMY_LIBC_INIT
DUMMY_TIMEZONE
KERNEL_NOPATCH
NEWLIB_NANO
```

## Results
Here you can check the different sizes based in the matrix option selected.

| Flags/Dockers                                    	| v1.0   	| v1.1   	| v1.2.0 	| latest 	|
|--------------------------------------------------	|--------	|--------	|--------	|--------	|
| DEFAULT=1                                        	| 24 KB 	| 52.2 KB 	| 101 KB 	| 99.3 KB 	|
| DUMMY_LIBC_INIT=1                                	| 21.9 KB 	| 51.5 KB 	| 53.7 KB 	| 30.8 KB 	|
| DUMMY_TIMEZONE=1                                 	| 24 KB 	| 52.2 KB 	| 54.6 KB 	| 31.9 KB 	|
| KERNEL_NOPATCH=1                                 	| 24 KB 	| 52.2 KB 	| 101 KB 	| 92.1 KB 	|
| NEWLIB_NANO=1                                    	| 24 KB 	| 52.2 KB 	| 101 KB 	| 39.1 KB 	|
| KERNEL_NOPATCH=1 NEWLIB_NANO=1                   	| 24 KB 	| 52.2 KB 	| 101 KB 	| 31.9 KB 	|
| KERNEL_NOPATCH=1 NEWLIB_NANO=1 DUMMY_TIMEZONE=1  	| 24 KB 	| 52.2 KB 	| 54.6 KB 	| 15.2 KB 	|
| KERNEL_NOPATCH=1 NEWLIB_NANO=1 DUMMY_LIBC_INIT=1 	| 21.9 KB 	| 51.5 KB 	| 53.7 KB 	| 14.3 KB 	|

## How to use it in your project?
`KERNEL_NOPATCH` and `NEWLIB_NANO` variables are ready to be used in your project if you use makefiles from `ps2sdk`.
Just add in your `makefile`, 
```makefile
KERNEL_NOPATCH = 1
```
and/or
```makefile
NEWLIB_NANO = 1
````

To override `weak functions` is up to you, just put them in your `main.c`.

If you have doubts, please take a look to the `main.c` of this project.

## Conclusion
The legacy toolchain was created eariler 2003 (almost 20 years ago), during that time the main tools used (binutils, gcc and newlib) have been improving and adding a lot of functionality. However this doesn't mean to increase binary sizes.

One of the most important topic in the tools is to keep binaries as much as possible, having the best performance, and this is why even 20 years later just tunning up some flags and configuration we can have even a smaller binary.

Let's go to move on in the same direction and keep improving our PS2 Environent.

## ENJOY!