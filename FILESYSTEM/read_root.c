#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED_ENTRY 0x00
#define FIRST_BYTE_0xE5 0x05
#define DOT_DIR 0x2E
#define DELETED_FILE 0xE5


typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    char starting_cluster[4];
    char file_size[4];
} __attribute((packed)) PartitionTable;

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size;
	unsigned char cluster_size; // 1 byte
    unsigned short reserved_sectors; // 2 bytes
    unsigned char num_fats; // 1 byte
    unsigned short root_entries; // 2 bytes
    unsigned short total_sectors; // 2 bytes
    unsigned char media_type; // 1 byte
    unsigned short fat_size; // 2 bytes
    unsigned short sectors_per_track; // 2 bytes
    unsigned short num_heads; // 2 bytes
    unsigned int hidden_sectors; // 4 bytes
    unsigned int total_sectors_large; // 4 bytes
    unsigned char drive_number; // 1 byte
    unsigned char reserved; // 1 byte
    unsigned char boot_signature; // 1 byte
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8];
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat12BootSector;

typedef struct {
    char filename[11];
    unsigned char attribute;
    unsigned char reserved;
    unsigned char creation_time_tenths; // 0.1 seconds
    unsigned short creation_time_HMS; // hour, minutes, seconds
    unsigned short creation_date;
    unsigned short acces_date;
    unsigned short high_cluster_address; // 0 in FAT12
    unsigned short modified_time_HMS;
    unsigned short modified_date;
    unsigned short low_cluster_address;
    unsigned int file_size; // 0 for directories
} __attribute((packed)) Fat12Entry;

void print_file_info(Fat12Entry *entry) {

    switch(entry->filename[0]) {
    case UNUSED_ENTRY:
        return; 
    case DELETED_FILE: 
        printf("Archivo borrado: [?%.7s.%.3s]\n", entry-> filename +1 , entry -> filename + 8);
        //printf("Archivo borrado: [?%.11s]\n", entry->filename);
        return;
    case FIRST_BYTE_0xE5: 
        printf("Archivo que comienza con 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry -> filename +1, entry -> filename +8);
        //printf("Archivo que comienza con 0xE5: [%.11s]\n", entry -> filename);
        break;
    case DOT_DIR: 
        printf("Directorio: [%.8s.%.3s]\n", entry -> filename , entry -> filename+8);
        //printf("Directorio: [%.11s]\n", entry -> filename);
        break;
    default:
        printf("Archivo: [%.8s.%.3s]\n", entry -> filename, entry -> filename+8);
        //printf("Archivo: [%.11s]\n",entry -> filename);
    }
}

int main() {
    FILE * in = fopen("test.img", "rb");
    int i;
    int offset_partition_tables = 0x1BE;

    PartitionTable pt[4];
    Fat12BootSector bs;
    Fat12Entry entry;

     
    fseek(in, offset_partition_tables , SEEK_SET);
    fread(pt, sizeof(PartitionTable), 4, in);
    
    for(i=0; i<4; i++) {        
        if(pt[i].partition_type == 1) {
            printf("Encontrada particion FAT12 %d\n", i);
            break;
        }
    }
    
    if(i == 4) {
        printf("No encontrado filesystem FAT12, saliendo...\n");
        return -1;
    }
    
    fseek(in, 0, SEEK_SET);
    fread(&bs, sizeof(Fat12BootSector), 1, in);
    
    printf("En  0x%lX, sector size %d, FAT size %d sectors, %d FATs\n\n", 
           ftell(in), bs.sector_size, bs.fat_size, bs.num_fats);
           
    fseek(in, (bs.reserved_sectors-1 + bs.fat_size * bs.num_fats) * bs.sector_size, SEEK_CUR);
    
    printf("Root dir_entries %d \n", bs.root_entries);
    for(i=0; i<bs.root_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);
        print_file_info(&entry);
    }
    
    printf("\nLeido Root directory, ahora en 0x%lX\n", ftell(in));
    fclose(in);
    return 0;
}
