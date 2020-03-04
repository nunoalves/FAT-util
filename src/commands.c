#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <stdint.h>
#include <byteswap.h>

#include "commands.h"

char *get_month(int month)
{
        static char *str_month[12] = {
        "Jan", "Feb", "Mar", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep",
        "Oct", "Nov", "Dec"
        };

        return str_month[month-1];
}

/* the main info function, this read all fat in structs, print the bpb and 
 * specific fat structs.
 */
int fat_info(int argc, char *argv[])
{
        int ret = 0;

        ret = read_all_fat(argv[argc-1]);
        if (ret != -1) {
                ret = print_fat_bpb();
                if (fat == FAT32)
                        ret = print_fat_ext32();
                else
                        ret = print_fat_ext16();
        }
        return ret;
}

long int get_dir_pos(char *image, char *dir, int seek_pos)
{
        fat_file_t *fat_file;
        int fd_fat, i, dir_loc;
        long int root_clus;
        char *temp_dir;

        temp_dir = malloc(12*sizeof(char));

        if ((strcmp(dir,"/") == 0) || (seek_pos == 0)){
                root_clus = first_root_sec(fat_bpb, fat_ext32);
                free(temp_dir);
                return root_clus * fat_bpb->byt4sec;
        } 

        if (read_all_fat(image) == -1) 
                goto fail;

        if ((fd_fat = open(image,O_RDONLY)) < 0)
                goto fail;

        root_clus = first_root_sec(fat_bpb, fat_ext32);
        if (seek_pos == 0) 
                lseek(fd_fat, root_clus * fat_bpb->byt4sec, SEEK_SET);
        else
                lseek(fd_fat, seek_pos, SEEK_SET);

        fat_file = malloc(sizeof(fat_file_t));

        if (fat_file == NULL) goto fail;

        dir_loc = -1;
        int nroot;
        if (fat==FAT32) 
                nroot = 65536;
        else
                nroot = fat_bpb->nroot;
        for(i=0;i<nroot;i++) {
                if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                        goto fail;

                if ((fat_file->name[0] != 0x00) &&
                    (fat_file->name[0] != 0xE5) &&
                    (fat_file->attr != 0x0F) &&
                    (is_dir(fat_file))) {
                        if (strncmp(get_file_name(fat_file),dir,
                                    FAT_FILE_LEN) == 0) {
                                dir_loc=first_sec_clus(fat_bpb, 
                                                       fat_file->l_firstclus);
                                break;
                        }
                }
        }

        if (fd_fat) close(fd_fat);
        if (fat_file) free(fat_file);
        free(temp_dir);
        return dir_loc;
fail:
        printf("Failed to lookup dir in FAT\n");
        if (fd_fat) close(fd_fat);
        if (fat_file) free(fat_file);
        free(temp_dir);
        return -1;
}

u_int get_next_clust12(char *image, u_short cluster)
{
        int fd_fat, fat_begin;

        int cluster_size = 3;
        
        unsigned int active_cluster = cluster;


        fat_begin = fat_bpb->rsvd_sec * fat_bpb->byt4sec;
        unsigned char FAT_table[cluster_size];
        unsigned int fat_offset = active_cluster + (active_cluster / 2);
        unsigned int ent_offset = fat_offset % cluster_size;

        /* At this point you need to read from sector "fat_sector" on 
           the disk into "FAT_table". */

        if ((active_cluster % 2) != 0) fat_offset -= 1;

        //printf("FAT OFFSET: %d | ", fat_offset );
        //printf("Cluster: %d (%x) | ", active_cluster, active_cluster);
        if ((fd_fat = open(image,O_RDONLY)) < 0)
                goto fail;
        
        lseek(fd_fat, fat_begin + fat_offset, SEEK_SET);
        read(fd_fat, FAT_table, sizeof(FAT_table));

        unsigned short table_value = *(unsigned short*)&FAT_table[ent_offset];

        //printf("%.2x", FAT_table[0]);
        //printf("%.2x", FAT_table[1]);
        //printf("%.2x | ", FAT_table[2]);

        if(active_cluster & 0x0001) 
                table_value = table_value >> 4;
        else
                table_value = table_value & 0x0FFF;
        //printf("Next Cluster: %d (%x)\n", table_value, table_value);

        /* The variable "table_value" now has the information you need 
            about the next cluster in the chain. */

        if (table_value >= 0xFF8) goto fail;

        if (fd_fat) close(fd_fat);
        return table_value;
fail:
        if (fd_fat) close(fd_fat);
        return -1;
}


