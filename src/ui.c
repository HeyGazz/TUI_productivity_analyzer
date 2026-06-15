#define _XOPEN_SOURCE 600
#include "ui.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ------------------------------------------------------------------ helpers */

void ui_format_duration(char *buf, int size, long seconds) {
    long h = seconds / 3600;
    long m = (seconds % 3600) / 60;
    long s = seconds % 60;
    if (h > 0)
        snprintf(buf, (size_t)size, "%ldh %02ldm", h, m);
    else if (m > 0)
        snprintf(buf, (size_t)size, "%ldm %02lds", m, s);
    else
        snprintf(buf, (size_t)size, "%lds", s);
}

static void draw_hline(int y, int x, int width, int cp) {
    int i;
    attron(COLOR_PAIR(cp));
    move(y, x);
    for (i = 0; i < width; i++) addch(ACS_HLINE);
    attroff(COLOR_PAIR(cp));
}

static void draw_box(int y, int x, int h, int w, int cp) {
    int i;
    attron(COLOR_PAIR(cp));
    mvaddch(y, x, ACS_ULCORNER);
    for (i = 1; i < w - 1; i++) addch(ACS_HLINE);
    addch(ACS_URCORNER);
    for (i = 1; i < h - 1; i++) {
        mvaddch(y + i, x,         ACS_VLINE);
        mvaddch(y + i, x + w - 1, ACS_VLINE);
    }
    mvaddch(y + h - 1, x, ACS_LLCORNER);
    for (i = 1; i < w - 1; i++) addch(ACS_HLINE);
    addch(ACS_LRCORNER);
    attroff(COLOR_PAIR(cp));
}

static void draw_title_bar(int cols) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char timebuf[32];
    char titlebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d  %H:%M:%S", tm_info);

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    move(0, 0);
    clrtoeol();
    snprintf(titlebuf, sizeof(titlebuf), "  Productivity Tracker");
    mvaddstr(0, 0, titlebuf);
    mvaddstr(0, cols - (int)strlen(timebuf) - 2, timebuf);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    draw_hline(1, 0, cols, CP_HEADER);
}

static void draw_footer(int rows, int cols, ViewType view) {
    draw_hline(rows - 2, 0, cols, CP_DIM);
    attron(COLOR_PAIR(CP_DIM));
    move(rows - 1, 0);
    clrtoeol();
    switch (view) {
    case VIEW_DASHBOARD:
        mvaddstr(rows - 1, 1,
            "[n]ew  [l]ist  [Enter]start/stop  [f]inish  [d]elete  [q]uit");
        break;
    case VIEW_PROJECTS:
        mvaddstr(rows - 1, 1,
            "[n]ew  [d]ashboard  [Enter]start/stop  [f]inish  [x]delete  [q]uit");
        break;
    case VIEW_ADD:
        mvaddstr(rows - 1, 1,
            "[Tab]next field  [Enter]save  [Esc]cancel");
        break;
    case VIEW_DETAIL:
        mvaddstr(rows - 1, 1,
            "[Esc/b]back  [Enter]start/stop  [f]inish");
        break;
    case VIEW_CONFIRM_DEL:
        mvaddstr(rows - 1, 1,
            "[y]es delete  [n/Esc]cancel");
        break;
    }
    attroff(COLOR_PAIR(CP_DIM));
}

/* ---------------------------------------------------------------- stat card */

static void draw_stat_card(int y, int x, int w, const char *label,
                           const char *value, int cp_val) {
    draw_box(y, x, 4, w, CP_DIM);
    attron(COLOR_PAIR(CP_DIM));
    int lx = x + (w - (int)strlen(label)) / 2;
    mvaddstr(y + 1, lx, label);
    attroff(COLOR_PAIR(CP_DIM));
    attron(COLOR_PAIR(cp_val) | A_BOLD);
    int vx = x + (w - (int)strlen(value)) / 2;
    mvaddstr(y + 2, vx, value);
    attroff(COLOR_PAIR(cp_val) | A_BOLD);
}

/* --------------------------------------------------------------- dashboard */

