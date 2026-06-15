# Productivity Analyzer

A terminal-based productivity tracker written in C using ncurses.  
Track time spent on projects with automatic detection of **active** (typing/mouse) vs **passive** (idle) time.

```
┌─────────────────────────────────────────────  2026-06-15  14:32:05 ─┐
│─────────────────────────────────────────────────────────────────────│
│ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐     │
│ │ In Progress │ │  Finished   │ │ Total Time  │ │ Active Time │     │
│ │      2      │ │      1      │ │  5h 12m     │ │  4h 03m     │     │
│ └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘     │
│                                                                     │
│ ┌ Running Session ──────────────────────────────────────────────┐   │
│ │ Backend API refactor                                          │   │
│ │ Session: 1h 05m    Active: 52m    Passive: 13m                │   │
│ │ [ACTIVE]   idle 4s / 30s threshold                            │   │
│ └───────────────────────────────────────────────────────────────┘   │
│                                                                     │
│ Recent Activities                                                   │
│ > Backend API refactor       [RUNNING ]  5h 12m                     │
│ - Frontend redesign          [paused  ]  2h 08m                     │
│   Write unit tests           [finished]  1h 44m                     │
│─────────────────────────────────────────────────────────────────────│
│ [n]ew  [l]ist  [Enter]start/stop  [f]inish  [d]elete  [q]uit        │
└─────────────────────────────────────────────────────────────────────┘
```

## Features

- **Dashboard** — live counters for in-progress, finished, and total/active time
- **Project list** — tabular view with per-project time breakdown
- **Session timer** — start and stop a timer on any project; only one session runs at a time
- **Active / passive time** — keyboard and mouse events are monitored; after 30 seconds of silence the timer switches to _passive_ mode automatically
- **Detail view** — per-project active-time progress bar
- **Persistent storage** — data is written to `productivity.dat` after every action and on quit
- **Confirmation dialogs** — destructive actions (delete) require an explicit confirmation

## Requirements

- `clang` (or `gcc`) with C99 support
- `libncurses` development headers

### Install dependencies

**Debian / Ubuntu**
```bash
sudo apt install clang libncurses-dev
```

**Arch Linux**
```bash
sudo pacman -S clang ncurses
```

**macOS (Homebrew)**
```bash
brew install llvm ncurses
```

## Build

```bash
make          # compiles into ./productivity
make run      # build + launch
make clean    # remove build artefacts and binary
```

Object files land in `build/`; the data file (`productivity.dat`) is created in the working directory on first run.

## Key bindings

### Dashboard & Project list

| Key | Action |
|-----|--------|
| `n` | New activity |
| `l` | Switch to project list |
| `d` | Switch to dashboard |
| `↑` / `↓` | Navigate |
| `Enter` | Start / stop timer on selected project |
| `f` | Mark selected project as finished |
| `x` / `d` | Delete selected project (confirmation required) |
| `v` / `i` | Open detail view (project list only) |
| `q` | Quit and save |

### Add dialog

| Key | Action |
|-----|--------|
| `Tab` / `Shift-Tab` | Move between Name and Description fields |
| `Enter` | Save and close |
| `Esc` | Cancel |

### Detail view

| Key | Action |
|-----|--------|
| `Esc` / `b` | Back to project list |
| `Enter` | Start / stop timer |
| `f` | Mark as finished |

### Confirmation dialog

| Key | Action |
|-----|--------|
| `y` | Confirm delete |
| `n` / `Esc` | Cancel |

## Project structure

```
.
├── Makefile
├── README.md
├── .gitignore
├── src/
│   ├── main.c        # event loop, input dispatch
│   ├── activity.c/h  # data structures, session and timer logic
│   ├── storage.c/h   # binary file persistence
│   └── ui.c/h        # ncurses rendering, all views
└── build/            # generated — ignored by git
```

## Extending

| What | Where |
|------|-------|
| Change idle threshold | `IDLE_THRESHOLD` in `src/activity.h` |
| Add a new view | Add a `VIEW_*` entry in `ui.h`, a `view_*()` function in `ui.c`, and a handler branch in `main.c` |
| Add new fields to `Activity` | Extend the struct in `activity.h`; bump `VERSION` in `storage.c` so old files are rejected cleanly |
| Switch to a different compiler | Change `CC` in `Makefile` |

## License

MIT
