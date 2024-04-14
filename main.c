#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void reset_IOP() {
    SifInitRpc(0);
    /* Comment this line if you don't wanna debug the output */
    while (!SifIopReset(NULL, 0)) {};

    while (!SifIopSync()) {};
    SifInitRpc(0);
    sbv_patch_enable_lmb();
}

static void init_drivers() {
    init_ps2_filesystem_driver();
}

static void deinit_drivers() {
    deinit_ps2_filesystem_driver();
}

int main() {
    reset_IOP();
    init_drivers();
    
    // Use mass: device to write to a file
    chdir("mass:/");

    // Open a file in binary write mode
    FILE *file = fopen("dummy_file.bin", "wb");
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }
    // Good 0x1ffe550
    // Bad  0x35c1fa
    // Generate dummy content for to be at 0x35c1fa address
    // char *dummy_content = (unsigned char *)0x35c1fa;
    // unsigned char dummy_content[2000]; // Dummy content of size 1250 bytes
    unsigned char *dummy_content = (unsigned char *)malloc(2000);

    for (int i = 0; i < 2000; i++) {
        dummy_content[i] = (unsigned char)(i % 256); // Fill with values from 0 to 255
    }

    // create a pointer with 6 bits offset
    unsigned char *dummy_content_offset = dummy_content + 6;
    printf("Dummy content address is %p, offset address is %p\n", dummy_content, dummy_content_offset);

    // Write the dummy content to the file
    size_t elements_written = fwrite(dummy_content_offset, 1, 1200, file);
    if (elements_written != 1200) {
        perror("Error writing to file");
        fclose(file);
        free(dummy_content);
        SleepThread();
        return 1;
    }

    // Close the file
    fclose(file);
    free(dummy_content);
    printf("Dummy content written to file successfully.\n");

    deinit_drivers();
    SleepThread();
    return 0;
}