static void draw_session_panel(int y, int x, int w,
                               const ActivityList *list, const Session *s) {
    char dur[32], adur[32], pdur[32];
    int inner = w - 4;

    draw_box(y, x, 6, w, CP_RUNNING);
    attron(COLOR_PAIR(CP_RUNNING) | A_BOLD);
    mvaddstr(y, x + 2, " Running Session ");
    attroff(COLOR_PAIR(CP_RUNNING) | A_BOLD);

    if (!s->is_running || s->activity_idx < 0) {
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(y + 2, x + (w - 18) / 2, "No active session");
        attroff(COLOR_PAIR(CP_DIM));
        return;
    }

    const Activity *a = &list->items[s->activity_idx];
    long total   = a->total_seconds   + s->session_total;
    long active  = a->active_seconds  + s->session_active;
    long passive = a->passive_seconds + s->session_passive;

    ui_format_duration(dur,  sizeof(dur),  s->session_total);
    ui_format_duration(adur, sizeof(adur), s->session_active);
    ui_format_duration(pdur, sizeof(pdur), s->session_passive);

    char line1[128], line2[128], line3[128];
    snprintf(line1, sizeof(line1), "%-*s", inner, a->name);
    snprintf(line2, sizeof(line2), "Session: %-10s  Active: %-10s  Passive: %s",
             dur, adur, pdur);

    long idle_secs = (long)(time(NULL) - s->last_input);
    if (s->is_passive)
        snprintf(line3, sizeof(line3), "[PASSIVE]  idle %lds", idle_secs);
    else if (idle_secs > 0)
        snprintf(line3, sizeof(line3), "[ACTIVE]   idle %lds / %ds threshold",
                 idle_secs, IDLE_THRESHOLD);
    else
        snprintf(line3, sizeof(line3), "[ACTIVE]");

    (void)total; (void)active; (void)passive;

    attron(COLOR_PAIR(CP_ACTIVE) | A_BOLD);
    mvaddstr(y + 1, x + 2, line1);
    attroff(COLOR_PAIR(CP_ACTIVE) | A_BOLD);

    attron(COLOR_PAIR(CP_NORMAL));
    mvaddnstr(y + 2, x + 2, line2, inner);
    attroff(COLOR_PAIR(CP_NORMAL));

    int cp = s->is_passive ? CP_PASSIVE : CP_ACTIVE;
    attron(COLOR_PAIR(cp));
    mvaddnstr(y + 3, x + 2, line3, inner);
    attroff(COLOR_PAIR(cp));
}

