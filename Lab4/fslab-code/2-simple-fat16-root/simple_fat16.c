#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#include "fat16.h"
#include "fat16_utils.h"

/* FAT16 volume data with a file handler of the FAT16 image file */
// 存储文件系统所需要的元数据的数据结构
// "抄袭"了https://elixir.bootlin.com/linux/latest/source/fs/fat/inode.c#L44
typedef struct {
    uint32_t sector_size;           // 逻辑扇区大小（字节）
    uint32_t sec_per_clus;          // 每簇扇区数
    uint32_t reserved;              // 保留扇区数
    uint32_t fats;                  // FAT表的数量
    uint32_t dir_entries;           // 根目录项数量
    uint32_t sectors;               // 文件系统总扇区数
    uint32_t sec_per_fat;           // 每个FAT表所占扇区数

    sector_t fat_sec;               // FAT表开始扇区
    sector_t root_sec;              // 根目录区域开始扇区
    uint32_t root_sectors;          // 根目录区域扇区数
    sector_t data_sec;              // 数据区域开始扇区
    
    uint32_t clusters;              // 文件系统簇数
    uint32_t cluster_size;          // 簇大小（字节）

    uid_t fs_uid;                   // 可忽略，挂载FAT的用户ID，所有文件的拥有者都显示为该用户
    gid_t fs_gid;                   // 可忽略，挂载FAT的组ID，所有文件的用户组都显示为该组
    struct timespec atime;          // 访问时间
    struct timespec mtime;          // 修改时间
    struct timespec ctime;          // 创建时间
} FAT16;

FAT16 meta;

sector_t cluster_first_sector(cluster_t clus) {
    assert(is_cluster_inuse(clus));
    return ((clus - 2) * meta.sec_per_clus) + meta.data_sec;
}

cluster_t read_fat_entry(cluster_t clus)
{
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    /**
     * TODO: 4.1 读取FAT表项 [约5行代码]
     * Hint: 你需要读取FAT表中clus对应的表项，然后返回该表项的值。
     *       表项在哪个扇区？在扇区中的偏移量是多少？表项的大小是多少？
     */
    // ================== Your code here =================
    // debug
    printf("read_fat_entry: clus=%u\n", clus);
    sector_t sec = meta.fat_sec + ((clus * sizeof(cluster_t)) / meta.sector_size); // 计算FAT表项所在的扇区
    // clus * 2 是因为FAT16中每个簇占用2个字节, 如 0x0002->0x0003
    size_t offset = (clus * sizeof(cluster_t)) % meta.sector_size; // 计算FAT表项在扇区中的偏移量
    int ret = sector_read(sec, sector_buffer); // 读取FAT表所在的扇区
    if(ret != 0) {
        return CLUSTER_END; // 读取失败，返回错误值
    }
    
    // ===================================================
    return *(cluster_t *)(sector_buffer + offset);    // TODO: 记得删除或者修改这一行
}

typedef struct {
    DIR_ENTRY dir;
    sector_t sector;
    size_t offset;
} DirEntrySlot;

/**
 * @brief 寻找对应的目录项，从 name 开始的 len 字节为要搜索的文件/目录名
 * 
 * @param name 指向文件名的指针，不一定以'\0'结尾，因此需要len参数
 * @param len 文件名的长度
 * @param from_sector 开始搜索的扇区
 * @param sectors_count 需要搜索的扇区数
 * @param slot 输出参数，存放找到的目录项和位置
 * @return long 找到entry时返回 FIND_EXIST，找到空槽返回 FIND_EMPTY，扇区均满了返回 FIND_FULL，错误返回错误代码的负值
 */
