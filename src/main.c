#define _XOPEN_SOURCE 600
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "activity.h"
#include "storage.h"
#include "ui.h"

#define DATA_FILE "productivity.dat"

static volatile int g_resize = 0;

static void handle_sigwinch(int sig) {
    (void)sig;
    g_resize = 1;
}

static void clamp_selected(UIState *ui, const ActivityList *list) {
    if (list->count == 0) { ui->selected = 0; ui->scroll = 0; return; }
    if (ui->selected < 0) ui->selected = 0;
    if (ui->selected >= list->count) ui->selected = list->count - 1;

    /* keep scroll window around selected */
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)cols;
    int visible = rows - 7;   /* approximate list area height */
    if (visible < 1) visible = 1;
    if (ui->selected < ui->scroll) ui->scroll = ui->selected;
    if (ui->selected >= ui->scroll + visible) ui->scroll = ui->selected - visible + 1;
}

/* --------------------------------------------------------- input handlers */

static void input_char(UIState *ui, int ch) {
    int field = ui->active_field;
    int max   = (field == FIELD_NAME) ? MAX_NAME_LEN - 1 : MAX_DESC_LEN - 1;
    if (ui->input_len[field] < max) {
        ui->input[field][ui->input_len[field]++] = (char)ch;
        ui->input[field][ui->input_len[field]]   = '\0';
    }
}

static void input_backspace(UIState *ui) {
    int field = ui->active_field;
    if (ui->input_len[field] > 0)
        ui->input[field][--ui->input_len[field]] = '\0';
}

static void open_add_dialog(UIState *ui) {
    ui->view = VIEW_ADD;
    ui->active_field = FIELD_NAME;
    memset(ui->input, 0, sizeof(ui->input));
    memset(ui->input_len, 0, sizeof(ui->input_len));
}

static void toggle_session(UIState *ui, ActivityList *list, Session *s, int idx) {
    (void)ui;
    if (idx < 0 || idx >= list->count) return;
    if (list->items[idx].status == STATUS_FINISHED) return;

    if (s->is_running) {
        if (s->activity_idx == idx) {
            session_stop(s, list);
        } else {
            session_stop(s, list);
            session_start(s, idx);
        }
    } else {
        session_start(s, idx);
    }
}

/* ----------------------------------------------- per-view input dispatch */

static int handle_dashboard(UIState *ui, ActivityList *list, Session *s, int ch) {
    switch (ch) {
    case 'n': open_add_dialog(ui); break;
    case 'l': ui->view = VIEW_PROJECTS; ui->scroll = 0; break;
    case KEY_UP:    ui->selected--; clamp_selected(ui, list); break;
    case KEY_DOWN:  ui->selected++; clamp_selected(ui, list); break;
    case '\n':
    case KEY_ENTER:
        if (list->count > 0) {
            int idx = list->count - 1 - ui->selected;
            if (idx < 0) idx = 0;
            toggle_session(ui, list, s, idx);
        }
        break;
    case 'f':
        if (list->count > 0) {
            int idx = list->count - 1 - ui->selected;
            if (s->is_running && s->activity_idx == idx)
                session_stop(s, list);
            activity_finish(list, idx);
        }
        break;
    case 'd':
        if (list->count > 0) {
            int idx = list->count - 1 - ui->selected;
            ui->confirm_idx    = idx;
            ui->confirm_choice = 0;
            ui->view = VIEW_CONFIRM_DEL;
        }
        break;
    case KEY_ENTER+1: /* detail on specific key */
        break;
    default: return 0;
    }
    return 1;
}

static int handle_projects(UIState *ui, ActivityList *list, Session *s, int ch) {
    switch (ch) {
    case 'n': open_add_dialog(ui); break;
    case 'd': ui->view = VIEW_DASHBOARD; break;
    case KEY_UP:   ui->selected--; clamp_selected(ui, list); break;
    case KEY_DOWN: ui->selected++; clamp_selected(ui, list); break;
    case '\n':
    case KEY_ENTER:
        toggle_session(ui, list, s, ui->selected);
        break;
    case 'v':
    case 'i':
        ui->view = VIEW_DETAIL;
        break;
    case 'f':
        if (list->count > 0) {
            int idx = ui->selected;
            if (s->is_running && s->activity_idx == idx)
                session_stop(s, list);
            activity_finish(list, idx);
        }
        break;
    case 'x':
        if (list->count > 0) {
            ui->confirm_idx    = ui->selected;
            ui->confirm_choice = 0;
            ui->view = VIEW_CONFIRM_DEL;
        }
        break;
    default: return 0;
    }
    return 1;
}