static void draw_recent_list(int y, int x, int w, int max_rows,
                             const ActivityList *list, const Session *s,
                             int selected) {
    int i;
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvaddstr(y, x, "Recent Activities");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
    y++;

    int shown = 0;
    /* show running first, then rest in reverse order */
    for (i = list->count - 1; i >= 0 && shown < max_rows; i--) {
        const Activity *a = &list->items[i];
        int is_sel     = (i == selected);
        int is_running = (s->is_running && s->activity_idx == i);
        char dur[32];
        long total = a->total_seconds + (is_running ? s->session_total : 0);
        ui_format_duration(dur, sizeof(dur), total);

        if (is_sel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (is_running) attron(COLOR_PAIR(CP_ACTIVE));
        else if (a->status == STATUS_FINISHED) attron(COLOR_PAIR(CP_FINISHED));
        else attron(COLOR_PAIR(CP_NORMAL));

        move(y + shown, x);
        clrtoeol();

        char prefix = is_running ? '>' : (a->status == STATUS_FINISHED ? ' ' : '-');
        char line[128];
        const char *status_str = is_running      ? "RUNNING " :
                                  a->status == STATUS_FINISHED ? "FINISHED" : "paused  ";
        snprintf(line, sizeof(line), "%c %-28s [%s]  %s", prefix,
                 a->name, status_str, dur);
        mvaddnstr(y + shown, x, line, w - 1);

        if (is_sel) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (is_running) attroff(COLOR_PAIR(CP_ACTIVE));
        else if (a->status == STATUS_FINISHED) attroff(COLOR_PAIR(CP_FINISHED));
        else attroff(COLOR_PAIR(CP_NORMAL));

        shown++;
    }

    if (shown == 0) {
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(y, x, "No activities yet. Press [n] to add one.");
        attroff(COLOR_PAIR(CP_DIM));
    }
}

static void view_dashboard(UIState *ui, const ActivityList *list,
                           const Session *s) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int body_y = 2;

    /* Stats row */
    int in_progress = 0, finished = 0;
    long total_secs = 0, total_active = 0;
    int i;
    for (i = 0; i < list->count; i++) {
        const Activity *a = &list->items[i];
        if (a->status == STATUS_FINISHED) finished++;
        else in_progress++;
        long t  = a->total_seconds  + (s->is_running && s->activity_idx == i ? s->session_total  : 0);
        long ac = a->active_seconds + (s->is_running && s->activity_idx == i ? s->session_active : 0);
        total_secs   += t;
        total_active += ac;
    }

    char v1[32], v2[32], v3[32], v4[32];
    snprintf(v1, sizeof(v1), "%d", in_progress);
    snprintf(v2, sizeof(v2), "%d", finished);
    ui_format_duration(v3, sizeof(v3), total_secs);
    ui_format_duration(v4, sizeof(v4), total_active);

    int card_w = (cols - 2) / 4;
    draw_stat_card(body_y, 0,           card_w, "In Progress",  v1, CP_ACTIVE);
    draw_stat_card(body_y, card_w,      card_w, "Finished",     v2, CP_FINISHED);
    draw_stat_card(body_y, card_w * 2,  card_w, "Total Time",   v3, CP_HEADER);
    draw_stat_card(body_y, card_w * 3,  cols - card_w * 3,
                   "Active Time", v4, CP_RUNNING);

    body_y += 5;

    /* Running session panel */
    draw_session_panel(body_y, 0, cols, list, s);
    body_y += 7;

    /* Recent list */
    int list_rows = rows - body_y - 2;
    if (list_rows > 0)
        draw_recent_list(body_y, 0, cols, list_rows, list, s, ui->selected);
}

/* --------------------------------------------------------------- projects */

static void view_projects(UIState *ui, const ActivityList *list,
                          const Session *s) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int body_y = 2;
    int max_rows = rows - body_y - 2;
    int i;

    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvaddstr(body_y, 0, " All Projects");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);
    draw_hline(body_y + 1, 0, cols, CP_DIM);
    body_y += 2;

    /* Column headers */
    attron(COLOR_PAIR(CP_DIM) | A_BOLD);
    char hdr[256];
    snprintf(hdr, sizeof(hdr), "  %-30s %-10s %10s %10s %10s",
             "Name", "Status", "Total", "Active", "Passive");
    mvaddnstr(body_y, 0, hdr, cols - 1);
    attroff(COLOR_PAIR(CP_DIM) | A_BOLD);
    body_y++;

    int start = ui->scroll;
    int end = start + max_rows - 2;
    if (end > list->count) end = list->count;

    for (i = start; i < end; i++) {
        const Activity *a = &list->items[i];
        int is_sel     = (i == ui->selected);
        int is_running = (s->is_running && s->activity_idx == i);
        long total   = a->total_seconds   + (is_running ? s->session_total   : 0);
        long active  = a->active_seconds  + (is_running ? s->session_active  : 0);
        long passive = a->passive_seconds + (is_running ? s->session_passive : 0);
        char tdur[20], adur[20], pdur[20];
        ui_format_duration(tdur, sizeof(tdur), total);
        ui_format_duration(adur, sizeof(adur), active);
        ui_format_duration(pdur, sizeof(pdur), passive);

        const char *status_str = is_running      ? "RUNNING " :
                                  a->status == STATUS_FINISHED ? "finished" : "paused  ";

        if (is_sel) attron(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (is_running) attron(COLOR_PAIR(CP_ACTIVE));
        else if (a->status == STATUS_FINISHED) attron(COLOR_PAIR(CP_FINISHED));
        else attron(COLOR_PAIR(CP_DIM));

        move(body_y + (i - start), 0);
        clrtoeol();
        char line[256];
        snprintf(line, sizeof(line), "  %-30s %-10s %10s %10s %10s",
                 a->name, status_str, tdur, adur, pdur);
        mvaddnstr(body_y + (i - start), 0, line, cols - 1);

        if (is_sel) attroff(COLOR_PAIR(CP_SELECTED) | A_BOLD);
        else if (is_running) attroff(COLOR_PAIR(CP_ACTIVE));
        else if (a->status == STATUS_FINISHED) attroff(COLOR_PAIR(CP_FINISHED));
        else attroff(COLOR_PAIR(CP_DIM));
    }

    if (list->count == 0) {
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(body_y, 2, "No activities yet. Press [n] to add one.");
        attroff(COLOR_PAIR(CP_DIM));
    }

    /* scroll indicator */
    if (list->count > max_rows - 2) {
        attron(COLOR_PAIR(CP_DIM));
        char si[32];
        snprintf(si, sizeof(si), " %d/%d ", ui->selected + 1, list->count);
        mvaddstr(rows - 2, cols - (int)strlen(si) - 1, si);
        attroff(COLOR_PAIR(CP_DIM));
    }
}