int find_entry_in_sectors(const char* name, size_t len, 
            sector_t from_sector, size_t sectors_count, 
            DirEntrySlot* slot) {
    char buffer[MAX_LOGICAL_SECTOR_SIZE];

    /**
     * TODO: 2.1 在指定扇区中找到文件名为 name 的目录项 [约20行代码]  
     * Hint: 你可以参考 fill_entries_in_sectors 函数的实现。（非常类似！）
     *       区别在于，你需要找到对应目录项并给slot赋值，然后返回正确的返回值，而不是调用 filler 函数。
     *       你可以使用 check_name 函数来检查目录项是否符合你要找的文件名。
     *       注意：找到entry时返回 FIND_EXIST，找到空槽返回 FIND_EMPTY，所有扇区都满了返回 FIND_FULL。
     */
    // ================== Your code here =================
    for(size_t i = 0; i < sectors_count; i++) {
        sector_t sec = from_sector + i;
        int ret = sector_read(sec, buffer);
        if(ret < 0) {
            return -EIO; // 读取扇区失败
        }
        for(size_t off = 0; off < meta.sector_size; off += DIR_ENTRY_SIZE) { // 每个目录项的大小是 DIR_ENTRY_SIZE
            DIR_ENTRY* entry = (DIR_ENTRY*)(buffer + off);
            char shortname[12];
            if(de_is_valid(entry)) {
                // 先判断name是否是长文件名，需要先转换成短文件名并在目录项中查找
                if(len >= 11){
                    ret = to_shortname(name, len, shortname); // 将长文件名转换为短文件名
                    if(ret < 0) {
                        return ret;
                    }
                } else{
                    strncpy(shortname, name, 11);
                }
                if(check_name(shortname, 11, entry)) {
                    slot->dir = *entry;
                    slot->sector = sec;
                    slot->offset = off;
                    // debug
                    printf("find_entry_in_sectors use: FIND_EXIST, name='%s', shortname='%s', len=%ld\n", name, shortname, len);
                    return FIND_EXIST; // 找到匹配的目录项，返回 FIND_EXIST
                }
            }
            if(de_is_free(entry)) {
                // debug
                printf("find_entry_in_sectors use: FIND_EMPTY, name='%s', shortname='%s', len=%ld\n", name, shortname, len);
                return FIND_EMPTY; // 如果是空目录项，直接返回 FIND_EMPTY
            }
        }
    }
    // debug
    printf("find_entry_in_sectors use: FIND_FULL\n");    
    // ===================================================
    return FIND_FULL;
}

/**
 * @brief 找到path所对应路径的目录项，如果最后一级路径不存在，则找到能创建最后一级文件/目录的空目录项。
 * 
 * @param path 要查找的路径
 * @param slot 输出参数，存放找到的目录项和位置
 * @param remains 输出参数，指向未找到的路径部分。如路径为 "/a/b/c"，a存在，b不存在，remains会指指向 "b/c"。
 * @return int 找到entry时返回 FIND_EXIST，找到空槽返回 FIND_EMPTY，扇区均满了返回 FIND_FULL，
 *             错误返回错误代码的负值，可能的错误参见brief部分。
 */
int find_entry_internal(const char* path, DirEntrySlot* slot, const char** remains) {
    *remains = path;
    *remains += strspn(*remains, "/");    // 跳过开头的'/'
    
    // 根目录
    sector_t first_sec = meta.root_sec;
    size_t nsec = meta.root_sectors;
    size_t len = strcspn(*remains, "/"); // 目前要搜索的文件名长度
    int state = find_entry_in_sectors(*remains, len, first_sec, nsec, slot);    // 请补全 find_entry_in_sectors 函数

    // 找到下一层名字开头
    const char* next_level = *remains + len;
    next_level += strspn(next_level, "/");

    // 以下是根目录搜索的结果判断和错误处理
    if(state < 0 || *next_level == '\0') {   // 出错，或者只有一层已经找到了，直接返回结果
        return state;
    }

    // 这份代码只考虑根目录
    return -ENOSYS;
}


/**
 * @brief 将 path 对应的目录项写入 slot 中，slot 实际上记录了目录项本身，以及目录项的位置（扇区号和在扇区中的偏移）。
 *        该函数的主体实现在 find_entry_internal 函数中。你不需要修改这个函数。
 * @param path 
 * @param slot 
 * @return int 如果找到目录项，返回0；如果文件不存在，返回-ENOENT；如果出现其它错误，返回错误代码的负值。
 */
