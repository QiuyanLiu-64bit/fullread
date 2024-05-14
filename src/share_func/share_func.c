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


int replace_filename_suffix(char *filename, int suffix)
{
    // Find the position of the last '-' character
    char *dash_pos = strrchr(filename, '_');
    if (dash_pos == NULL)
        ERROR_RETURN_VALUE("Invalid filename");

    // Find the position of the character to replace
    int pos = dash_pos - filename + 1;

    // Replace the number with the new number
    char new_num_str[12];
    sprintf(new_num_str, "%d", suffix);
    strcpy(&filename[pos], new_num_str);

    return EC_OK;
}


