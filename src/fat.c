#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "fat.h"

char *rtrim(char *string)
{
        char *original = string + strlen(string);
        while(*--original == ' ');
                *(original + 1) = '\0';
        return string;
}



/* get a dir entry (fat_file_t) and return a string with filename  */
char *get_file_name(fat_file_t *dir_entry)
{
        int i;
        char *name;
        name = malloc(FAT_FILE_LEN+1*sizeof(char));

        for(i=0; i<FAT_FILE_LEN; i++) {
                name[i] = dir_entry->name[i];
        }
        return rtrim(name);
}

/* get a dir entry (fat_file_t) and return a string with attributes
 * of dir entry on format rhsvda (with hifens for 0'bits.           
 */
char *get_file_attrs(fat_file_t *dir_entry)
{
        int i;
        static char attributes[6] = { 'r', 'h', 's', 'v', 'd', 'a' };
        static char attrs[6];

        for(i=0; i<6; i++) {
                if (dir_entry->attr & 01)
                        attrs[i] = attributes[i];
                else
                        attrs[i] = '-';
                dir_entry->attr>>=1;
        }
        return attrs;
}

/* get a dir_entry (fat_file_t) and return true if is a dir entry */
int is_dir(fat_file_t *dir_entry)
{
        if (dir_entry->attr & 0x10)
                return 1;
        else
                return 0;
}

/* get a dir entry (fat_file_t) and return true if have extension chars */
int has_ext(fat_file_t *dir_entry)
{
        if ((dir_entry->ext[0] == 0x20) &&
            (dir_entry->ext[1] == 0x20) &&
              (dir_entry->ext[2] == 0x20)) 
                return 0;
        else
                return 1;
}

unsigned int size_root_dir(fat_bs_t *fat_bpb)
{
        return ((fat_bpb->nroot * 32) + (fat_bpb->byt4sec - 1)) / 
                 fat_bpb->byt4sec;
}

unsigned int left_to_read(fat_bs_t *fat_bpb, int size)
{
        if (size < bytes4clust(fat_bpb))
                return size;
        else
                return bytes4clust(fat_bpb);
}

unsigned int tot_data_sec(fat_bs_t *fat_bpb)
{
        return fat_bpb->sec_vol - (fat_bpb->rsvd_sec + 
                (fat_bpb->nfat * fat_bpb->sec4fat) + size_root_dir(fat_bpb));
}

/* return the first FAT sector */
unsigned int first_fat_sec(fat_bs_t *fat_bpb)
{
        return fat_bpb->rsvd_sec;
}

/* All cluster numbers for directories that are given in the file system 
 * will be relative to the first data cluster. For instance, 
 * "fat_ext32->root_cluster" often reports that the root cluster is at 
 * cluster 2. Here is how you can find the absolute cluster when given 
 * the relative cluster.
 */
unsigned int first_root_sec(fat_bs_t *fat_bpb, fat_ext32_t *fat_ext32)
{
        int fst_data_sec, relative_cluster;

        fst_data_sec = fat_bpb->rsvd_sec + (fat_bpb->nfat * fat_bpb->sec4fat);

        if (fat == FAT32) {
                //relative_cluster = fat_ext32->root_cluster;
                relative_cluster = (fst_data_sec - 1) * fat_bpb->sec4clus * fat_bpb->sec4clus;
        //        relative_cluster = 1984;
        }
        else
                relative_cluster = fst_data_sec;
        
        return relative_cluster;
}

unsigned int first_sec_clus(fat_bs_t *fat_bpb, unsigned int cluster)
{
         return (((cluster - 2) * fat_bpb->sec4clus) + first_data_sec(fat_bpb))*
                fat_bpb->byt4sec;
}

unsigned int bytes4clust(fat_bs_t *fat_bpb)
{
        return fat_bpb->byt4sec * fat_bpb->sec4clus;
}

/* return the first data sector
 * that is, the first sector in which directories and files may be stored
 */
unsigned int first_data_sec(fat_bs_t *fat_bpb)
{
        return first_root_sec(fat_bpb, fat_ext32) + size_root_dir(fat_bpb);
}


/* return the correct sectors per FAT */
unsigned int sec4fat(fat_bs_t *fat_bpb)
{
        if (fat_bpb->sec4fat != 0)
                return fat_bpb->sec4fat;
        else
                return fat_bpb->huge_sec4fat;
}

/* return the correct sectors per volume */
unsigned int sec4vol(fat_bs_t *fat_bpb)
{
        if (fat_bpb->sec_vol != 0)
                return fat_bpb->sec_vol;
        else
                return fat_bpb->huge_nsec_vol;
}

/* Read bpb and specific fats structs.
 * After this, fat_bpb, fat_ext16/32 and fat will be filled
 */
