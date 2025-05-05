#include <stdbool.h>
#include <stdio.h>

bool is_end_of_chain(int cluster);
unsigned short get_next_cluster(int start_sector, int fat_start, int cluster, FILE* in);