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

int initialize_network(int *sockfd_p, int port, int ip_offset)
{
    struct sockaddr_in server_addr;
    int ip_addr_start = STORAGENODES_START_IP_ADDR;
    int sockfd;

    /* Create socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR_RETURN_VALUE("Fail create socket");

    /* Create sockaddr */
    char ip_addr[16];
    sprintf(ip_addr, "%s%d", IP_PREFIX, ip_addr_start + ip_offset);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_addr, &server_addr.sin_addr) <= 0)
        ERROR_RETURN_VALUE("Invalid IP address");

    /* Connect data datanode of ip_addr */
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Error connecting to %s\n", ip_addr);
        return EC_ERROR;
    }
    *sockfd_p = sockfd;
    return EC_OK;
}

int server_initialize_network(int *server_fd_p, int port)
{
    int server_fd;
    struct sockaddr_in server_addr;

    /* create socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        ERROR_RETURN_VALUE(" Fail create server socket");

    /* can reuse */
    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    /* bind address and port */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        ERROR_RETURN_VALUE("Fail bind socket");

    /* listen socket */
    if (listen(server_fd, EC_N) == -1)
        ERROR_RETURN_VALUE("Fail listen socket");

    *server_fd_p = server_fd;
    return EC_OK;
}

int get_local_ip_lastnum(int *lastnum_p)
{
    struct ifaddrs *ifAddrStruct = NULL;
    struct ifaddrs *ifa = NULL;
    void *tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr->sa_family == AF_INET)
        { // IPv4 address
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (strncmp(addressBuffer, IP_PREFIX, 10) == 0)
            {
                char *lastNumStr = strrchr(addressBuffer, '.') + 1;
                *lastnum_p = atoi(lastNumStr);
                return EC_OK;
            }
        }
    }

    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);
    return EC_ERROR;
}

void *Send(int sockfd, const void *buf, size_t len, const char *str)
{
    if (send(sockfd, buf, len, 0) < 0)
        ERROR_RETURN_NULL(str);
    return NULL;
}
void *Recv(int sockfd, void *buf, size_t len, char *str)
{
    if (recv(sockfd, buf, len, MSG_WAITALL) < 0)
        ERROR_RETURN_NULL(str);
    return NULL;
}

void *Send_Response(int sockfd)
{
    int error_response = 1;
    if (send(sockfd, &error_response, sizeof(error_response), 0) < 0)
        ERROR_RETURN_NULL("send response");
    return NULL;
}
void *Recv_Response(int sockfd)
{
    int error_response = 0;
    if (recv(sockfd, &error_response, sizeof(error_response), MSG_WAITALL) < 0)
        ERROR_RETURN_NULL("recv response case 1");
    if (error_response == 0)
        ERROR_RETURN_NULL("recv response case 2");
    return NULL;
}

int client_init_all_node_port(int *client_fd, int *data_multiport_fd, int *nok, int *ok_node_list, int *nerrs, int *err_node_list){
    int server_multiport_fd[NUMBER_OF_PORTS * EC_N];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    *nok = 0;
    *nerrs = 0;
    for (int i = 0; i < EC_N; i++) {
        ok_node_list[i] = -1;
        err_node_list[i] = -1;
    }

    for (int i = 0; i < EC_N; i++) {
        if (initialize_network(&client_fd[i], EC_CLIENT_STORAGENODES_PORT, i) == EC_ERROR) {
            err_node_list[*nerrs] = i;
            (*nerrs)++;
            printf("Error connecting to node %d\n", i);
            continue;
        } else {
            ok_node_list[*nok] = i;
            (*nok)++;
        }

        for (int j = 0; j < NUMBER_OF_PORTS; j++) {
            printf("Client listening on port %d\n", PORT_START + i * NUMBER_OF_PORTS + j);
            if (server_initialize_network(&server_multiport_fd[i * NUMBER_OF_PORTS + j], PORT_START + i * NUMBER_OF_PORTS + j) == EC_ERROR) {
                ERROR_RETURN_VALUE("initialize_multiport");
            }
            Send(client_fd[i], &i, sizeof(i), "send port");
            data_multiport_fd[i * NUMBER_OF_PORTS + j] = accept(server_multiport_fd[i * NUMBER_OF_PORTS + j], (struct sockaddr *)&client_addr, &client_addr_len);
            close(server_multiport_fd[i * NUMBER_OF_PORTS + j]);
        }
    }

    return EC_OK;
}


