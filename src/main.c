/* 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Author: Beraldo Leal <beraldo@beraldoleal.com>
 * Copyright (c) 2001 - 2010 Beraldo Leal, All Rights Reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "main.h"

void print_syntax(void)
{
        printf("%s version %s\n", PROG_NAME, PROG_VERSION);
        printf("Copyright (c) 2001-2010 %s, All Rights Reserved.\n", AUTHOR);
        printf("Manage FAT12, FAT16 and FAT32 images.\n\n");
        printf("Usage: %s command [dir|entry|file|src] [dst] image_file\n", 
               PROG_NAME);
        printf("Commands:\n");
        printf("  info               Show FAT detailed info.\n");
        printf("  list dir           List dir content.\n");
        printf("  remove entry       Remove file or directory.\n");
        printf("  extract file       Extract file to current directory.\n");
        printf("  add src dst        Add src file to directory entry dst.\n");
}

int main(int argc, char *argv[])
{
        int ret = 0;

        /* parse args */
        if (argc < 2) {
                print_syntax();
                goto fail;
        }

        if (strcmp(argv[1],"info")==0) ret = fat_info(argc, argv);
        else if (strcmp(argv[1],"list")==0) ret = fat_list(argc, argv);
        else if (strcmp(argv[1],"remove")==0) fat_remove(argc, argv);
        else if (strcmp(argv[1], "extract")==0) fat_extract(argc, argv);
        else if (strcmp(argv[1], "add")==0) fat_add(argc, argv);
        else if (strcmp(argv[1], "--help")==0) {
                print_syntax();
        }else {
                printf("%s: command not found\n", argv[1]);
                printf("Try '%s --help' for help.\n",
                        PROG_NAME);
                goto fail;
        }

        if (ret == -1) goto fail;

        if (fat_bpb) free(fat_bpb); 
        if ((fat == FAT16) || (fat == FAT12)) free(fat_ext16); 
        if (fat == FAT32) free(fat_ext32); 
        return 0;
        
fail:
        if (fat_bpb) free(fat_bpb);
        if ((fat == FAT16) || (fat == FAT12)) free(fat_ext16);
        if (fat == FAT32) free(fat_ext32);
        exit(EXIT_FAILURE);
}