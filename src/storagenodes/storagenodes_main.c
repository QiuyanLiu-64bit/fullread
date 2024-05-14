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

int recv_k_chunks_storagenodes(char **data, int server_fd)
{
    int i;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pthread_t tid[EC_K];

    metadata_t *metadata_array = (metadata_t *)malloc(sizeof(metadata_t) * EC_N); // only the func free
    /* Recv to k storages that saved data chunk */
    for (i = 0; i < EC_K; i++)
        metadata_array[i].data = data[i];

    /* wait for storagenodes connection */
    for (i = 0; i < EC_K; i++)
    {
        /* accept storagenodes */
        if ((metadata_array[i].sockfd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1)
        {
            printf("Fail accept socket\n");
            continue;
        }
        /* create thread to handle storagenodes */
        if (pthread_create(&tid[i], NULL, recv_chunk, (void *)&metadata_array[i]) != 0)
        {
            printf("Fail create thread\n");
            close(metadata_array[i].sockfd);
            continue;
        }
    }
    for (i = 0; i < EC_K; i++)
        if (pthread_join(tid[i], NULL) != 0)
            ERROR_RETURN_VALUE("Fail join thread");
    free(metadata_array);
    return EC_OK;
}

void *handle_client_tra_repair_healthy(metadata_t *metadata, int client_fd, int *data_multiport_fd)
{
    printf("[handle_client_tra_repair_healthy begin]\n");

    Send_Response(client_fd);
    printf("end request_tra_repair from client...\n");

    /* read chunk from disk */
    char *buffer_chunk = (char *)malloc(sizeof(char) * CHUNK_SIZE); // buffer for EC chunk
    if (open_read_file(metadata->dst_filename_storagenodes, "rb", buffer_chunk, CHUNK_SIZE) == EC_ERROR)
        ERROR_RETURN_NULL("open_read_file");

    /* send chunks to new storagenodes */
    int sockfd_server;
    if ((initialize_network(&sockfd_server, EC_STORAGENODES_PORT, STORAGENODE_FAIL)) == EC_ERROR) // only the func close socket
        ERROR_RETURN_NULL("Fail initialize network");
    Send(sockfd_server, buffer_chunk, CHUNK_SIZE, "send chunk");
    Recv_Response(sockfd_server);
    free(buffer_chunk);
    shutdown(sockfd_server, SHUT_RDWR);

    printf("[handle_client_tra_repair_healthy end]\n");
    return NULL;
}

void *handle_client_tra_repair_fail(metadata_t *metadata, int client_fd, int *data_multiport_fd)
{
    printf("[handle_client_tra_repair_fail begin]\n");
    Send_Response(client_fd);
    printf("end request_tra_repair from client...\n");

    int i, j; // loop control variables

    /* EC arguments */
    int buffer_size = EC_K * CHUNK_SIZE;                       // stripe size
    char *buffer = (char *)malloc(sizeof(char) * buffer_size); // buffer for EC stripe
    if (buffer == NULL)
        ERROR_RETURN_NULL("Fail malloc data buffer");

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
            ERROR_RETURN_NULL("Fail malloc coding buffer");
    }
    for (i = 0; i < EC_K; i++)
        data[i] = buffer + (i * CHUNK_SIZE);

    /* repair arguments */
    int err_node_list[EC_N]; // location of nodes_error
    int ok_node_list[EC_N];  // location of nodes_ok
    int nerrs = 0;           // number of nodes_error
    int nok = 0;             // number of nodes_ok

    /* get number&location of nodes_error and nodes_ok */
    for (i = 0; i < EC_N; i++)
        if (i == STORAGENODE_FAIL || i >= EC_K + 1)
            err_node_list[nerrs++] = i;
        else
            ok_node_list[nok++] = i;
    if (nok < EC_K)
        ERROR_RETURN_NULL("error num_nodes more than EC_M");

    /* recv chunk from k storagenodes */
    int server_fd;                                                                 // server socket
    if ((server_initialize_network(&server_fd, EC_STORAGENODES_PORT)) == EC_ERROR) //  initialize server network
        ERROR_RETURN_NULL("error server_initialize_network");
    if ((recv_k_chunks_storagenodes(data, server_fd)) == EC_ERROR)
        ERROR_RETURN_NULL("Fail recv k chunks");
    shutdown(server_fd, SHUT_RDWR);

    /* repair data chunks */
    if (repair_data_chunks(ok_node_list, nok, err_node_list, nerrs, data, coding, repair_data, CHUNK_SIZE) == EC_ERROR)
        ERROR_RETURN_NULL("repair_data_chunks");

    if (open_write_file_mul(metadata->dst_filename_storagenodes, "wb", repair_data, CHUNK_SIZE, EC_K) == EC_ERROR)
        ERROR_RETURN_NULL("open_write_file_mul");

    for (i = 0; i < EC_M; i++)
        free(coding[i]);
    free(repair_data);
    free(coding);
    free(data);
    free(buffer);

    printf("[handle_client_tra_repair_fail end]\n");
    return NULL;
}

