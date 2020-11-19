#pragma once

#include "ff.h"
#include <stddef.h> // size_t get size of any object
#include <string.h>

typedef char song_memory_t[128];
typedef char song_memory_t1[128];
typedef char song_memory_t2[128];

/* Do not declare variables in a header file */
#if 0
static song_memory_t list_of_songs[32];
static size_t number_of_songs;
#endif

void song_list__populate(void);
void open_directory_READ(void);
size_t song_list__get_item_count(void);
size_t song_list__get_item_count2(void);
const char *song_list__get_name_for_item(size_t item_number);
const char *get_songName_on_INDEX(size_t item_number);