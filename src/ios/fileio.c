#include "driver.h"
#include "unzip.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>

#include "ios.h"

#define MAX_OPEN_FILES		  16
#define FILE_BUFFER_SIZE	  256
#define INVALID_HANDLE_VALUE  -1

char* rompath_extra;

/**
 * Type Definitions
 */

struct pathdata {
	const char* rawpath;
	const char** path;
	int pathcount;
};

struct _osd_file {
	int		        handle;
	UINT64		    filepos;
	UINT64		    end;
	UINT64		    offset;
	UINT64		    bufferbase;
	unsigned long	bufferbytes;
	UINT8		    buffer[FILE_BUFFER_SIZE];
};

static osd_file openfile[MAX_OPEN_FILES];
static struct pathdata pathlist[FILETYPE_end];

/**
 * set_pathlist
 */

void set_pathlist(int file_type, const char* new_rawpath) {
	struct pathdata *list = &pathlist[file_type];
    
	if (list->pathcount != 0) {
		int pathindex;
        
		for (pathindex = 0; pathindex < list->pathcount; pathindex++) {
			free((void *)list->path[pathindex]);
        }
        
		free((void *)list->path);
	}
    
	list->path = NULL;
	list->pathcount = 0;
    
	list->rawpath = new_rawpath;    
}

/**
 * is_pathsep
 */

INLINE int is_pathsep(char c) {
	return (c == '/' || c == '\\' || c == ':');
}

/**
 * find_reverse_path_sep
 */

static char* find_reverse_path_sep(char* name) {
	char* p = name + strlen(name) - 1;
	while (p >= name && !is_pathsep(*p)) {
		p--;
    }
	return (p >= name) ? p : NULL;
}

/**
 * create_path
 */

static void create_path(char* path, int has_filename) {
    struct stat st;

	char* sep = find_reverse_path_sep(path);
    
    if (sep && sep > path && !is_pathsep(sep[-1])) {
		*sep = 0;
		create_path(path, 0);
		*sep = '/';
	}
    
	if (has_filename) {
		return;
    }
    
    if (!stat(path, &st)) {
        return;
    }
    
    ipinmame_logger("create_path(): creating path - path=%s, has_filename=%d", path, has_filename);
    
    mkdir(path, 0777);
}

/**
 * is_variablechar
 */

INLINE int is_variablechar(char c) {
    return (isalnum(c) || c == '_' || c == '-');
}

/**
 * parse_variable
 */

static const char* parse_variable(const char** start, const char* end) {
	const char* src = *start;
    const char* var;
    
	char variable[1024];
	char *dest = variable;
    
	for (src = *start; src < end && is_variablechar(*src); src++) {
		*dest++ = *src;
    }
    
	if(src == *start) {
		return("$");
    }
    
	*dest = 0;
	*start = src;
    
	var = getenv(variable);
	return (var) ? var : "";
}

/**
 * copy_and_expand_variables
 */

static char* copy_and_expand_variables(const char* path, int len) {
	char *dst, *result;
	const char *src;
	int length = 0;
    
	for (src = path; src < path + len; ) {
		if (*src++ == '$') {
			length += strlen(parse_variable(&src, path + len));
        }
		else {
			length++;
        }
    }
    
	result = malloc(length + 1);

	if (!result) {
        exit(1);
    }
    
	for (src = path, dst = result; src < path + len;) {
		char c = *src++;
		if (c == '$') {
			dst += sprintf(dst, "%s", parse_variable(&src, path + len));
        }
		else {
			*dst++ = c;
        }
	}
    
	*dst = 0;
	return result;
}

/**
 * expand_pathlist
 */

