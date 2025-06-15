#ifndef DISK_SIMULATOR_H
#define DISK_SIMULATOR_H

#include <stdint.h>
#include <stdbool.h>

#define PHYSICAL_SECTOR_SIZE 512        // 每个物理扇区的大小
#define SEC_PER_TRACK  512              // 每个物理磁道的物理扇区数 （仅用于模拟磁盘）


typedef uint64_t sector_t;

/**
 * @brief 初始化磁盘模拟，磁盘镜像文件路径为 path，寻道时间为 seek_time_ns。
 */
void init_disk(const char* path, uint64_t seek_time_ns);

/**
 * @brief 读取模拟磁盘中扇区号为 sec_num 的扇区数据到 buffer 中。注意 buffer 大小必须至少为 PHYSICAL_SECTOR_SIZE。
 * @return int 成功返回0，失败返回1
 */
int sector_read(sector_t sec_num, void *buffer);

/**
 * @brief 向模拟磁盘中扇区号为 sec_num 的扇区写入buffer中的数据。注意 buffer 大小必须至少为 PHYSICAL_SECTOR_SIZE。
 * @return int 成功返回0，失败返回1
 */
int sector_write(sector_t sec_num, const void *buffer);

#endif // DISK_SIMULATOR_H