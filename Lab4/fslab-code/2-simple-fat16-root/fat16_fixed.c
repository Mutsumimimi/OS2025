#include <string.h>
#include "fat16.h"

typedef struct {
    const char* image_path;
    uint64_t seek_time_us;
} Options;

#define OPTION(t, p) { t, offsetof(Options, p), 1 }
static const struct fuse_opt option_spec[] = {
    OPTION("--img=%s", image_path),
    OPTION("--seek_time=%lu", seek_time_us),
    FUSE_OPT_END
};

extern struct fuse_operations fat16_oper;

int main(int argc, char *argv[])
{   
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    Options opts;
    opts.image_path = strdup(DEFAULT_IMAGE);
    opts.seek_time_us = 0;
    int ret = fuse_opt_parse(&args, &opts, option_spec, NULL);
    if(ret < 0) {
        return EXIT_FAILURE;
    }
    init_disk(opts.image_path, opts.seek_time_us);
    ret = fuse_main(args.argc, args.argv, &fat16_oper, NULL);
    fuse_opt_free_args(&args);
    return ret;
}