#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define N 13
int file_size[N] = {6, 8, 12, 24, 40, 48, 96, 128, 192, 384, 640, 768, 1536};

// arg1 is buffer_size
// arg2 is final_size
// arg3 is src_filename
// arg4 is dst_filename
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Error format\n");
        printf("./cut_file_test src_filename\n");
        return 0;
    }
    int buffer_size_MB = 2;
    int i;
    for (i = 0; i < N; i++)
    {
        int final_size_MB = file_size[i];
        if (final_size_MB % buffer_size_MB != 0)
        {
            printf("Error final_size or buffer_size\n");
            printf("Should final_size %% buffer_size == 0\n");
            return 0;
        }
        FILE *src_fp; // src_file pointers
        FILE *dst_fp; // dst_file pointers
        int times = final_size_MB / buffer_size_MB;
        int buffer_size = buffer_size_MB * 1024 * 1024;            // Byte
        char *buffer = (char *)malloc(sizeof(char) * buffer_size); // buffer
        int size_fread;
        int i;

        src_fp = fopen(argv[1], "rb");
        if (src_fp == NULL)
        {
            printf("Failed to open src file \n");
            return -1;
        }
        char src_filename[256] = {0};
        sprintf(src_filename, "%d%s",final_size_MB, "MB_src");
        dst_fp = fopen(src_filename, "wb");
        if (dst_fp == NULL)
        {
            printf("Failed to open dst file \n");
            return -1;
        }

        for (i = 0; i < times; i++)
        {
            size_fread = fread(buffer, sizeof(char), buffer_size, src_fp);
            if (size_fread < buffer_size)
            {
                printf("Error src_file: src_file size less %d MB\n", final_size_MB);
                fclose(src_fp);
                fclose(dst_fp);
                return -1;
            }
            fwrite(buffer, sizeof(char), (size_t)buffer_size, dst_fp);
        }
        fclose(src_fp);
        fclose(dst_fp);
        printf("Successfully: dst_file size %d MB\n", final_size_MB);
    }

    return 0;
}