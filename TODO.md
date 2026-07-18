# Tasks and Bugs Tracking

This file tracks the upcoming ToDo features, known bugs, completed features, and fixed bugs found during the testing of the doTUI application.

---

## 📋 ToDo

- Planned to implement a VIM Style keybinds
- Redesign the UI with the left pane for Categories, middle pane for Tasks, and right pane for Task Details
- Planned to create a command feature for quick task creation and management
- When adding/editing a calendar dates, the date picker will open an intuitive calendar view for easier selection
- Redesign the keybinds to be configurable and user-friendly
- The keybinds help menu will open and cover the entire screen for better visibility and usability
- When the Tasks are empty within a category, a help showing the what keybinds can do to help the user in to do within that features.
- Themes selection feature to allow users to customize the look and feel of the application
- There will be a set of given themes to choose from for the based configuration, and users can also create their own themes by modifying the color scheme and other visual elements.
- The configuration file should all be in ~/.config/dotui/. Any configuration files should be stored in this directory, and the application should read from and write to this location for all user settings and preferences.

---

## 🐛 Known Bugs

*(No active bugs at the moment)*

---

## ✅ Completed ToDo

- [x] **Improve Snappy and Responsiveness of TUI**:
  - Decreased ESC key latency from 1s to 25ms (`set_escdelay`).
  - Replaced individual `wrefresh()` calls in the main draw loop with `wnoutrefresh()` and added a single `doupdate()` frame update.
  - Removed unnecessary global `clear()` calls when closing modal/popup windows to eliminate physical clear flicker.
- [x] **Update / Edit existing Task or List**:
  - The creation, deletion, and editing/renaming functionalities are successfully implemented.
- [x] **Every Task Completed Menu**:
  - Completed tasks are moved to a distinct "Completed Tasks" visual group at the bottom of the list.
- [x] **Sorting Task or List**:
  - Tasks can be sorted by title, date/time, or manually shifted up and down.
  - Categories can be sorted alphabetically or manually shifted.

---

## 🛡️ Fixed Bugs

- [x] **Popup Menu Text Overflow**:
  - **Issue**: Instructional words overflowed in popup menus.
  - **Solution**: Enlarged popups to 60 columns and centered instructions to fit perfectly. Included dynamic wrapping for confirmation dialog messages.
