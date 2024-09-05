#include <stdio.h>
#include <stdlib.h>

typedef struct {
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    char start_sector[4];
    char length_sectors[4];
} __attribute((packed)) PartitionTable;

typedef struct {
    unsigned char jmp[3];
    char oem[8];
    unsigned short sector_size; // 2 bytes
	// COMPLETAMOS EL RESTO DE DATOS DEL MBR
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
    // 
    unsigned int volume_id;
    char volume_label[11];
    char fs_type[8]; // Type en ascii
    char boot_code[448];
    unsigned short boot_sector_signature;
} __attribute((packed)) Fat12BootSector;

int main() {
    FILE * in = fopen("test.img", "rb");
    int i;
    PartitionTable pt[4];
    Fat12BootSector bs;
    
    fseek(in, 0x1BE , SEEK_SET); // Ir al inicio de la tabla de particiones -> 0x1BE
    fread(pt, sizeof(PartitionTable), 4, in); // leo entradas 
    
    for(i=0; i<4; i++) {        
        printf("Partiion type: %d\n", pt[i].partition_type);
        if(pt[i].partition_type == 1) {
            printf("Encontrado FAT12 %d\n", i);
            break;
        }
    }
    
    if(i == 4) {
        printf("No se encontró filesystem FAT12, saliendo ...\n");
        return -1;
    }
    
    fseek(in, 0, SEEK_SET);
    fread(&bs, sizeof(Fat12BootSector), 1, in);
    
    printf("  Jump code: %02X:%02X:%02X\n", bs.jmp[0], bs.jmp[1], bs.jmp[2]);
    printf("  OEM code: [%.8s]\n", bs.oem);
    printf("  sector_size: %d\n", bs.sector_size);
    // COMPLETAMOS los prints de los datos del MBR faltantes
    printf("  Sectors per cluster: %d\n", bs.cluster_size);
    printf("  Reserved sectors: %d\n", bs.reserved_sectors);
    printf("  Number of FATs: %d\n", bs.num_fats);
    printf("  Maximum number of files in the Root Directory: %d\n", bs.root_entries);
    printf("  Total sectors : %d\n", bs.total_sectors);
    printf("  Media type: %02X\n", bs.media_type);
    printf("  FAT size : %d\n", bs.fat_size);
    printf("  Sectors per track: %d\n", bs.sectors_per_track);
    printf("  Number of heads: %d\n", bs.num_heads);
    printf("  Number of sectors before the start partition: %d\n", bs.hidden_sectors);
    printf("  Total sectors (large): %d\n", bs.total_sectors_large);
    printf("  Drive number: %02X\n", bs.drive_number);
    printf("  Boot signature: %02X\n", bs.boot_signature);
    //
    printf("  volume_id: 0x%08X\n", bs.volume_id);
    printf("  Volume label: [%.11s]\n", bs.volume_label);
    printf("  Filesystem type: [%.8s]\n", bs.fs_type);
    printf("  Boot sector signature: 0x%04X\n", bs.boot_sector_signature);
    
    fclose(in);
    return 0;
}
