#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <elf-loader.h>

int main(int argc, char *argv[]) {
    char *argvv[1];
    int counter = 0;
    char scounter[11] = {0};

    // Check if there are command-line arguments
    if (argc > 0) {
        // Print each argument
        for (int i = 0; i < argc; i++) {
            printf("argv[%d]: %s\n", i, argv[i]);
            if (i == 1) {
                counter = atoi(argv[i]);
            }
        }
    } else {
        printf("No command-line arguments provided.\n");
    }

    // Wait 5 seconds
    printf("Waiting 3 seconds...\n");
    sleep(3);

    // Load the ELF file
    itoa(counter + 1, scounter, 10);
    argvv[0] = scounter;
    LoadELFFromFile(argv[0], 1, argvv);
    return 0;
}