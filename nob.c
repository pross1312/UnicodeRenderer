#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "src/nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};
    cmd_append(&cmd, "gcc",
            "-Wall", "-Wextra", "-ggdb",
            "-o", "main",
            "-I/usr/include/freetype2",
            "-Iraylib-5.5/include/",
            "src/main.c",
            "src/unicode_renderer.c",
            "-Lraylib-5.5/lib",
            "-lfreetype", "-lraylib", "-lm");
    cmd_run_sync_and_reset(&cmd);
    return 0;
}
