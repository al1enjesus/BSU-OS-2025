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

static char *base_path = NULL;

static void get_full_path(char *fullpath, const char *path) {
    if (base_path == NULL) {
        strcpy(fullpath, path);
    } else {
        snprintf(fullpath, PATH_MAX, "%s%s", base_path, path);
    }
}

static void log_operation(const char *op, const char *path, int result) {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    
    if (result == 0) {
        fprintf(stderr, "[%s] %s: %s (success)\n", timestamp, op, path);
    } else {
        fprintf(stderr, "[%s] %s: %s (result: %d)\n", timestamp, op, path, result);
    }
}

static int passthrough_getattr(const char *path, struct stat *stbuf, 
                               struct fuse_file_info *fi) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = lstat(fullpath, stbuf);
    if (res == -1) {
        log_operation("GETATTR", path, -errno);
        return -errno;
    }
    
    log_operation("GETATTR", path, 0);
    return 0;
}

static int passthrough_readdir(const char *path, void *buf, 
                              fuse_fill_dir_t filler, off_t offset,
                              struct fuse_file_info *fi, 
                              enum fuse_readdir_flags flags) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    DIR *dp = opendir(fullpath);
    if (dp == NULL) {
        log_operation("READDIR", path, -errno);
        return -errno;
    }
    
    struct dirent *de;
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        
        if (filler(buf, de->d_name, &st, 0, 0) != 0) {
            closedir(dp);
            return -ENOMEM;
        }
    }
    
    closedir(dp);
    log_operation("READDIR", path, 0);
    return 0;
}

static int passthrough_open(const char *path, struct fuse_file_info *fi) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, fi->flags);
    if (fd == -1) {
        log_operation("OPEN", path, -errno);
        return -errno;
    }
    
    close(fd);
    log_operation("OPEN", path, 0);
    return 0;
}

static int passthrough_read(const char *path, char *buf, size_t size, 
                           off_t offset, struct fuse_file_info *fi) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, O_RDONLY);
    if (fd == -1) {
        log_operation("READ", path, -errno);
        return -errno;
    }
    
    int res = pread(fd, buf, size, offset);
    close(fd);
    
    if (res == -1) {
        log_operation("READ", path, -errno);
        return -errno;
    }
    
    fprintf(stderr, "[READ] %s: %zu bytes at offset %ld (result: %d)\n", 
            path, size, offset, res);
    return res;
}

static int passthrough_write(const char *path, const char *buf, size_t size,
                            off_t offset, struct fuse_file_info *fi) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, O_WRONLY);
    if (fd == -1) {
        log_operation("WRITE", path, -errno);
        return -errno;
    }
    
    int res = pwrite(fd, buf, size, offset);
    close(fd);
    
    if (res == -1) {
        log_operation("WRITE", path, -errno);
        return -errno;
    }
    
    fprintf(stderr, "[WRITE] %s: %zu bytes at offset %ld (result: %d)\n", 
            path, size, offset, res);
    return res;
}

static int passthrough_create(const char *path, mode_t mode,
                             struct fuse_file_info *fi) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, fi->flags, mode);
    if (fd == -1) {
        log_operation("CREATE", path, -errno);
        return -errno;
    }
    
    close(fd);
    log_operation("CREATE", path, 0);
    return 0;
}


static int passthrough_unlink(const char *path) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = unlink(fullpath);
    if (res == -1) {
        log_operation("UNLINK", path, -errno);
        return -errno;
    }
    
    log_operation("UNLINK", path, 0);
    return 0;
}


static int passthrough_mkdir(const char *path, mode_t mode) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = mkdir(fullpath, mode);
    if (res == -1) {
        log_operation("MKDIR", path, -errno);
        return -errno;
    }
    
    log_operation("MKDIR", path, 0);
    return 0;
}

static int passthrough_rmdir(const char *path) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = rmdir(fullpath);
    if (res == -1) {
        log_operation("RMDIR", path, -errno);
        return -errno;
    }
    
    log_operation("RMDIR", path, 0);
    return 0;
}

static const struct fuse_operations passthrough_oper = {
    .getattr = passthrough_getattr,
    .readdir = passthrough_readdir,
    .open = passthrough_open,
    .read = passthrough_read,
    .write = passthrough_write,
    .create = passthrough_create,
    .unlink = passthrough_unlink,
    .mkdir = passthrough_mkdir,
    .rmdir = passthrough_rmdir,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <source_dir> <mount_point> [FUSE_OPTIONS]\n", argv[0]);
        return 1;
    }
    
    base_path = realpath(argv[1], NULL);
    if (base_path == NULL) {
        perror("realpath");
        return 1;
    }
    
    fprintf(stderr, "Mounting %s at %s\n", base_path, argv[2]);
    
    argv[1] = argv[2];
    argc--;
    
    int res = fuse_main(argc, argv, &passthrough_oper, NULL);
    free(base_path);
    
    return res;
}
