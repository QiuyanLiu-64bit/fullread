#ifndef FILE_OPT_H
#define FILE_OPT_H

int read_file_to_buffer(FILE *src_fp, char *buffer, int buffer_size);

int open_write_file(const char *filename, const char *mode, char *data, int size);
int open_read_file(const char *filename, const char *mode, char *data, int size);
int read_group_strategy(const char* group_strategy_filename, int group_chunk_size[], int nok);
int open_read_file_fr(const char *filename, const char *mode, char *data, int size, CElement ce);
int open_write_file_mul(const char *filename, const char *mode, char *data, int size, int num);
int open_write_file_fr(const char *filename, const char *mode, char ***data, int *group_size, int p, int num);

int clear_file(const char *filename);
int get_size_file(const char *filename);

#endif // FILE_OPT_H