#include "fat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* get_attributes(unsigned char attributes)
{
  if (attributes & 0x01) {
    return "<RDO>";
  } else if (attributes & 0x02) {
    return "<HID>";
  } else if (attributes & 0x04) {
    return "<SYS>>";
  } else if (attributes & 0x08) {
    return "<VOL>";
  } else if (attributes & 0x10) {
    return "<DIR>";
  } else if (attributes & 0x20) {
    return "<FIL>";
  } else {
    return "<UNK>";
  }
}

void read(char* filename)
{
  if (filename == NULL) 
  {
    printf("Filename is NULL\n");
    return;
  }

  char* name = strtok(filename, ".");
  char* ext = strtok(NULL, ".");
  char namecmp[12];
  memset(namecmp, 0x20, 12);
  for (int i = 0; i < 8; i++) 
  {
    if (i < strlen(name)) 
    {
      namecmp[i] = name[i];
    } 
    else 
    {
      namecmp[i] = ' ';
    }
  }
  if (ext != NULL) 
  {
    for (int i = 0; i < 3; i++) 
    {
      if (i < strlen(ext)) 
      {
        namecmp[i + 8] = ext[i];
      } 
      else 
      {
        namecmp[i + 8] = ' ';
      }
    }
  }
  
  FILE* in = fopen("sd.img", "rb");
  int i;
  PartitionTable pt[4];
  Fat16BootSector bs;
  Fat16Entry entry;

  fseek(in, 0x1BE, SEEK_SET);                // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
  fread(pt, sizeof(PartitionTable), 4, in);  // read all entries (4)
  fseek(in, 512 * pt[0].start_sector, SEEK_SET); // Boot sector starts here (seek in bytes)
  fread(&bs, sizeof(Fat16BootSector), 1, in);    // Read boot sector content, see http://www.tavi.co.uk/phobos/fat.html#boot_block
  fseek(in, (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);
  int root_loc = ftell(in);

  int first_data_sector = (pt[0].start_sector + bs.reserved_sectors + 
    (bs.number_of_fats * bs.fat_size_sectors) + ((bs.root_dir_entries * 32) / bs.sector_size)) * bs.sector_size;
  int fat_start = bs.reserved_sectors * bs.sector_size;

  for (i = 0; i < bs.root_dir_entries; i++) 
  {
    fread(&entry, sizeof(entry), 1, in);
    if (entry.filename[0] != 0x00 && strncmp(entry.filename, namecmp, strlen(entry.filename)) == 0) 
    {
      int cluster_size = bs.sectors_per_cluster * bs.sector_size;

      int current_cluster = entry.starting_cluster;
      int cluster_data[cluster_size];
      size_t bytes_read = 0;
      while (current_cluster < 0xFFF8 && bytes_read < entry.file_size) // 0xFFF8 is end of chain marker
      {
        int cluster_offset = first_data_sector + ((current_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
        fseek(in, cluster_offset, SEEK_SET);
        
        // Read cluster data
        size_t bytes_to_read = (entry.file_size - bytes_read < cluster_size) ? 
                              entry.file_size - bytes_read : cluster_size;
        fread(cluster_data, 1, bytes_to_read, in);
        
        fwrite(cluster_data, 1, bytes_to_read, stdout);
        bytes_read += bytes_to_read;
        
        int fat_entry_offset = (pt[0].start_sector * 512) + fat_start + (current_cluster * 2);
        fseek(in, fat_entry_offset, SEEK_SET);
        int fat_entry[2];
        fread(fat_entry, 1, 2, in);
        current_cluster = fat_entry[0] | (fat_entry[1] << 8);
      }
      return;
    }
  }
  printf("File not found\n");
}

void print_info()
{
  FILE* in = fopen("sd.img", "rb");
  int i;
  PartitionTable pt[4];
  Fat16BootSector bs;
  Fat16Entry entry;

  fseek(in, 0x1BE, SEEK_SET);                // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
  fread(pt, sizeof(PartitionTable), 4, in);  // read all entries (4)
 
  printf("Partition table\n-----------------------\n");
  for (i = 0; i < 4; i++) { // for all partition entries print basic info
    printf("Partition %d, type %02X, ", i, pt[i].partition_type);
    printf("start sector %8d, length %8d sectors\n", pt[i].start_sector, pt[i].length_sectors);
  }

  printf("\nSeeking to first partition by %d sectors\n", pt[0].start_sector);
  fseek(in, 512 * pt[0].start_sector, SEEK_SET); // Boot sector starts here (seek in bytes)
  fread(&bs, sizeof(Fat16BootSector), 1, in);    // Read boot sector content, see http://www.tavi.co.uk/phobos/fat.html#boot_block
  printf("Volume_label %.11s, %d sectors size\n", bs.volume_label, bs.sector_size);

  // Seek to the beginning of root directory, it's position is fixed
  fseek(in, (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);

  // Read all entries of root directory
  printf("\nFilesystem root directory listing\n-----------------------\n");
  for (i = 0; i < bs.root_dir_entries; i++) {
    fread(&entry, sizeof(entry), 1, in);
    // Skip if filename was never used, see http://www.tavi.co.uk/phobos/fat.html#file_attributes
    if (entry.filename[0] != 0x00) {
      int hour = (entry.modify_time >> 11) & 0x1F;
      int minute = (entry.modify_time >> 5) & 0x3F;
      int second = (entry.modify_time & 0x1F) * 2;
      int year = ((entry.modify_date >> 9) & 0x7F) + 1980;
      int month = (entry.modify_date >> 5) & 0x0F;
      int day = entry.modify_date & 0x1F;
      printf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
      printf(" %.5s %.8s.%.3s %8d B\n", get_attributes(entry.attributes), entry.filename, entry.ext, entry.file_size);
    }
  }

  fclose(in);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Usage: %s -l | -r <filename>\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "-l") == 0) 
  {
    print_info();
  } 
  else if (strcmp(argv[1], "-r") == 0) 
  {
    if (argc < 3) 
    {
      printf("Error: No filename provided\n");
      return 1;
    }
    read(argv[2]);
  } 
  else 
  {
    printf("Invalid option. Use -l or -r <filename>\n");
    return 1;
  }

  return 0;
}
