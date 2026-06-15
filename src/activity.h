#ifndef ACTIVITY_H
#define ACTIVITY_H

#include <time.h>

#define MAX_NAME_LEN    64
#define MAX_DESC_LEN    256
#define MAX_ACTIVITIES  512
#define IDLE_THRESHOLD  30

typedef enum {
    STATUS_IN_PROGRESS = 0,
    STATUS_FINISHED    = 1
} ActivityStatus;

typedef struct {
    int            id;
    char           name[MAX_NAME_LEN];
    char           description[MAX_DESC_LEN];
    ActivityStatus status;
    time_t         created_at;
    long           total_seconds;
    long           active_seconds;
    long           passive_seconds;
} Activity;

typedef struct {
    Activity items[MAX_ACTIVITIES];
    int      count;
    int      next_id;
} ActivityList;

typedef struct {
    int    activity_idx;   /* -1 = none */
    int    is_running;
    time_t session_start;
    time_t last_input;
    long   session_total;
    long   session_active;
    long   session_passive;
    int    is_passive;
} Session;

void activity_list_init(ActivityList *list);
int  activity_add(ActivityList *list, const char *name, const char *desc);
void activity_finish(ActivityList *list, int idx);
void activity_delete(ActivityList *list, int idx);

void session_init(Session *s);
void session_start(Session *s, int idx);
void session_stop(Session *s, ActivityList *list);
void session_tick(Session *s);
void session_record_input(Session *s, time_t now);
int  session_is_idle(const Session *s, time_t now);

#endif
