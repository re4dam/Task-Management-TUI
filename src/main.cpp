#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ncurses.h>
#include "task.hpp"
#include "list.hpp"
#include "storage.hpp"
#include "tui.hpp"

const std::string DATA_FILE = Storage::get_data_filepath();

inline bool contains_substring(const std::string& str, const std::string& sub) {
    if (sub.empty()) return true;
    auto it = std::search(
        str.begin(), str.end(),
        sub.begin(), sub.end(),
        [](char ch1, char ch2) { return std::tolower(ch1) == std::tolower(ch2); }
    );
    return it != str.end();
}

void setup_windows(WINDOW*& header_win, WINDOW*& lists_win, WINDOW*& tasks_win, WINDOW*& details_win, WINDOW*& footer_win) {
    if (header_win) delwin(header_win);
    if (lists_win) delwin(lists_win);
    if (tasks_win) delwin(tasks_win);
    if (details_win) delwin(details_win);
    if (footer_win) delwin(footer_win);
    
    header_win = newwin(3, COLS, 0, 0);
    int main_h = LINES - 6;
    int lists_w = 28;
    if (lists_w > COLS - 15) lists_w = COLS / 3;
    int right_w = COLS - lists_w;
    
    lists_win = newwin(main_h, lists_w, 3, 0);
    
    int tasks_h = main_h * 0.6;
    if (tasks_h < 5) tasks_h = 5;
    int details_h = main_h - tasks_h;
    
    tasks_win = newwin(tasks_h, right_w, 3, lists_w);
    details_win = newwin(details_h, right_w, 3 + tasks_h, lists_w);
    
    footer_win = newwin(3, COLS, LINES - 3, 0);
    
    keypad(lists_win, TRUE);
    keypad(tasks_win, TRUE);
    keypad(details_win, TRUE);
}