u_int get_next_clust16(char *image, u_short cluster)
{
        int fd_fat, fat_begin;

        int cluster_size = 2;
        
        unsigned int active_cluster = cluster;

        fat_begin = fat_bpb->rsvd_sec * fat_bpb->byt4sec;
        //fat_begin = fat_bpb->rsvd_sec;
        unsigned char FAT_table[cluster_size];
        unsigned int fat_offset = active_cluster * 2;
        //unsigned int fat_sector = fat_begin + (fat_offset / cluster_size);
        unsigned int ent_offset = fat_offset % cluster_size;

        /* At this point you need to read from sector "fat_sector" on 
           the disk into "FAT_table". */

        if ((fd_fat = open(image,O_RDONLY)) < 0)
                goto fail;

        lseek(fd_fat, fat_begin + fat_offset , SEEK_SET);
        read(fd_fat, FAT_table, sizeof(FAT_table));

        unsigned short table_value = *(unsigned short*)&FAT_table[ent_offset];

        /* The variable "table_value" now has the information you need 
            about the next cluster in the chain. */

        if (table_value >= 0xFFF8) goto fail;

        if (fd_fat) close(fd_fat);
        return table_value;
fail:
        if (fd_fat) close(fd_fat);
        return -1;
}


u_int get_next_clust32(char *image, u_short cluster)
{
        int fd_fat, fat_begin;

        int cluster_size = 4;

        unsigned int active_cluster = cluster;

        fat_begin = fat_bpb->rsvd_sec * fat_bpb->byt4sec;
        //fat_begin = fat_bpb->rsvd_sec;
        unsigned char FAT_table[cluster_size];
        unsigned int fat_offset = active_cluster * 4;
        //unsigned int fat_sector = fat_begin + (fat_offset / cluster_size);
        unsigned int ent_offset = fat_offset % cluster_size;

        /* At this point you need to read from sector "fat_sector" on 
           the disk into "FAT_table". */

        if ((fd_fat = open(image,O_RDONLY)) < 0)
                goto fail;

        lseek(fd_fat, fat_begin + fat_offset , SEEK_SET);
        read(fd_fat, FAT_table, sizeof(FAT_table));

        unsigned int table_value = *(unsigned int*)&FAT_table[ent_offset] & 0x0FFFFFFF;

        /* The variable "table_value" now has the information you need 
            about the next cluster in the chain. */

        if (table_value >= 0x0FFFFFF8) goto fail;

        if (fd_fat) close(fd_fat);
        return table_value;
fail:
        if (fd_fat) close(fd_fat);
        return -1;
}


u_int get_next_clust(char *image, u_short cluster)
{
        if (fat == FAT12)
                return get_next_clust12(image, cluster);
        else if (fat == FAT16)
                return get_next_clust16(image, cluster);
        else
                return get_next_clust32(image, cluster);
}

