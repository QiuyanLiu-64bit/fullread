#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 100
#define NUM_BAND 5
#define EC_N_MAX 16
#define EC_N 16
#define INT_MAX 9999999

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

int main()
{
    int i, j;
    FILE *file;
    char file_path[] = "C:\\Users\\22806\\Desktop\\light_band.txt";
    char line[MAX_LINE_LENGTH];
    int node_count;
    int uplink_bandwidth[NUM_BAND][EC_N_MAX];
    int downlink_bandwidth[NUM_BAND][EC_N_MAX];

    // Open the file
    file = fopen(file_path, "r");
    if (file == NULL)
    {
        fprintf(stderr, "Unable to open the file: %s\n", file_path);
        return 1;
    }

    // Read and validate the identifier line
    while (1)
    {
        if (fgets(line, MAX_LINE_LENGTH, file) == NULL || sscanf(line, "light %d", &node_count) != 1)
        {
            printf("%s", line);
            fprintf(stderr, "File format error, unable to read or validate the identifier line.\n");
            fclose(file);
            return 1;
        }
        if (node_count == EC_N)
            break;
        for (j = 0; j < 2 * NUM_BAND; j++)
            if (fgets(line, MAX_LINE_LENGTH, file) == NULL)
            {
                fprintf(stderr, "File format error, unable to line.\n");
                fclose(file);
                return 1;
            }
    }

    for (j = 0; j < NUM_BAND; j++)
    {
        // Read uplink bandwidth data
        if (fgets(line, MAX_LINE_LENGTH, file) != NULL)
        {
            char *token = strtok(line, " ");
            for (i = 0; i < node_count; ++i)
            {
                if (token != NULL)
                {
                    sscanf(token, "%d", &uplink_bandwidth[j][i]);
                    token = strtok(NULL, " ");
                }
                else
                {
                    fprintf(stderr, "File format error, unable to correctly read uplink bandwidth data.\n");
                    fclose(file);
                    return 1;
                }
            }
        }
        else
        {
            fprintf(stderr, "File format error, unable to read uplink bandwidth data.\n");
            fclose(file);
            return 1;
        }

        // Read downlink bandwidth data
        if (fgets(line, MAX_LINE_LENGTH, file) != NULL)
        {
            char *token = strtok(line, " ");
            for (i = 0; i < node_count; ++i)
            {
                if (token != NULL)
                {
                    sscanf(token, "%d", &downlink_bandwidth[j][i]);
                    token = strtok(NULL, " ");
                }
                else
                {
                    fprintf(stderr, "File format error, unable to correctly read downlink bandwidth data.\n");
                    fclose(file);
                    return 1;
                }
            }
        }
        else
        {
            fprintf(stderr, "File format error, unable to read downlink bandwidth data.\n");
            fclose(file);
            return 1;
        }
    }

    // Close the file
    fclose(file);

    // Sort the bandwidth arrays
    for (int j = 0; j < NUM_BAND; j++)
    {
        // Sort uplink bandwidth
        bubbleSort(uplink_bandwidth[j], node_count);

        // Sort downlink bandwidth
        bubbleSort(downlink_bandwidth[j], node_count);
    }

    for (j = 0; j < NUM_BAND; j++)
    {
        // Print the read bandwidth data
        printf("Uplink bandwidth: \t");
        for (i = 0; i < node_count; ++i)
        {
            printf("%d ", uplink_bandwidth[j][i]);
        }

        printf("\nDownlink bandwidth: \t");
        for (i = 0; i < node_count; ++i)
        {
            printf("%d ", downlink_bandwidth[j][i]);
        }
        printf("\n");
    }

    // Print the read bandwidth data
    printf("Uplink bandwidth: \t");
    for (i = 0; i < node_count; ++i)
    {
        printf("%d ", uplink_bandwidth[0][i]);
    }

    printf("\nDownlink bandwidth: \t");
    for (i = 0; i < node_count; ++i)
    {
        printf("%d ", downlink_bandwidth[0][i]);
    }
    printf("\n");
    int **nodes_bandwidth = (int **)malloc(EC_N * sizeof(int *));
    int *nodes_bandwidth_temp = (int *)malloc(EC_N * EC_N * sizeof(int));
    for (i = 0; i < EC_N; i++)
        nodes_bandwidth[i] = nodes_bandwidth_temp + (i * EC_N);
    get_bandwidth_between_nodes(uplink_bandwidth[0], downlink_bandwidth[0], nodes_bandwidth);
    for (i = 0; i < EC_N; i++)
    {
        for (j = 0; j < EC_N; j++)
            printf("%d ", nodes_bandwidth[i][j]);
        printf("\n");
    }

    return 0;
}