int find_entry(const char* path, DirEntrySlot* slot) {
    const char* remains = NULL;
    int ret = find_entry_internal(path, slot, &remains); // 请查看并补全 find_entry_internal 函数
    if(ret < 0) {
        return ret;
    }
    if(ret == FIND_EXIST) {
        return 0;
    }
    return -ENOENT;
}

/**
 * @brief 找到一个能创建 path 对应文件/目录的空槽（空目录项），如果文件/目录已经存在，则返回错误。
 * 
 * @param path 
 * @param slot 
 * @param last_name 
 * @return int 如果找到空槽，返回0；如果文件已经存在，返回-EEXIST；如果目录已满，返回-ENOSPC。
 */
int find_empty_slot(const char* path, DirEntrySlot *slot, const char** last_name) {
    // debug
    printf("find_empty_slot(path='%s')\n", path);
    int ret = find_entry_internal(path, slot, last_name);
    if(ret < 0) {
        return ret;
    }
    if(ret == FIND_EXIST) { // 文件已经存在
        return -EEXIST;
    }
    if(ret == FIND_FULL) { // 所有槽都已经满了
        // debug
        printf("find_empty_slot: no empty slot found\n");
        return -ENOSPC;
    }
    return 0;
}

mode_t get_mode_from_attr(uint8_t attr) {
    mode_t mode = 0;
    mode |= attr_is_directory(attr) ? S_IFDIR : S_IFREG;
    return mode;
}

// ===========================文件系统接口实现===============================

/**
 * @brief 文件系统初始化，无需修改。但你可以阅读这个函数来了解如何使用 sector_read 来读出文件系统元数据信息。
 * 
 * @param conn 
 * @return void* 
 */
void *fat16_init(struct fuse_conn_info * conn, struct fuse_config *config) {
    /* 读取启动扇区 */
    BPB_BS bpb;
    sector_read(0, &bpb);
    meta.sector_size = bpb.BPB_BytsPerSec;
    meta.sec_per_clus = bpb.BPB_SecPerClus;
    meta.reserved = bpb.BPB_RsvdSecCnt;
    meta.fats = bpb.BPB_NumFATS;
    meta.dir_entries = bpb.BPB_RootEntCnt;
    meta.sectors = bpb.BPB_TotSec16 != 0 ? bpb.BPB_TotSec16 : bpb.BPB_TotSec32;
    meta.sec_per_fat = bpb.BPB_FATSz16;

    meta.fat_sec = meta.reserved;
    meta.root_sec = meta.fat_sec + (meta.fats * meta.sec_per_fat);
    meta.root_sectors = (meta.dir_entries * DIR_ENTRY_SIZE) / meta.sector_size;
    meta.data_sec = meta.root_sec + meta.root_sectors;
    meta.clusters = (meta.sectors - meta.data_sec) / meta.sec_per_clus;
    meta.cluster_size = meta.sec_per_clus * meta.sector_size;

    meta.fs_uid = getuid();
    meta.fs_gid = getgid();

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    meta.atime = meta.mtime = meta.ctime = now;
    return NULL;
}