int main() {
    Tui::init_tui();
    
    std::vector<List> lists = Storage::load_data(DATA_FILE);
    
    // Add default list if empty
    if (lists.empty()) {
        List default_list;
        default_list.name = "Inbox";
        lists.push_back(default_list);
    }
    
    size_t selected_list_idx = 0;
    size_t selected_task_idx = 0;
    Focus active_focus = Focus::Lists;
    std::string search_query = "";
    
    WINDOW* header_win = nullptr;
    WINDOW* lists_win = nullptr;
    WINDOW* tasks_win = nullptr;
    WINDOW* details_win = nullptr;
    WINDOW* footer_win = nullptr;
    
    setup_windows(header_win, lists_win, tasks_win, details_win, footer_win);
    
    bool running = true;
    while (running) {
        // Enforce bounds
        if (lists.empty()) {
            selected_list_idx = 0;
        } else {
            selected_list_idx = std::min(selected_list_idx, lists.size() - 1);
        }
        
        // Virtual index mapping for tasks in the selected list (pending first, completed last)
        std::vector<size_t> pending_indices;
        std::vector<size_t> completed_indices;
        if (!lists.empty() && selected_list_idx < lists.size()) {
            const auto& active_list = lists[selected_list_idx];
            for (size_t i = 0; i < active_list.tasks.size(); ++i) {
                const auto& task = active_list.tasks[i];
                if (!search_query.empty() && !contains_substring(task.title, search_query)) {
                    continue;
                }
                if (task.completed) {
                    completed_indices.push_back(i);
                } else {
                    pending_indices.push_back(i);
                }
            }
        }
        
        size_t visible_task_count = pending_indices.size() + completed_indices.size();
        if (visible_task_count == 0) {
            selected_task_idx = 0;
        } else {
            selected_task_idx = std::min(selected_task_idx, visible_task_count - 1);
        }
        
        // --- DRAW HEADER ---
        werase(header_win);
        wattron(header_win, COLOR_PAIR(Tui::CP_BLUE) | A_BOLD);
        box(header_win, 0, 0);
        wattroff(header_win, COLOR_PAIR(Tui::CP_BLUE) | A_BOLD);
        
        wattron(header_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
        std::string title = "doTUI";
        mvwprintw(header_win, 1, (COLS - title.length()) / 2, "%s", title.c_str());
        wattroff(header_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
        wnoutrefresh(header_win);
        
        // --- DRAW FOOTER ---
        werase(footer_win);
        wattron(footer_win, COLOR_PAIR(Tui::CP_BLUE) | A_BOLD);
        box(footer_win, 0, 0);
        wattroff(footer_win, COLOR_PAIR(Tui::CP_BLUE) | A_BOLD);
        
        if (active_focus == Focus::Search) {
            mvwprintw(footer_win, 1, 2, "Search: %s", search_query.c_str());
        } else if (!search_query.empty()) {
            mvwprintw(footer_win, 1, 2, "Search (Active): %s  |  Esc to clear", search_query.c_str());
        } else {
            std::string footer_text = "Tab: Pane | Space: Toggle | n: New | d: Del | e: Edit | s: Sort | q: Exit | /: Search";
            mvwprintw(footer_win, 1, (COLS - footer_text.length()) / 2, "%s", footer_text.c_str());
        }
        wnoutrefresh(footer_win);
        
        // --- DRAW LISTS ---
        werase(lists_win);
        int lists_w = getmaxx(lists_win);
        
        if (active_focus == Focus::Lists) {
            wattron(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            box(lists_win, 0, 0);
            mvwprintw(lists_win, 0, 2, " Categories ");
            wattroff(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
        } else {
            wattron(lists_win, COLOR_PAIR(Tui::CP_BLUE));
            box(lists_win, 0, 0);
            mvwprintw(lists_win, 0, 2, " Categories ");
            wattroff(lists_win, COLOR_PAIR(Tui::CP_BLUE));
        }
        
        for (size_t i = 0; i < lists.size(); ++i) {
            bool is_selected = (i == selected_list_idx);
            std::string list_name = lists[i].name;
            size_t count = lists[i].tasks.size();
            std::string display_str = list_name + " (" + std::to_string(count) + ")";
            
            int max_len = lists_w - 6;
            if (max_len < 5) max_len = 5;
            if (display_str.length() > (size_t)max_len) {
                display_str = display_str.substr(0, max_len - 3) + "...";
            }
            
            if (is_selected) {
                if (active_focus == Focus::Lists) {
                    wattron(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_REVERSE | A_BOLD);
                    mvwprintw(lists_win, 2 + i, 2, " > %-*s", max_len, display_str.c_str());
                    wattroff(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_REVERSE | A_BOLD);
                } else {
                    wattron(lists_win, COLOR_PAIR(Tui::CP_BLUE) | A_REVERSE);
                    mvwprintw(lists_win, 2 + i, 2, " > %-*s", max_len, display_str.c_str());
                    wattroff(lists_win, COLOR_PAIR(Tui::CP_BLUE) | A_REVERSE);
                }
            } else {
                mvwprintw(lists_win, 2 + i, 2, "   %-*s", max_len, display_str.c_str());
            }
        }
        wnoutrefresh(lists_win);
        
        // --- DRAW TASKS ---
        werase(tasks_win);
        int tasks_w = getmaxx(tasks_win);
        
        std::string tasks_title = " Tasks ";
        if (!lists.empty() && selected_list_idx < lists.size()) {
            tasks_title = " Tasks: " + lists[selected_list_idx].name + " ";
        }
        if (tasks_title.length() > (size_t)tasks_w - 6) {
            tasks_title = tasks_title.substr(0, tasks_w - 9) + "... ";
        }
        
        if (active_focus == Focus::Tasks) {
            wattron(tasks_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            box(tasks_win, 0, 0);
            mvwprintw(tasks_win, 0, 2, "%s", tasks_title.c_str());
            wattroff(tasks_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
        } else {
            wattron(tasks_win, COLOR_PAIR(Tui::CP_BLUE));
            box(tasks_win, 0, 0);
            mvwprintw(tasks_win, 0, 2, "%s", tasks_title.c_str());
            wattroff(tasks_win, COLOR_PAIR(Tui::CP_BLUE));
        }
        
        if (!lists.empty() && selected_list_idx < lists.size()) {
            const auto& active_list = lists[selected_list_idx];
            if (active_list.tasks.empty()) {
                mvwprintw(tasks_win, 2, 2, "No tasks in this list.");
                mvwprintw(tasks_win, 3, 2, "Press 'n' to add one.");
            } else if (pending_indices.empty() && completed_indices.empty()) {
                mvwprintw(tasks_win, 2, 2, "No tasks match your search.");
                mvwprintw(tasks_win, 3, 2, "Press Esc to clear search.");
            } else {
                int row = 2;
                // Draw pending tasks
                for (size_t i = 0; i < pending_indices.size(); ++i) {
                    size_t actual_idx = pending_indices[i];
                    const auto& task = active_list.tasks[actual_idx];
                    bool is_selected = (i == selected_task_idx);
                    
                    std::string status_box = "[ ]";
                    std::string display_str = status_box + " " + task.title;
                    
                    int max_len = tasks_w - 6;
                    if (max_len < 10) max_len = 10;
                    if (display_str.length() > (size_t)max_len) {
                        display_str = display_str.substr(0, max_len - 3) + "...";
                    }
                    
                    if (is_selected) {
                        if (active_focus == Focus::Tasks) {
                            wattron(tasks_win, COLOR_PAIR(Tui::CP_CYAN) | A_REVERSE | A_BOLD);
                            mvwprintw(tasks_win, row++, 2, " > %-*s", max_len, display_str.c_str());
                            wattroff(tasks_win, COLOR_PAIR(Tui::CP_CYAN) | A_REVERSE | A_BOLD);
                        } else {
                            wattron(tasks_win, COLOR_PAIR(Tui::CP_BLUE) | A_REVERSE);
                            mvwprintw(tasks_win, row++, 2, " > %-*s", max_len, display_str.c_str());
                            wattroff(tasks_win, COLOR_PAIR(Tui::CP_BLUE) | A_REVERSE);
                        }
                    } else {
                        mvwprintw(tasks_win, row++, 2, "   %s  %s", status_box.c_str(), task.title.substr(0, max_len - 6).c_str());
                    }
                }
                
                // Draw Completed divider and tasks
                if (!completed_indices.empty()) {
                    wattron(tasks_win, COLOR_PAIR(Tui::CP_BLUE) | A_DIM);
                    mvwprintw(tasks_win, row++, 2, " --- Completed Tasks ---");
                    wattroff(tasks_win, COLOR_PAIR(Tui::CP_BLUE) | A_DIM);
                    
                    for (size_t i = 0; i < completed_indices.size(); ++i) {
                        size_t actual_idx = completed_indices[i];
                        const auto& task = active_list.tasks[actual_idx];
                        size_t virtual_idx = pending_indices.size() + i;
                        bool is_selected = (virtual_idx == selected_task_idx);
                        
                        std::string status_box = "[X]";
                        std::string display_str = status_box + " " + task.title;
                        
                        int max_len = tasks_w - 6;
                        if (max_len < 10) max_len = 10;
                        if (display_str.length() > (size_t)max_len) {
                            display_str = display_str.substr(0, max_len - 3) + "...";
                        }
                        
                        if (is_selected) {
                            if (active_focus == Focus::Tasks) {
                                wattron(tasks_win, COLOR_PAIR(Tui::CP_CYAN) | A_REVERSE | A_BOLD);
                                mvwprintw(tasks_win, row++, 2, " > %-*s", max_len, display_str.c_str());
                                wattroff(tasks_win, COLOR_PAIR(Tui::CP_CYAN) | A_REVERSE | A_BOLD);
                            } else {
                                wattron(tasks_win, COLOR_PAIR(Tui::CP_BLUE) | A_REVERSE);
                                mvwprintw(tasks_win, row++, 2, " > %-*s", max_len, display_str.c_str());
                                wattroff(tasks_win, COLOR_PAIR(Tui::CP_BLUE) | A_REVERSE);
                            }
                        } else {
                            wattron(tasks_win, COLOR_PAIR(Tui::CP_GREEN));
                            mvwprintw(tasks_win, row, 2, "   %s", status_box.c_str());
                            wattroff(tasks_win, COLOR_PAIR(Tui::CP_GREEN));
                            mvwprintw(tasks_win, row++, 8, "%s", task.title.substr(0, max_len - 6).c_str());
                        }
                    }
                }
            }
        }
        wnoutrefresh(tasks_win);
        
        // --- DRAW DETAILS ---
        werase(details_win);
        int details_h, details_w;
        getmaxyx(details_win, details_h, details_w);
        
        wattron(details_win, COLOR_PAIR(Tui::CP_BLUE));
        box(details_win, 0, 0);
        mvwprintw(details_win, 0, 2, " Task Details ");
        wattroff(details_win, COLOR_PAIR(Tui::CP_BLUE));
        
        bool has_selection = false;
        Task task;
        if (!lists.empty() && selected_list_idx < lists.size()) {
            const auto& active_list = lists[selected_list_idx];
            if (selected_task_idx < pending_indices.size() + completed_indices.size()) {
                size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                         ? pending_indices[selected_task_idx]
                                         : completed_indices[selected_task_idx - pending_indices.size()];
                task = active_list.tasks[actual_task_idx];
                has_selection = true;
            }
        }
        
        if (has_selection) {
            // Draw Title
            wattron(details_win, A_BOLD | COLOR_PAIR(Tui::CP_CYAN));
            mvwprintw(details_win, 2, 2, "Title: %s", task.title.c_str());
            wattroff(details_win, A_BOLD | COLOR_PAIR(Tui::CP_CYAN));
            
            // Draw Status
            mvwprintw(details_win, 3, 2, "Status: ");
            if (task.completed) {
                wattron(details_win, COLOR_PAIR(Tui::CP_GREEN) | A_BOLD);
                wprintw(details_win, "Completed");
                wattroff(details_win, COLOR_PAIR(Tui::CP_GREEN) | A_BOLD);
            } else {
                wattron(details_win, COLOR_PAIR(Tui::CP_RED) | A_BOLD);
                wprintw(details_win, "Pending");
                wattroff(details_win, COLOR_PAIR(Tui::CP_RED) | A_BOLD);
            }
            
            // Draw Type & Time
            std::string relative_time = get_relative_time_string(task.time_value);
            wprintw(details_win, "    Type: ");
            if (task.type == TaskType::Activity) {
                wattron(details_win, COLOR_PAIR(Tui::CP_YELLOW) | A_BOLD);
                wprintw(details_win, "Activity");
                wattroff(details_win, COLOR_PAIR(Tui::CP_YELLOW) | A_BOLD);
                
                mvwprintw(details_win, 4, 2, "Starts at: %s%s", task.time_value.c_str(), relative_time.c_str());
            } else {
                wattron(details_win, COLOR_PAIR(Tui::CP_YELLOW) | A_BOLD);
                wprintw(details_win, "Assignment");
                wattroff(details_win, COLOR_PAIR(Tui::CP_YELLOW) | A_BOLD);
                
                mvwprintw(details_win, 4, 2, "Due by: %s%s", task.time_value.c_str(), relative_time.c_str());
            }
            
            // Draw Recurrence
            mvwprintw(details_win, 5, 2, "Recurrence: ");
            wattron(details_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            wprintw(details_win, "%s", recurrence_to_string(task.recurrence).c_str());
            wattroff(details_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            
            // Draw Description
            mvwprintw(details_win, 6, 2, "Description:");
            int max_desc_w = details_w - 6;
            auto wrapped = Tui::wrap_text(task.description, max_desc_w);
            int start_row = 7;
            int max_rows = details_h - start_row - 2;
            for (int r = 0; r < (int)wrapped.size() && r < max_rows; ++r) {
                mvwprintw(details_win, start_row + r, 4, "%s", wrapped[r].c_str());
            }
        } else {
            mvwprintw(details_win, 2, 2, "No task selected.");
        }
        wnoutrefresh(details_win);
        if (active_focus == Focus::Search) {
            curs_set(1);
            wmove(footer_win, 1, 10 + search_query.length());
            wnoutrefresh(footer_win);
        } else {
            curs_set(0);
        }
        doupdate();
        
        // --- INPUT LOOP ---
        int ch = wgetch(active_focus == Focus::Search ? footer_win : (active_focus == Focus::Lists ? lists_win : tasks_win));
        
        if (ch == KEY_RESIZE) {
            setup_windows(header_win, lists_win, tasks_win, details_win, footer_win);
            clear();
            continue;
        }
        
        if (active_focus == Focus::Search) {
            if (ch == 27) { // ESC
                search_query.clear();
                active_focus = Focus::Tasks;
            } else if (ch == '\n' || ch == '\r') {
                active_focus = Focus::Tasks;
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (!search_query.empty()) {
                    search_query.pop_back();
                }
            } else if (ch >= 32 && ch <= 126) {
                if (search_query.length() < 40) {
                    search_query.push_back(ch);
                }
            }
        } else {
            if (ch == '/') {
                active_focus = Focus::Search;
            } else if (ch == 27) { // ESC in navigation
                if (!search_query.empty()) {
                    search_query.clear();
                }
            } else if (ch == '?') {
                Tui::show_help_dialog();
                touchwin(header_win);
                touchwin(lists_win);
                touchwin(tasks_win);
                touchwin(details_win);
                touchwin(footer_win);
                wnoutrefresh(header_win);
                wnoutrefresh(lists_win);
                wnoutrefresh(tasks_win);
                wnoutrefresh(details_win);
                wnoutrefresh(footer_win);
            } else if (ch == 'q') {
                Storage::save_data(lists, DATA_FILE);
                running = false;
            } else if (ch == '\t') {
                active_focus = (active_focus == Focus::Lists) ? Focus::Tasks : Focus::Lists;
            } else if (active_focus == Focus::Lists) {
                if (ch == KEY_LEFT || ch == KEY_UP || ch == 'k') {
                    if (selected_list_idx > 0) {
                        selected_list_idx--;
                        selected_task_idx = 0;
                    }
                } else if (ch == KEY_RIGHT || ch == KEY_DOWN || ch == 'j') {
                    if (!lists.empty() && selected_list_idx < lists.size() - 1) {
                        selected_list_idx++;
                        selected_task_idx = 0;
                    }
                } else if (ch == 'l') {
                    active_focus = Focus::Tasks;
                } else if (ch == 'n' || ch == 'a') {
                    bool cancelled = false;
                    std::string list_name = Tui::show_list_input_dialog(cancelled);
                    if (!cancelled && !list_name.empty()) {
                        List nl;
                        nl.name = list_name;
                        lists.push_back(nl);
                        selected_list_idx = lists.size() - 1;
                        selected_task_idx = 0;
                    }
                } else if (ch == 'e') {
                    if (!lists.empty()) {
                        bool cancelled = false;
                        std::string new_name = Tui::show_list_edit_dialog(lists[selected_list_idx].name, cancelled);
                        if (!cancelled && !new_name.empty()) {
                            lists[selected_list_idx].name = new_name;
                        }
                    }
                } else if (ch == 'd' || ch == 'x') {
                    if (!lists.empty()) {
                        std::string confirm_msg = "Delete list '" + lists[selected_list_idx].name + "'?";
                        if (Tui::show_confirm_dialog("Confirm Delete", confirm_msg)) {
                            lists.erase(lists.begin() + selected_list_idx);
                            if (selected_list_idx > 0 && selected_list_idx >= lists.size()) {
                                selected_list_idx = lists.size() - 1;
                            }
                            selected_task_idx = 0;
                            if (lists.empty()) {
                                List default_list;
                                default_list.name = "Inbox";
                                lists.push_back(default_list);
                                selected_list_idx = 0;
                            }
                        }
                    }
                } else if (ch == 's') {
                    if (!lists.empty() && lists.size() > 1) {
                        bool cancelled = false;
                        std::vector<std::string> options = {
                            "Sort Alphabetically (A-Z)",
                            "Sort Alphabetically (Z-A)",
                            "Move Category Left/Up",
                            "Move Category Right/Down"
                        };
                        int choice = Tui::show_selection_dialog("Sort Categories", "Select sorting option:", options, cancelled);
                        if (!cancelled && choice != -1) {
                            if (choice == 0) {
                                std::sort(lists.begin(), lists.end(), [](const List& a, const List& b) {
                                    return a.name < b.name;
                                });
                            } else if (choice == 1) {
                                std::sort(lists.begin(), lists.end(), [](const List& a, const List& b) {
                                    return a.name > b.name;
                                });
                            } else if (choice == 2) {
                                if (selected_list_idx > 0) {
                                    std::swap(lists[selected_list_idx], lists[selected_list_idx - 1]);
                                    selected_list_idx--;
                                }
                            } else if (choice == 3) {
                                if (selected_list_idx < lists.size() - 1) {
                                    std::swap(lists[selected_list_idx], lists[selected_list_idx + 1]);
                                    selected_list_idx++;
                                }
                            }
                        }
                    }
                }
            } else if (active_focus == Focus::Tasks) {
                if (ch == KEY_UP || ch == 'k') {
                    if (selected_task_idx > 0) {
                        selected_task_idx--;
                    }
                } else if (ch == KEY_DOWN || ch == 'j') {
                    if (!lists.empty() && selected_task_idx < pending_indices.size() + completed_indices.size() - 1) {
                        selected_task_idx++;
                    }
                } else if (ch == 'h') {
                    active_focus = Focus::Lists;
                } else if (ch == ' ') {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (selected_task_idx < pending_indices.size() + completed_indices.size()) {
                            size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                                     ? pending_indices[selected_task_idx]
                                                     : completed_indices[selected_task_idx - pending_indices.size()];
                            auto& task = active_list.tasks[actual_task_idx];
                            if (!task.completed) {
                                task.completed = true;
                                if (task.recurrence != RecurrenceRule::None) {
                                    Task next_task = task;
                                    next_task.completed = false;
                                    if (!task.time_value.empty()) {
                                        bool success = false;
                                        std::tm tm = string_to_tm(task.time_value, success);
                                        if (success) {
                                            if (task.recurrence == RecurrenceRule::Daily) {
                                                tm.tm_mday += 1;
                                            } else if (task.recurrence == RecurrenceRule::Weekly) {
                                                tm.tm_mday += 7;
                                            } else if (task.recurrence == RecurrenceRule::Monthly) {
                                                tm.tm_mon += 1;
                                            }
                                            std::mktime(&tm);
                                            next_task.time_value = tm_to_string(tm);
                                        }
                                    }
                                    active_list.tasks.push_back(next_task);
                                }
                            } else {
                                task.completed = false;
                            }
                        }
                    }
                } else if (ch == 'p') {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (!active_list.tasks.empty() && selected_task_idx < pending_indices.size() + completed_indices.size()) {
                            size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                                     ? pending_indices[selected_task_idx]
                                                     : completed_indices[selected_task_idx - pending_indices.size()];
                            auto& task = active_list.tasks[actual_task_idx];
                            if (!task.time_value.empty()) {
                                bool cancelled = false;
                                int choice = Tui::show_snooze_dialog(cancelled);
                                if (!cancelled && choice != -1) {
                                    bool success = false;
                                    std::tm tm = string_to_tm(task.time_value, success);
                                    if (success) {
                                        if (choice == 0) {
                                            tm.tm_hour += 1;
                                        } else if (choice == 1) {
                                            tm.tm_mday += 1;
                                        } else if (choice == 2) {
                                            tm.tm_mday += 7;
                                        }
                                        std::mktime(&tm);
                                        task.time_value = tm_to_string(tm);
                                    }
                                }
                            } else {
                                Tui::show_confirm_dialog("Error", "Task has no date/time to snooze!");
                            }
                        }
                    }
                } else if (ch == 'm') {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (!active_list.tasks.empty() && selected_task_idx < pending_indices.size() + completed_indices.size()) {
                            size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                                     ? pending_indices[selected_task_idx]
                                                     : completed_indices[selected_task_idx - pending_indices.size()];
                            
                            std::vector<std::string> category_names;
                            for (size_t i = 0; i < lists.size(); ++i) {
                                category_names.push_back(lists[i].name);
                            }
                            
                            bool cancelled = false;
                            int dest_idx = Tui::show_selection_dialog("Move Task", "Select destination category:", category_names, cancelled, (int)selected_list_idx);
                            if (!cancelled && dest_idx != -1 && (size_t)dest_idx != selected_list_idx) {
                                Task task_to_move = active_list.tasks[actual_task_idx];
                                active_list.tasks.erase(active_list.tasks.begin() + actual_task_idx);
                                lists[dest_idx].tasks.push_back(task_to_move);
                                
                                if (selected_task_idx > 0 && selected_task_idx >= active_list.tasks.size()) {
                                    selected_task_idx = active_list.tasks.size() - 1;
                                }
                            }
                        }
                    }
                } else if (ch == 'n' || ch == 'a') {
                    if (lists.empty()) {
                        Tui::show_confirm_dialog("Error", "Create a list first!");
                    } else {
                        bool cancelled = false;
                        TaskType type = Tui::show_task_type_dialog(cancelled);
                        if (!cancelled) {
                            std::string title, desc, time_val;
                            RecurrenceRule recurrence = RecurrenceRule::None;
                            if (Tui::show_task_input_dialog(type, title, desc, time_val, recurrence)) {
                                Task nt;
                                nt.title = title;
                                nt.description = desc;
                                nt.completed = false;
                                nt.type = type;
                                nt.time_value = time_val;
                                nt.recurrence = recurrence;
                                lists[selected_list_idx].tasks.push_back(nt);
                                selected_task_idx = lists[selected_list_idx].tasks.size() - 1;
                            }
                        }
                    }
                } else if (ch == 'e') {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (!active_list.tasks.empty() && selected_task_idx < pending_indices.size() + completed_indices.size()) {
                            size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                                     ? pending_indices[selected_task_idx]
                                                     : completed_indices[selected_task_idx - pending_indices.size()];
                            auto& task = active_list.tasks[actual_task_idx];
                            std::string title, desc, time_val;
                            RecurrenceRule recurrence = RecurrenceRule::None;
                            if (Tui::show_task_edit_dialog(task.type, task.title, task.description, task.time_value, task.recurrence, title, desc, time_val, recurrence)) {
                                task.title = title;
                                task.description = desc;
                                task.time_value = time_val;
                                task.recurrence = recurrence;
                            }
                        }
                    }
                } else if (ch == 'd' || ch == 'x') {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (!active_list.tasks.empty() && selected_task_idx < pending_indices.size() + completed_indices.size()) {
                            size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                                     ? pending_indices[selected_task_idx]
                                                     : completed_indices[selected_task_idx - pending_indices.size()];
                            std::string confirm_msg = "Delete task '" + active_list.tasks[actual_task_idx].title + "'?";
                            if (Tui::show_confirm_dialog("Confirm Delete", confirm_msg)) {
                                active_list.tasks.erase(active_list.tasks.begin() + actual_task_idx);
                                if (selected_task_idx > 0 && selected_task_idx >= active_list.tasks.size()) {
                                    selected_task_idx = active_list.tasks.size() - 1;
                                }
                            }
                        }
                    }
                } else if (ch == 's') {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (active_list.tasks.size() > 1) {
                            bool cancelled = false;
                            std::vector<std::string> options = {
                                "Sort by Title (A-Z)",
                                "Sort by Title (Z-A)",
                                "Sort by Date/Time (Earliest First)",
                                "Sort by Date/Time (Latest First)",
                                "Move Task Up",
                                "Move Task Down"
                            };
                            int choice = Tui::show_selection_dialog("Sort Tasks", "Select sorting option:", options, cancelled);
                            if (!cancelled && choice != -1) {
                                if (choice == 0) {
                                    std::sort(active_list.tasks.begin(), active_list.tasks.end(), [](const Task& a, const Task& b) {
                                        return a.title < b.title;
                                    });
                                } else if (choice == 1) {
                                    std::sort(active_list.tasks.begin(), active_list.tasks.end(), [](const Task& a, const Task& b) {
                                        return a.title > b.title;
                                    });
                                } else if (choice == 2) {
                                    std::sort(active_list.tasks.begin(), active_list.tasks.end(), [](const Task& a, const Task& b) {
                                        if (a.time_value.empty() && b.time_value.empty()) return false;
                                        if (a.time_value.empty()) return false;
                                        if (b.time_value.empty()) return true;
                                        return a.time_value < b.time_value;
                                    });
                                } else if (choice == 3) {
                                    std::sort(active_list.tasks.begin(), active_list.tasks.end(), [](const Task& a, const Task& b) {
                                        if (a.time_value.empty() && b.time_value.empty()) return false;
                                        if (a.time_value.empty()) return false;
                                        if (b.time_value.empty()) return true;
                                        return a.time_value > b.time_value;
                                    });
                                } else if (choice == 4) {
                                    if (selected_task_idx < pending_indices.size()) {
                                        if (selected_task_idx > 0) {
                                            size_t cur_actual = pending_indices[selected_task_idx];
                                            size_t prev_actual = pending_indices[selected_task_idx - 1];
                                            std::swap(active_list.tasks[cur_actual], active_list.tasks[prev_actual]);
                                            selected_task_idx--;
                                        }
                                    } else {
                                        size_t completed_sel_idx = selected_task_idx - pending_indices.size();
                                        if (completed_sel_idx > 0) {
                                            size_t cur_actual = completed_indices[completed_sel_idx];
                                            size_t prev_actual = completed_indices[completed_sel_idx - 1];
                                            std::swap(active_list.tasks[cur_actual], active_list.tasks[prev_actual]);
                                            selected_task_idx--;
                                        }
                                    }
                                } else if (choice == 5) {
                                    if (selected_task_idx < pending_indices.size()) {
                                        if (selected_task_idx < pending_indices.size() - 1) {
                                            size_t cur_actual = pending_indices[selected_task_idx];
                                            size_t next_actual = pending_indices[selected_task_idx + 1];
                                            std::swap(active_list.tasks[cur_actual], active_list.tasks[next_actual]);
                                            selected_task_idx++;
                                        }
                                    } else {
                                        size_t completed_sel_idx = selected_task_idx - pending_indices.size();
                                        if (completed_sel_idx < completed_indices.size() - 1) {
                                            size_t cur_actual = completed_indices[completed_sel_idx];
                                            size_t next_actual = completed_indices[completed_sel_idx + 1];
                                            std::swap(active_list.tasks[cur_actual], active_list.tasks[next_actual]);
                                            selected_task_idx++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    Tui::end_tui();
    return 0;
}
