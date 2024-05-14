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


void bubbleSort(int arr[], int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = 0; j < n - i - 1; j++)
        {
            if (arr[j] < arr[j + 1])
            {
                // Swap arr[j] and arr[j + 1]
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

/* read_updown_bandwidth EC_N nodes */
int read_updown_bandwidth(int *uplink_bandwidth, int *downlink_bandwidth, int light_heavy_flag, int band_location)
{
    // int uplink_bandwidth_mul[NUM_BAND][EC_N_MAX], downlink_bandwidth_mul[NUM_BAND][EC_N_MAX];

    int i, j; // loop control variables
    int return_temp;

    if (light_heavy_flag != 1 && light_heavy_flag != 2)
        ERROR_RETURN_VALUE("error light_heavy_flag: light=1,heavy=2");
    if (band_location < 1 || band_location > NUM_BAND)
        ERROR_RETURN_VALUE("error band_location:band_location < 1 || band_location > NUM_BAND");

    /* File arguments */
    FILE *band_fp; // src_file pointers
    int file_size; // size of file
    char band_filename[MAX_PATH_LEN] = {0};
    char cur_directory[MAX_PATH_LEN] = {0}, ec_directory[MAX_PATH_LEN] = {0};
    getcwd(cur_directory, sizeof(cur_directory));
    strncpy(ec_directory, cur_directory, strlen(cur_directory) - 6); // -6 to sub script/
    if (light_heavy_flag == 1)
        sprintf(band_filename, "%s%s%s", ec_directory, BANDWIDTH_PATH, "light_band.txt"); // get light_band_filename
    else if (light_heavy_flag == 2)
        sprintf(band_filename, "%s%s%s", ec_directory, BANDWIDTH_PATH, "heavy_band.txt"); // get heavy_band_filename
    printf("band_filename = %s\n", band_filename);

    int node_count_temp;
    char line[MAX_LINE_LENGTH];

    if ((band_fp = fopen(band_filename, "r")) == NULL) // Open the file
        ERROR_RETURN_VALUE("Fail open file band_filename");

    /* Read and validate the identifier line */
    while (1)
    {
        if (fgets(line, MAX_LINE_LENGTH, band_fp) == NULL || sscanf(line, "light %d", &node_count_temp) != 1)
        {
            if (light_heavy_flag == 1)
                return_temp = sscanf(line, "light %d", &node_count_temp);
            else if (light_heavy_flag == 2)
                return_temp = sscanf(line, "heavy %d", &node_count_temp);
            if (return_temp != 1)
                ERROR_RETURN_VALUE("File format error: should light num or heavt num");

            if (node_count_temp == EC_N)
                break;
            for (j = 0; j < 2 * NUM_BAND; j++)
                if (fgets(line, MAX_LINE_LENGTH, band_fp) == NULL)
                    ERROR_RETURN_VALUE("File format error: No data.");
        }
    }

    for (j = 0; j < band_location - 1; j++) //  get band line
        if (fgets(line, MAX_LINE_LENGTH, band_fp) == NULL)
            ERROR_RETURN_VALUE("File format error: No data.");

    /* Read uplink and downlink bandwidth data */
    if (fgets(line, MAX_LINE_LENGTH, band_fp) != NULL)
    {
        char *token = strtok(line, " ");
        for (i = 0; i < node_count_temp; ++i)
        {
            if (token == NULL)
                ERROR_RETURN_VALUE("File format error: format uplink data.");
            sscanf(token, "%d", &uplink_bandwidth[i]);
            token = strtok(NULL, " ");
        }
    }
    else
        ERROR_RETURN_VALUE("File format error: no uplink data.");

    if (fgets(line, MAX_LINE_LENGTH, band_fp) != NULL)
    {
        char *token = strtok(line, " ");
        for (i = 0; i < node_count_temp; ++i)
        {
            if (token == NULL)
                ERROR_RETURN_VALUE("File format error: format downlink data.");
            sscanf(token, "%d", &downlink_bandwidth[i]);
            token = strtok(NULL, " ");
        }
    }
    else
        ERROR_RETURN_VALUE("File format error: no downlink data.");

    fclose(band_fp); // Close the file

    bubbleSort(uplink_bandwidth, node_count_temp); // Sort the bandwidth arrays-The bandwidth of the uplink and downlink links is close to
    bubbleSort(downlink_bandwidth, node_count_temp);

    printf("Uplink bandwidth: \t"); // Print the read bandwidth data
    for (i = 0; i < node_count_temp; ++i)
        printf("%d ", uplink_bandwidth[i]);
    printf("\nDownlink bandwidth: \t");
    for (i = 0; i < node_count_temp; ++i)
        printf("%d ", downlink_bandwidth[i]);
    printf("\n");
    return EC_OK;
}

/* get_bandwidth_between_nodes nodes */
void get_bandwidth_between_nodes(int *uplink_bandwidth, int *downlink_bandwidth, int **nodes_bandwidth)
{
    // int uplink_bandwidth[NUM_BAND][EC_N_MAX], downlink_bandwidth[NUM_BAND][EC_N_MAX],nodes_bandwidth[EC_N_MAX][EC_N_MAX];

    int i, j; // loop control variables

    for (i = 0; i < EC_N; i++)
    {
        for (j = 0; j < EC_N; j++)
            if (i == j)
                nodes_bandwidth[i][j] = INT_MAX;
            else
                nodes_bandwidth[i][j] = uplink_bandwidth[i] > downlink_bandwidth[j] ? downlink_bandwidth[j] : uplink_bandwidth[i];
    }
    return;
}