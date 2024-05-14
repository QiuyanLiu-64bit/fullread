#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

#define BUFFER_SIZE 1024 // 可以根据需要调整缓冲区大小

void computeMD5Hash(FILE *fp, unsigned char *hashResult) {
    MD5_CTX md5;
    char buffer[BUFFER_SIZE];
    size_t bytesRead;

    MD5_Init(&md5);

    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, fp)) != 0) {
        MD5_Update(&md5, buffer, bytesRead);
    }

    MD5_Final(hashResult, &md5);
}

int compareFiles(const char *file1, const char *file2) {
    FILE *fp1 = fopen(file1, "rb");
    FILE *fp2 = fopen(file2, "rb");
    unsigned char hash1[MD5_DIGEST_LENGTH];
    unsigned char hash2[MD5_DIGEST_LENGTH];
    int isEqual = 0;

    if (fp1 == NULL || fp2 == NULL) {
        perror("Error opening file");
        return -1; // 文件打开失败
    }

    computeMD5Hash(fp1, hash1);
    computeMD5Hash(fp2, hash2);

    isEqual = (memcmp(hash1, hash2, MD5_DIGEST_LENGTH) == 0);

    fclose(fp1);
    fclose(fp2);

    return isEqual;
}

int main() {
    const char *file1 = "/home/ych/ec_test/test_file/read/64MB_src";
    const char *file2 = "/home/ych/ec_test/test_file/write/64MB_src";

    int result = compareFiles(file1, file2);
    if (result == 1) {
        printf("Files are the same.\n");
    } else if (result == 0) {
        printf("Files are different.\n");
    } else {
        printf("An error occurred.\n");
    }

    return 0;
}

/*
gcc -o CheckSameFile CheckSameFile.c -lcrypto
*/

