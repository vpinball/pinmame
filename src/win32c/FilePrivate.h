/* FilePrivate.h - MSH 12/19/98
 *
 * Private File structures used in File.c
 * Exposed to allow custom JPEG data source with zipfiles
 */

#ifndef FILEPRIVATE_H
#define FILEPRIVATE_H

#include "file.h"

typedef struct
{
    int index; /* into file_list, to keep track of left open files */
    int access_type;
    unsigned int crc;
    
    /* ACCESS_FILE */
    FILE *fptr;
    
    /* ACCESS_ZIP */
    char *file_data;
    int file_length;
    int file_offset;
    
} mame_file;

enum
{
    ACCESS_FILE = 1,
    ACCESS_ZIP  = 2,
    ACCESS_RAMFILE,
};

#endif