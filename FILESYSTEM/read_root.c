#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define UNUSED_ENTRY 0x00
#define FIRST_BYTE_0xE5 0x05
#define DOT_DIR 0x2E
#define DELETED_FILE 0xE5

#define LONG_FILE_NAME 0x0F
#define DIRECTORY 0x10
#define ARCHIVE 0x20

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
    unsigned char filename[11];
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

typedef struct {
    unsigned char sequence_number;
    unsigned char filename_a[10];
    unsigned char attribute;
    unsigned char reserved_a;
    unsigned char checksum;
    unsigned char filename_b[12];
    unsigned short reserved_b;
    unsigned char filename_c[4];
} __attribute((packed)) Fat12LFNEntry;

//Auxiliar structures for name entries
char full_name[256] = "";  // max name length in FAT12,  255 char + 1 end of string. 
char buff[256] = "";
char nameLFN[14] = ""; 

void print_file_info(Fat12Entry *entry, FILE *f, Fat12BootSector bs);

void getName(char nameLFN[14], Fat12Entry *entry){

    Fat12LFNEntry *entryLFN = (Fat12LFNEntry *) entry;
    
    nameLFN[0]=entryLFN->filename_a[0]; // 5 char UNICODE
    nameLFN[1]=entryLFN->filename_a[2];
    nameLFN[2]=entryLFN->filename_a[4];
    nameLFN[3]=entryLFN->filename_a[6];
    nameLFN[4]=entryLFN->filename_a[8];

    nameLFN[5]=entryLFN->filename_b[0]; 
    nameLFN[6]=entryLFN->filename_b[2];
    nameLFN[7]=entryLFN->filename_b[4];
    nameLFN[8]=entryLFN->filename_b[6];
    nameLFN[9]=entryLFN->filename_b[8];
    nameLFN[10]=entryLFN->filename_b[10];

    nameLFN[11]=entryLFN->filename_c[0];
    nameLFN[12]=entryLFN->filename_c[2];

    nameLFN[13]='\0';
    return;
}


void print_content(Fat12Entry *entry, FILE *f , Fat12BootSector bs){
    
    if(entry -> filename[0] == DELETED_FILE){
        printf("\nSin contenido, intente recuperar el archivo.\n\n");
        return;
    }

    unsigned long file_index = ftell(f);

    int num_cluster = entry -> low_cluster_address;
    int cluster_siz = bs.sector_size * bs.cluster_size;
    char file_content[cluster_siz + 1]; // + 1 end of file
    
    long int offset_content = sizeof(Fat12BootSector) 
                            + (bs.reserved_sectors-1 + bs.fat_size * bs.num_fats) * bs.sector_size
                            +  bs.root_entries*sizeof(Fat12Entry)
                            + ((num_cluster)-2) * cluster_siz; // there are 2 reserved clusters 

    fseek(f, offset_content, SEEK_SET);          
    fread(file_content, entry->file_size, 1, f);
    file_content[entry->file_size] = '\0';
    
    printf("\n%s\n\n", file_content);
    fseek(f, file_index, SEEK_SET);
}

void print_inside_directory(Fat12Entry *entry, FILE *f , Fat12BootSector bs){
    printf("----o----\n\n");

    if(entry -> filename[0] == DELETED_FILE){
        printf("\nSin contenido, intente recuperar el directorio.\n\n");
        return;
    }

    Fat12Entry directory_entry;

    unsigned long directory_index = ftell(f);  
    int cluster_siz = bs.sector_size * bs.cluster_size;
    int num_cluster = entry -> low_cluster_address;
    unsigned short num_entries = cluster_siz / sizeof(Fat12Entry);


    long int offset_content = sizeof(Fat12BootSector) 
                            + (bs.reserved_sectors-1 + bs.fat_size * bs.num_fats) * bs.sector_size
                            +  bs.root_entries*sizeof(Fat12Entry)
                            + ((num_cluster)-2) * cluster_siz; // there are 2 reserved clusters 

    fseek(f, offset_content , SEEK_SET);

    for (int i = 0 ; i < num_entries; i++){
        fread(&directory_entry, sizeof(directory_entry), 1, f);
        print_file_info(&directory_entry, f , bs);
    }

    fseek(f, directory_index, SEEK_SET);
    printf("\n----o----\n");
}

void print_file_info(Fat12Entry *entry, FILE *f , Fat12BootSector bs) {

    bool deleted; 

    switch(entry->filename[0]) {
        case UNUSED_ENTRY:
            return; 
        case DELETED_FILE:
            deleted = true;
            break;
        case FIRST_BYTE_0xE5: 
            break;
        case DOT_DIR: 
            return;
        default:
            deleted = false;
    }

    switch(entry->attribute) {
        case LONG_FILE_NAME:
            strcpy(buff,full_name);
            full_name[0] = '\0';
            getName(nameLFN,entry);
            strcat(full_name,nameLFN);
            strcat(full_name,buff);     
            break;

        case DIRECTORY:
            printf("%s%s\n", deleted ? "[!]Directorio borrado: ?" : "[-]Directorio: ", full_name);
            full_name[0] = '\0';
            print_inside_directory(entry, f, bs);
            break;

        case ARCHIVE:
            printf("%s%s\n", deleted ? "[!]Archivo borrado: ?" : "[*]Archivo: ", full_name);
            full_name[0] = '\0';
            print_content(entry, f, bs);
            break;
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
    
    printf("Root dir_entries %d \n\n", bs.root_entries);
    for(i=0; i<bs.root_entries; i++) {
        fread(&entry, sizeof(entry), 1, in);
        print_file_info(&entry, in , bs);
    }
    
    printf("\nLeido Root directory, ahora en 0x%lX\n", ftell(in));
    fclose(in);
    return 0;
}