static int handle_add(UIState *ui, ActivityList *list, int ch) {
    switch (ch) {
    case 27:  /* Esc */
        ui->view = (ui->view == VIEW_ADD) ? VIEW_DASHBOARD : ui->view;
        ui->view = VIEW_DASHBOARD;
        curs_set(0);
        break;
    case '\t':
        ui->active_field = (ui->active_field + 1) % FIELD_COUNT;
        break;
    case KEY_BTAB:
        ui->active_field = (ui->active_field - 1 + FIELD_COUNT) % FIELD_COUNT;
        break;
    case '\n':
    case KEY_ENTER:
        if (ui->input_len[FIELD_NAME] > 0) {
            activity_add(list, ui->input[FIELD_NAME], ui->input[FIELD_DESC]);
            ui->selected = 0;
            clamp_selected(ui, list);
            ui->view = VIEW_DASHBOARD;
            curs_set(0);
        }
        break;
    case KEY_BACKSPACE:
    case 127:
    case '\b':
        input_backspace(ui);
        break;
    default:
        if (ch >= 32 && ch < 127)
            input_char(ui, ch);
        break;
    }
    return 1;
}

static int handle_detail(UIState *ui, ActivityList *list, Session *s, int ch) {
    switch (ch) {
    case 27:
    case 'b':
        ui->view = VIEW_PROJECTS;
        break;
    case '\n':
    case KEY_ENTER:
        toggle_session(ui, list, s, ui->selected);
        break;
    case 'f':
        if (ui->selected >= 0 && ui->selected < list->count) {
            if (s->is_running && s->activity_idx == ui->selected)
                session_stop(s, list);
            activity_finish(list, ui->selected);
        }
        break;
    default: return 0;
    }
    return 1;
}

static int handle_confirm_del(UIState *ui, ActivityList *list, Session *s, int ch) {
    switch (ch) {
    case KEY_LEFT:
    case KEY_RIGHT:
        ui->confirm_choice ^= 1;
        break;
    case 'y':
    case 'Y':
        ui->confirm_choice = 1;
        /* fall through */
    case '\n':
    case KEY_ENTER:
        if (ui->confirm_choice == 1) {
            if (s->is_running && s->activity_idx == ui->confirm_idx)
                session_stop(s, list);
            activity_delete(list, ui->confirm_idx);
            clamp_selected(ui, list);
        }
        ui->view = VIEW_PROJECTS;
        break;
    case 'n':
    case 'N':
    case 27:
        ui->view = VIEW_PROJECTS;
        break;
    default: return 0;
    }
    return 1;
}

/* --------------------------------------------------------------- main loop */

int main(void) {
    ActivityList list;
    Session      session;
    UIState      ui;

    signal(SIGWINCH, handle_sigwinch);

    storage_load(&list, DATA_FILE);
    session_init(&session);
    ui_init(&ui);

    time_t last_tick = time(NULL);

    for (;;) {
        if (g_resize) {
            g_resize = 0;
            ui_resize(&ui);
        }

        if (ui.needs_redraw) {
            ui_draw(&ui, &list, &session);
            ui.needs_redraw = 0;
        }

        int ch = getch();
        time_t now = time(NULL);

        /* timer tick: once per second */
        if (now != last_tick) {
            last_tick = now;
            if (session.is_running) {
                session.is_passive = session_is_idle(&session, now);
                session_tick(&session);
            }
            ui.needs_redraw = 1;
        }

        if (ch == ERR) continue;   /* timeout, no key */

        /* record any input for active/passive tracking */
        session_record_input(&session, now);

        /* global keys */
        if (ui.view != VIEW_ADD) {
            if (ch == 'q' || ch == 'Q') break;
        }

        /* mouse events count as input but we don't act on them further */
        if (ch == KEY_MOUSE) {
            MEVENT ev;
            getmouse(&ev);
            ui.needs_redraw = 1;
            continue;
        }

        /* per-view input */
        switch (ui.view) {
        case VIEW_DASHBOARD:   handle_dashboard(&ui, &list, &session, ch); break;
        case VIEW_PROJECTS:    handle_projects(&ui, &list, &session, ch);  break;
        case VIEW_ADD:         handle_add(&ui, &list, ch);                 break;
        case VIEW_DETAIL:      handle_detail(&ui, &list, &session, ch);    break;
        case VIEW_CONFIRM_DEL: handle_confirm_del(&ui, &list, &session, ch); break;
        }

        ui.needs_redraw = 1;
        storage_save(&list, DATA_FILE);
    }

    session_stop(&session, &list);
    storage_save(&list, DATA_FILE);
    ui_cleanup();
    return 0;
}
