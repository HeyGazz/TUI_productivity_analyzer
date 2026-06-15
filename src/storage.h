#ifndef STORAGE_H
#define STORAGE_H

#include "activity.h"

int storage_load(ActivityList *list, const char *path);
int storage_save(const ActivityList *list, const char *path);

#endif
