#include "fat.h"
#include "fat_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

Fat16Entry current_directory_entry;
char current_directory[1024] = "/root";
int current_directory_offset = 0;
int root_directory_offset = 0;

void remove_right_including(char *str, char target) {
  char *p = strrchr(str, target);
  if (p) {
      *p = '\0';
  }
}

char* get_attributes(unsigned char attributes)
{
  if (attributes & 0x01) {
    return "<RDO>";
  } else if (attributes & 0x02) {
    return "<HID>";
  } else if (attributes & 0x04) {
    return "<SYS>";
  } else if (attributes & 0x08) {
    return "<LBL>";
  } else if (attributes & 0x10) {
    return "<DIR>";
  } else if (attributes & 0x20) {
    return "<FIL>";
  } else {
    return "<UNK>";
  }
}

void print_entry(Fat16Entry entry)
{
  int hour = (entry.modify_time >> 11) & 0x1F;
  int minute = (entry.modify_time >> 5) & 0x3F;
  int second = (entry.modify_time & 0x1F) * 2;
  int year = ((entry.modify_date >> 9) & 0x7F) + 1980;
  int month = (entry.modify_date >> 5) & 0x0F;
  int day = entry.modify_date & 0x1F;
  
  printf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
  printf(" %.5s %.8s.%.3s %8d B\n", get_attributes(entry.attributes), entry.filename, entry.ext, entry.file_size);
}

void list_files(FILE* in, PartitionTable pt[4], Fat16BootSector bs)
{
  if (in == NULL) 
  {
    printf("File pointer is NULL\n");
    return;
  }
  
  fseek(in, current_directory_offset, SEEK_SET);
  Fat16Entry entry;
  if (current_directory_offset == root_directory_offset) 
  {
    for (int i = 0; i < bs.root_dir_entries; i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00) {
        print_entry(entry);
      }
    }
    return;
  }

  int first_data_sector = (pt[0].start_sector + bs.reserved_sectors + 
    (bs.number_of_fats * bs.fat_size_sectors) + ((bs.root_dir_entries * 32) / bs.sector_size)) * bs.sector_size;
  int fat_start = bs.reserved_sectors * bs.sector_size;
  int cluster_size = bs.sectors_per_cluster * bs.sector_size;
  int current_cluster = current_directory_entry.starting_cluster;
  while (!is_end_of_chain(current_cluster))
  {
    int cluster_offset = first_data_sector + ((current_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
    fseek(in, cluster_offset, SEEK_SET);
    
    for (int i = 0; i < cluster_size / sizeof(Fat16Entry); i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00) {
        print_entry(entry);
      }
    }

    current_cluster = get_next_cluster(pt[0].start_sector, fat_start, current_cluster, in);
  }
}

void change_directory(char* new_directory, FILE* in, PartitionTable pt[4], Fat16BootSector bs)
{
  if (new_directory == NULL) 
  {
    printf("New directory is NULL\n");
    return;
  }
  char name[8];
  memset(name, 0x20, 8);
  if (strlen(new_directory) > 8) 
  {
    printf("Directory name is too long\n");
    return;
  }

  for (int i = 0; i < strlen(new_directory); i++) 
  {
    name[i] = new_directory[i];
  }

  fseek(in, current_directory_offset, SEEK_SET);
  Fat16Entry entry;
  int first_data_sector = (pt[0].start_sector + bs.reserved_sectors + 
    (bs.number_of_fats * bs.fat_size_sectors) + ((bs.root_dir_entries * 32) / bs.sector_size)) * bs.sector_size;
  int fat_start = bs.reserved_sectors * bs.sector_size;
  int cluster_size = bs.sectors_per_cluster * bs.sector_size;
  int current_cluster = current_directory_entry.starting_cluster;

  if (current_directory_offset == root_directory_offset) 
  {
    for (int i = 0; i < bs.root_dir_entries; i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00 && (entry.attributes & 0x10)) 
      {
        if (strncmp(entry.filename, name, 8) == 0) 
        {
          current_directory_entry = entry;
          current_directory_offset = entry.starting_cluster * bs.sector_size;
          snprintf(current_directory + strlen(current_directory),
          sizeof(current_directory) - strlen(current_directory),
          "%c%s", '/', new_directory);
          return;
        }
      }
    }
    return;
  }

  while (!is_end_of_chain(current_cluster))
  {
    int cluster_offset = first_data_sector + ((current_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
    fseek(in, cluster_offset, SEEK_SET);
    
    for (int i = 0; i < cluster_size / sizeof(Fat16Entry); i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00 && (entry.attributes & 0x10)) 
      {
        if (strncmp(entry.filename, name, 8) == 0) 
        {
          current_directory_entry = entry;
          if (entry.starting_cluster == 0) 
          {
            current_directory_offset = root_directory_offset;
            remove_right_including(current_directory, '/');
          } 
          else 
          {
            current_directory_offset = entry.starting_cluster * bs.sector_size;
            if (current_directory_offset == entry.starting_cluster * bs.sector_size) 
            {
              return;
            }

            snprintf(current_directory + strlen(current_directory),
            sizeof(current_directory) - strlen(current_directory),
            "%c%s", '/', new_directory);
          }
          return;
        }
      }
    }
    current_cluster = get_next_cluster(pt[0].start_sector, fat_start, current_cluster, in);
  }

  return;
}

