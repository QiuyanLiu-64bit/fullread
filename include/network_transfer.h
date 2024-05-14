#ifndef NETWORK_TRANSFER_H
#define NETWORK_TRANSFER_H

int initialize_network(int *sockfd_p, int port, int ip_offset);
int server_initialize_network(int *server_fd_p, int port);
int get_local_ip_lastnum(int *lastnum_p);

void *Send(int sockfd, const void *buf, size_t len, const char *str);
void *Recv(int sockfd, void *buf, size_t len, char *str);
void *Send_Response(int sockfd);
void *Recv_Response(int sockfd);

int client_init_all_node_port(int *client_fd, int *data_multiport_fd, int *nok, int *ok_node_list, int *nerrs, int *err_node_list);
int server_init_all_node_port(int *server_fd, int *data_multiport_fd);
void client_close_all_node_port(int *client_fd, int *data_multiport_fd, int nok, int *ok_node_list);
void *send_metadata_chunk(void *arg);
void *recv_metadata_chunk(void *arg);
void *receive_data_multiport(void *arg);
void *recv_metadata_chunk_multiport(void *arg);
void *send_data_multiport(void *arg);
void *server_send_multiport(const void *buf, size_t len, int *data_multiport_fd);
void *send_metadata(void *arg);
void *recv_chunk(void *arg);
void *send_chunk(void *arg);

#endif // NETWORK_TRANSFER_H
