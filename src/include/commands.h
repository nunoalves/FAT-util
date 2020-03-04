#ifndef _COMMANDS_H
#define _COMMANDS_H

#include "main.h"
#include "fat.h"

char *get_month(int month);
int fat_info(int argc, char *argv[]);
long int get_dir_pos(char *image, char *dir, int seek_pos);
u_int get_next_clust12(char *image, u_short cluster);
u_int get_next_clust16(char *image, u_short cluster);
u_int get_next_clust32(char *image, u_short cluster);
u_int get_next_clust(char *image, u_short cluster);
int fat_extract_file(char *image, char *entry);
int compare_file(char *filename, fat_file_t *dir_entry);
int fat_remove_entry(char *image, char *entry);
int fat_print_dir(char *image, char *dir);
int fat_list(int argc, char *argv[]);
int fat_remove(int argc, char *argv[]);
int fat_extract(int argc, char *argv[]);
int fat_add(int argc, char *argv[]);

#endif