int fat_extract_file(char *image, char *entry)
{
        fat_file_t *fat_file;
        int ret, fd_fat, seek_loc, seek_loc2, found, to_read;
        char *entry2, *e_basename, *e_dirname, *result = NULL, *buff;
        u_int left_size, cluster;

        FILE *fp;

        ret = read_all_fat(image);
        if (ret == -1) 
                goto fail;

        if ((fd_fat = open(image, O_RDWR)) < 0) 
                goto fail;

        if ((char) entry[0] != '/') {
                printf("Invalid path name\n");
                goto fail;
        }
        entry2 = malloc(sizeof(char)*strlen(entry));
        strcpy(entry2,entry);
        e_basename = basename(entry2);
        e_dirname = dirname(entry);
        
        result = strtok( e_dirname, "/" );
        seek_loc = get_dir_pos( image, "/", 0);
        seek_loc2 = seek_loc;
        while (result!=NULL) {
                seek_loc = get_dir_pos(image, result, seek_loc);
                seek_loc2 = seek_loc;
                if (seek_loc == -1) {
                        printf("%s not found.\n", result);
                        return -1;
                }
                result = strtok( NULL, "/");
        }

        if (lseek(fd_fat, seek_loc, SEEK_SET) == -1) 
                goto fail;

        fat_file = malloc(sizeof(fat_file_t));

        if (fat_file == NULL) goto fail;

        if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                goto fail;
        seek_loc2 += sizeof(fat_file_t);
       
        found = 0; 
        while (fat_file->name[0] != 0x00) { 
                if ((fat_file->name[0] != 0xE5) &&
                   (fat_file->attr != 0x0F)) {
                        if (compare_file(e_basename, fat_file) == 0) {
                                found = 1;
                                break;
                        }
                }

                if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                        goto fail;
                seek_loc2 += sizeof(fat_file_t);
        
        }
        if (found == 0) {
                printf("File or dir does not exists\n");
                goto fail;
        }else{
                if (is_dir(fat_file)) {
                        printf("Cannot extract dir.\n");
                        goto fail;
                }
                /* extract here */
                left_size = fat_file->size;
                cluster = fat_file->l_firstclus;
                buff = malloc(bytes4clust(fat_bpb));

                fp = fopen(e_basename, "wb");
                if (fp == NULL) {
                        printf("Failed to open the file.\n");
                        fclose(fp);
                        goto fail;
                }
        
                while(cluster!=-1) {
                        if (lseek(fd_fat,
                                  first_sec_clus(fat_bpb, cluster), 
                                  SEEK_SET) == -1)
                                goto fail;
                        to_read = left_to_read(fat_bpb, left_size);
                        read(fd_fat, buff, to_read);
                        fwrite(buff, to_read, 1, fp);
                        left_size -= to_read; 
                        cluster = get_next_clust(image, cluster);
                }
                printf("%d bytes written.\n", fat_file->size);
                
        }

        if (fp) fclose(fp);        
        if (buff) free(buff);
        if(fat_file) free(fat_file);
        return ret;
fail:
        printf("Failed to extract entry from FAT\n");
        if (fat_file) free(fat_file);
        return -1;
}

int compare_file(char *filename, fat_file_t *dir_entry)
{
        char *buff;

        if (has_ext(dir_entry)) {        
                buff = malloc(strlen(get_file_name(dir_entry))*sizeof(char)+3);

                sprintf(buff, "%s.%c%c%c", get_file_name(dir_entry),
                                           dir_entry->ext[0],
                                           dir_entry->ext[1],
                                           dir_entry->ext[2]);
        }else{
                buff = malloc(strlen(get_file_name(dir_entry))*sizeof(char));
                sprintf(buff,"%s",get_file_name(dir_entry));
        }

        
        if (strcmp(rtrim(buff), filename) == 0)
                return 0;
        else
                return -1;

        free(buff);
}

int fat_remove_entry(char *image, char *entry)
{
        fat_file_t *fat_file;
        int ret, fd_fat, seek_loc, seek_loc2, found;
        char *entry2, *e_basename, *e_dirname, *result = NULL;
        char buff[1];

        ret = read_all_fat(image);
        if (ret == -1) 
                goto fail;

        if ((fd_fat = open(image, O_RDWR)) < 0) 
                goto fail;

        if ((char) entry[0] != '/') {
                printf("Invalid path name\n");
                goto fail;
        }
        entry2 = malloc(sizeof(char)*strlen(entry));
        strcpy(entry2,entry);
        e_basename = basename(entry2);
        e_dirname = dirname(entry);
        
        result = strtok( e_dirname, "/" );
        seek_loc = get_dir_pos( image, "/", 0);
        seek_loc2 = seek_loc;
        while (result!=NULL) {
                seek_loc = get_dir_pos(image, result, seek_loc);
                seek_loc2 = seek_loc;
                if (seek_loc == -1) {
                        printf("%s not found.\n", result);
                        return -1;
                }
                result = strtok( NULL, "/");
        }

        if (lseek(fd_fat, seek_loc, SEEK_SET) == -1) 
                goto fail;

        fat_file = malloc(sizeof(fat_file_t));

        if (fat_file == NULL) goto fail;

        if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                goto fail;
        seek_loc2 += sizeof(fat_file_t);
       
        found = 0; 
        while (fat_file->name[0] != 0x00) { 
                if ((fat_file->name[0] != 0xE5) &&
                   (fat_file->attr != 0x0F)) {
                        if (compare_file(e_basename, fat_file) == 0) {
                                found = 1;
                                break;
                        }
                }

                if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                        goto fail;
                seek_loc2 += sizeof(fat_file_t);
        
        }
        if (found == 0) {
                printf("File or dir does not exists\n");
                goto fail;
        }else{
                if (is_dir(fat_file)) {
                        if ((count_file(image, seek_loc) != 0) ||
                            (count_dir(image, seek_loc) != 2)) {
                                printf("Cannot remove dir: Is not empty.\n");
                                goto fail;
                        }
                }
                /* set first byte of fat_file to 0xE5 (remove) */
                if (lseek(fd_fat, seek_loc2 - sizeof(fat_file_t), 
                          SEEK_SET) == -1)
                        goto fail;
                buff[0] = 0xE5;
                write(fd_fat, buff, 1);

                printf("File %s removed.\n", e_basename);
        }

        if(fat_file) free(fat_file);
        return ret;
fail:
        printf("Failed to remove entry in FAT\n");
        if(fat_file) free(fat_file);
        return -1;
}