int server_init_all_node_port(int *server_fd, int *data_multiport_fd){
    int server_multiport_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    if (server_initialize_network(&server_multiport_fd, EC_CLIENT_STORAGENODES_PORT) == EC_ERROR)
        ERROR_RETURN_VALUE("server_initialize_network");
    *server_fd = accept(server_multiport_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    close(server_multiport_fd);

    printf("Server port %d connected\n", EC_CLIENT_STORAGENODES_PORT);
    
    int node_i;
    for (int j = 0; j < NUMBER_OF_PORTS; j++){
        Recv(*server_fd, &node_i, sizeof(node_i), "recv port");
        // sleep(1); // for test
        printf("Server connecting on port %d\n", PORT_START + node_i * NUMBER_OF_PORTS + j);
        if (initialize_network(&data_multiport_fd[j], PORT_START + node_i * NUMBER_OF_PORTS + j, -1) == EC_ERROR)
            ERROR_RETURN_VALUE("initialize_network");
    } 
    return EC_OK;
}

void client_close_all_node_port(int *client_fd, int *data_multiport_fd, int nok, int *ok_node_list) {
    for (int i = 0; i < nok; i++) {
        int node_index = ok_node_list[i];
        if (client_fd[node_index] != -1) {
            close(client_fd[node_index]);
            client_fd[node_index] = -1;

            for (int j = 0; j < NUMBER_OF_PORTS; j++) {
                int fd_index = node_index * NUMBER_OF_PORTS + j;
                close(data_multiport_fd[fd_index]);
                data_multiport_fd[fd_index] = -1;
            }
            
            printf("Closed node %d\n", node_index);
        }
    }    
}

/* pthread func: Used to send to or receive from multiple nodes */
void *send_metadata_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");
    Send(metadata->sockfd, metadata->data, CHUNK_SIZE, "send chunk");
    Recv_Response(metadata->sockfd);
    return NULL;
}

void *recv_metadata_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");
    Recv(metadata->sockfd, metadata->data, metadata->block_size, "recv chunk");
    Send_Response(metadata->sockfd);
    return NULL;
}

void *receive_data_multiport(void *arg) {
    MultiPortArgs *args = (MultiPortArgs *)arg;

    #if DEBUGGING_COMMENTS
    printf("Client Waiting on port %d\n", args->port);
    #endif

    Recv(args->sockfd, args->data, args->size, "recv chunk");

    #if DEBUGGING_COMMENTS
    printf("Received data on port %d\n", args->port);
    #endif

    // close(client_fd);
    // close(args->sockfd);

    return NULL;
}

void *recv_metadata_chunk_multiport(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;

    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");

    int read_size = metadata->block_size;

    int base_size = read_size / NUMBER_OF_PORTS;
    int last_block_size = base_size + read_size % NUMBER_OF_PORTS;

    pthread_t threads[NUMBER_OF_PORTS];
    MultiPortArgs args[NUMBER_OF_PORTS];

    for (int i = 0; i < NUMBER_OF_PORTS; i++) {
        int port = PORT_START + metadata->node_i * NUMBER_OF_PORTS + i;
        
        args[i].sockfd = metadata->data_fd[i];
        args[i].data = metadata->data + i * base_size;
        args[i].size = (i == NUMBER_OF_PORTS - 1) ? last_block_size : base_size;
        args[i].port = port;

        if (pthread_create(&threads[i], NULL, receive_data_multiport, (void *)&args[i]) != 0)
            ERROR_RETURN_NULL("pthread_create");
    }

    for (int i = 0; i < NUMBER_OF_PORTS; i++) {
        pthread_join(threads[i], NULL);
    }

    #if DEBUGGING_COMMENTS
    printf("--------Recieved all data from node %d\n", metadata->node_i);
    #endif

    // receive node io time
    Recv(metadata->sockfd, metadata->node_i_io, sizeof(double), "recv node io time");
    
    Send_Response(metadata->sockfd);
    return NULL;
}

void *send_data_multiport(void *arg) {
    MultiPortArgs *args = (MultiPortArgs *)arg;

    #if DEBUGGING_COMMENTS
    printf("Sending data to port %d\n", args->port );
    #endif

    Send(args->sockfd, args->data, args->size, "send chunk");

    // printf("Received return signal from port %d\n", args->port);

    // close(args->sockfd);
    return NULL;
}

void *server_send_multiport(const void *buf, size_t len, int *data_multiport_fd) {
    int base_size = len / NUMBER_OF_PORTS;
    int last_block_size = base_size + len % NUMBER_OF_PORTS;

    pthread_t threads[NUMBER_OF_PORTS];
    MultiPortArgs args[NUMBER_OF_PORTS];

    for (int i = 0; i < NUMBER_OF_PORTS; i++) {
        int port = PORT_START + i;
        
        args[i].sockfd = data_multiport_fd[i];
        args[i].data = buf + i * base_size;
        args[i].size = (i == NUMBER_OF_PORTS - 1) ? last_block_size : base_size;
        args[i].port = port;

        if (pthread_create(&threads[i], NULL, send_data_multiport, &args[i]) != 0)
            ERROR_RETURN_NULL("pthread_create");
    }

    for (int i = 0; i < NUMBER_OF_PORTS; i++) {
        pthread_join(threads[i], NULL);
    }

    return NULL;
}

void *send_metadata(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata, sizeof(metadata_t), "send metadata");
    Recv_Response(metadata->sockfd);
    return NULL;
}

void *recv_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Recv(metadata->sockfd, metadata->data, CHUNK_SIZE, "recv chunk");
    Send_Response(metadata->sockfd);
    return NULL;
}

void *send_chunk(void *arg)
{
    metadata_t *metadata = (metadata_t *)arg;
    Send(metadata->sockfd, metadata->data, CHUNK_SIZE, "send chunk");
    Recv_Response(metadata->sockfd);
    return NULL;
}