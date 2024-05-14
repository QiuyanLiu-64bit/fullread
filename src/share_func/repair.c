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
#include <isa-l.h>

#include <netdb.h>
#include <ifaddrs.h>

#include "ec_config.h"

int repair_data_chunks(int *ok_node_list, int nok, int *err_node_list, int nerrs, char **data, char **coding, char **repair_data, int chunk_size)
{
    int i, j;

    /* Create decoding arguments for ISA-L */
    unsigned char *encode_matrix = (unsigned char *)malloc(sizeof(unsigned char) * EC_N * EC_K);
    unsigned char *decode_matrix = (unsigned char *)malloc(sizeof(unsigned char) * EC_N * EC_K);
    unsigned char *invert_matrix = (unsigned char *)malloc(sizeof(unsigned char) * EC_N * EC_K);
    unsigned char *temp_matrix = (unsigned char *)malloc(sizeof(unsigned char) * EC_N * EC_K);
    unsigned char *g_tbls = (unsigned char *)malloc(sizeof(unsigned char) * EC_K * EC_M * 32);
    if (encode_matrix == NULL || decode_matrix == NULL || invert_matrix == NULL || temp_matrix == NULL || g_tbls == NULL)
        ERROR_RETURN_VALUE("Fail malloc:encode_matrix||decode_matrix||invert_matrix||temp_matrix ");

    if (ok_node_list[EC_K - 1] > EC_K - 1) // some nodes that storage data chunks have failed
    {
        gf_gen_rs_matrix(encode_matrix, EC_N, EC_K);

        /* Construct temp_matrix by add ok rows */
        for (i = 0; i < EC_K; i++)
            for (j = 0; j < EC_K; j++)
                temp_matrix[EC_K * i + j] = encode_matrix[EC_K * ok_node_list[i] + j];

        /* get invert matrix */
        if (gf_invert_matrix(temp_matrix, invert_matrix, EC_K) < 0)
            ERROR_RETURN_VALUE("Fail gf_invert_matrix");

        /* Get decode matrix with only wanted recovery rows (data chunk err) */
        for (i = 0; i < nerrs; i++)
        {
            if (err_node_list[i] < EC_K)
                for (j = 0; j < EC_K; j++)
                    decode_matrix[EC_K * i + j] =
                        invert_matrix[EC_K * err_node_list[i] + j];
        }

        int num_need_repair_data = 0;
        for (i = 0; i < nerrs; i++)
            if (err_node_list[i] < EC_K)
                num_need_repair_data++;

        // Recover data
        ec_init_tables(EC_K, num_need_repair_data, decode_matrix, g_tbls);
        ec_encode_data(chunk_size, EC_K, num_need_repair_data, g_tbls, (unsigned char **)data, (unsigned char **)coding);

        for (i = 0; i < nerrs; i++)
            if (err_node_list[i] < EC_K)
                repair_data[err_node_list[i]] = coding[i];
    }
    for (i = 0; i < nok; i++)
        if (ok_node_list[i] < EC_K)
            repair_data[ok_node_list[i]] = data[i];

    free(encode_matrix);
    free(decode_matrix);
    free(invert_matrix);
    free(temp_matrix);
    free(g_tbls);
    return EC_OK;
}

void *repair_data_chunks_thread(void *arg)
{
    RepairArg *targ = (RepairArg *)arg;
    if (repair_data_chunks(targ->ok_node_list, targ->nok, targ->err_node_list, targ->nerrs, targ->data, targ->coding, targ->repair_data, targ->chunk_size) != EC_OK)
        ERROR_RETURN_NULL("Fail repair_data_chunks");
    return NULL;
}

int encode_data_chunks(char **data, char **coding)
{
    int i, j;

    unsigned char *encode_matrix = (unsigned char *)malloc(sizeof(unsigned char) * EC_N * EC_K);
    unsigned char *g_tbls = (unsigned char *)malloc(sizeof(unsigned char) * EC_K * EC_M * 32);
    if (encode_matrix == NULL || g_tbls == NULL)
        ERROR_RETURN_VALUE("Fail malloc: encode_matrix||g_tbls");
        
    gf_gen_rs_matrix(encode_matrix, EC_N, EC_K);
    ec_init_tables(EC_K, EC_M, &encode_matrix[EC_K * EC_K], g_tbls);
    ec_encode_data(CHUNK_SIZE, EC_K, EC_M, g_tbls, (unsigned char **)data, (unsigned char **)coding);

    free(encode_matrix);
    free(g_tbls);
    return EC_OK;
}
