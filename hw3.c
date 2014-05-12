/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

  This program can be distributed under the terms of the GNU GPL.
  See the file COPYING.

  gcc -Wall hello.c `pkg-config fuse --cflags --libs` -o hello
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>

static const char *hello_str = "Hello World!\n1234567890_1234567890_1234567890\n";
static const char *myproc_path = "/myproc";
#define BUFF_SIZE 100000

void listdir(const char *name, int level, void *buf, fuse_fill_dir_t filler);
int checkNum(const char *p);
char* readProcInfo(const char *filename);

static int hello_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, myproc_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	} else if (checkNum(path)) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(readProcInfo(path));
	} 
	else
		res = -ENOENT;

	return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{

	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	// filler(buf, myproc_path + 1, NULL, 0);

	// char *temp_path = "1143";
	// filler(buf, temp_path + 1, NULL, 0);

	char *proc_path = "/proc/";
	listdir(proc_path, 0, buf, filler);

	return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
	if (!checkNum(path + 1)) return -ENOENT;
	// if (strcmp(path, myproc_path) != 0) return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	
	if(checkNum(path + 1)) {
	    const char *p = readProcInfo(path);

	    len = strlen(p);
	    // len = strlen(hello_str);
	    
	    if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, p + offset, size);
	  	// memcpy(buf, hello_str + offset, size);
	    } 
	    else {
		size = 0;
	    }
	    
	    return size;
	}
	else {
	    return 0;
	}
	
	// if(strcmp(path, myproc_path) != 0) return -ENOENT;
	/*
	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
	*/
}

static struct fuse_operations hello_oper = {
	.getattr	= hello_getattr,
	.readdir	= hello_readdir,
	.open		= hello_open,
	.read		= hello_read,
};

// get the content of given direcotry
void listdir(const char *name, int level, void *buf, fuse_fill_dir_t filler) {
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

	    // Get the process directory
	    if (checkNum(entry->d_name)) {
		filler(buf, entry->d_name, NULL, 0);
		// printf("%*s[%s], %d\n", level*2, "", entry->d_name, checkNum(entry->d_name));
	    }
            // listdir(path, level + 1);
        }
        else {
            // printf("%*s- %s\n", level*2, "", entry->d_name);
	}
    } while (entry = readdir(dir));
    closedir(dir);
}

// Check if the string is consisted of numbers
int checkNum(const char *p) {
    while(*p) {
	if (*p != '/' && (*p < '0' || *p > '9')) return 0;
	else p++;
    }
    return 1;
}

// read process info
char* readProcInfo(const char *filename) {
    char full_path[100] = "";

    const char *path = "/proc";
    const char *sepreator = "/";
    const char *target = "status";

    strcat(full_path, path);
    strcat(full_path, filename);
    strcat(full_path, sepreator);
    strcat(full_path, target);

 
    char source[BUFF_SIZE + 1];

    FILE *fp = fopen(full_path, "r");
    if (fp != NULL) {
        size_t newLen = fread(source, sizeof(char), BUFF_SIZE, fp);
        if (newLen == 0) {
            fputs("Error reading file", stderr);
        } else {
            source[++newLen] = '\0'; /* Just to be safe. */
        }

        fclose(fp);
    }

    return source;
}

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