/**
 * @brief 获取path对应的文件的属性，无需修改。
 * 
 * @param path    要获取属性的文件路径
 * @param stbuf   输出参数，需要填充的属性结构体
 * @return int    成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_getattr(const char* path, struct stat* stbuf, struct fuse_file_info* fi) {
    printf("getattr(path='%s')\n", path);
    // 清空所有属性
    memset(stbuf, 0, sizeof(struct stat));

    // 这些属性被提前计算好，不会改变
    stbuf->st_uid = meta.fs_uid;
    stbuf->st_gid = meta.fs_gid;
    stbuf->st_blksize = meta.cluster_size;

    // 这些属性需要根据文件设置
    // st_mode, st_size, st_blocks, a/m/ctim
    if (path_is_root(path)) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_size = 0;
        stbuf->st_blocks = 0;
        stbuf->st_atim = meta.atime;
        stbuf->st_mtim = meta.mtime;
        stbuf->st_ctim = meta.ctime;
        return 0;
    }

    DirEntrySlot slot = {{}, 0, 0};
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);
    if(ret < 0) {
        return ret;
    }
    stbuf->st_mode = get_mode_from_attr(dir->DIR_Attr);
    stbuf->st_size = dir->DIR_FileSize;
    stbuf->st_blocks = dir->DIR_FileSize / PHYSICAL_SECTOR_SIZE;
    
    time_fat_to_unix(&stbuf->st_atim, dir->DIR_LstAccDate, 0, 0);
    time_fat_to_unix(&stbuf->st_mtim, dir->DIR_WrtDate, dir->DIR_WrtTime, 0);
    time_fat_to_unix(&stbuf->st_ctim, dir->DIR_CrtDate, dir->DIR_CrtTime, dir->DIR_CrtTimeTenth);
    return 0;
}

/**
 * @brief 读取扇区号为 first_sec 开始的 nsec 个扇区的目录项，并使用 filler 函数将目录项填充到 buffer 中
 *        调用 filler 的方式为 filler(buffer, 完整的文件名/目录名, NULL, 0, 0)
 *        例如，filler(buffer, "file1.txt", NULL, 0, 0) 表示该目录中有文件 file1.txt
 * @param first_sec 开始的扇区号
 * @param nsec      扇区数
 * @param filler    用于填充结果的函数
 * @param buf       结果缓冲区
 * @return int      成功返回0，失败返回POSIX错误代码的负值
 */
int fill_entries_in_sectors(sector_t first_sec, size_t nsec, fuse_fill_dir_t filler, void* buf) {
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    char name[MAX_NAME_LEN];
    for(size_t i=0; i < nsec; i++) {
        /**
         * TODO: 1.2 读取扇区中的目录项 [3行代码]
         * Hint：（为降低难度，我们实现了大部分代码，你只需要修改和补全以下带 TODO 的几行。）
         *       你需要补全这个循环。这个循环读取每个扇区的内容，然后遍历扇区中的目录项，并正确调用 filler 函数。
         *       你可以参考以下步骤（大部分已经实现）：
         *          1. 使用 sector_read 读取第i个扇区内容到 sector_buffer 中（扇区号是什么？）
         *          2. 遍历 sector_buffer 中的每个目录项，如果是有效的目录项，转换文件名并调用 filler 函数。（每个目录项多长？）
         *             可能用到的函数（请自行阅读函数实现来了解用法）：
         *              de_is_valid: 判断目录项是否有效。
         *              to_longname: 将 FAT 短文件名转换为长文件名。
         *              filler: 用于填充结果的函数。
         *          3. 如果遇到空目录项，说明该扇区的目录项已经读取完，无需继续遍历。（为什么？）
         *             可能用到的函数：
         *              de_is_free: 判断目录项是否为空。
         *       为降低难度，我们实现了大部分代码，你只需要修改和补全以下带 TODO 的几行。
         */
        sector_t sec = first_sec + i; // TODO: 请填写正确的扇区号 
        int ret = sector_read(sec, sector_buffer);
        if(ret < 0) {
            return ret;
        }
        for(size_t off = 0; off < meta.sector_size; off += DIR_ENTRY_SIZE ) { // TODO: 遍历扇区中的所有目录项
            // 每个目录项的大小是 DIR_ENTRY_SIZE
            DIR_ENTRY* entry = (DIR_ENTRY*)(sector_buffer + off);
            if(de_is_valid(entry)) {
                int ret = to_longname(entry->DIR_Name, name, MAX_NAME_LEN); // TODO: 将短文件名转换为长文件名
                if(ret < 0) {
                    return ret;
                }
                filler(buf, name, NULL, 0, 0);
            }
            if(de_is_free(entry)) {
                return 0;
            }
        }
    }
    return 0;
}