/* --------------------------------------------------------------- add dialog */

static void view_add(UIState *ui) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int dh = 12, dw = 60;
    int dy = (rows - dh) / 2;
    int dx = (cols - dw) / 2;

    /* dim background */
    attron(COLOR_PAIR(CP_DIM));
    int r;
    for (r = 2; r < rows - 1; r++) {
        move(r, 0);
        clrtoeol();
    }
    attroff(COLOR_PAIR(CP_DIM));

    draw_box(dy, dx, dh, dw, CP_HEADER);
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    mvaddstr(dy, dx + 2, " New Activity ");
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    /* Name field */
    int cp_name = (ui->active_field == FIELD_NAME) ? CP_RUNNING : CP_DIM;
    attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvaddstr(dy + 2, dx + 2, "Name:");
    attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    attron(COLOR_PAIR(cp_name));
    draw_box(dy + 3, dx + 2, 3, dw - 4, cp_name);
    mvaddnstr(dy + 4, dx + 3, ui->input[FIELD_NAME],
              ui->input_len[FIELD_NAME]);
    attroff(COLOR_PAIR(cp_name));

    /* Description field */
    int cp_desc = (ui->active_field == FIELD_DESC) ? CP_RUNNING : CP_DIM;
    attron(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    mvaddstr(dy + 7, dx + 2, "Description (optional):");
    attroff(COLOR_PAIR(CP_NORMAL) | A_BOLD);
    attron(COLOR_PAIR(cp_desc));
    draw_box(dy + 8, dx + 2, 3, dw - 4, cp_desc);
    mvaddnstr(dy + 9, dx + 3, ui->input[FIELD_DESC],
              ui->input_len[FIELD_DESC]);
    attroff(COLOR_PAIR(cp_desc));

    /* Cursor */
    int cur_field = ui->active_field;
    int cur_y = (cur_field == FIELD_NAME) ? dy + 4 : dy + 9;
    int cur_x = dx + 3 + ui->input_len[cur_field];
    move(cur_y, cur_x);
    curs_set(1);
}

/* --------------------------------------------------------------- detail */

