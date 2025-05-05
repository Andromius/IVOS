#include "fat_helpers.h"

bool is_end_of_chain(int cluster)
{
    return cluster >= 0xFFFF;
}

unsigned short get_next_cluster(int start_sector, int fat_start, int cluster, FILE *in)
{
    int fat_entry_offset = (start_sector * 512) + fat_start + (cluster * 2);
    fseek(in, fat_entry_offset, SEEK_SET);
    unsigned short next;
    fread(&next, sizeof(unsigned short), 1, in);
    return next;
}
