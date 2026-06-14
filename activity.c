#include "activity.h"
#include <string.h>
#include <stdlib.h>

void activity_list_init(ActivityList *list) {
    memset(list, 0, sizeof(*list));
    list->next_id = 1;
}

int activity_add(ActivityList *list, const char *name, const char *desc) {
    if (list->count >= MAX_ACTIVITIES) return -1;
    Activity *a = &list->items[list->count];
    memset(a, 0, sizeof(*a));
    a->id = list->next_id++;
    strncpy(a->name, name, MAX_NAME_LEN - 1);
    if (desc) strncpy(a->description, desc, MAX_DESC_LEN - 1);
    a->status = STATUS_IN_PROGRESS;
    a->created_at = time(NULL);
    return list->count++;
}

void activity_finish(ActivityList *list, int idx) {
    if (idx < 0 || idx >= list->count) return;
    list->items[idx].status = STATUS_FINISHED;
}

void activity_delete(ActivityList *list, int idx) {
    int i;
    if (idx < 0 || idx >= list->count) return;
    for (i = idx; i < list->count - 1; i++)
        list->items[i] = list->items[i + 1];
    list->count--;
}

void session_init(Session *s) {
    memset(s, 0, sizeof(*s));
    s->activity_idx = -1;
}

void session_start(Session *s, int idx) {
    s->activity_idx   = idx;
    s->is_running     = 1;
    s->session_start  = time(NULL);
    s->last_input     = s->session_start;
    s->session_total  = 0;
    s->session_active = 0;
    s->session_passive = 0;
    s->is_passive     = 0;
}

void session_stop(Session *s, ActivityList *list) {
    if (!s->is_running) return;
    if (s->activity_idx >= 0 && s->activity_idx < list->count) {
        Activity *a = &list->items[s->activity_idx];
        a->total_seconds   += s->session_total;
        a->active_seconds  += s->session_active;
        a->passive_seconds += s->session_passive;
    }
    s->is_running     = 0;
    s->activity_idx   = -1;
}

void session_tick(Session *s) {
    if (!s->is_running) return;
    s->session_total++;
    if (s->is_passive)
        s->session_passive++;
    else
        s->session_active++;
}

void session_record_input(Session *s, time_t now) {
    if (!s->is_running) return;
    s->last_input = now;
    s->is_passive = 0;
}

int session_is_idle(const Session *s, time_t now) {
    if (!s->is_running) return 0;
    return (int)(now - s->last_input) >= IDLE_THRESHOLD;
}