/**
 * @brief 读取path对应的目录，结果通过filler函数写入buffer中
 * 
 * @param path    要读取目录的路径
 * @param buf     结果缓冲区
 * @param filler  用于填充结果的函数，本次实验按filler(buffer, 文件名, NULL, 0, 0)的方式调用即可。
 *                你也可以参考<fuse.h>第58行附近的函数声明和注释来获得更多信息。
 * @param offset  忽略
 * @param fi      忽略
 * @return int    成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, 
                    struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
    printf("readdir(path='%s')\n", path);

    if(path_is_root(path)) {
        /**
         * TODO: 1.1 读取根目录区域 [2行代码]
         * Hint: 请正确修改以下两行，删掉 _placeholder_() 并替换为正确的数值。提示，使用 meta 中的成员变量。
         *       （注：_placeholder_() 在之后的 TODO 中也会出现。它只是个占位符，相当于你要填的空，你应该删除它，修改成正确的值。）
         */
        sector_t first_sec = meta.root_sec; // TODO: 请填写正确的根目录区域开始扇区号，你可以参考 meta 的定义。
        size_t nsec = meta.root_sectors; // TODO: 请填写正确的根目录区域扇区数
        fill_entries_in_sectors(first_sec, nsec, filler, buf);
        return 0;
    }
    return -ENOSYS; // 该版本只涉及根目录的读取，其它目录直接返回错误
}

/**
 * @brief 从簇号为 clus 的簇的 offset 处开始读取 size 字节的数据到 data 中，并返回实际读取的字节数。
 * 
 * @param clus 簇号
 * @param offset 开始读取的偏移
 * @param data 输出缓冲区
 * @param size 要读取数据的长度
 * @return int 成功返回实际读取数据的长度，错误返回 POSIX 错误代码的负值
 */
int read_from_cluster_at_offset(cluster_t clus, off_t offset, char* data, size_t size) {
    assert(offset + size <= meta.cluster_size);  // offset + size 必须小于簇大小
    char sector_buffer[PHYSICAL_SECTOR_SIZE];
    /**
     * TODO: 2.2 从簇中读取数据 [约5行代码]
     * Hint: 步骤如下: 
     *       1. 计算 offset 对应的扇区号和扇区内偏移量。你可以用 cluster_first_sector 函数找到簇的第一个扇区。
     *          但 offset 可能超过一个扇区，所以你要计算出实际的扇区号和扇区内偏移量。
     *       2. 读取扇区（使用 sector_read）
     *       3. 将扇区正确位置的内容移动至 data 中正确位置
     *       你只需要补全以下 TODO 部分。主要是计算扇区号和扇区内偏移量；以及使用 memcpy 将数据移动到 data 中。
     */
    // debug
    printf("read_from_cluster_at_offset use\n");
    uint32_t sec = cluster_first_sector(clus) + (offset / meta.sector_size); // TODO: 请填写正确的扇区号。
    size_t sec_off = offset % meta.sector_size; // TODO: 请填写正确的扇区内偏移量。
    size_t pos = 0; // 实际已经读取的字节数
    while(pos < size) { // 还没有读取完毕
        int ret = sector_read(sec, sector_buffer);
        if(ret < 0) {
            return ret;
        }
        // Hint: 使用 memcpy 挪数据，从 sec_off 开始挪。挪到 data 中哪个位置？挪多少？挪完后记得更新 pos （约3行代码）
        // ================== Your code here =================
        memcpy(data + pos, sector_buffer + sec_off, min(size - pos, meta.sector_size - sec_off));
        pos += min(size - pos, meta.sector_size - sec_off); // 更新已读取的字节数        
        // ===================================================
        sec_off = 0;
        sec ++ ;
    }
    return size;
}

/**
 * @brief 从path对应的文件的 offset 字节处开始读取 size 字节的数据到 buffer 中，并返回实际读取的字节数。
 * Hint: 文件大小属性是 Dir.DIR_FileSize。
 * 
 * @param path    要读取文件的路径
 * @param buffer  结果缓冲区
 * @param size    需要读取的数据长度
 * @param offset  要读取的数据所在偏移量
 * @param fi      忽略
 * @return int    成功返回实际读写的字符数，失败返回0。
 */