void read(char* filename, FILE* in, PartitionTable pt[4], Fat16BootSector bs)
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
  int first_data_sector = (pt[0].start_sector + bs.reserved_sectors + 
    (bs.number_of_fats * bs.fat_size_sectors) + ((bs.root_dir_entries * 32) / bs.sector_size)) * bs.sector_size;
  int fat_start = bs.reserved_sectors * bs.sector_size;
  int cluster_size = bs.sectors_per_cluster * bs.sector_size;
  
  Fat16Entry entry;
  fseek(in, current_directory_offset, SEEK_SET);
  if (current_directory_offset == root_directory_offset) 
  {
    for (int i = 0; i < bs.root_dir_entries; i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00 && strncmp(entry.filename, namecmp, strlen(entry.filename)) == 0) 
      {
        int current_cluster = entry.starting_cluster;
        int cluster_data[cluster_size];
        size_t bytes_read = 0;
        while (current_cluster < 0xFFF8 && bytes_read < entry.file_size) // 0xFFF8 is end of chain marker
        {
          int cluster_offset = first_data_sector + ((current_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
          fseek(in, cluster_offset, SEEK_SET);
          
          size_t bytes_to_read = (entry.file_size - bytes_read < cluster_size) ? 
                                entry.file_size - bytes_read : cluster_size;
          fread(cluster_data, 1, bytes_to_read, in);
          
          fwrite(cluster_data, 1, bytes_to_read, stderr);
          bytes_read += bytes_to_read;
          
          current_cluster = get_next_cluster(pt[0].start_sector, fat_start, current_cluster, in);
        }
        return;
      }
    }
    printf("File not found\n");
    return;
  }

  int current_cluster = current_directory_entry.starting_cluster;
  while (!is_end_of_chain(current_cluster)) // 0xFFF8 is end of chain marker
  {
    int cluster_offset = first_data_sector + ((current_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
    fseek(in, cluster_offset, SEEK_SET);
    
    for (int i = 0; i < cluster_size / sizeof(Fat16Entry); i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00 && strncmp(entry.filename, namecmp, strlen(entry.filename)) == 0) 
      {
        int current_entry_cluster = entry.starting_cluster;
        int cluster_data[cluster_size];
        size_t bytes_read = 0;
        while (!is_end_of_chain(current_entry_cluster) && bytes_read < entry.file_size) // 0xFFF8 is end of chain marker
        {
          int cluster_offset = first_data_sector + ((current_entry_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
          fseek(in, cluster_offset, SEEK_SET);
          
          size_t bytes_to_read = (entry.file_size - bytes_read < cluster_size) ? 
                                entry.file_size - bytes_read : cluster_size;
          fread(cluster_data, 1, bytes_to_read, in);
          
          fwrite(cluster_data, 1, bytes_to_read, stderr);
          bytes_read += bytes_to_read;
          
          current_entry_cluster = get_next_cluster(pt[0].start_sector, fat_start, current_entry_cluster, in);
        }
        return;
      }
    }
    
    current_cluster = get_next_cluster(pt[0].start_sector, fat_start, current_cluster, in);
  }
  printf("File not found\n");
  return;
}

void print_info(FILE* in, PartitionTable pt[4], Fat16BootSector bs)
{
  int i;

  printf("Partition table\n-----------------------\n");
  for (i = 0; i < 4; i++) { // for all partition entries print basic info
    printf("Partition %d, type %02X, ", i, pt[i].partition_type);
    printf("start sector %8d, length %8d sectors\n", pt[i].start_sector, pt[i].length_sectors);
  }

  printf("Volume_label %.11s, %d sectors size\n", bs.volume_label, bs.sector_size);

  fseek(in, root_directory_offset, SEEK_SET);
  Fat16Entry entry;
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
}

void print_tree(Fat16Entry* directory, int depth, FILE* in, PartitionTable pt[4], Fat16BootSector bs)
{
  Fat16Entry entry;
  depth++;
  if (directory == NULL)
  {
    fseek(in, root_directory_offset, SEEK_SET);
    printf("/root\n");
    for (int i = 0; i < bs.root_dir_entries; i++) 
    {
      fread(&entry, sizeof(entry), 1, in);
      if (entry.filename[0] != 0x00) 
      {
        printf("|\t%.8s.%.3s\n", entry.filename, entry.ext);
        if (entry.attributes & 0x10)
        {
          print_tree(&entry, depth, in, pt, bs);
          fseek(in, root_directory_offset + (i + 1) * sizeof(entry), SEEK_SET);
        }
      }
    }
  }
  else
  {
    int first_data_sector = (pt[0].start_sector + bs.reserved_sectors + 
      (bs.number_of_fats * bs.fat_size_sectors) + ((bs.root_dir_entries * 32) / bs.sector_size)) * bs.sector_size;
    int fat_start = bs.reserved_sectors * bs.sector_size;
    int cluster_size = bs.sectors_per_cluster * bs.sector_size;
    int current_cluster = directory->starting_cluster;
    while (!is_end_of_chain(current_cluster))
    {
      int cluster_offset = first_data_sector + ((current_cluster - 2) * bs.sectors_per_cluster) * bs.sector_size;
      fseek(in, cluster_offset, SEEK_SET);
      
      for (int i = 0; i < cluster_size / sizeof(Fat16Entry); i++) 
      {
        fread(&entry, sizeof(entry), 1, in);
        if (entry.filename[0] != 0x00 &&
            entry.starting_cluster != directory->starting_cluster
          ) 
        {
          if (entry.attributes & 0x10 && entry.starting_cluster == 0)
            continue;
          for (int j = 0; j < depth; j++)
            printf("|\t");
          printf("|\t%.8s.%.3s\n", entry.filename, entry.ext);
          if (entry.attributes & 0x10 &&
            entry.starting_cluster != 0)
          {
            print_tree(&entry, depth, in, pt, bs);
            fseek(in, cluster_offset + (i + 1) * sizeof(entry), SEEK_SET);
          }
        }
      }
  
      current_cluster = get_next_cluster(pt[0].start_sector, fat_start, current_cluster, in);
    }
  }
}

void write()
{

}

void delete()
{

}

int run(FILE* in, PartitionTable pt[4], Fat16BootSector bs)
{
  fseek(in, root_directory_offset, SEEK_SET);
  while (true)
  {
    char *cmd = NULL;
    size_t len = 0;
    printf("\033[32mfat\033[0m:\033[32m%s\033[0m$ ", current_directory);
    getline(&cmd, &len, stdin);
    cmd[strcspn(cmd, "\n")] = 0; // Remove newline character
    if (strcmp(cmd, "exit") == 0) 
    {
      return 0;
    }
    else if (strcmp(cmd, "info") == 0) 
    {
      print_info(in, pt, bs);
    }
    else if (strcmp(cmd, "ls") == 0) 
    {
      list_files(in, pt, bs);
    }
    else if (strncmp(cmd, "read", 4) == 0) 
    {
      cmd = cmd + 5;
      if (strlen(cmd) == 0) 
      {
        printf("Filename is empty\n");
        continue;
      }
      read(cmd, in, pt, bs);
    }
    else if(strncmp(cmd, "cd", 2) == 0)
    {
      cmd = cmd + 3;
      if (strlen(cmd) == 0) 
      {
        printf("Directory name is empty\n");
        continue;
      }
      change_directory(cmd, in, pt, bs);
    }
    else if(strcmp(cmd, "tree") == 0)
    { 
      print_tree(NULL, -1, in, pt, bs);
    }
    else if(strcmp(cmd, "help") == 0)
    {
      printf("Available commands:\n");
      printf("\thelp - show this help\n");
      printf("\tinfo - show partition table and root directory\n");
      printf("\tls - list files in current directory\n");
      printf("\tread <filename_on_FAT> - read file from root directory\n");
      printf("\tcd <directory> - change directory\n");
      printf("\ttree - show directory tree\n");
      printf("\tdelete <filename_on_FAT> - delete file (not implemented)\n");
      printf("\texit - exit the program\n");
    }
  }
  return 1;
}


int main(int argc, char *argv[]) 
{
  if (argc > 1)
  {
    for (int i = 1; i < argc; i++)
    {
      if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--write") == 0)
      {
        printf("Writing to FAT16\n");
        write();
        return 0;
      }
    }
    return 1;
  }


  FILE* in = fopen("sd.img", "rb");
  PartitionTable pt[4];
  Fat16BootSector bs;

  fseek(in, 0x1BE, SEEK_SET);                // go to partition table start, partitions start at offset 0x1BE, see http://www.cse.scu.edu/~tschwarz/coen252_07Fall/Lectures/HDPartitions.html
  fread(pt, sizeof(PartitionTable), 4, in);  // read all entries (4)

  fseek(in, 512 * pt[0].start_sector, SEEK_SET); // Boot sector starts here (seek in bytes)
  fread(&bs, sizeof(Fat16BootSector), 1, in);    // Read boot sector content, see http://www.tavi.co.uk/phobos/fat.html#boot_block

  // Seek to the beginning of root directory, it's position is fixed
  fseek(in, (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);
  current_directory_offset = ftell(in);
  root_directory_offset = current_directory_offset;


  print_tree(NULL, -1, in, pt, bs);
  return run(in, pt, bs);
}
