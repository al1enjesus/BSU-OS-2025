#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define BLOCKSIZE 512
#define NAMESIZE 100

typedef struct {
    char name[NAMESIZE];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag[1];
    char linkname[NAMESIZE];
} tar_header_t;

typedef struct tar_file {
    char path[1024];
    off_t offset;
    size_t size;
    mode_t mode;
    time_t mtime;
    struct tar_file *next;
} tar_file_t;

static int tar_fd = -1;
static tar_file_t *file_list = NULL;
static pthread_mutex_t file_list_mutex = PTHREAD_MUTEX_INITIALIZER;


static size_t tar_parse_octal(const char *str, int len) {
    size_t result = 0;
    for (int i = 0; i < len && str[i] != '\0' && str[i] != ' '; i++) {
        if (str[i] >= '0' && str[i] <= '7') {
            result = result * 8 + (str[i] - '0');
        }
    }
    return result;
}


static void archive_init(const char *archive_path) {
    tar_fd = open(archive_path, O_RDONLY);
    if (tar_fd == -1) {
        perror("open archive");
        return;
    }
    
    tar_header_t header;
    off_t offset = 0;
    
    while (1) {
        ssize_t res = pread(tar_fd, &header, BLOCKSIZE, offset);
        if (res < BLOCKSIZE) break;
        
        int is_empty = 1;
        for (int i = 0; i < BLOCKSIZE; i++) {
            if (((char *)&header)[i] != '\0') {
                is_empty = 0;
                break;
            }
        }
        if (is_empty) break;
        
        if (header.typeflag[0] == 'x' || header.typeflag[0] == 'g') {
            size_t file_size = tar_parse_octal(header.size, sizeof(header.size));
            offset += BLOCKSIZE + ((file_size + BLOCKSIZE - 1) / BLOCKSIZE) * BLOCKSIZE;
            continue;
        }
        

       if (header.typeflag[0] == '0' || header.typeflag[0] == '\0') {

            tar_file_t *file = malloc(sizeof(tar_file_t));
            strncpy(file->path, header.name, sizeof(file->path) - 1);
            file->path[sizeof(file->path) - 1] = '\0';
            
            file->offset = offset + BLOCKSIZE;
            file->size = tar_parse_octal(header.size, sizeof(header.size));
            file->mode = tar_parse_octal(header.mode, sizeof(header.mode));
            file->mtime = (time_t)tar_parse_octal(header.mtime, sizeof(header.mtime));
            file->next = file_list;
            file_list = file;
            

            offset += BLOCKSIZE + ((file->size + BLOCKSIZE - 1) / BLOCKSIZE) * BLOCKSIZE;
        } else if (header.typeflag[0] == '5') {

            tar_file_t *dir = malloc(sizeof(tar_file_t));
            strncpy(dir->path, header.name, sizeof(dir->path) - 1);
            if (dir->path[strlen(dir->path) - 1] == '/') {
                dir->path[strlen(dir->path) - 1] = '\0';
            }
            
            dir->offset = 0;
            dir->size = 0;
            dir->mode = S_IFDIR | 0755;
            dir->mtime = (time_t)tar_parse_octal(header.mtime, sizeof(header.mtime));
            dir->next = file_list;
            file_list = dir;
            
            offset += BLOCKSIZE;
        } else {
            size_t file_size = tar_parse_octal(header.size, sizeof(header.size));
            offset += BLOCKSIZE + ((file_size + BLOCKSIZE - 1) / BLOCKSIZE) * BLOCKSIZE;
        }
    }
    
    fprintf(stderr, "Archive parsed successfully\n");
}


static tar_file_t *find_file(const char *path) {
    pthread_mutex_lock(&file_list_mutex);
    
    tar_file_t *current = file_list;
    while (current) {

        if (strcmp(current->path, path + 1) == 0 || strcmp(current->path, path) == 0) {
            pthread_mutex_unlock(&file_list_mutex);
            return current;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&file_list_mutex);
    return NULL;
}

static int archive_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    tar_file_t *file = find_file(path);
    if (!file) {
        return -ENOENT;
    }
    
    if (S_ISDIR(file->mode)) {
        stbuf->st_mode = file->mode;
        stbuf->st_nlink = 2;
    } else {
        stbuf->st_mode = S_IFREG | (file->mode & 0777);
        stbuf->st_size = file->size;
        stbuf->st_nlink = 1;
    }
    
    stbuf->st_mtime = file->mtime;
    stbuf->st_atime = file->mtime;
    stbuf->st_ctime = file->mtime;
    
    return 0;
}

static int archive_readdir(const char *path, void *buf,
                          fuse_fill_dir_t filler, off_t offset,
                          struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    pthread_mutex_lock(&file_list_mutex);
    
    char path_prefix[1024];
    if (strcmp(path, "/") == 0) {
        strcpy(path_prefix, "");
    } else {
        snprintf(path_prefix, sizeof(path_prefix), "%s/", path + 1);
    }
    
    int prefix_len = strlen(path_prefix);
    
    tar_file_t *current = file_list;
    while (current) {

        if (strncmp(current->path, path_prefix, prefix_len) == 0) {

            const char *rel_path = current->path + prefix_len;
            char *slash = strchr(rel_path, '/');
            
            if (slash == NULL) {

                filler(buf, rel_path, NULL, 0, 0);
            } else if (slash == rel_path + strlen(rel_path) - 1) {

                char dirname[256];
                strncpy(dirname, rel_path, slash - rel_path);
                dirname[slash - rel_path] = '\0';
                filler(buf, dirname, NULL, 0, 0);
            }
        }
        
        current = current->next;
    }
    
    pthread_mutex_unlock(&file_list_mutex);
    
    return 0;
}

static int archive_open(const char *path, struct fuse_file_info *fi) {
    if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)) {
        return -EACCES; 
    }
    
    tar_file_t *file = find_file(path);
    if (!file) {
        return -ENOENT;
    }
    
    if (S_ISDIR(file->mode)) {
        return -EISDIR;
    }
    
    return 0;
}

static int archive_read(const char *path, char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    tar_file_t *file = find_file(path);
    if (!file) {
        return -ENOENT;
    }
    
    if (offset >= (off_t)file->size) {
        return 0;
    }
    
    if (offset + size > file->size) {
        size = file->size - offset;
    }
    
    ssize_t res = pread(tar_fd, buf, size, file->offset + offset);
    if (res == -1) {
        return -errno;
    }
    
    return res;
}

static const struct fuse_operations archive_oper = {
    .getattr = archive_getattr,
    .readdir = archive_readdir,
    .open = archive_open,
    .read = archive_read,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <archive.tar> <mount_point> [FUSE_OPTIONS]\n", argv[0]);
        return 1;
    }
    
    archive_init(argv[1]);
    
    if (tar_fd == -1) {
        return 1;
    }
    
    argv[1] = argv[2];
    argc--;
    
    int res = fuse_main(argc, argv, &archive_oper, NULL);
    

    tar_file_t *current = file_list;
    while (current) {
        tar_file_t *next = current->next;
        free(current);
        current = next;
    }
    
    if (tar_fd != -1) {
        close(tar_fd);
    }
    
    return res;
}