int fat16_read(const char *path, char *buffer, size_t size, off_t offset,
               struct fuse_file_info *fi) {
    printf("read(path='%s', offset=%ld, size=%lu)\n", path, offset, size);
    if(path_is_root(path)) {
        return -EISDIR;
    }

    DirEntrySlot slot = {{}, 0, 0};
    DIR_ENTRY* dir = &(slot.dir);
    int ret = find_entry(path, &slot);  // 寻找文件对应的目录项，请查看并补全 find_entry 函数
    if(ret < 0) { // 寻找目录项出错
        return ret;
    }
    if(attr_is_directory(dir->DIR_Attr)) { // 找到的是目录
        return -EISDIR;
    }
    if(offset > dir->DIR_FileSize) { // 要读取的偏移量超过文件大小
        return -EINVAL;
    }
    size = min(size, dir->DIR_FileSize - offset);  // 读取的数据长度不能超过文件大小


    if(offset + size <= meta.cluster_size) {    // 文件在一个簇内的情况
        cluster_t clus = dir->DIR_FstClusLO;
        int ret = read_from_cluster_at_offset(clus, offset, buffer, size); // 请补全该函数
        return ret;
    }

    // 读取跨簇的文件
    cluster_t clus = dir->DIR_FstClusLO;
    size_t p = 0;   // 实际读取的字节数
    while(offset >= meta.cluster_size) {    // 移动到正确的簇号
        if(!is_cluster_inuse(clus)) {
            return -EUCLEAN;
        }
        offset -= meta.cluster_size;    // 如果 offset 大于一个簇的大小，需要移动到下一个簇，offset 减去一个簇的大小
        clus = read_fat_entry(clus);    // 请查看并补全 read_fat_entry 函数
    }

    /**
     * TODO: 4.2 读取多个簇中的数据 [约10行代码]
     * Hint: 你需要写一个循环，读取当前簇中的正确数据。步骤如下
     *       0. 写一个 while 循环，确认还有数据要读取，且簇号 clus 有效。
     *            1. 你可以使用 is_cluster_inuse() 函数
     *       1. 计算你要读取的（从offset开始的）数据长度。这有两种情况：
     *            1. 要读到簇的结尾，长度是？（注意 offset）
     *            2. 剩余的数据不需要到簇的结尾，长度是？
     *       2. 使用 read_from_cluster_at_offset 函数读取数据。注意检查返回值。
     *       3. 更新 p （已读字节数）、offset （下一个簇偏移）、clus（下一个簇号）。
     *          实际上，offset肯定是0，因为除了第一个簇，后面的簇都是从头开始读取。
     *          clus 则需要读取 FAT 表，别忘了你已经实现了 read_fat_entry 函数。
     */
    // ================== Your code here =================
    while(is_cluster_inuse(clus) && p < size) {
        size_t read_size = 0;
        if(offset + size - p >= meta.cluster_size) { // 要读到簇的结尾
            read_size = meta.cluster_size - offset; // 读取到簇的结尾
        } else { // 剩余的数据不需要到簇的结尾
            read_size = size - p; // 只读取剩余的数据
        }
        ret = read_from_cluster_at_offset(clus, offset, buffer + p, read_size); 
        if(ret < 0) {
            return ret; // 读取失败，返回错误
        }
        p += ret; // 更新已读取的字节数
        offset = 0; // 除了第一个簇，后面的簇都是从头开始读取，所以 offset 重置为0
        clus = read_fat_entry(clus); // 获取下一个簇号
    }
    // ===================================================
    return p;
}

int dir_entry_write(DirEntrySlot slot) {
    /**
     * TODO: 3.2 写入目录项 [3行代码]
     * Hint: （你只需补全带 TODO 的几行）
     *       使用 sector_write 将目录项写入文件系统。
     *       sector_write 只能写入一个扇区，但一个目录项只占用一个扇区的一部分。
     *       为了不覆盖其他目录项，你需要先读取该扇区，修改目录项对应的位置，然后再将整个扇区写回。
     */
    printf("dir_entry_write use: ");    // debug
    char sector_buffer[MAX_LOGICAL_SECTOR_SIZE];
    int ret = sector_read(slot.sector, sector_buffer); // TODO: 使用 sector_read 读取扇区
    if(ret < 0) {
        return ret;
    }
    memcpy(sector_buffer + slot.offset, &slot.dir, DIR_ENTRY_SIZE); // TODO: 使用 memcpy 将 slot.dir 里的目录项写入 buffer 中的正确位置
    // 目录项的大小是 DIR_ENTRY_SIZE
    ret = sector_write(slot.sector, sector_buffer); // TODO: 使用 sector_write 写回扇区
    if(ret < 0) {
        return ret;
    }
    // debug
    char ent[DIR_ENTRY_SIZE+1];
    memcpy(ent, sector_buffer+slot.offset, DIR_ENTRY_SIZE); 
    printf("entry='%s'\n", ent);
    return 0;
}

