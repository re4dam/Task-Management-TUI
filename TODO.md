# Tasks and Bugs Tracking

This file tracks the upcoming ToDo features, known bugs, completed features, and fixed bugs found during the testing of the Task Management TUI application.

---

## 📋 ToDo

*(No active items at the moment)*

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

