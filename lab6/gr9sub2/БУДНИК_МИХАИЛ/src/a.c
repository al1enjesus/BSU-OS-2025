#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static char *real_root_path = NULL;
static int full_path(char fpath[PATH_MAX], const char *path) {
    if (real_root_path == NULL) {
        return -1;
    }

    if (strlen(real_root_path) + strlen(path) >= PATH_MAX) {
        return -1;
    }
    
    strcpy(fpath, real_root_path);
    strcat(fpath, path);
    return 0;
}

static void log_operation(const char *op_name, const char *path, int result) {
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    char timestamp_buf[32];
    
    strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%d %H:%M:%S", lt);
    fprintf(stderr, "[%s] %s: %s (%d)\n", timestamp_buf, op_name, path, result);
}

static int mishafuse_getattr(const char *path, struct stat *stbuf) {
    char fpath[PATH_MAX];
    int res;
    
    if (full_path(fpath, path) != 0) {
        log_operation("getattr", path, -E2BIG);
        return -E2BIG;
    }

    res = lstat(fpath, stbuf) == -1 ? -errno: 0;
    log_operation("getattr", path, res);
    return res;
}

static int mishafuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    DIR *dp;
    struct dirent *de;
    int res = 0;

    if (full_path(fpath, path) != 0) {
        log_operation("readdir", path, -E2BIG);
        return -E2BIG;
    }

    dp = opendir(fpath);
    if (dp == NULL) {
        res = -errno;
        log_operation("readdir", path, res);
        return res;
    }

    seekdir(dp, offset);
    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, de->d_off)) break;
    }

    closedir(dp);
    log_operation("readdir", path, 0);
    return 0;
}

static int mishafuse_open(const char *path, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    int res;

    if (full_path(fpath, path) != 0) {
        log_operation("open", path, -E2BIG);
        return -E2BIG;
    }
    
    res = open(fpath, fi->flags);
    if (res == -1) {
        res = -errno;
        log_operation("open", path, res);
        return res;
    }

    fi->fh = res;
    log_operation("open", path, 0);
    return 0;
}

static int mishafuse_read(const char *path, char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi) {
    int res = pread(fi->fh, buf, size, offset);

    if (res == -1) {
        res = -errno;
        log_operation("read", path, res);
        return res;
    }
    
    log_operation("read", path, res);
    return res;
}

static int mishafuse_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    int res = pwrite(fi->fh, buf, size, offset);

    if (res == -1) {
        res = -errno;
        log_operation("write", path, res);
        return res;
    }
    
    log_operation("write", path, res); // Log number of bytes written
    return res;
}

static int mishafuse_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    char fpath[PATH_MAX];
    int res;
    
    if (full_path(fpath, path) != 0) {
        log_operation("create", path, -E2BIG);
        return -E2BIG;
    }

    res = open(fpath, fi->flags, mode);
    if (res == -1) {
        res = -errno;
        log_operation("create", path, res);
        return res;
    }

    fi->fh = res; 
    log_operation("create", path, 0);
    return 0;
}

static int mishafuse_unlink(const char *path) {
    char fpath[PATH_MAX];
    int res;
    
    if (full_path(fpath, path) != 0) {
        log_operation("unlink", path, -E2BIG);
        return -E2BIG;
    }

    res = unlink(fpath) == -1 ? -errno: 0;
    log_operation("unlink", path, res);
    return res;
}

static int mishafuse_mkdir(const char *path, mode_t mode) {
    char fpath[PATH_MAX];
    int res;
    
    if (full_path(fpath, path) != 0) {
        log_operation("mkdir", path, -E2BIG);
        return -E2BIG;
    }

    res = mkdir(fpath, mode) == -1 ? -errno: 0;

    log_operation("mkdir", path, res);
    return res;
}

static int mishafuse_rmdir(const char *path) {
    char fpath[PATH_MAX];
    int res;
    
    if (full_path(fpath, path) != 0) {
        log_operation("rmdir", path, -E2BIG);
        return -E2BIG;
    }

    res = rmdir(fpath) == -1 ? -errno: 0;
    log_operation("rmdir", path, res);
    return res;
}

static struct fuse_operations mishafuse_operations = {
    .getattr    = mishafuse_getattr,
    .readdir    = mishafuse_readdir,
    .open       = mishafuse_open,
    .read       = mishafuse_read,
    .write      = mishafuse_write,
    .create     = mishafuse_create,
    .unlink     = mishafuse_unlink,
    .mkdir      = mishafuse_mkdir,
    .rmdir      = mishafuse_rmdir,
    .release    = NULL,
    .flush      = NULL,
};

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <source_dir> <mount_point> [opts...]\n", argv[0]);
        return 1;
    }
    
    real_root_path = realpath(argv[1], NULL); 
    if (real_root_path == NULL) {
        fprintf(stderr, "error: invalid source directory: %s\n", argv[1]);
        return 1;
    }

    size_t len = strlen(real_root_path);
    if (real_root_path[len - 1] != '/') {
        char *temp = (char*)realloc(real_root_path, len + 2); 
        if (temp == NULL) {
            fprintf(stderr, "error: memory allocation failed\n");
            free(real_root_path);
            return 1;
        }
        real_root_path = temp;
        real_root_path[len] = '/';
        real_root_path[len + 1] = '\0';
    }

    int fuse_argc = argc - 1;
    char **fuse_argv = malloc(sizeof(char *) * fuse_argc);
    fuse_argv[0] = argv[0];
    for (int i = 2; i < argc; i++) fuse_argv[i - 1] = argv[i];
    
    int res = fuse_main(fuse_argc, fuse_argv, &mishafuse_operations, NULL);
    free(fuse_argv); free(real_root_path);
    return res;
}