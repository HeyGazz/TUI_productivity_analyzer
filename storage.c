#include "storage.h"
#include "activity.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Binary format: magic(4) + version(4) + ActivityList struct */
#define MAGIC   0x50524F44u   /* "PROD" */
#define VERSION 1u

int storage_load(ActivityList *list, const char *path) {
    FILE *f;
    uint32_t magic, version;

    activity_list_init(list);
    f = fopen(path, "rb");
    if (!f) return 0;   /* first run, empty list is fine */

    if (fread(&magic,   sizeof(magic),   1, f) != 1 || magic   != MAGIC   ||
        fread(&version, sizeof(version), 1, f) != 1 || version != VERSION ||
        fread(list,     sizeof(*list),   1, f) != 1) {
        fclose(f);
        activity_list_init(list);
        return -1;
    }
    fclose(f);
    return 0;
}

int storage_save(const ActivityList *list, const char *path) {
    FILE *f;
    uint32_t magic   = MAGIC;
    uint32_t version = VERSION;

    f = fopen(path, "wb");
    if (!f) return -1;
    fwrite(&magic,   sizeof(magic),   1, f);
    fwrite(&version, sizeof(version), 1, f);
    fwrite(list,     sizeof(*list),   1, f);
    fclose(f);
    return 0;
}