void *handle_ec_read(metadata_t *metadata, int client_fd, int *data_multiport_fd)
{
    printf("[handle_ec_read begin]\n");

    char *buffer_chunk = (char *)malloc(sizeof(char) * CHUNK_SIZE); // buffer for EC chunk
    /* file IO */
    struct timeval start, end;
    gettimeofday(&start, NULL);

    if (open_read_file(metadata->dst_filename_storagenodes, "rb", buffer_chunk, CHUNK_SIZE) == EC_ERROR)
        ERROR_RETURN_NULL("open_read_file");
    
    gettimeofday(&end, NULL);

    /* send chunk data to client*/
    server_send_multiport(buffer_chunk, CHUNK_SIZE, data_multiport_fd);

    double time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    // send time to client
    Send(client_fd, &time, sizeof(double), "send io time");

    Recv_Response(client_fd);

    free(buffer_chunk);
    printf("[handle_ec_read end]\n");
    return NULL;
}

void *handle_ec_write(metadata_t *metadata, int client_fd, int *data_multiport_fd)
{
    printf("[handle_ec_write begin]\n");

    /* recv chunk data and send response */
    char *buffer_chunk = (char *)malloc(sizeof(char) * CHUNK_SIZE); // buffer for EC chunk
    Recv(client_fd, buffer_chunk, CHUNK_SIZE, "recv chunk");
    Send_Response(client_fd);

    /* file IO */
    if (open_write_file(metadata->dst_filename_storagenodes, "wb", buffer_chunk, CHUNK_SIZE) == EC_ERROR)
        ERROR_RETURN_NULL("open_write_file");
    free(buffer_chunk);

    printf("[handle_ec_write end]\n");
    return NULL;
}

void *handle_ec_read_fr(metadata_t *metadata, int client_fd, int *data_multiport_fd)
{
    printf("[handle_ec_read_fr begin]\n");

    char *buffer_chunk = (char *)malloc(sizeof(char) * metadata->block_size); // buffer for EC chunk
    /* file IO */
    struct timeval start, end;
    gettimeofday(&start, NULL);

    if (open_read_file_fr(metadata->dst_filename_storagenodes, "rb", buffer_chunk, metadata->block_size, metadata->ce) == EC_ERROR)
        ERROR_RETURN_NULL("open_read_file");
    
    gettimeofday(&end, NULL);

    /* send chunk data to client*/
    server_send_multiport(buffer_chunk, metadata->block_size, data_multiport_fd);

    double time = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    // send time to client
    Send(client_fd, &time, sizeof(double), "send io time");

    Recv_Response(client_fd);

    free(buffer_chunk);
    printf("[handle_ec_read_fr end]\n");
    return NULL;
}

void *handle_client(int client_fd, int *data_multiport_fd)
{
    while(1){
        printf("[ec_storagenodes begin]\n");

        // system("sync; echo 3 > /proc/sys/vm/drop_caches");// clear cache
        if (system("sync; echo 3 > /proc/sys/vm/drop_caches") != 0)
            ERROR_RETURN_NULL("Fail clear cache");

        metadata_t *metadata = (metadata_t *)malloc(sizeof(metadata_t)); // only the func free

        if (metadata == NULL)
            ERROR_RETURN_NULL("Fail malloc metadata");

        /* recv metadata and send response */
        // Recv(client_fd, metadata, sizeof(metadata_t), "recv metadata");
        if (recv(client_fd, metadata, sizeof(metadata_t), MSG_WAITALL) <= 0)
            ERROR_RETURN_NULL("recv metadata");

        type_handle_func handle_func[] = {handle_ec_write, handle_ec_read, handle_client_tra_repair_fail, handle_client_tra_repair_healthy, handle_ec_read_fr};
        handle_func[metadata->opt_type](metadata, client_fd, data_multiport_fd);
        free(metadata);

        printf("[ec_storagenodes end]\n");

        PRINT_FLUSH;
    }
    return NULL;
}

int main()
{

    printf("[ec_storagenodes begin]\n");
    PRINT_FLUSH;

    int data_multiport_fd[NUMBER_OF_PORTS]; // server socket for data multiport
    int client_fd; // client socket

    /* initialize server network */
    if (server_init_all_node_port(&client_fd, data_multiport_fd) == EC_ERROR)
        ERROR_RETURN_VALUE("Fail server_init_all_node_port");

    handle_client(client_fd, data_multiport_fd);

    PRINT_FLUSH;

    printf("[ec_storagenodes end]\n");
    
    return EC_OK;
}