int fat_print_dir(char *image, char *dir)
{
        fat_file_t *fat_file;
        int ret, fd_fat, seek_loc;
        char *attrs, *result = NULL;

        ret = read_all_fat(image);
        if (ret == -1) 
                goto fail;

        if ((fd_fat = open(image,O_RDONLY)) < 0) 
                goto fail;

        result = strtok( dir, "/" );
        seek_loc = get_dir_pos( image, "/", 0);
        while (result!=NULL) {
                seek_loc = get_dir_pos(image, result, seek_loc);
                if (seek_loc == -1) {
                        printf("%s not found.\n", result);
                        return -1;
                }
                result = strtok( NULL, "/");
        }

        if (lseek(fd_fat, seek_loc, SEEK_SET) == -1) 
                goto fail;

        fat_file = malloc(sizeof(fat_file_t));

        if (fat_file == NULL) goto fail;

        printf("%d file(s), %d dir(s)\n", count_file(image, seek_loc),
                                          count_dir(image, seek_loc));

        if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                goto fail;
        
        while (fat_file->name[0] != 0x00) { 
                if ((fat_file->name[0] != 0xE5) &&
                   (fat_file->attr != 0x0F)) {
                        attrs = get_file_attrs(fat_file);
                        printf("%s ", attrs);
                        printf("%8d ", fat_file->size);
                        printf("%d %s %.02d ", 1980+fat_file->lm_date.year,
                                   get_month(fat_file->lm_date.month),
                               fat_file->lm_date.day);
                        printf("%s", get_file_name(fat_file));
                        if (has_ext(fat_file))
                                printf(".%s", (char *)fat_file->ext);
                        printf("\n");
                }

                if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                        goto fail;
        
        }

        if(fat_file) free(fat_file);
        return ret;
fail:
        printf("Failed to print dir in FAT\n");
        if(fat_file) free(fat_file);
        return -1;
}

int fat_list(int argc, char *argv[])
{
        int ret;
        if (argc!=4) {
                printf("Syntax invalid.\n");
                printf("Try '%s --help' for help.\n",
                        PROG_NAME);
                return -1;
        }

        ret = fat_print_dir(argv[argc-1], argv[argc-2]);
        return ret;
}

int fat_remove(int argc, char *argv[])
{
        int ret;
        if (argc!=4) {
                printf("Syntax invalid.\n");
                printf("Try '%s --help' for help.\n",
                        PROG_NAME);
                return -1;
        }
        ret = fat_remove_entry(argv[argc-1], argv[argc-2]);
        return ret;
}

int fat_extract(int argc, char *argv[])
{
        int ret;
        if (argc!=4) {
                printf("Syntax invalid.\n");
                printf("Try '%s --help' for help.\n",
                        PROG_NAME);
                return -1;
        }
        ret = fat_extract_file(argv[argc-1], argv[argc-2]);
        return ret;
}

int fat_add(int argc, char *argv[])
{
        return 0;
}