int dir_entry_create(DirEntrySlot slot, const char *shortname, 
            attr_t attr, cluster_t first_clus, size_t file_size) {
    DIR_ENTRY* dir = &(slot.dir);
    memset(dir, 0, sizeof(DIR_ENTRY));
    
    /**
     * TODO: 3.1 创建目录项 [约5行代码]
     * Hint: 请给目录项的 DIR_Name、Dir_Attr、DIR_FstClusHI、DIR_FstClusLO、DIR_FileSize 设置正确的值。
     *       你可以用 memcpy 函数来设置 DIR_Name。
     *       DIR_FstClusHI 在我们的系统中永远为 0。
     */

    // ================== Your code here =================
    // debug
    printf("dir_entry_create: shortname='%s', attr=%02x, first_clus=%u, file_size=%lu\n", 
            shortname, attr, first_clus, file_size);
    memcpy(dir->DIR_Name, shortname, sizeof(dir->DIR_Name)); // 将短文件名复制到目录项中
    dir->DIR_Attr = attr; // 设置文件属性
    dir->DIR_FstClusHI = 0; // TODO 要求DIR_FstClusHI 永远为 0
    dir->DIR_FstClusLO = first_clus; // 设置文件的首簇号
    dir->DIR_FileSize = file_size; // 设置文件大小
    // ===================================================
    
    // 设置文件时间戳，无需修改
    struct timespec ts;
    int ret = clock_gettime(CLOCK_REALTIME, &ts);
    if(ret < 0) {
        return -errno;
    }
    time_unix_to_fat(&ts, &(dir->DIR_CrtDate), &(dir->DIR_CrtTime), &(dir->DIR_CrtTimeTenth));
    time_unix_to_fat(&ts, &(dir->DIR_WrtDate), &(dir->DIR_WrtTime), NULL);
    time_unix_to_fat(&ts, &(dir->DIR_LstAccDate), NULL, NULL);

    ret = dir_entry_write(slot);    // 请实现该函数
    if(ret < 0) {
        return ret;
    }
    return 0;
}


/**
 * @brief 在 path 对应的路径创建新文件（一个目录项对应一个文件，创建文件实际上就是创建目录项）
 * 
 * @param path    要创建的文件路径
 * @param mode    要创建文件的类型，本次实验可忽略，默认所有创建的文件都为普通文件
 * @param devNum  忽略，要创建文件的设备的设备号
 * @return int    成功返回0，失败返回POSIX错误代码的负值
 */
int fat16_mknod(const char *path, mode_t mode, dev_t dev) {
    printf("mknod(path='%s', mode=%03o, dev=%lu)\n", path, mode, dev);
    DirEntrySlot slot = {{}, 0, 0};
    const char* filename = NULL;
    int ret = find_empty_slot(path, &slot, &filename);  // 找一个空的目录项，如果正确实现了 find_entry_internal 函数，这里不需要修改
    if(ret < 0) {
        return ret;
    }

    char shortname[11];
    ret = to_shortname(filename, MAX_NAME_LEN, shortname); // 将长文件名转换为短文件名
    if(ret < 0) {
        return ret;
    }
    printf("filename='%s'\n",filename);
    ret = dir_entry_create(slot, shortname, ATTR_REGULAR, 0, 0); // 创建目录项，请查看并补全 dir_entry_create 函数
    if(ret < 0) {
        return ret;
    }
    return 0;
}

struct fuse_operations fat16_oper = {
    .init = fat16_init,         // 文件系统初始化
    .getattr = fat16_getattr,   // 获取文件属性

    .readdir = fat16_readdir,   // 读取目录
    .read = fat16_read,         // 读取文件

    .mknod = fat16_mknod,       // 创建文件
};
