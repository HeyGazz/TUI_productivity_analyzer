#ifndef UI_H
#define UI_H

#include <ncurses.h>
#include "activity.h"

/* Color pair IDs */
#define CP_NORMAL     1
#define CP_ACTIVE     2
#define CP_FINISHED   3
#define CP_SELECTED   4
#define CP_HEADER     5
#define CP_PASSIVE    6
#define CP_DANGER     7
#define CP_DIM        8
#define CP_RUNNING    9

typedef enum {
    VIEW_DASHBOARD = 0,
    VIEW_PROJECTS,
    VIEW_ADD,
    VIEW_DETAIL,
    VIEW_CONFIRM_DEL
} ViewType;

/* Input field indices for the add dialog */
#define FIELD_NAME  0
#define FIELD_DESC  1
#define FIELD_COUNT 2

typedef struct {
    ViewType view;
    int      selected;
    int      scroll;
    int      needs_redraw;

    /* add/edit dialog */
    char     input[FIELD_COUNT][MAX_NAME_LEN > MAX_DESC_LEN ? MAX_NAME_LEN : MAX_DESC_LEN];
    int      input_len[FIELD_COUNT];
    int      active_field;

    /* confirmation dialog */
    int      confirm_idx;
    int      confirm_choice;   /* 0=no, 1=yes */
} UIState;

void ui_init(UIState *ui);
void ui_cleanup(void);
void ui_resize(UIState *ui);
void ui_draw(UIState *ui, const ActivityList *list, const Session *session);
void ui_format_duration(char *buf, int size, long seconds);

#endif