static void view_detail(UIState *ui, const ActivityList *list,
                        const Session *s) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    int body_y = 2;
    (void)rows;

    if (ui->selected < 0 || ui->selected >= list->count) {
        mvaddstr(body_y, 2, "No activity selected.");
        return;
    }

    const Activity *a = &list->items[ui->selected];
    int is_running = (s->is_running && s->activity_idx == ui->selected);

    long total   = a->total_seconds   + (is_running ? s->session_total   : 0);
    long active  = a->active_seconds  + (is_running ? s->session_active  : 0);
    long passive = a->passive_seconds + (is_running ? s->session_passive : 0);

    char tdur[32], adur[32], pdur[32];
    ui_format_duration(tdur, sizeof(tdur), total);
    ui_format_duration(adur, sizeof(adur), active);
    ui_format_duration(pdur, sizeof(pdur), passive);

    draw_box(body_y, 1, 12, cols - 2, CP_HEADER);
    int cp = is_running ? CP_ACTIVE : (a->status == STATUS_FINISHED ? CP_FINISHED : CP_NORMAL);
    attron(COLOR_PAIR(cp) | A_BOLD);
    mvaddstr(body_y + 1, 3, a->name);
    attroff(COLOR_PAIR(cp) | A_BOLD);

    if (a->description[0]) {
        attron(COLOR_PAIR(CP_DIM));
        mvaddnstr(body_y + 2, 3, a->description, cols - 6);
        attroff(COLOR_PAIR(CP_DIM));
    }

    draw_hline(body_y + 3, 2, cols - 4, CP_DIM);

    /* Time breakdown */
    attron(COLOR_PAIR(CP_NORMAL));
    mvaddstr(body_y + 4, 3, "Total time: ");
    attroff(COLOR_PAIR(CP_NORMAL));
    attron(COLOR_PAIR(CP_HEADER) | A_BOLD);
    addstr(tdur);
    attroff(COLOR_PAIR(CP_HEADER) | A_BOLD);

    attron(COLOR_PAIR(CP_NORMAL));
    mvaddstr(body_y + 5, 3, "Active time: ");
    attroff(COLOR_PAIR(CP_NORMAL));
    attron(COLOR_PAIR(CP_ACTIVE) | A_BOLD);
    addstr(adur);
    attroff(COLOR_PAIR(CP_ACTIVE) | A_BOLD);

    attron(COLOR_PAIR(CP_NORMAL));
    mvaddstr(body_y + 6, 3, "Passive time: ");
    attroff(COLOR_PAIR(CP_NORMAL));
    attron(COLOR_PAIR(CP_PASSIVE) | A_BOLD);
    addstr(pdur);
    attroff(COLOR_PAIR(CP_PASSIVE) | A_BOLD);

    /* Active % bar */
    if (total > 0) {
        int bar_w = cols - 20;
        int filled = (int)((double)active / (double)total * bar_w);
        attron(COLOR_PAIR(CP_DIM));
        mvaddstr(body_y + 8, 3, "Active:  [");
        attroff(COLOR_PAIR(CP_DIM));
        attron(COLOR_PAIR(CP_ACTIVE) | A_BOLD);
        int k;
        for (k = 0; k < filled; k++) addch('=');
        attroff(COLOR_PAIR(CP_ACTIVE) | A_BOLD);
        attron(COLOR_PAIR(CP_PASSIVE));
        for (k = filled; k < bar_w; k++) addch('-');
        attroff(COLOR_PAIR(CP_PASSIVE));
        attron(COLOR_PAIR(CP_DIM));
        char pct[16];
        snprintf(pct, sizeof(pct), "] %d%%",
                 (int)((double)active / (double)total * 100));
        addstr(pct);
        attroff(COLOR_PAIR(CP_DIM));
    }

    /* Status */
    attron(COLOR_PAIR(CP_NORMAL));
    mvaddstr(body_y + 10, 3, "Status: ");
    attroff(COLOR_PAIR(CP_NORMAL));
    attron(COLOR_PAIR(cp) | A_BOLD);
    addstr(is_running ? "RUNNING" :
           a->status == STATUS_FINISHED ? "Finished" : "Paused");
    attroff(COLOR_PAIR(cp) | A_BOLD);
}

/* --------------------------------------------------------------- confirm delete */