static void expand_pathlist(struct pathdata *list) {
	const char *rawpath = (list->rawpath) ? list->rawpath : "";
	const char *token;
    
    if (list->pathcount != 0) {
		int pathindex;
        
		for (pathindex = 0; pathindex < list->pathcount; pathindex++) {
			free((void *)list->path[pathindex]);
        }
        
		free((void *)list->path);
	}
    
	list->path = NULL;
	list->pathcount = 0;
    
	token = strchr(rawpath, ';');

	if (!token) {
		token = rawpath + strlen(rawpath);
    }
    
	while (1) {
		list->path = realloc((void *)list->path, (list->pathcount + 1) * sizeof(char *));
		if (!list->path) {
            exit(1);
        }
        
		list->path[list->pathcount++] = copy_and_expand_variables(rawpath, token - rawpath);
        
		if (*token == 0) {
			break;
        }
        
		rawpath = token + 1;
        
		token = strchr(rawpath, ';');

		if (!token) {
			token = rawpath + strlen(rawpath);
        }
	}
    
	return;    
}

/**
 * get_path_for_filetype
 */

static const char* get_path_for_filetype(int filetype, int pathindex, UINT32* count) {
	struct pathdata* list;
    
	switch (filetype) {
#ifndef MESS
		case FILETYPE_IMAGE:
			list = &pathlist[FILETYPE_ROM];
			break;
#endif
            
		default:
			list = &pathlist[filetype];
			break;
	}
    
	if (list->pathcount == 0 || list->rawpath) {
		if (list == &pathlist[FILETYPE_ROM] && rompath_extra) {
			const char* rawpath = (list->rawpath) ? list->rawpath : "";
			char* newpath = malloc(strlen(rompath_extra) + strlen(rawpath) + 2);
			sprintf(newpath, "%s;%s", rompath_extra, rawpath);
			list->rawpath = newpath;
		}
        
		expand_pathlist(list);
	}
    
	if (count) {
		*count = list->pathcount;
    }
    
	return (pathindex < list->pathcount) ? list->path[pathindex] : "";
}

/**
 * compose_path
 */
 
static void compose_path(char* output, int pathtype, int pathindex, const char* filename) {
	const char* basepath = get_path_for_filetype(pathtype, pathindex, NULL);
	char* p;
    
	*output = 0;

	if (basepath) {
        strcat(output, basepath);
    }
    
	if (*output && !is_pathsep(output[strlen(output) - 1])) {
        strcat(output,  "/");
    }
    
    strcat(output, filename);
    
	for (p = output; *p; p++) {
		if (*p == '\\') {
			*p = '/';
        }
    }
}

/**
 * osd_display_loading_rom_message
 */

int osd_display_loading_rom_message(const char* name,struct rom_load_data *romdata) {
    if (name) {
		ipinmame_logger("osd_display_loading_rom_message(): loading %-12s...", name);
    }
	else {
		ipinmame_logger("osd_display_loading_rom_message():");
    }
    
    return 0;
}

/**
 * osd_get_path_count
 */

int osd_get_path_count(int pathtype) {
	UINT32 count;
    
	get_path_for_filetype(pathtype, 0, &count);
	
    return (int)count;
}

/**
 * osd_get_path_info
 */

int osd_get_path_info(int pathtype, int pathindex, const char *filename) {
    struct stat st;
    char fullpath[1024];
    
    compose_path(fullpath, pathtype, pathindex, filename);
           
    if (stat(fullpath, &st) != 0) {
        return PATH_NOT_FOUND;
    }
    
    if (S_ISDIR(st.st_mode)) {
       return PATH_IS_DIRECTORY;
    }
    
    return PATH_IS_FILE;
}

/**
 * osd_fopen
 */

