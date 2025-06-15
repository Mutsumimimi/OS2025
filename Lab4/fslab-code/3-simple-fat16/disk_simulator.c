#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include "disk_simulator.h"

static int fd;

struct disk_info {
    uint64_t seek_time_us;      // 磁头移动一个磁道所需时间
    long dist_size;
    long dist_sectors;
    long last_track;
    long total_track;
};
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static struct disk_info di;

// 循环等待 us 微秒，模拟寻道时间
void busywait(long us) {
    struct timespec s, t;
    clock_gettime(CLOCK_MONOTONIC, &s);
    while (true) {
        clock_gettime(CLOCK_MONOTONIC, &t);
        long dus = (long)(t.tv_sec - s.tv_sec) * 1000000 + (t.tv_nsec - s.tv_nsec) / 1000;
        if (dus >= us) {
            break;
        }
        if (dus < 0) {
            printf("fetal: time error\n");
            break;
        }
    }
}

// 寻道到指定扇区，会使用 busywait 模拟寻道时间
void seek_to(sector_t sec) {
    long track = sec / SEC_PER_TRACK;
    long delta = labs(track - di.last_track);
    busywait(delta * di.seek_time_us);
    di.last_track = track;
}


int sector_read(sector_t sec_num, void *buffer) {
    if(sec_num >= di.dist_sectors) {
        printf("read sector %lu error: out of range.\n", sec_num);
        memset(buffer, 0, PHYSICAL_SECTOR_SIZE);
        return 1;
    }
    if(pthread_mutex_lock(&mutex) != 0) {
        printf("read sector %lu error: lock failed.\n", sec_num);
        return 1;
    }
    seek_to(sec_num);
    ssize_t ret = pread(fd, buffer, PHYSICAL_SECTOR_SIZE, sec_num * PHYSICAL_SECTOR_SIZE);
    pthread_mutex_unlock(&mutex);
    if(ret != PHYSICAL_SECTOR_SIZE) {
        printf("read sector %lu error: image read failed.\n", sec_num);
        return 1;
    }
    return 0;
}

int sector_write(sector_t sec_num, const void *buffer) {
    if(sec_num >= di.dist_sectors) {
        printf("write sector %lu error: out of range.\n", sec_num);
        return 1;
    }
    if(pthread_mutex_lock(&mutex) != 0) {
        printf("write sector %lu error: lock failed.\n", sec_num);
        return 1;
    }
    seek_to(sec_num);
    ssize_t ret = pwrite(fd, buffer, PHYSICAL_SECTOR_SIZE, sec_num * PHYSICAL_SECTOR_SIZE);
    pthread_mutex_unlock(&mutex);
    if(ret != PHYSICAL_SECTOR_SIZE) {
        printf("write sector %lu error: image write failed.\n", sec_num);
        return 1;
    }
    return 0;
}

void init_disk(const char* path, uint64_t seek_time_ns) {
    fd = open(path, O_RDWR | O_DSYNC);
    if(fd < 0) {
        fprintf(stderr, "Open image file %s failed: %s\n", path, strerror(errno));
        exit(ENOENT);
    }
    di.seek_time_us = seek_time_ns;
    di.dist_size = lseek(fd, 0, SEEK_END);
    di.dist_sectors = di.dist_size / PHYSICAL_SECTOR_SIZE;
    di.last_track = 0;
    di.total_track = di.dist_sectors / SEC_PER_TRACK;
}

