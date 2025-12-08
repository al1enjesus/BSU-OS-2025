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

static char *base_path = NULL;

typedef struct {
    pthread_mutex_t lock;
    unsigned long reads;
    unsigned long writes;
    unsigned long opens;
    unsigned long creates;
    unsigned long deletes;
    unsigned long mkdirs;
    unsigned long rmdirs;
    unsigned long getatts;
    unsigned long readdirs;
    unsigned long bytes_read;
    unsigned long bytes_written;
} stats_t;

static stats_t stats = {
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .reads = 0,
    .writes = 0,
    .opens = 0,
    .creates = 0,
    .deletes = 0,
    .mkdirs = 0,
    .rmdirs = 0,
    .getatts = 0,
    .readdirs = 0,
    .bytes_read = 0,
    .bytes_written = 0,
};

static void get_full_path(char *fullpath, const char *path) {
    if (base_path == NULL) {
        strcpy(fullpath, path);
    } else {
        snprintf(fullpath, PATH_MAX, "%s%s", base_path, path);
    }
}

static void log_operation(const char *op, const char *path) {
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
    fprintf(stderr, "[%s] %s: %s\n", timestamp, op, path);
}


static void update_stats(const char *op, size_t bytes) {
    pthread_mutex_lock(&stats.lock);
    
    if (strcmp(op, "read") == 0) {
        stats.reads++;
        stats.bytes_read += bytes;
    } else if (strcmp(op, "write") == 0) {
        stats.writes++;
        stats.bytes_written += bytes;
    } else if (strcmp(op, "open") == 0) {
        stats.opens++;
    } else if (strcmp(op, "create") == 0) {
        stats.creates++;
    } else if (strcmp(op, "unlink") == 0) {
        stats.deletes++;
    } else if (strcmp(op, "mkdir") == 0) {
        stats.mkdirs++;
    } else if (strcmp(op, "rmdir") == 0) {
        stats.rmdirs++;
    } else if (strcmp(op, "getattr") == 0) {
        stats.getatts++;
    } else if (strcmp(op, "readdir") == 0) {
        stats.readdirs++;
    }
    
    pthread_mutex_unlock(&stats.lock);
}


static int generate_stats_file(char *buf, size_t size) {
    pthread_mutex_lock(&stats.lock);
    
    char stats_content[4096];
    int len = snprintf(stats_content, sizeof(stats_content),
        "=== FUSE Filesystem Statistics ===\n\n"
        "Operation Counts:\n"
        "reads: %lu\n"
        "writes: %lu\n"
        "opens: %lu\n"
        "creates: %lu\n"
        "deletes: %lu\n"
        "mkdirs: %lu\n"
        "rmdirs: %lu\n"
        "getatts: %lu\n"
        "readdirs: %lu\n\n"
        "Data Transfer:\n"
        "bytes_read: %lu\n"
        "bytes_written: %lu\n\n"
        "Summary:\n"
        "total_operations: %lu\n"
        "total_bytes: %lu\n",
        stats.reads, stats.writes, stats.opens, stats.creates,
        stats.deletes, stats.mkdirs, stats.rmdirs, stats.getatts,
        stats.readdirs, stats.bytes_read, stats.bytes_written,
        stats.reads + stats.writes + stats.opens + stats.creates +
        stats.deletes + stats.mkdirs + stats.rmdirs + stats.getatts + stats.readdirs,
        stats.bytes_read + stats.bytes_written
    );
    
    pthread_mutex_unlock(&stats.lock);
    
    if (len > size) {
        len = size;
    }
    
    memcpy(buf, stats_content, len);
    return len;
}

static int monitor_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi) {

    if (strcmp(path, "/.stats") == 0) {
        memset(stbuf, 0, sizeof(struct stat));
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = 4096;
        return 0;
    }
    
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = lstat(fullpath, stbuf);
    if (res == -1) {
        return -errno;
    }
    
    update_stats("getattr", 0);
    return 0;
}


static int monitor_readdir(const char *path, void *buf,
                          fuse_fill_dir_t filler, off_t offset,
                          struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    DIR *dp = opendir(fullpath);
    if (dp == NULL) {
        return -errno;
    }
    
    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    
    if (strcmp(path, "/") == 0) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_mode = S_IFREG | 0444;
        filler(buf, ".stats", &st, 0, 0);
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
    update_stats("readdir", 0);
    return 0;
}

static int monitor_open(const char *path, struct fuse_file_info *fi) {
    if (strcmp(path, "/.stats") == 0) {
        if ((fi->flags & O_WRONLY) || (fi->flags & O_RDWR)) {
            return -EACCES;
        }
        return 0;
    }
    
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, fi->flags);
    if (fd == -1) {
        return -errno;
    }
    
    close(fd);
    update_stats("open", 0);
    return 0;
}

static int monitor_read(const char *path, char *buf, size_t size,
                       off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/.stats") == 0) {
        return generate_stats_file(buf, size);
    }
    
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }
    
    int res = pread(fd, buf, size, offset);
    close(fd);
    
    if (res == -1) {
        return -errno;
    }
    
    update_stats("read", res);
    return res;
}

static int monitor_write(const char *path, const char *buf, size_t size,
                        off_t offset, struct fuse_file_info *fi) {
    if (strcmp(path, "/.stats") == 0) {
        return -EACCES; 
    }
    
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, O_WRONLY);
    if (fd == -1) {
        return -errno;
    }
    
    int res = pwrite(fd, buf, size, offset);
    close(fd);
    
    if (res == -1) {
        return -errno;
    }
    
    update_stats("write", res);
    return res;
}

static int monitor_create(const char *path, mode_t mode,
                         struct fuse_file_info *fi) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int fd = open(fullpath, fi->flags, mode);
    if (fd == -1) {
        return -errno;
    }
    
    close(fd);
    update_stats("create", 0);
    return 0;
}

static int monitor_unlink(const char *path) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = unlink(fullpath);
    if (res == -1) {
        return -errno;
    }
    
    update_stats("unlink", 0);
    return 0;
}


static int monitor_mkdir(const char *path, mode_t mode) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = mkdir(fullpath, mode);
    if (res == -1) {
        return -errno;
    }
    
    update_stats("mkdir", 0);
    return 0;
}


static int monitor_rmdir(const char *path) {
    char fullpath[PATH_MAX];
    get_full_path(fullpath, path);
    
    int res = rmdir(fullpath);
    if (res == -1) {
        return -errno;
    }
    
    update_stats("rmdir", 0);
    return 0;
}

static const struct fuse_operations monitor_oper = {
    .getattr = monitor_getattr,
    .readdir = monitor_readdir,
    .open = monitor_open,
    .read = monitor_read,
    .write = monitor_write,
    .create = monitor_create,
    .unlink = monitor_unlink,
    .mkdir = monitor_mkdir,
    .rmdir = monitor_rmdir,
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
    
    fprintf(stderr, "Mounting monitoring FS: %s at %s\n", base_path, argv[2]);
    fprintf(stderr, "View statistics with: cat %s/.stats\n", argv[2]);
    
    argv[1] = argv[2];
    argc--;
    
    int res = fuse_main(argc, argv, &monitor_oper, NULL);
    free(base_path);
    
    return res;
}