osd_file* osd_fopen(int pathtype, int pathindex, const char *filename, const char *mode) {
    UINT32 access;
    osd_file *file;
    int i;
    struct stat st;
    char fullpath[1024];
    
    for (i = 0; i < MAX_OPEN_FILES; i++) {
        if (openfile[i].handle == (int)NULL || openfile[i].handle == INVALID_HANDLE_VALUE) {
            break;
        }
    }
    
    if (i == MAX_OPEN_FILES) {
        return NULL;
    }
    
    file = &openfile[i];
    memset(file, 0, sizeof(*file));
    
    if (strchr(mode, 'r')) {
        access = O_RDONLY; 
    }
    else if (strchr(mode, 'w')) {
        access = O_WRONLY; 
        access |= (O_CREAT | O_TRUNC);
    }
    else if (strchr(mode, '+')) {
        access = O_RDWR;
    }
    
   	compose_path(fullpath, pathtype, pathindex, filename);    
    ipinmame_logger("osd_fopen(): access=%08X, fullpath=%s", access, fullpath);
    
    stat(fullpath, &st);
    
    file->handle = open(fullpath, access, 0666);
    
    if (file->handle == INVALID_HANDLE_VALUE) {
        if (!(access & O_WRONLY) || errno != ENOENT) {
            ipinmame_logger("osd_fopen(): unable to open");
            return NULL;
        }
        
        create_path(fullpath, 1);
        file->handle = open(fullpath, access, 0666);
        
        if (file->handle == INVALID_HANDLE_VALUE) {
            ipinmame_logger("osd_fopen(): unable to open");
            return NULL;
        }
    }
    
    fstat(file->handle, &st);
    file->end = st.st_size;
    
    return file;    
}

/**
 * osd_fclose
 */

void osd_fclose(osd_file *file) {
    if (file->handle) {
        close(file->handle);
    }
    file->handle = (int)NULL;
}

/**
 * osd_fread
 */

UINT32 osd_fread(osd_file *file, void *buffer, UINT32 length) {
	UINT32 bytes_left = length;
	int bytes_to_copy;
	UINT32 result;
    
	if (file->offset >= file->bufferbase && file->offset < file->bufferbase + file->bufferbytes) {
		bytes_to_copy = file->bufferbase + file->bufferbytes - file->offset;
		
        if (bytes_to_copy > length) {
			bytes_to_copy = length;
        }
        
		memcpy(buffer, &file->buffer[file->offset - file->bufferbase], bytes_to_copy);
        
		bytes_left -= bytes_to_copy;
		file->offset += bytes_to_copy;
		buffer = (UINT8 *)buffer + bytes_to_copy;
        
		if (bytes_left == 0) {
			return length;
        }
	}
    
	if (file->offset != file->filepos) {
        if (lseek(file->handle, file->offset, SEEK_SET) == -1) {
            file->filepos = ~0;
            return length - bytes_left;
        }
		file->filepos = file->offset;
	}
    
	if (length < FILE_BUFFER_SIZE / 2) {
		file->bufferbase = file->offset;		        
        file->bufferbytes = read(file->handle, file->buffer, FILE_BUFFER_SIZE);
        
        file->filepos += file->bufferbytes;
        
		bytes_to_copy = bytes_left;
		
        if (bytes_to_copy > file->bufferbytes) {
			bytes_to_copy = file->bufferbytes;
        }
        
		memcpy(buffer, file->buffer, bytes_to_copy);
        
		file->offset += bytes_to_copy;
		bytes_left -= bytes_to_copy;
		return length - bytes_left;
	}
	else {
        result = read(file->handle, buffer, bytes_left);
		file->filepos += result;
        
		file->offset += result;
		bytes_left -= result;
		return length - bytes_left;
	}
}

/**
 * osd_fwrite
 */

UINT32 osd_fwrite(osd_file *file, const void *buffer, UINT32 length) {
	UINT32 result;
    
    file->bufferbytes = 0;
    
    if (lseek(file->handle, file->offset, SEEK_SET) == -1) {
        return 0;
    }
    
    result = write(file->handle, buffer, length);
    
    file->filepos += result;
    file->offset += result;
    
    if (file->offset > file->end) {
        file->end = file->offset;
    }
    
    return result;
}


UINT64 osd_fsize(osd_file *file)
{
	return file->end;
}


/**
 * osd_fseek
 */

int osd_fseek(osd_file *file, INT64 offset, int whence) {
	switch (whence) {
		default:
		case SEEK_SET:	
            file->offset = offset;		
            break;
		case SEEK_CUR:	
            file->offset += offset;
            break;
		case SEEK_END:
            file->offset = file->end + offset;
            break;
	}
	return 0;
}

/**
 * osd_ftell
 */

UINT64 osd_ftell(osd_file *file) {
    return file->offset;
}

/**
 * osd_feof
 */

int osd_feof(osd_file *file) {
	return (file->offset >= file->end);
}
