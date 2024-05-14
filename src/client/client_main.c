#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "ec_config.h"
#include "share_func.h"
#include "file_opt.h"
#include "network_transfer.h"
#include "bandwidth.h"
#include "repair.h"

typedef struct {
    int rowNumber;// The row number in data[]
    int chunkSize;
    int columnNumber;// The column number in data[]
    int start;
} BElement;

// Function to calculate Greatest Common Divisor (GCD)
// int gcd(int a, int b) {
//     if (b == 0)
//         return a;
//     return gcd(b, a % b);
// }

int read_k_chunks_storagenodes(char **data, char *dst_filename, int *sockfd_array, int *data_multiport_fd, int ok_node_list[], double *node_io)
{
    int i, j;
    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    pthread_t tid[EC_N];

    /* Recv to k storages that saved data chunk */
    for (i = 0; i < EC_K; i++)
    {
        j = ok_node_list[i];
        metadata_array[i].sockfd = sockfd_array[j];
        metadata_array[i].data_fd = data_multiport_fd + j * NUMBER_OF_PORTS;
        metadata_array[i].node_i = j;
        metadata_array[i].opt_type = 1;
        metadata_array[i].block_size = CHUNK_SIZE;
        metadata_array[i].data = data[i];
        metadata_array[i].node_i_io = node_io + i;
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename, j + 1); // filename on storages

        /* Create thread to send one chunk to one storages */
        if (pthread_create(&tid[i], NULL, recv_metadata_chunk_multiport, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }
    for (i = 0; i < EC_K; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);
    return EC_OK;
}

int read_k_chunks_storagenodes_fr(char **data_pre, char *dst_filename, int *sockfd_array, int *data_multiport_fd, int ok_node_list[], int nok, CElement *C, int *node_chunk_size, double *node_io)
{
    int i, j;
    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * nok); // only the func free
    pthread_t tid[nok];

    /* Recv to nok storages */
    for (i = 0; i < nok; i++)
    {
        j = ok_node_list[i];
        metadata_array[i].sockfd = sockfd_array[j];
        metadata_array[i].data_fd = data_multiport_fd + j * NUMBER_OF_PORTS;
        metadata_array[i].node_i = j;
        metadata_array[i].opt_type = 4;
        metadata_array[i].ce = C[i];
        metadata_array[i].block_size = node_chunk_size[i];
        metadata_array[i].data = data_pre[i];
        metadata_array[i].node_i_io = node_io + i;
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename, j + 1); // filename on storages

        /* Create thread to send one chunk to one storages */
        if (pthread_create(&tid[i], NULL, recv_metadata_chunk_multiport, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }
    for (i = 0; i < nok; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");

    free(metadata_array);
    return EC_OK;
}

int write_chunks_storagenodes(char **data, char **coding, char *dst_filename, int *sockfd_array)
{
    int i; // loop control variables

    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    pthread_t tid[EC_N];

    /* Send to each storagenodes */
    for (i = 0; i < EC_N; i++)
    {
        metadata_array[i].sockfd = sockfd_array[i];
        metadata_array[i].opt_type = 0;
        if (i < EC_K)
            metadata_array[i].data = data[i];
        else
            metadata_array[i].data = coding[i - EC_K];
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename, i + 1); // filename on storagenodes

        /* Create thread to send one chunk to one storagenodes */
        if (pthread_create(&tid[i], NULL, send_metadata_chunk, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }

    for (i = 0; i < EC_N; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);
    return EC_OK;
}

int request_tra_repair(int storagenodes_fail, char *dst_filename_healthy, char *dst_filename_failure, int *sockfd_array)
{
    int i; // loop control variables

    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    pthread_t tid[EC_N];

    /* Send to each storagenodes */
    for (i = 0; i < EC_K + 1; i++)
    {
        metadata_array[i].sockfd = sockfd_array[i];
        memset(metadata_array[i].dst_filename_storagenodes, 0, sizeof(metadata_array[i].dst_filename_storagenodes));
        if (i == storagenodes_fail)
        {
            metadata_array[i].opt_type = 2;
            sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename_failure, i + 1); // filename on storagenodes
        }
        else
        {
            metadata_array[i].opt_type = 3;
            sprintf(metadata_array[i].dst_filename_storagenodes, "%s_%d", dst_filename_healthy, i + 1); // filename on storagenodes
        }

        /* Create thread to send one request to one storagenodes */
        if (pthread_create(&tid[i], NULL, send_metadata, (void *)&metadata_array[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread");
    }

    for (i = 0; i < EC_K + 1; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);

    return EC_OK;
}

int ec_read(int argc, char **argv, PerformanceData *perfData, double *node_io, int *sockfd_array, int *ok_node_list, int nok, int *err_node_list, int nerrs, int *data_multiport_fd)
{
    printf("[ec_read begin]\n");
    int i, j; // loop control variables

    /* File arguments */
    int file_size; // size of file
    char src_filename[MAX_PATH_LEN] = {0}, dst_filename[MAX_PATH_LEN] = {0}, file_size_filename[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(src_filename, "%s%s%s", ec_directory, READ_PATH, argv[2]);                 // get src_filename
    sprintf(dst_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);                // get dst_filename
    sprintf(file_size_filename, "%s%s%s", ec_directory, FILE_SIZE_PATH, argv[3]);      // get file_size_filename to save file size of src file
    // printf("src_filename = %s, dst_filename = %s, file_size_filename = %s\n", src_filename, dst_filename, file_size_filename);

    /* EC arguments */
    int buffer_size = EC_K * CHUNK_SIZE;                       // stripe size
    char *buffer = (char *)malloc(sizeof(char) * buffer_size); // buffer for EC stripe
    if (buffer == NULL)
        ERROR_RETURN_VALUE("Fail malloc data buffer");

    /* ISA-L arguments */
    int *matrix = NULL;                                          // coding matrix
    char **data = (char **)malloc(sizeof(char *) * EC_K);        // data chunk
    char **coding = (char **)malloc(sizeof(char *) * EC_M);      // coding chunk
    char **repair_data = (char **)malloc(sizeof(char *) * EC_K); // repair data chunk

    /* initialize data buffer and coding buffer */
    for (i = 0; i < EC_M; i++)
    {
        coding[i] = (char *)malloc(sizeof(char) * CHUNK_SIZE);
        if (coding[i] == NULL)
            ERROR_RETURN_VALUE("Fail malloc coding buffer");
    }
    for (i = 0; i < EC_K; i++)
        data[i] = buffer + (i * CHUNK_SIZE);

    /* check file_size */
    char file_size_buffer[MAX_PATH_LEN] = {0};
    if (open_read_file(file_size_filename, "rb", file_size_buffer, sizeof(file_size_buffer)) == EC_ERROR)
        ERROR_RETURN_VALUE("open_read_file");
    if ((file_size = atoi(file_size_buffer)) != buffer_size) // check readsize
        ERROR_RETURN_VALUE("error readsize: different chunksize or ec_k+ec_m");

    if (nok < EC_K)
        ERROR_RETURN_VALUE("error num_nodes more than EC_M");
    
    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);
    perfData->readStart = startTime.tv_sec * 1000 + startTime.tv_usec / 1000;

    if ((read_k_chunks_storagenodes(data, dst_filename, sockfd_array, data_multiport_fd, ok_node_list, node_io)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail recv k chunks");
    
    gettimeofday(&endTime, NULL);
    perfData->readEnd = endTime.tv_sec * 1000 + endTime.tv_usec / 1000;
    
    /* repair data chunks */
    gettimeofday(&startTime, NULL);
    perfData->decodeStart = startTime.tv_sec * 1000 + startTime.tv_usec / 1000;
    if (repair_data_chunks(ok_node_list, nok, err_node_list, nerrs, data, coding, repair_data, CHUNK_SIZE) == EC_ERROR)
        ERROR_RETURN_VALUE("repair_data_chunks");
    gettimeofday(&endTime, NULL);
    perfData->decodeEnd = endTime.tv_sec * 1000 + endTime.tv_usec / 1000;

    /* clear file and write data */
    if (open_write_file_mul(src_filename, "wb", repair_data, CHUNK_SIZE, EC_K) == EC_ERROR)
        ERROR_RETURN_VALUE("open_write_file_mul");

    for (i = 0; i < EC_M; i++)
        free(coding[i]);
    free(repair_data);
    free(coding);
    free(data);
    free(buffer);

    printf("[ec_read end]\n");
    return EC_OK;
}

int ec_read_fr(int argc, char **argv, PerformanceData *perfData, double *node_io, int *sockfd_array, int *ok_node_list, int nok, int *err_node_list, int nerrs, int *data_multiport_fd, int *io_hetero)
{
    printf("[ec_read_fr begin]\n");
    int i, j; // loop control variables
    int p, q; // Number of groups p, number of blank chunks q

    /* File arguments */
    int file_size; // size of file
    char src_filename[MAX_PATH_LEN] = {0}, dst_filename[MAX_PATH_LEN] = {0}, file_size_filename[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(src_filename, "%s%s%s", ec_directory, READ_PATH, argv[2]);                 // get src_filename
    #if HETEROGENEOUS
    #if HETEROGENEOUS_REPAIR
    char read_opt_filename[MAX_PATH_LEN] = {0};
    sprintf(read_opt_filename, "%s%s%s", ec_directory, READ_PATH, "read_opt.py");
    #endif
    char group_strategy_dst_filename[MAX_PATH_LEN] = {0};
    sprintf(group_strategy_dst_filename, "%s%s", ec_directory, GROUP_STRATEGY_DST_PATH);       // get group_strategy_dst_filename
    #endif
    sprintf(dst_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);                // get dst_filename
    sprintf(file_size_filename, "%s%s%s", ec_directory, FILE_SIZE_PATH, argv[3]);      // get file_size_filename to save file size of src file
    // printf("src_filename = %s, dst_filename = %s, file_size_filename = %s\n", src_filename, dst_filename, file_size_filename);

    /* check file_size */
    char file_size_buffer[MAX_PATH_LEN] = {0};
    if (open_read_file(file_size_filename, "rb", file_size_buffer, sizeof(file_size_buffer)) == EC_ERROR)
        ERROR_RETURN_VALUE("open_read_file");
    if ((file_size = atoi(file_size_buffer)) != EC_K * CHUNK_SIZE) // check readsize
        ERROR_RETURN_VALUE("error readsize: different chunksize or ec_k+ec_m");

    if (nok < EC_K)
        ERROR_RETURN_VALUE("error num_nodes more than EC_M");
    
    /* Calculate parameters p and q */
    // if (nok - EC_K == 0) { // Check for division by zero
    //     q = 0;
    //     p = 1;
    // } else {
    //     if (nok % (nok - EC_K) == 0) { // Check if EC_K is divisible by (nok - EC_K)
    //         q = 1;
    //         p = nok / (nok - EC_K);
    //     } else { // Simplify the fraction
    //         int numerator = nok;
    //         int denominator = nok - EC_K;
    //         int commonDivisor = gcd(abs(numerator), abs(denominator));

    //         q = numerator / commonDivisor;
    //         p = denominator / commonDivisor;
    //     }
    // }

    p = nok;
    q = nok - EC_K;

    int *node_chunk_size = (int *)malloc(nok * sizeof(int));// Count the amount of data that each node needs to read

    int chunk_size1 = CHUNK_SIZE / p;
    int chunk_size2 = CHUNK_SIZE % p + chunk_size1;// The last group may have a different chunk size

    int *group_chunk_size = (int *)malloc(p * sizeof(int));

    #if HETEROGENEOUS
    // 67108864
    // 9
    // 6
    // 1 1 1 1 1 1 1 1 1

    // 6 5.5 6 6 6 6 6 6.5 6 
    // 1 2 3 1 2 3 1 2 3
    // 1 2 3 4 5 6 7 8 9
    // python3 /home/ych/ec_test/test_file/read/read_opt.py /home/ych/ec_test/test_file/read/ec_heterogeneous_read_dst.txt 67108864 9 6 1 1 1 1 1 1 1 1 1
    #if HETEROGENEOUS_REPAIR
    // construct command
    char command[512] = {0};
    sprintf(command, "python3 %s %s %d %d %d", read_opt_filename, group_strategy_dst_filename, STRIPE_DATA_SIZE, nok, EC_K);
    for (i = 0; i < nok; i++) {
        sprintf(command, "%s %d", command, io_hetero[i]);
    }
    #if DEBUGGING_COMMENTS
    printf("command = %s\n", command);
    #endif
    // execute command
    if (system(command) == -1) {
        ERROR_RETURN_VALUE("Fail system command");
    }
    #endif
    if (read_group_strategy(group_strategy_dst_filename, group_chunk_size, nok) == EC_ERROR)
        ERROR_RETURN_VALUE("read_group_strategy");
    #else
    for (i = 0; i < p; i++)
        if (i < p - 1) {
            group_chunk_size[i] = chunk_size1;
        } else {
            group_chunk_size[i] = chunk_size2;
        }
    #endif

    /* For each group, count its node_ok and "node_error" */
    int **ok_node_group = (int **)malloc(p * sizeof(int *));
    int **err_node_tag = (int **)malloc(p * sizeof(int *));
    int **err_node_group = (int **)malloc(p * sizeof(int *));
    for (int i = 0; i < p; i++) {
        ok_node_group[i] = (int *)malloc(EC_N * sizeof(int));
        err_node_tag[i] = (int *)calloc(EC_N, sizeof(int));
        err_node_group[i] = (int *)malloc(EC_M * sizeof(int));
    }

    for (int i = 0; i < nerrs; i++) {
        for (int j = 0; j < p; j++)
            err_node_tag[j][err_node_list[i]] = 1;
    }

    /* Simulate the allocation process */
    int **A = (int **)malloc(p * sizeof(int *));
    for (int i = 0; i < p; i++) {
        A[i] = (int *)calloc(nok, sizeof(int));// Allocate memory for each row of matrix A and initialize all elements to 0
    }

    BElement **B = (BElement **)malloc((p - q) * sizeof(BElement *));
    for (int i = 0; i < p - q; i++) {
        B[i] = (BElement *)malloc(nok * sizeof(BElement));
    }

    for (int i = 0; i < nok; i++) {
        for (int j = 0; j < q; j++) {
            int row = (i + j) % p;
            A[row][i] = 1;
        }
    }

    int zeroCount[p];
    for (int i = 0; i < p; i++) {
    zeroCount[i] = 0;
    }

    for (int i = 0; i < nok; i++) {
        int bRow = 0;
        int start = 0;
        for (int j = 0; j < p; j++) {
            if (!A[j][i]) {
                B[bRow][i].rowNumber = j;
                B[bRow][i].chunkSize = group_chunk_size[j]; // (j == p - 1) ? chunk_size2 : chunk_size1;
                B[bRow][i].columnNumber = zeroCount[j];
                B[bRow][i].start = start;
                ok_node_group[j][zeroCount[j]] = ok_node_list[i];
                zeroCount[j]++;
                bRow++;
            }
            else {
                err_node_tag[j][ok_node_list[i]] = 1;
            }
            start += group_chunk_size[j];
        }
    }

    #if DEBUGGING_COMMENTS
    // Print err_node_tag
    printf("err_node_tag: \n");
    for (int i = 0; i < p; i++) {
        for (int j = 0; j < EC_N; j++) {
            printf("%d ", err_node_tag[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    #endif

    for (int i = 0; i < p; i++) {
        int errCount = 0;
        for (int j = 0; j < EC_N; j++) {
            if (err_node_tag[i][j]) {
                err_node_group[i][errCount] = j;
                errCount++;
            }
        }
    }

    #if DEBUGGING_COMMENTS
    printf("ok_node_group: \n");
    for (int i=0;i<p;i++){
        for(int j=0;j<EC_K;j++){
            printf("%d ",ok_node_group[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("err_node_group: \n");
    for (int i=0;i<p;i++){
        for(int j=0;j<EC_M;j++){
            printf("%d ",err_node_group[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    #endif

    CElement *C = (CElement *)malloc(nok * sizeof(CElement));// Optimization of continuous reading
    for (int i = 0; i < nok; i++) {
        C[i].part1.start = B[0][i].start;
        C[i].part1.end = C[i].part1.start + B[0][i].chunkSize;
        C[i].part2.start = 0;
        C[i].part2.end = 0;

        int lastRow = B[0][i].rowNumber;
        for (int j = 1; j < p - q; j++) {
            if (B[j][i].rowNumber == lastRow + 1) {
                C[i].part1.end += B[j][i].chunkSize;
                lastRow = B[j][i].rowNumber;
            } else {
                C[i].part2.start = B[j][i].start;
                C[i].part2.end = CHUNK_SIZE;
                break;
            }
        }
        node_chunk_size[i] = C[i].part1.end - C[i].part1.start + C[i].part2.end - C[i].part2.start;
    }

    #if DEBUGGING_COMMENTS
    // Print Matrix A
    printf("Matrix A:\n");
    for (int i = 0; i < p; i++) {
        for (int j = 0; j < nok; j++) {
            printf("%d ", A[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    // Print Matrix B and Array C
    printf("Matrix B:\n");
    for (int i = 0; i < p - q; i++) {
        for (int j = 0; j < nok; j++) {
            printf("[(%d, %d, %d)] ", B[i][j].rowNumber, B[i][j].chunkSize, B[i][j].columnNumber);
        }
        printf("\n");
    }
    printf("\n");

    printf("Array C:\n");
    for (int i = 0; i < nok; i++) {
        printf("[(%d, %d), (%d, %d)]\n",
               C[i].part1.start, C[i].part1.end, 
               C[i].part2.start, C[i].part2.end);
    }
    printf("\n");
    #endif
    

    /* ISA-L arguments */
    char **data_pre = (char **)malloc(sizeof(char*)*nok);
    if (data_pre == NULL)
        ERROR_RETURN_VALUE("Fail malloc data_pre");
    char ***data = (char ***)malloc(sizeof(char**)*p);
    if (data == NULL)
        ERROR_RETURN_VALUE("Fail malloc data");
    char ***coding = (char ***)malloc(sizeof(char**)*p);
    if (coding == NULL)
        ERROR_RETURN_VALUE("Fail malloc coding");
    char ***repair_data = (char ***)malloc(sizeof(char**)*p);
    if (repair_data == NULL)
        ERROR_RETURN_VALUE("Fail malloc repair_data");

    /* Count the size of blocks in each group and assign pointers to each group */
    for (i = 0; i < p; i++) {
        // printf("group_chunk_size[%d] = %d\n", i, group_chunk_size[i]);

        data[i] = (char **)malloc(sizeof(char *) * EC_K);
        coding[i] = (char **)malloc(sizeof(char *) * EC_M);
        repair_data[i] = (char **)malloc(sizeof(char *) * EC_K);

        for(j = 0; j < EC_M; j++) {
            coding[i][j] = (char *)malloc(sizeof(char) * group_chunk_size[i]);
            if (coding[i][j] == NULL)
                ERROR_RETURN_VALUE("Fail malloc coding buffer");
        }
    }

    #if DEBUGGING_COMMENTS
    // Print group_chunk_size
    printf("group_chunk_size: \n");
    for (i = 0; i < p; i++) {
        printf("%d ", group_chunk_size[i]);
    }
    printf("\n\n");

    // Print coding addresses
    printf("coding addresses: \n");
    for (i = 0; i < p; i++) {
        for (j = 0; j < EC_M; j++) {
            printf("%p ", coding[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    // Print node_chunk_size
    printf("node_chunk_size: \n");
    for (i = 0; i < nok; i++) {
        printf("%d ", node_chunk_size[i]);
    }
    printf("\n\n");
    #endif

    /* Assign a buffer to each node and use a data pointer array to partition the buffer */
    for(i = 0; i < nok; i++) {
        data_pre[i] = (char *)malloc(sizeof(char) * node_chunk_size[i]);
        if (data_pre[i] == NULL)
            ERROR_RETURN_VALUE("Fail malloc data_pre_i");
        int count_size = 0;
        for (j = 0; j < p -q; j++) {
            data[B[j][i].rowNumber][B[j][i].columnNumber] = data_pre[i] + count_size;
            count_size += B[j][i].chunkSize;
            if (data[B[j][i].rowNumber][B[j][i].columnNumber] == NULL)
                ERROR_RETURN_VALUE("Fail malloc data_i");
        }
    }

    #if DEBUGGING_COMMENTS
    // Print data_pre addresses
    printf("data_pre addresses: \n");
    for (i = 0; i < nok; i++) {
        printf("%p ", data_pre[i]);
    }
    printf("\n\n");

    // Print data addresses
    printf("data addresses: \n");
    for (i = 0; i < p; i++) {
        for (j = 0; j < EC_K; j++) {
            printf("%p ", data[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    // check data max
    printf("data[0][0][group_chunk_size[0] - 1] = %d\n", data[0][0][group_chunk_size[0] - 1]);
    // printf("data[1][0][group_chunk_size[1] - 1] = %d\n", data[1][0][group_chunk_size[1] - 1]);
    // printf("data[2][0][group_chunk_size[2] - 1] = %d\n", data[2][0][group_chunk_size[2] - 1]);
    // printf("\n");
    #endif

    struct timeval startTime, endTime;
    gettimeofday(&startTime, NULL);
    perfData->readStart = startTime.tv_sec * 1000 + startTime.tv_usec / 1000;

    if ((read_k_chunks_storagenodes_fr(data_pre, dst_filename, sockfd_array, data_multiport_fd, ok_node_list, nok, C, node_chunk_size, node_io)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail recv k chunks");
    
    gettimeofday(&endTime, NULL);
    perfData->readEnd = endTime.tv_sec * 1000 + endTime.tv_usec / 1000;

    gettimeofday(&startTime, NULL);
    perfData->decodeStart = startTime.tv_sec * 1000 + startTime.tv_usec / 1000;
    pthread_t tid[p];
    RepairArg *repair_data_thread_args = (RepairArg *)malloc(p * sizeof(RepairArg));

    for (i = 0; i < p; i++) {
        repair_data_thread_args[i].ok_node_list = ok_node_group[i];
        repair_data_thread_args[i].nok = EC_K;
        repair_data_thread_args[i].err_node_list = err_node_group[i];
        repair_data_thread_args[i].nerrs = EC_M;
        repair_data_thread_args[i].data = data[i];
        repair_data_thread_args[i].coding = coding[i];
        repair_data_thread_args[i].repair_data = repair_data[i];
        repair_data_thread_args[i].chunk_size = group_chunk_size[i];

        if (pthread_create(&tid[i], NULL, repair_data_chunks_thread, (void *)&repair_data_thread_args[i]) != 0)
            ERROR_RETURN_VALUE("Fail create thread"); 
    }

    for (i = 0; i < p; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    gettimeofday(&endTime, NULL);
    perfData->decodeEnd = endTime.tv_sec * 1000 + endTime.tv_usec / 1000;
    
    #if DEBUGGING_COMMENTS
    // Print repair_data addresses
    printf("repair_data addresses: \n");
    for (i = 0; i < p; i++) {
        for (j = 0; j < EC_K; j++) 
            printf("%p ", repair_data[i][j]);
        printf("\n");
    }
    printf("\n");
    #endif


    /* clear file and write data */
    if (open_write_file_fr(src_filename, "wb", repair_data, group_chunk_size, p, EC_K) == EC_ERROR)
        ERROR_RETURN_VALUE("open_write_file_fr");

    free(repair_data_thread_args);
    for(i = 0; i < nok; i++) {
        free(data_pre[i]);
    }
    for (i = 0; i < p; i++) {
        for (j = 0; j < EC_M; j++) {
            free(coding[i][j]);
        }
        free(repair_data[i]);
        free(coding[i]);
        free(data[i]);
    }
    free(repair_data);
    free(coding);
    free(data);
    free(data_pre);
    free(C);
    for (int i = 0; i < p - q; i++) {
        free(B[i]);
    }
    free(B);
    for (int i = 0; i < p; i++) {
        free(A[i]);
    }
    free(A);
    for (int i = 0; i < p; i++) {
        free(err_node_group[i]);
        free(err_node_tag[i]);
        free(ok_node_group[i]);
    }
    free(err_node_group);
    free(err_node_tag);
    free(ok_node_group);
    free(group_chunk_size);
    free(node_chunk_size);

    printf("[ec_read_fr end]\n");
    return EC_OK;
}

int ec_write(int argc, char **argv, int *sockfd_array)
{
    printf("[ec_write begin]\n");

    int i; // loop control variables

    /* File arguments */
    int file_size;           // size of file
    struct stat file_status; // finding file size
    char src_filename[MAX_PATH_LEN] = {0}, dst_filename[MAX_PATH_LEN] = {0}, file_size_filename[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(src_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[2]);                // get src_filename
    sprintf(dst_filename, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);                // get dst_filename
    sprintf(file_size_filename, "%s%s%s", ec_directory, FILE_SIZE_PATH, argv[3]);      // get file_size_filename to save file size of src file
    // printf("src_filename = %s, dst_filename = %s, file_size_filename = %s\n", src_filename, dst_filename, file_size_filename);

    /* EC arguments */
    int buffer_size = EC_K * CHUNK_SIZE;                       // stripe size
    char *buffer = (char *)malloc(sizeof(char) * buffer_size); // buffer for EC stripe
    if (buffer == NULL)
        ERROR_RETURN_VALUE("Fail malloc data buffer");

    /* ISA-L arguments */
    int *matrix = NULL;                                     // coding matrix
    char **data = (char **)malloc(sizeof(char *) * EC_K);   // data chunk s
    char **coding = (char **)malloc(sizeof(char *) * EC_M); // coding chunks

    /* initialize data buffer and coding buffer */
    for (i = 0; i < EC_M; i++)
    {
        coding[i] = (char *)malloc(sizeof(char) * CHUNK_SIZE);
        if (coding[i] == NULL)
            ERROR_RETURN_VALUE("Fail malloc coding buffer");
    }
    for (i = 0; i < EC_K; i++)
        data[i] = buffer + (i * CHUNK_SIZE);

    /*One IO read file to buffer*/
    if ((file_size = get_size_file(src_filename)) != buffer_size)
        ERROR_RETURN_VALUE("file_size != EC_K * CHUNK_SIZE");
    if (read_file_to_buffer(src_filename, buffer, buffer_size) == EC_ERROR)
        ERROR_RETURN_VALUE("error read_file_to_buffer");

    /* Encode */
    encode_data_chunks(data, coding);

    /* write chunks to each storages */
    // int sockfd_array[EC_N];
    // for (i = 0; i < EC_N; i++)
    //     if ((initialize_network(&sockfd_array[i], EC_CLIENT_STORAGENODES_PORT, i)) == EC_ERROR) // only the func close socket
    //         ERROR_RETURN_VALUE("Fail initialize network");
    if ((write_chunks_storagenodes(data, coding, dst_filename, sockfd_array)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail send chunks to storagenodes");
    // for (i = 0; i < EC_N; i++)
    //     shutdown(sockfd_array[i], SHUT_RDWR);

    /* write file size to file_size filename */
    char file_size_buffer[MAX_PATH_LEN] = {0};
    sprintf(file_size_buffer, "%d", file_size);
    if (open_write_file(file_size_filename, "wb", file_size_buffer, sizeof(file_size_buffer)) == EC_ERROR)
        ERROR_RETURN_VALUE("open_write_file");

    for (i = 0; i < EC_M; i++)
        free(coding[i]);
    free(coding);
    free(data);
    free(buffer);

    printf("[ec_write end]\n");
    return EC_OK;
}

int ec_test_tra_repair(int argc, char **argv, int *sockfd_array)
{
    printf("[ec_test_tra_repair begin]\n");
    int i; // loop control variables

    /* File arguments */
    int file_size; // size of file
    char dst_filename_failure[MAX_PATH_LEN] = {0}, dst_filename_healthy[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - strlen(SCRIPT_PATH)); // -6 to sub script/
    sprintf(dst_filename_failure, "%s%s%s", ec_directory, REPAIR_PATH, argv[2]);       // get dst_filename_failure
    sprintf(dst_filename_healthy, "%s%s%s", ec_directory, WRITE_PATH, argv[3]);        // get dst_filename_healthy
    // printf("dst_filename_healthy = %s, dst_filename_failure = %s\n", dst_filename_healthy, dst_filename_failure);

    int storagenodes_fail = STORAGENODE_FAIL; // 1 failure storagenodes

    /* request each storagenodes for 1 failure storagenodes and k healthy storagenodes */
    // int sockfd_array[EC_N];
    // for (i = 0; i < EC_K + 1; i++)
    //     if ((initialize_network(&sockfd_array[i], EC_CLIENT_STORAGENODES_PORT, i)) == EC_ERROR) // only the func close socket
    //         ERROR_RETURN_VALUE("Fail initialize network");
    if ((request_tra_repair(storagenodes_fail, dst_filename_healthy, dst_filename_failure, sockfd_array)) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail send request_tra_repair to storagenodes");
    // for (i = 0; i < EC_K + 1; i++)
    //     shutdown(sockfd_array[i], SHUT_RDWR);

    printf("[ec_test_tra_repair end]\n");
    return EC_OK;
}
int ec_test_piv_repair(int argc, char **argv)
{
    printf("[ec_test_piv_repair begin]\n");

    printf("[ec_test_piv_repair end]\n");
    return EC_OK;
}
int ec_test_ppt_repair(int argc, char **argv)
{
    printf("[ec_test_ppt_repair begin]\n");

    printf("[ec_test_ppt_repair end]\n");
    return EC_OK;
}
int ec_test_rp_repair(int argc, char **argv)
{
    printf("[ec_test_rp_repair begin]\n");

    int i; // loop control variables

    int uplink_bandwidth[EC_N_MAX], downlink_bandwidth[EC_N_MAX], light_heavy_flag = 1;
    int **nodes_bandwidth = (int **)malloc(EC_N * sizeof(int *));
    int *nodes_bandwidth_temp = (int *)malloc(EC_N * EC_N * sizeof(int));
    for (i = 0; i < EC_N; i++)
        nodes_bandwidth[i] = nodes_bandwidth_temp + (i * EC_N);

    if (read_updown_bandwidth(uplink_bandwidth, downlink_bandwidth, light_heavy_flag, BAND_LOCATION) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail read_updown_bandwidth");
    get_bandwidth_between_nodes(uplink_bandwidth, downlink_bandwidth, nodes_bandwidth);

    /* Code Need */

    free(nodes_bandwidth_temp);
    free(nodes_bandwidth);

    printf("[ec_test_ppt_repair end]\n");
    return EC_OK;
}
int ec_test_new_repair(int argc, char **argv)
{
    printf("[ec_test_new_repair begin]\n");

    printf("[ec_test_new_repair end]\n");
    return EC_OK;
}

void help(int argc, char **argv)
{
    fprintf(stderr, "Usage: %s cmd...\n"
                    "\t -h \n"
                    "\t -w <src_filename> <dst_filename> \n"
                    "\t -r <src_filename> <dst_filename> \n"
                    "\t -fr <src_filename> <dst_filename> \n"
                    "\t -kw <src_filename> <dst_filename> \n"
                    "\t  \n"
                    "\t Example1: -w src_10MB.mp4 dst_10MB.mp4 \n"
                    "\t <src_filename> saved on ec_test/test_file/write/ for client\n"
                    "\t <dst_filename> saved on ec_test/test_file/write/ for storages \n"
                    "\t Example2: -r read_10MB.mp4 dst_10MB.mp4  \n"
                    "\t <src_filename> saved on ec_test/test_file/read/ for storages\n"
                    "\t <dst_filename> saved on ec_test/test_file/write/ for client \n"
                    "\t Tip: saved filename on storages actually is dst_filenameX_Y, \n"
                    "\t the X is Xth stripe and the Y is Yth chunk.\n"
                    "\t  \n",
            argv[0]);
}

int main(int argc, char *argv[]) // cur_dir is script
{
    printf("[ec_client begin]\n");

    PerformanceData perfData = {0};

    int nok = EC_N;
    int ok_node_list[EC_N] = {0};
    int nerrs = 0;
    int err_node_list[EC_N] = {0};

    int client_fd[EC_N];
    int data_multiport_fd[NUMBER_OF_PORTS * EC_N];
    
    int io_hetero[EC_N];
    for (int i = 0; i < EC_N; i++) {
        io_hetero[i] = 1;
    }
    double node_io[EC_N];
    for (int j = 0; j < EC_N; j++) 
            node_io[j] = 0;

    /* Commands*/
    if (argc < 2)
    {
        help(argc, argv);
        return EC_OK;
    }
    char cmd[64] = {0};
    strcpy(cmd, argv[1]);

    if (0 == strncmp(cmd, "-h", strlen("-h"))) // cmd -h: help information
    {
        help(argc, argv);
        return EC_OK;
    }
    else if (0 == strncmp(cmd, "-w", strlen("-w"))) // cmd -w: erasure coding write
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");

        if (ec_write(argc, argv, client_fd) == EC_ERROR)
            printf("Fail ec_write\n");
        
        double network_io_time = perfData.readEnd - perfData.readStart;
        double decodetime = perfData.decodeEnd - perfData.decodeStart;
        printf("Network and I/O time: %.3f ms,\nDecode time: %.3f ms.\n", network_io_time, decodetime);

        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-r", strlen("-r"))) // cmd -r: erasure coding read
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");

        if (ec_read(argc, argv, &perfData, node_io, client_fd, ok_node_list, nok, err_node_list, nerrs, data_multiport_fd) == EC_ERROR)
            printf("Fail ec_read\n");
        
        for (int j = 0; j < nok; j++) {
            printf("Node %d I/O time: %.3f ms\n", ok_node_list[j], node_io[j]);
        }

        double network_io_time = perfData.readEnd - perfData.readStart;
        double decodetime = perfData.decodeEnd - perfData.decodeStart;
        printf("Network and I/O time: %.3f ms,\nDecode time: %.3f ms.\n", network_io_time, decodetime);

        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-fr", strlen("-fr"))) // cmd -fr: erasure coding fast read
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");
        
        for (int j = 0; j < nok; j++) {
            printf("Node %d I/O time: %.3f ms\n", ok_node_list[j], node_io[j]);
        }

        if (ec_read_fr(argc, argv, &perfData, node_io, client_fd, ok_node_list, nok, err_node_list, nerrs, data_multiport_fd, io_hetero) == EC_ERROR)
            printf("Fail ec_read_fr\n");
        
        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-trtra", strlen("-trtra"))) // cmd -trtra: erasure coding repair test: traditional
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");
        
        if (ec_test_tra_repair(argc, argv, client_fd) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
        
        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-trpiv", strlen("-trpiv"))) // cmd -trpiv: erasure coding repair test: pivot
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");

        if (ec_test_tra_repair(argc, argv, client_fd) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
        
        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-trppt", strlen("-trppt"))) // cmd -trppt: erasure coding repair test: ppt
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");

        if (ec_test_tra_repair(argc, argv, client_fd) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
        
        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-trrp", strlen("-trrp"))) // cmd -trrp: eerasure coding repair test: repair pipeline
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");

        if (ec_test_tra_repair(argc, argv, client_fd) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");

        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else if (0 == strncmp(cmd, "-trnew", strlen("-trnew"))) // cmd -trnew: erasure coding repair test: new method
    {
        if (client_init_all_node_port(client_fd, data_multiport_fd, &nok, ok_node_list, &nerrs, err_node_list) == EC_ERROR)
            ERROR_RETURN_VALUE("client_init_all_node_port");

        if (ec_test_tra_repair(argc, argv, client_fd) == EC_ERROR)
            printf("Fail ec_test_tra_repair\n");
        
        client_close_all_node_port(client_fd, data_multiport_fd, nok, ok_node_list);
    }
    else
    {
        printf("Error operation, please read help\n");
        help(argc, argv);
    }

    return EC_OK;
}
