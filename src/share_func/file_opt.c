#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "ec_config.h"


int read_file_to_buffer(const char *filename, char *data, int size)
{
    FILE *fp;
    if ((fp = fopen(filename, "rb")) == NULL)
        ERROR_RETURN_VALUE("Fail open file");

    /* Read file to data */
    int size_fread;
    size_fread = fread(data, sizeof(char), size, fp);

    /* Padding data */
    if (size_fread < size)
    {
        for (int j = size_fread; j < size; j++)
            data[j] = '0';
    }
    fclose(fp);
    return EC_OK;
}

int open_write_file(const char *filename, const char *mode, char *data, int size)
{
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    if (fwrite(data, sizeof(char), (size_t)size, fp) != (size_t)size)
        ERROR_RETURN_VALUE("Fail write file");
    fclose(fp);
    return EC_OK;
}

int open_read_file(const char *filename, const char *mode, char *data, int size)
{
    // FILE *fp = fopen(filename, mode);
    // if (fp == NULL)
    //     ERROR_RETURN_VALUE("Fail open file");
    // if (fread(data, sizeof(char), (size_t)size, fp) != (size_t)size) // read for test, fread for production
    //     ERROR_RETURN_VALUE("Fail read file");
    // fclose(fp);
    // return EC_OK;

    /* read for test, fread for production */
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        ERROR_RETURN_VALUE("Fail to open file");

    int total_read = 0, bytes_read = 0;

    #if DEBUGGING_COMMENTS
    struct timeval start,end;
    gettimeofday(&start,NULL);
    #endif
    /* if (read(fd, data, size) != size) {
        close(fd);  
        ERROR_RETURN_VALUE("Fail to read file");
    } */
    while (total_read < size) {
        if ((bytes_read = read(fd, data + total_read, size - total_read)) == -1) {
            close(fd);
            ERROR_RETURN_VALUE("Fail to read file");
        }
        total_read += bytes_read;
    }
    #if DEBUGGING_COMMENTS
    gettimeofday(&end,NULL);
    printf("Original read io time: ==================================%f\n",(end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);
    #endif

    close(fd);
    return EC_OK;
}

int read_group_strategy(const char* group_strategy_filename, int group_chunk_size[], int nok) {
    FILE* fp = fopen(group_strategy_filename, "r");
    if (fp == NULL) 
        ERROR_RETURN_VALUE("Fail open file");
    double temp;
    for (int i = 0; i < nok; ++i) {
        if (fscanf(fp, "%lf", &temp) != 1) {
            ERROR_RETURN_VALUE("Fail read file");
            break;
        }
        group_chunk_size[i] = (int)temp;
    }

    fclose(fp);
    return EC_OK;
}

int open_read_file_fr(const char *filename, const char *mode, char *data, int size, CElement ce) {
    // FILE *fp = fopen(filename, mode);
    // if (fp == NULL)
    //     ERROR_RETURN_VALUE("Fail open file");

    // int total_read = 0;  // Track the total number of bytes read

    // // Read part1
    // int part1_size = ce.part1.end - ce.part1.start;
    // fseek(fp, ce.part1.start, SEEK_SET);
    // total_read += fread(data, sizeof(char), part1_size, fp); // read for test, fread for production

    // // Read part2 if applicable
    // if (ce.part2.start != 0 && ce.part2.end > ce.part2.start) {
    //     int part2_size = ce.part2.end - ce.part2.start;
    //     fseek(fp, ce.part2.start, SEEK_SET);
    //     total_read += fread(data + total_read, sizeof(char), part2_size, fp); // read for test, fread for production
    // }

    // fclose(fp);

    // // Check if the total read size matches the expected size
    // if (total_read != size)
    //     ERROR_RETURN_VALUE("Read size mismatch");

    // return EC_OK;

    /* read for test, fread for production */
    int fd = open(filename, O_RDONLY);
    if (fd == -1)
        ERROR_RETURN_VALUE("Fail to open file");

    int total_read = 0, bytes_read = 0;

    // Read part1
    #if DEBUGGING_COMMENTS
    struct timeval start,end;
    gettimeofday(&start,NULL);
    #endif
    int part1_size = ce.part1.end - ce.part1.start;
    lseek(fd, ce.part1.start, SEEK_SET);
    // total_read += read(fd, data, part1_size);
    while (total_read < part1_size) {
        if ((bytes_read = read(fd, data + total_read, part1_size - total_read)) == -1) {
            close(fd);
            ERROR_RETURN_VALUE("Fail to read file");
        }
        total_read += bytes_read;
    }
    #if DEBUGGING_COMMENTS
    gettimeofday(&end,NULL);
    printf("Optimized read io time-part1: ==================================%f\n",(end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);
    #endif

    total_read = 0; bytes_read = 0;
    
    // Read part2 if applicable
    #if DEBUGGING_COMMENTS
    gettimeofday(&start,NULL);
    #endif
    if (ce.part2.start != 0) {
        int part2_size = ce.part2.end - ce.part2.start;
        lseek(fd, ce.part2.start, SEEK_SET);
        // total_read += read(fd, data + total_read, part2_size);
        while (total_read < part2_size) {
            if ((bytes_read = read(fd, data + part1_size + total_read, part2_size - total_read)) == -1) {
                close(fd);
                ERROR_RETURN_VALUE("Fail to read file");
            }
            total_read += bytes_read;
        }
    }
    #if DEBUGGING_COMMENTS
    gettimeofday(&end,NULL);
    printf("Optimized read io time-part2: ==================================%f\n",(end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0);
    #endif

    close(fd);

    // Check if the total read size matches the expected size
    // if (total_read != size)
    //     ERROR_RETURN_VALUE("Read size mismatch");

    return EC_OK;
}

int open_write_file_mul(const char *filename, const char *mode, char **data, int size, int num)
{
    int i;
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    for (i = 0; i < num; i++)
        if (fwrite(data[i], sizeof(char), (size_t)size, fp) != (size_t)size)
            ERROR_RETURN_VALUE("Fail write file");
    fclose(fp);
    return EC_OK;
}

int open_write_file_fr(const char *filename, const char *mode, char ***data, int * group_size, int p, int num)
{
    FILE *fp = fopen(filename, mode);
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    for (int i = 0; i < num; i++)
        for(int j = 0; j < p; j++)
            if (fwrite(data[j][i], sizeof(char), (size_t)group_size[j], fp) != (size_t)group_size[j])
                ERROR_RETURN_VALUE("Fail write file");
    fclose(fp);
    return EC_OK;
}

int clear_file(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
        ERROR_RETURN_VALUE("Fail open file");
    fclose(fp);
    return EC_OK;
}

int get_size_file(const char *filename)
{
    struct stat file_status;
    stat(filename, &file_status);
    return file_status.st_size;
}
