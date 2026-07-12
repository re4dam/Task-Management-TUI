# Task Manager TUI (C++)

A lightweight, highly responsive Terminal User Interface (TUI) application written in modern C++ for managing categorized tasks, assignments, and activities. Built on top of `ncurses`, this application is optimized for performance, snappiness, and zero screen flickering.

---

## 🚀 Features

### 📋 Core Task Management
- **Categorized Tasks**: Organize tasks into different lists/categories (e.g., Inbox, Work, Personal).
- **Task Types**: 
  - **Activity**: Tasks with a designated start date/time.
  - **Assignment**: Tasks with a designated deadline date/time.
- **Completed Tasks Visual Group**: Completed tasks automatically move to a distinct visual section at the bottom of the list.
- **Sorting & Reordering**:
  - Sort categories alphabetically or shift their positions manually.
  - Sort tasks alphabetically, by date/time (earliest/latest), or shift task priorities up/down manually.

### ⚡ Advanced Actions & Automation
- **Task Snooze/Postponement (`p`)**: Postpone a task's start/due date by +1 Hour, +1 Day, or +1 Week instantly through a fast selection picker.
- **Task Transfer/Movement (`m`)**: Move a task out of its current category list and directly into any other user-selected category.
- **Task Recurrence Rules**: Add daily, weekly, or monthly recurrence rules to tasks. Toggling them complete automatically spawns the next instance advanced by the designated interval.
- **Date/Time Preset Picker**: Prompts for date/time using friendly shortcuts (**Today**, **Tomorrow**, **Next Week**, **Custom**, **None**) and an optional time field (defaulting to `12:00`), bypassing manual format typing.

### 🎨 Advanced Terminal UX & Polish
- **Double-Buffered Rendering**: Utilizes `wnoutrefresh()` and `doupdate()` to update the terminal window in a single frame update, preventing any screen flickering.
- **Zero ESC Delay**: Configured with a `25ms` ESC timeout for instant dialog cancellation responsiveness.
- **Vim-Style Navigation**: Support for standard vim keys (`j`/`k` to navigate list items, `h`/`l` to switch pane focus).
- **Interactive Real-Time Filter/Search (`/`)**: Drops input focus into a bottom border text search field. As you type, the task pane filters titles in real-time.
- **Relative Countdown Display**: Displays a dynamic, relative countdown (e.g. `(In 2 days, 3 hours)` or `(Overdue by 5 hours)`) underneath the static timestamp in the Task Details Pane.

### 💾 Storage & Portability
- **XDG Base Directory Compliance**: Saves configuration dynamically to `$XDG_CONFIG_HOME/task-tui/tasks.txt`. Falls back to `$HOME/.config/task-tui/tasks.txt` if `$XDG_CONFIG_HOME` is unset. Parent directories are created automatically on startup.
- **Extended Serialization**: Employs a fully backwards-compatible serialization piping layout to save and load recurrence states cleanly.

---

## 🛠️ Technology Stack

- **Language**: C++17 (compiled using `g++`)
- **UI Library**: `ncurses` (terminal display and keyboard event processing library)
- **Build System**: `GNU Make`

---

## 📂 Project Structure

```
Task-Management-TUI/
├── src/
│   ├── main.cpp        # Application entry point & main event/drawing loop
│   ├── tui.hpp         # Ncurses wrapper functions, modal rendering & input dialogs
│   ├── storage.hpp     # Flat-file serializer and deserializer logic
│   ├── list.hpp        # Data structure for categories (List)
│   └── task.hpp        # Data structure for tasks (Task / TaskType)
├── Makefile            # Build instructions
├── TODO.md             # Project roadmap and completed tracker
└── README.md           # Documentation (This file)
```

---

## 🎮 Keybindings & Controls

The interface is divided into pane columns: **Categories Pane** on the left and **Tasks Pane** + **Task Details Pane** on the right.

### General Navigation
- `Tab` : Switch focus between the **Categories** pane and the **Tasks** pane.
- `h` / `l` : Vim-style pane switching (pressing `h` in Tasks pane switches to Categories, pressing `l` in Categories pane switches to Tasks).
- `/` : Drop focus to the bottom search bar (type query and press `Enter` to scroll matches, press `Esc` to clear search).
- `q` : Save progress and exit.

### Pane Controls (Varies by focus)

| Key | Categories Focused | Tasks Focused |
|:---:|:---|:---|
| `Up` / `Down` or `k` / `j` | Navigate through categories | Navigate through tasks |
| `Space` | *N/A* | Toggle selected task completion (triggers recurrence if configured) |
| `n` or `a` | Create a new Category | Create a new Task (selects Type, Date Preset, and Recurrence) |
| `e` | Rename selected Category | Edit selected Task (Title, Description, Date Preset, Recurrence) |
| `d` or `x` | Delete selected Category | Delete selected Task |
| `s` | Open Sort/Shift menu for Categories | Open Sort/Shift menu for Tasks |
| `p` | *N/A* | Snooze task (postpone date/time by +1 hour, +1 day, or +1 week) |
| `m` | *N/A* | Move task to a different Category list |

---

## 🏁 Getting Started

### Prerequisites
Make sure you have a C++ compiler supporting C++17 and the `ncurses` development library installed.

**On Fedora / RHEL / CentOS:**
```bash
sudo dnf install gcc-c++ ncurses-devel make
```

**On Debian / Ubuntu:**
```bash
sudo apt-get install g++ libncurses5-dev libncursesw5-dev make
```

**On macOS (using Homebrew):**
```bash
brew install ncurses
```

### Installation & Build

1. Clone this repository:
   ```bash
   git clone https://github.com/your-username/Task-Management-TUI.git
   cd Task-Management-TUI
   ```

2. Build the binary target:
   ```bash
   make
   ```

3. Install the application:
   - **System-wide installation** (installs to `/usr/local/bin`):
     ```bash
     sudo make install
     ```
   - **User-only installation** (installs to your home directory, e.g. `~/.local/bin`):
     ```bash
     make install PREFIX=$HOME/.local
     ```

4. Run the application from anywhere:
   ```bash
   dotui
   ```

---

## 💾 Storage Format

Data is serialized to the XDG config folder (`$HOME/.config/task-tui/tasks.txt`). The serialization format uses plain piping:
- **Category format**: `L|<Category Name>`
- **Task format**: `T|<Completed (0/1)>|<Type (ACT/ASM)>|<Title>|<Description>|<DateTime>|<Recurrence (NONE/DAILY/WEEKLY/MONTHLY)>`

Example contents of `tasks.txt`:
```txt
L|Inbox
T|0|ACT|Prepare meeting|Prepare presentation slides|2026-07-15 10:00|NONE
T|1|ASM|Write essay|Submit via portal|2026-07-17 17:00|WEEKLY
L|Work
T|0|ACT|Team Sync|Discuss sprint goals|2026-07-13 09:00|DAILY
```