int read_all_fat(char *image)
{
       /* read bpb to struct */
        if ((fat_bpb = read_bpb(image)) == NULL) {
                printf("Error to read fat_bpb\n");
                return -1;
        }

        /* read specific fat (12, 16 or 32) to struct */
        fat = fat_type(fat_bpb);
        if (fat == FAT32) {
                if ((fat_ext32 = read_ext_fat32(image)) == NULL) {
                        printf("Error to read fat_ext32\n");
                        return -1;
                }
        }else{
                if ((fat_ext16 = read_ext_fat16(image)) == NULL) {
                        printf("Error to read fat_ext16\n");
                        return -1;
                }
        }

        return 0;
}

/* read bpb to struct */
fat_bs_t *read_bpb(char *image)
{
        int fd_fat;

        if ((fd_fat = open(image,O_RDONLY)) < 0) {
                printf("Cannot open file %s\n", image);
                goto fail;
        }

        if (fat_bpb == NULL) {
                fat_bpb = malloc(sizeof(*fat_bpb));
                if (fat_bpb == NULL) {
                        printf("Failed at malloc\n");
                        goto fail;
                }
        }

        if (read(fd_fat, fat_bpb, sizeof(fat_bs_t)) == -1) {
                printf("Cannot read file %s\n", image);
                goto fail;
        }

        close(fd_fat);
        return fat_bpb;

fail:
        close(fd_fat);
        if (fat_bpb) free(fat_bpb);
        return NULL;
}

/* read to struct specific boot sector extended for FAT12 and FAT16 */
fat_ext16_t *read_ext_fat16(char *image)
{
        int fd_fat; 

        if ((fd_fat = open(image,O_RDONLY)) < 0) 
                goto fail;

        if (fat_ext16 == NULL) {
                fat_ext16 = malloc(sizeof(*fat_ext16));
                if (fat_ext16 == NULL) 
                        goto fail;
        }

        if (lseek(fd_fat, EXTFAT_INIT, SEEK_SET) == -1) 
                goto fail;

        if (read(fd_fat, fat_ext16, sizeof(fat_ext16_t)) == -1) 
                goto fail;

        close(fd_fat);
        return fat_ext16;

fail:
        printf("Failed to read fat_ext16 struct.\n");
        close(fd_fat);
        if (fat_ext16) free(fat_ext16);
        return NULL;
}

/* read file to struct specific boot sector extended for FAT32 */
fat_ext32_t *read_ext_fat32(char *image)
{
        int fd_fat;

        if ((fd_fat = open(image,O_RDONLY)) < 0) 
                goto fail;

        if (fat_ext32 == NULL) {
                fat_ext32 = malloc(sizeof(*fat_ext32));
                if (fat_ext32 == NULL) 
                        goto fail;
        }

        if (lseek(fd_fat, EXTFAT_INIT, SEEK_SET) == -1) 
                goto fail;

        if (read(fd_fat, fat_ext32, sizeof(fat_ext32_t)) == -1) 
                goto fail;
        
        close(fd_fat);
        return fat_ext32;

fail:
        printf("Error to read fat_ext32 struct\n");
        close(fd_fat);
        if (fat_ext32) free(fat_ext32);
        return NULL;
}

/* discover the FAT type */
int fat_type(fat_bs_t *fat_bpb)
{
        int fat_type;
        unsigned int root_sectors, data_sectors, total_clusters;

        /* The size of the root directory (unless you have FAT32, */
        /* in which case the size will be 0) */
        root_sectors = ((fat_bpb->nroot * 32) + (fat_bpb->byt4sec - 1)) /
                        fat_bpb->byt4sec;

        /* The total number of data sectors */
        data_sectors =  sec4vol(fat_bpb) - (fat_bpb->rsvd_sec + 
                      (fat_bpb->nfat *  sec4fat(fat_bpb)) + root_sectors);
        
        /* The total number of clusters */
        total_clusters = data_sectors / fat_bpb->sec4clus;

        if (total_clusters < FAT12_CLUST_COUNT) 
                fat_type = FAT12;
        else{ 
                if(total_clusters < FAT16_CLUST_COUNT) 
                        fat_type = FAT16;
                   else 
                              fat_type = FAT32;
        }

        return fat_type;
}

int count_file(char *image, int seek_loc)
{
        fat_file_t *fat_file;
        int ret, fd_fat, count = 0 ;

        ret = read_all_fat(image);
        if (ret == -1) goto fail;

        if ((fd_fat = open(image,O_RDONLY)) < 0) goto fail;

        lseek(fd_fat, seek_loc, SEEK_SET);

        fat_file = malloc(sizeof(fat_file_t));

        if (fat_file == NULL) goto fail;

        if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                goto fail;

        while (fat_file->name[0] != 0x00) { 
                if ((fat_file->name[0] != 0xE5) &&
                   (fat_file->attr != 0x0F)) {

                if ((fat_file->name[0] != 0x00) &&
                    (fat_file->name[0] != 0xE5) &&
                    (fat_file->attr != 0x0F)) 
                        if (!is_dir(fat_file)) count++;
                }

                if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                        goto fail;
        }

        if (fd_fat) close(fd_fat);
        if (fat_file) free(fat_file);
        return count;
fail:
        printf("Failed to count dirs in FAT\n");
        if (fd_fat) close(fd_fat);
        if (fat_file) free(fat_file);
        return ret;
}


