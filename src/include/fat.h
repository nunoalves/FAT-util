/* 
 * This file defines the functions and structures to manipulate the FAT FS.
 * The BPB (BIOS Parameter Block) is composed by its own BPB struct (common 
 * to all types of FAT) and specific struct to the type of fat (fat_ext16 or
 * fat_ext32).
 *
 * NOTICE: 1. fat_ext16 struct is for FAT12 and FAT16;
 *         2. To get the FAT Type please use the fat_type() function;
 *
 * 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Author: Beraldo Leal <beraldo@beraldoleal.com>
 * Copyright (c) 2001 - 2010 Beraldo Leal, All Rights Reserved.
 *
 */

#ifndef _FAT_H
#define _FAT_H

#define FAT12 12
#define FAT16 16
#define FAT32 32

#define FAT12_CLUST_COUNT 4085
#define FAT16_CLUST_COUNT 65525
#define FAT32_CLUST_COUNT 4294967285

/* where the extented FAT begin */
#define EXTFAT_INIT 36

#define FAT_FILE_LEN 8

/* 1 byte */
typedef unsigned char u_char;

/* 2 bytes */
typedef unsigned short u_short;

/* 4 bytes */
typedef unsigned int u_int;

/* BOOT Super Block - FAT Boot Sector.
 * huge_sec4fat is only for determine FAT type and is valid only
 * for FAT32. After that you have the FAT type, use a specific struct
 * (fat_bs_ext16 or fat_bs_ext32).
 */
typedef struct fat_bs {        // of sz description
  u_char jump[3];              // 00 03 boot jump opcodes
  u_char oem[8];               // 03 08 OEM Name
  u_short byt4sec;             // 11 02 bytes per sector
  u_char sec4clus;             // 13 01 sectors per cluster
  u_short rsvd_sec;            // 14 02 # reserved sectors
  u_char nfat;                 // 16 01 # FATs on volume
  u_short nroot;               // 17 02 # root directory entries
  u_short sec_vol;             // 19 02 sectors in volume
  u_char mdt;                  // 21 01 Media Descriptor Type
  u_short sec4fat;             // 22 02 sectors per FAT
  u_short sec4trk;             // 24 02 sectors per track
  u_short heads;               // 26 02 # heads
  u_int hidd_sec;              // 28 04 # hidden sectors
  u_int huge_nsec_vol;         // 32 04 huge sectors in volume (sec_fat>65535)
  u_int huge_sec4fat;          // 36 04 sectors per FAT (sec4fat>65536)
}__attribute__((packed)) fat_bs_t;

/* Extended fat_boot for FAT12 or FAT16 */
typedef struct fat_ext16 {     // of sz description
  u_char drive_num;            // 36 01 drive number
  u_char winnt_flags;          // 37 01 WinNT reserved flags
  u_char signature;            // 38 01 signature (0x28 or 0x29)
  u_int volume_id;             // 39 04 volume id serial number
  u_char volume_label[11];     // 43 11 volume label string
  u_char fat_type_label[8];    // 54 08 fat label
}__attribute__((packed)) fat_ext16_t;

/* Extended fat_boot for FAT32 */
typedef struct fat_ext32 {     // of sz description
  u_int huge_sec4fat;          // 36 04 sectors per FAT (sec4fat>65536)
  u_short flags;               // 40 02 flags
  u_short fat_version;         // 42 02 fat version
  u_int root_cluster;          // 44 04 cluster number of the root dir
  u_short fsinfo_cluster;      // 48 02 cluster number of fsinfo strct
  u_short backup_cluster;      // 50 02 cluster number of backup bs
  u_char reserved[12];         // 52 12 reserved (when formated is 0 bytes)
  u_char drive_num;            // 64 01 drive number
  u_char winnt_flags;          // 65 01 WinNT reserved flags
  u_char signature;            // 66 01 signature (0x28 or 0x29)
  u_int volume_id;             // 67 04 volume id serial number
  u_char volume_label[11];     // 71 11 volume label string
  u_char fat_type_label[8];    // 82 08 fat label
}__attribute__((packed)) fat_ext32_t;

/* file attributes */
typedef struct fat_file_attr {
  u_int read_only:1;
  u_int hidden:1;
  u_int system:1;
  u_int vol:1;
  u_int dir:1;
  u_int archive:1;
  u_int device:1;
  u_int unused:1;
}__attribute__ ((packed)) fat_file_attr_t;

/* file date */
typedef struct fat_file_date {
  u_int day:5;
  u_int month:4;
  u_int year:7;
}__attribute__ ((packed)) fat_file_date_t;

/* file time */
typedef struct fat_file_time {
  u_int sec:5;
  u_int min:6;
  u_int hour:5;
}__attribute__ ((packed)) fat_file_time_t;

/* File info */
typedef struct fat_file {
  u_char name[8];              // 00 08
  u_char ext[3];               // 08 03
  u_char attr;		       // 11 01
  u_char winnt_flags;          // 12 01
  u_char creattimesecs;        // 13 01
  u_short creattime;           // 14 02
  u_short creatdate;           // 16 02
  u_short lastacces;           // 18 02
  u_short h_firstclus;         // 20 02
  fat_file_time_t lm_time;     // 22 02
  fat_file_date_t lm_date;     // 24 02
  u_short l_firstclus;         // 26 02
  u_int   size;                // 28 04
}__attribute__((packed)) fat_file_t;

/* Long file info */
typedef struct fat_file_long {
  u_char order;                // 00 01
  u_short first_5[5];          // 01 10
  u_char attr;		       // 11 01
  u_char long_entry_type;      // 12 01
  u_char checksum;             // 13 01
  u_char next_6[12];           // 14 12
  u_short zeros;               // 26 02
  u_char final_2[4];           // 28 04
}__attribute__((packed)) fat_file_long_t;

/* functions */
char *rtrim(char *string);
char *get_file_name(fat_file_t *dir_entry);
char *get_file_attrs(fat_file_t *dir_entry);
int is_dir(fat_file_t *dir_entry);
int has_ext(fat_file_t *dir_entry);
unsigned int size_root_dir(fat_bs_t *fat_bpb);
unsigned int left_to_read(fat_bs_t *fat_bpb, int size);
unsigned int tot_data_sec(fat_bs_t *fat_bpb);
unsigned int first_fat_sec(fat_bs_t *fat_bpb);
unsigned int first_root_sec(fat_bs_t *fat_bpb, fat_ext32_t *fat_ext32);
unsigned int first_sec_clus(fat_bs_t *fat_bpb, unsigned int cluster);
unsigned int bytes4clust(fat_bs_t *fat_bpb);
unsigned int first_data_sec(fat_bs_t *fat_bpb);
unsigned int sec4fat(fat_bs_t *fat_bpb);
unsigned int sec4vol(fat_bs_t *fat_bpb);
int read_all_fat(char *image);
fat_bs_t *read_bpb(char *image);
fat_ext16_t *read_ext_fat16(char *image);
fat_ext32_t *read_ext_fat32(char *image);
int fat_type(fat_bs_t *fat_bpb);
int count_file(char *image, int seek_loc);
int count_dir(char *image, int seek_loc);
int print_fat_bpb(void);
int print_fat_ext16(void);
int print_fat_ext32(void);

/* plubic structs and vars*/
fat_bs_t *fat_bpb;
fat_ext16_t *fat_ext16;
fat_ext32_t *fat_ext32;

/* 12, 16 or 32 */
int fat;

#endif

