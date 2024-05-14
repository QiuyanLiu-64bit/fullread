#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for read, close
#include <sys/time.h> // for gettimeofday

#define BUFFER_SIZE 64*1024*1024 // Buffer size in bytes
#define TEST_COUNT 10 // Number of tests to average

int main() {
    int file;
    char *buffer;
    ssize_t bytesRead;
    struct timeval start, end;
    double duration, totalDuration = 0, avgDuration;
    size_t totalBytesRead = 0, avgBytes;
    const char *filePath = "test_file.dat"; // Path to the test file

    // Allocate buffer
    buffer = (char *)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < TEST_COUNT; i++) {
        system("sync; echo 3 > /proc/sys/vm/drop_caches"); // Clear cache
        // Open the file
        file = open(filePath, O_RDONLY);
        if (file < 0) {
            perror("Failed to open file");
            free(buffer);
            return EXIT_FAILURE;
        }

        // Read from the file and measure time
        gettimeofday(&start, NULL);
        while ((bytesRead = read(file, buffer, BUFFER_SIZE)) > 0) {
            totalBytesRead += bytesRead;
        }
        gettimeofday(&end, NULL);

        // Calculate duration in seconds
        duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        totalDuration += duration;

        close(file); // Close the file after each test
    }

    avgDuration = totalDuration / TEST_COUNT;
    avgBytes = totalBytesRead / TEST_COUNT;

    // Calculate and display read speed in MB/s (note: 1MB = 1000 * 1000 bytes)
    printf("Average read speed: %.2f MB/s\n", (avgBytes / avgDuration) / (1000 * 1000));

    // Cleanup
    free(buffer);

    return EXIT_SUCCESS;
}