int count_dir(char *image, int seek_loc)
{
        fat_file_t *fat_file;
        int ret, fd_fat, count = 0 ;

        ret = read_all_fat(image);
        if (ret == -1) goto fail;

        if ((fd_fat = open(image,O_RDONLY)) < 0) goto fail;

        lseek(fd_fat, seek_loc, SEEK_SET);

        fat_file = malloc(sizeof(fat_file_t));

        if (fat_file == NULL) goto fail;

        if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                goto fail;

        while (fat_file->name[0] != 0x00) { 
                if ((fat_file->name[0] != 0xE5) &&
                   (fat_file->attr != 0x0F)) {

                if ((fat_file->name[0] != 0x00) &&
                    (fat_file->name[0] != 0xE5) &&
                    (fat_file->attr != 0x0F)) 
                        if (is_dir(fat_file)) count++;
                }

                if (read(fd_fat, fat_file, sizeof(fat_file_t)) == -1) 
                        goto fail;
        }

        if (fd_fat) close(fd_fat);
        if (fat_file) free(fat_file);
        return count;
fail:
        printf("Failed to count dirs in FAT\n");
        if (fd_fat) close(fd_fat);
        if (fat_file) free(fat_file);
        return ret;
}

/* print common boot sector for FAT12, FAT16 and FAT32 */
int print_fat_bpb(void)
{
        if (fat_bpb == NULL) {
                printf("ERROR: Failed to read fat_bpb\n");
                return -1;
        }
        printf("JMP opcodes: %.2X %.2X %.2X\n", fat_bpb->jump[0],
                                                   fat_bpb->jump[1],
                                                 fat_bpb->jump[2]);
        printf("OEM Name: %8s\n", &fat_bpb->oem[0]);
        printf("Bytes per sector: %d\n", fat_bpb->byt4sec);
        printf("Sectors per cluster: %d\n", fat_bpb->sec4clus);
        printf("# reserved sectors: %d\n", fat_bpb->rsvd_sec);
        printf("# FATs on volume: %d\n", fat_bpb->nfat);
        printf("# root directory entries: %d\n", fat_bpb->nroot);
        printf("Sectors in volume: %d\n", fat_bpb->sec_vol);
        printf("Media descriptor type: %d\n", fat_bpb->mdt);
        printf("Sectors per FAT: %d\n", fat_bpb->sec4fat);
        printf("Sectors per Track: %d\n", fat_bpb->sec4trk);
        printf("# heads: %d\n", fat_bpb->heads);
        printf("# hidden sectors: %d\n", fat_bpb->hidd_sec);
        printf("Huge sectors in volume: %d\n", fat_bpb->huge_nsec_vol);
        printf("FAT Type: %d\n", fat_type(fat_bpb));

        return 0;
}

/* print specific extended boot sector for FAT12 or FAT16 */
int print_fat_ext16(void)
{
        int i;

        if (fat_ext16 == NULL) {
                printf("ERROR: Failed to read fat_ext16\n");
                return -1;
        }
        printf("Drive number: %d\n", fat_ext16->drive_num);
        printf("Signature: %X\n", fat_ext16->signature);
        printf("Volume ID: %d\n", fat_ext16->volume_id);
        printf("Volume Label: ");
        for (i=0;i<11;i++)
                printf("%c", fat_ext16->volume_label[i]);
        printf("\n");
        printf("FAT Type: ");
        for (i=0;i<8;i++)
                printf("%c", fat_ext16->fat_type_label[i]);
        printf("\n");

        return 0;
}

/* print specific extended boot sector for FAT32 */
int print_fat_ext32(void)
{
        int i;
        if (fat_ext32 == NULL) {
                printf("ERROR: Failed to read fat_ext32\n");
                return -1;
        }
        printf("Drive number: %d\n", fat_ext32->drive_num);
        printf("Signature: %X\n", fat_ext32->signature);
        printf("Volume ID: %d\n", fat_ext32->volume_id);
        printf("Volume Label: ");
        for (i=0;i<11;i++)
                printf("%c", fat_ext32->volume_label[i]);
        printf("\n");
        printf("FAT Type: ");
        for (i=0;i<8;i++)
                printf("%c", fat_ext32->fat_type_label[i]);
        printf("\n");
        printf("Root Cluster: %d\n", fat_ext32->root_cluster);

        return 0;
}