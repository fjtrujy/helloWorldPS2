#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <sbv_patches.h>
#include <ps2_filesystem_driver.h>

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

static int finishExecution() {
    deinit_drivers();
    SleepThread();
    return 0;
}

// Dummy content to write to a file with harcoded values
static unsigned char dummy_content[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

static int checkWriteRead(int offset, int nbytes) {
    // Print the offset and nbytes
    printf("Checking write and read with offset %d and nbytes %d\n", offset, nbytes);

    // Open a file, remove content and write new content
    FILE *file = fopen("dummy_file.bin", "wb");

    if (file == NULL) {
        perror("Error opening file in read and write mode");
        return 1;
    }
    
    // Allocate memory for read buffer nbytes + offset
    unsigned char *read_buffer = (unsigned char *)malloc(nbytes + offset);

    // Copy content from dummy_content to read_buffer
    memcpy(read_buffer + offset, dummy_content, nbytes);

    // Write the read_buffer to the file
    size_t elements_written = fwrite(read_buffer + offset, 1, nbytes, file);

    if (elements_written != nbytes) {
        perror("Error writing to file");
        fclose(file);
        free(read_buffer);
        return 1;
    }

    // Close the file
    fclose(file);

    // Wipe the read buffer
    memset(read_buffer, 0, nbytes + offset);

    // Open the file in read mode
    file = fopen("dummy_file.bin", "rb");

    if (file == NULL) {
        perror("Error opening file to check");
        free(read_buffer);
        return 1;
    }

    // Read the content of the file
    size_t elements_read = fread(read_buffer, 1, nbytes, file);

    if (elements_read != nbytes) {
        perror("Error reading from file");
        fclose(file);
        free(read_buffer);
        return 1;
    }

    // Compare the read content with the dummy content
    if (memcmp(read_buffer, dummy_content, nbytes) != 0) {
        printf("Content read from file does not match the dummy content\n");
        fclose(file);
        free(read_buffer);
        return 1;
    }

    // Close the file
    fclose(file);

    // Free the read buffer
    free(read_buffer);

    printf("Content read from file matches the dummy content with offset %d and nbytes %d\n", offset, nbytes);

    // Remove the file
    remove("dummy_file.bin");

    return 0;
} 

int main(int argc, char *argv[]) {
    reset_IOP();
    init_drivers();
    
    // Use mass: device to write to a file
    chdir("mass:/");

    // Check randomly with different offsets and nbytes
    if (checkWriteRead(0, 256)) { return finishExecution(); }
    if (checkWriteRead(1, 255)) { return finishExecution(); }
    if (checkWriteRead(2, 254)) { return finishExecution(); }
    if (checkWriteRead(3, 253)) { return finishExecution(); }
    if (checkWriteRead(4, 252)) { return finishExecution(); }
    if (checkWriteRead(5, 251)) { return finishExecution(); }
    if (checkWriteRead(6, 250)) { return finishExecution(); }
    if (checkWriteRead(7, 249)) { return finishExecution(); }
    if (checkWriteRead(8, 248)) { return finishExecution(); }
    if (checkWriteRead(9, 247)) { return finishExecution(); }
    if (checkWriteRead(10, 246)) { return finishExecution(); }
    if (checkWriteRead(11, 245)) { return finishExecution(); }
    if (checkWriteRead(12, 244)) { return finishExecution(); }
    if (checkWriteRead(13, 243)) { return finishExecution(); }
    if (checkWriteRead(14, 242)) { return finishExecution(); }
    if (checkWriteRead(15, 241)) { return finishExecution(); }
    if (checkWriteRead(16, 240)) { return finishExecution(); }

    printf("All checks passed\n");
    return finishExecution();
}
