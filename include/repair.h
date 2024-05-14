int repair_data_chunks(int *ok_node_list, int nok, int *err_node_list, int nerrs, char **data, char **coding, char **repair_data, int chunk_size);
void *repair_data_chunks_thread(void *arg);
int encode_data_chunks(char **data, char **coding);