static void view_confirm_del(UIState *ui, const ActivityList *list) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (ui->confirm_idx < 0 || ui->confirm_idx >= list->count) return;

    int dh = 7, dw = 50;
    int dy = (rows - dh) / 2;
    int dx = (cols - dw) / 2;

    attron(COLOR_PAIR(CP_DIM));
    int r;
    for (r = 2; r < rows - 1; r++) { move(r, 0); clrtoeol(); }
    attroff(COLOR_PAIR(CP_DIM));

    draw_box(dy, dx, dh, dw, CP_DANGER);
    attron(COLOR_PAIR(CP_DANGER) | A_BOLD);
    mvaddstr(dy, dx + 2, " Delete Activity ");
    attroff(COLOR_PAIR(CP_DANGER) | A_BOLD);

    attron(COLOR_PAIR(CP_NORMAL));
    char msg[128];
    snprintf(msg, sizeof(msg), "Delete \"%s\"?",
             list->items[ui->confirm_idx].name);
    int mx = dx + (dw - (int)strlen(msg)) / 2;
    mvaddstr(dy + 2, mx, msg);
    attroff(COLOR_PAIR(CP_NORMAL));

    attron(COLOR_PAIR(CP_DIM));
    mvaddstr(dy + 3, dx + 3, "All tracked time will be lost.");
    attroff(COLOR_PAIR(CP_DIM));

    /* Yes / No buttons */
    int btn_y = dy + 5;
    if (ui->confirm_choice == 1) {
        attron(COLOR_PAIR(CP_DANGER) | A_BOLD | A_REVERSE);
        mvaddstr(btn_y, dx + dw / 2 - 12, "  [Y] Yes, delete  ");
        attroff(COLOR_PAIR(CP_DANGER) | A_BOLD | A_REVERSE);
        attron(COLOR_PAIR(CP_NORMAL));
        mvaddstr(btn_y, dx + dw / 2 + 8, "  [N] Cancel  ");
        attroff(COLOR_PAIR(CP_NORMAL));
    } else {
        attron(COLOR_PAIR(CP_DANGER));
        mvaddstr(btn_y, dx + dw / 2 - 12, "  [Y] Yes, delete  ");
        attroff(COLOR_PAIR(CP_DANGER));
        attron(COLOR_PAIR(CP_RUNNING) | A_BOLD | A_REVERSE);
        mvaddstr(btn_y, dx + dw / 2 + 8, "  [N] Cancel  ");
        attroff(COLOR_PAIR(CP_RUNNING) | A_BOLD | A_REVERSE);
    }
}

/* ---------------------------------------------------------------- public api */

void ui_init(UIState *ui) {
    int i;
    memset(ui, 0, sizeof(*ui));
    ui->view     = VIEW_DASHBOARD;
    ui->selected = 0;
    ui->needs_redraw = 1;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(500);   /* 500ms tick for timer updates */
    curs_set(0);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(CP_NORMAL,   COLOR_WHITE,   -1);
        init_pair(CP_ACTIVE,   COLOR_GREEN,   -1);
        init_pair(CP_FINISHED, COLOR_CYAN,    -1);
        init_pair(CP_SELECTED, COLOR_BLACK,   COLOR_WHITE);
        init_pair(CP_HEADER,   COLOR_CYAN,    -1);
        init_pair(CP_PASSIVE,  COLOR_MAGENTA, -1);
        init_pair(CP_DANGER,   COLOR_RED,     -1);
        init_pair(CP_DIM,      COLOR_WHITE,   -1);  /* will use A_DIM */
        init_pair(CP_RUNNING,  COLOR_GREEN,   -1);
    }

    /* enable mouse (movement + clicks) */
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    (void)i;
}

void ui_cleanup(void) {
    curs_set(1);
    endwin();
}

void ui_resize(UIState *ui) {
    endwin();
    refresh();
    clear();
    ui->needs_redraw = 1;
}

void ui_draw(UIState *ui, const ActivityList *list, const Session *session) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (ui->view != VIEW_ADD && ui->view != VIEW_CONFIRM_DEL) {
        curs_set(0);
        clear();
    }

    draw_title_bar(cols);

    switch (ui->view) {
    case VIEW_DASHBOARD:
        view_dashboard(ui, list, session);
        break;
    case VIEW_PROJECTS:
        view_projects(ui, list, session);
        break;
    case VIEW_ADD:
        view_add(ui);
        break;
    case VIEW_DETAIL:
        view_detail(ui, list, session);
        break;
    case VIEW_CONFIRM_DEL:
        view_confirm_del(ui, list);
        break;
    }

    draw_footer(rows, cols, ui->view);
    refresh();
}
