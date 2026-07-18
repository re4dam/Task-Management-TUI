#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ncurses.h>
#include "task.hpp"
#include "list.hpp"
#include "storage.hpp"
#include "tui.hpp"
#include "keybinds.hpp"

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

inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

inline int levenshtein_distance(const std::string& s1, const std::string& s2) {
    int len1 = s1.length();
    int len2 = s2.length();
    std::vector<std::vector<int>> d(len1 + 1, std::vector<int>(len2 + 1));
    for (int i = 0; i <= len1; ++i) d[i][0] = i;
    for (int j = 0; j <= len2; ++j) d[0][j] = j;
    for (int i = 1; i <= len1; ++i) {
        for (int j = 1; j <= len2; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            d[i][j] = std::min({ d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + cost });
        }
    }
    return d[len1][len2];
}

void execute_command(const std::string& command_query, std::vector<List>& lists, size_t& selected_list_idx, size_t& selected_task_idx, Focus& active_focus, bool& running, const std::vector<size_t>& pending_indices, const std::vector<size_t>& completed_indices, WINDOW* footer_win, Mode& active_mode) {
    std::string cmd = command_query;
    if (!cmd.empty() && cmd[0] == ':') {
        cmd = cmd.substr(1);
    }
    cmd = trim(cmd);
    if (cmd.empty()) return;
    
    std::string cmd_name = "";
    std::string cmd_args = "";
    size_t space_pos = cmd.find(' ');
    if (space_pos != std::string::npos) {
        cmd_name = cmd.substr(0, space_pos);
        cmd_args = trim(cmd.substr(space_pos + 1));
    } else {
        cmd_name = cmd;
    }
    
    std::transform(cmd_name.begin(), cmd_name.end(), cmd_name.begin(), ::tolower);
    
    // Autocorrect / Nearest command matching
    std::vector<std::string> valid_cmds = {"quit", "write", "new", "todo", "delete", "edit", "sort"};
    bool is_valid = (cmd_name == "q" || cmd_name == "d" || cmd_name == "e" || cmd_name == "s" || cmd_name == "w");
    if (!is_valid) {
        for (const auto& vc : valid_cmds) {
            if (cmd_name == vc) {
                is_valid = true;
                break;
            }
        }
    }
    
    if (!is_valid) {
        std::string closest_match = "";
        int min_dist = 999;
        
        for (const auto& vc : valid_cmds) {
            int d = levenshtein_distance(cmd_name, vc);
            if (d < min_dist) {
                min_dist = d;
                closest_match = vc;
            }
        }
        
        std::vector<std::pair<std::string, std::string>> short_aliases = {
            {"d", "delete"}, {"e", "edit"}, {"s", "sort"}, {"w", "write"}, {"q", "quit"}
        };
        for (const auto& sa : short_aliases) {
            int d = levenshtein_distance(cmd_name, sa.first);
            if (d < min_dist) {
                min_dist = d;
                closest_match = sa.second;
            }
        }
        
        if (min_dist <= 2) {
            cmd_name = closest_match;
        }
    }
    
    if (cmd_name == "q" || cmd_name == "quit") {
        Storage::save_data(lists, DATA_FILE);
        running = false;
    } else if (cmd_name == "w" || cmd_name == "write") {
        Storage::save_data(lists, DATA_FILE);
    } else if (cmd_name == "new" || cmd_name == "todo") {
        if (lists.empty()) {
            Tui::show_confirm_dialog("Error", "Create a category first!");
        } else if (cmd_args.empty()) {
            Tui::show_confirm_dialog("Error", "Task title cannot be empty!");
        } else {
            Task nt;
            nt.title = cmd_args;
            nt.completed = false;
            nt.type = TaskType::Assignment;
            nt.time_value = "";
            nt.recurrence = RecurrenceRule::None;
            lists[selected_list_idx].tasks.push_back(nt);
            selected_task_idx = lists[selected_list_idx].tasks.size() - 1;
        }
    } else if (cmd_name == "delete" || cmd_name == "d") {
        if (active_focus == Focus::Lists) {
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
        } else {
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
        }
    } else if (cmd_name == "edit" || cmd_name == "e") {
        if (active_focus == Focus::Lists) {
            if (!lists.empty()) {
                bool cancelled = false;
                std::string new_name = Tui::show_list_edit_dialog(lists[selected_list_idx].name, cancelled, footer_win, &active_mode);
                if (!cancelled && !new_name.empty()) {
                    lists[selected_list_idx].name = new_name;
                }
            }
        } else {
            if (!lists.empty() && selected_list_idx < lists.size()) {
                auto& active_list = lists[selected_list_idx];
                if (!active_list.tasks.empty() && selected_task_idx < pending_indices.size() + completed_indices.size()) {
                    size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                             ? pending_indices[selected_task_idx]
                                             : completed_indices[selected_task_idx - pending_indices.size()];
                    auto& task = active_list.tasks[actual_task_idx];
                    std::string title, desc, time_val;
                    RecurrenceRule recurrence = RecurrenceRule::None;
                    if (Tui::show_task_edit_dialog(task.type, task.title, task.description, task.time_value, task.recurrence, title, desc, time_val, recurrence, footer_win, &active_mode)) {
                        task.title = title;
                        task.description = desc;
                        task.time_value = time_val;
                        task.recurrence = recurrence;
                    }
                }
            }
        }
    } else if (cmd_name == "sort" || cmd_name == "s") {
        std::string args_lower = cmd_args;
        std::transform(args_lower.begin(), args_lower.end(), args_lower.begin(), ::tolower);
        
        if (active_focus == Focus::Lists) {
            if (!lists.empty() && lists.size() > 1) {
                if (args_lower == "alpha" || args_lower == "alphabetical" || args_lower.empty()) {
                    std::sort(lists.begin(), lists.end(), [](const List& a, const List& b) {
                        return a.name < b.name;
                    });
                }
            }
        } else {
            if (!lists.empty() && selected_list_idx < lists.size()) {
                auto& active_list = lists[selected_list_idx];
                if (active_list.tasks.size() > 1) {
                    if (args_lower == "title" || args_lower.empty()) {
                        std::sort(active_list.tasks.begin(), active_list.tasks.end(), [](const Task& a, const Task& b) {
                            return a.title < b.title;
                        });
                    } else if (args_lower == "date" || args_lower == "time") {
                        std::sort(active_list.tasks.begin(), active_list.tasks.end(), [](const Task& a, const Task& b) {
                            if (a.time_value.empty() && b.time_value.empty()) return false;
                            if (a.time_value.empty()) return false;
                            if (b.time_value.empty()) return true;
                            return a.time_value < b.time_value;
                        });
                    }
                }
            }
        }
    } else if (cmd_name == "theme") {
        if (cmd_args.empty()) {
            Tui::show_confirm_dialog("Error", "Theme name required!");
        } else {
            Tui::apply_theme(cmd_args);
            Tui::save_theme_to_config(cmd_args);
            clear();
        }
    } else {
        Tui::show_confirm_dialog("Error", "Unknown command: " + cmd_name);
    }
}

void setup_windows(WINDOW*& header_win, WINDOW*& lists_win, WINDOW*& tasks_win, WINDOW*& details_win, WINDOW*& footer_win) {
    if (header_win) delwin(header_win);
    if (lists_win) delwin(lists_win);
    if (tasks_win) delwin(tasks_win);
    if (details_win) delwin(details_win);
    if (footer_win) delwin(footer_win);
    
    header_win = nullptr;
    int main_h = LINES - 3;
    
    int lists_w = COLS * 0.25;
    if (lists_w < 20) lists_w = 20;
    int tasks_w = COLS * 0.40;
    if (tasks_w < 30) tasks_w = 30;
    if (lists_w + tasks_w > COLS - 15) {
        lists_w = COLS / 4;
        tasks_w = COLS / 3;
    }
    int details_w = COLS - lists_w - tasks_w;
    if (details_w < 10) details_w = 10;
    
    lists_win = newwin(main_h, lists_w, 0, 0);
    tasks_win = newwin(main_h, tasks_w, 0, lists_w);
    details_win = newwin(main_h, details_w, 0, lists_w + tasks_w);
    
    footer_win = newwin(3, COLS, LINES - 3, 0);
    
    keypad(lists_win, TRUE);
    keypad(tasks_win, TRUE);
    keypad(details_win, TRUE);
}

int main() {
    Tui::init_tui();
    
    Keymap keymap;
    keymap.load_from_file();
    
    Tui::generate_sample_dracula_theme();
    std::string active_theme = "default";
    try {
        std::string config_path = keymap.get_config_path();
        if (std::filesystem::exists(config_path)) {
            auto config = toml::parse_file(config_path);
            auto theme_tbl = config["theme"].as_table();
            if (theme_tbl) {
                auto active_val = theme_tbl->get("active");
                if (active_val && active_val->is_string()) {
                    active_theme = active_val->as_string()->get();
                }
            }
        }
    } catch (...) {}
    Tui::apply_theme(active_theme);
    
    Mode active_mode = Mode::Normal;
    
    std::vector<List> lists = Storage::load_data(DATA_FILE);
    
    // Add default list if empty
    if (lists.empty()) {
        List default_list;
        default_list.name = "Inbox";
        lists.push_back(default_list);
    }
    
    size_t selected_list_idx = 0;
    size_t selected_task_idx = 0;
    int details_scroll_idx = 0;
    Focus active_focus = Focus::Lists;
    std::string search_query = "";
    std::string command_query = "";
    
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
        
        static size_t last_list_idx = 0;
        static size_t last_task_idx = 0;
        if (selected_list_idx != last_list_idx || selected_task_idx != last_task_idx) {
            details_scroll_idx = 0;
            last_list_idx = selected_list_idx;
            last_task_idx = selected_task_idx;
        }
        
        // --- DRAW FOOTER ---
        Tui::draw_footer(footer_win, active_mode, active_focus, search_query, command_query);
        
        // --- DRAW LISTS ---
        werase(lists_win);
        int lists_w = getmaxx(lists_win);
        
        if (active_focus == Focus::Lists) {
            wattron(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            box(lists_win, 0, 0);
            mvwprintw(lists_win, 0, 2, " doTUI | Categories ");
            wattroff(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
        } else {
            wattron(lists_win, COLOR_PAIR(Tui::CP_BLUE));
            box(lists_win, 0, 0);
            wattroff(lists_win, COLOR_PAIR(Tui::CP_BLUE));
            
            wattron(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            mvwprintw(lists_win, 0, 2, " doTUI ");
            wattroff(lists_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            
            wattron(lists_win, COLOR_PAIR(Tui::CP_BLUE));
            mvwprintw(lists_win, 0, 9, "| Categories ");
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
                
                auto create_keys = keymap.get_keys_for_action(Action::Create);
                std::string create_str = "";
                if (!create_keys.empty()) {
                    for (size_t i = 0; i < create_keys.size(); ++i) {
                        if (i > 0) create_str += " or ";
                        create_str += "'" + create_keys[i] + "'";
                    }
                    mvwprintw(tasks_win, 4, 2, "Press %s to add a new task.", create_str.c_str());
                } else {
                    mvwprintw(tasks_win, 4, 2, "Press 'n' to add a new task.");
                }
                
                auto cmd_keys = keymap.get_keys_for_action(Action::EnterCommandMode);
                if (!cmd_keys.empty()) {
                    mvwprintw(tasks_win, 5, 2, "Or press '%s' and type ':new <title>'.", cmd_keys[0].c_str());
                } else {
                    mvwprintw(tasks_win, 5, 2, "Or use ':new <title>' via command bar.");
                }
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
        
        if (active_focus == Focus::Details) {
            wattron(details_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
            box(details_win, 0, 0);
            mvwprintw(details_win, 0, 2, " Task Details ");
            wattroff(details_win, COLOR_PAIR(Tui::CP_CYAN) | A_BOLD);
        } else {
            wattron(details_win, COLOR_PAIR(Tui::CP_BLUE));
            box(details_win, 0, 0);
            mvwprintw(details_win, 0, 2, " Task Details ");
            wattroff(details_win, COLOR_PAIR(Tui::CP_BLUE));
        }
        
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
            int max_scroll = (int)wrapped.size() - max_rows;
            if (max_scroll < 0) max_scroll = 0;
            details_scroll_idx = std::min(details_scroll_idx, max_scroll);
            details_scroll_idx = std::max(details_scroll_idx, 0);
            
            for (int r = 0; r < max_rows && (r + details_scroll_idx) < (int)wrapped.size(); ++r) {
                mvwprintw(details_win, start_row + r, 4, "%-s", wrapped[r + details_scroll_idx].c_str());
            }
        } else {
            mvwprintw(details_win, 2, 2, "No task selected.");
        }
        wnoutrefresh(details_win);
        if (active_mode == Mode::Command) {
            curs_set(1);
            int offset = 13 + 4; // length of "-- COMMAND --" + 4
            wmove(footer_win, 1, offset + 1 + command_query.length());
            wnoutrefresh(footer_win);
        } else if (active_focus == Focus::Search) {
            curs_set(1);
            std::string mode_str = "";
            if (active_mode == Mode::Normal) mode_str = "-- NORMAL --";
            else if (active_mode == Mode::Insert) mode_str = "-- INSERT --";
            else if (active_mode == Mode::Visual) mode_str = "-- VISUAL --";
            int offset = mode_str.length() + 4;
            wmove(footer_win, 1, offset + 8 + search_query.length()); // "Search: " is 8 chars
            wnoutrefresh(footer_win);
        } else {
            curs_set(0);
        }
        doupdate();
        
        // --- INPUT LOOP ---
        int ch = wgetch(active_focus == Focus::Search ? footer_win : (active_focus == Focus::Lists ? lists_win : (active_focus == Focus::Tasks ? tasks_win : details_win)));
        
        if (ch == KEY_RESIZE) {
            setup_windows(header_win, lists_win, tasks_win, details_win, footer_win);
            clear();
            continue;
        }
        
        if (active_mode == Mode::Command) {
            if (ch == 27) { // ESC
                active_mode = Mode::Normal;
                command_query.clear();
            } else if (ch == '\n' || ch == '\r') {
                execute_command(command_query, lists, selected_list_idx, selected_task_idx, active_focus, running, pending_indices, completed_indices, footer_win, active_mode);
                active_mode = Mode::Normal;
                command_query.clear();
            } else if (ch == '\t') { // Tab for autocomplete
                std::string cmd = command_query;
                bool leading_colon = false;
                if (!cmd.empty() && cmd[0] == ':') {
                    cmd = cmd.substr(1);
                    leading_colon = true;
                }
                cmd = trim(cmd);
                if (cmd.find(' ') == std::string::npos && !cmd.empty()) {
                    std::vector<std::string> valid_cmds = {"quit", "write", "new", "todo", "delete", "edit", "sort"};
                    std::vector<std::string> matches;
                    for (const auto& vc : valid_cmds) {
                        if (vc.rfind(cmd, 0) == 0) { // starts with prefix
                            matches.push_back(vc);
                        }
                    }
                    if (matches.size() == 1) {
                        std::string completed = matches[0];
                        if (completed == "new" || completed == "todo" || completed == "sort") {
                            completed += " ";
                        }
                        command_query = (leading_colon ? ":" : "") + completed;
                    }
                }
            } else if (ch == KEY_RIGHT) {
                std::string cmd = command_query;
                bool leading_colon = false;
                if (!cmd.empty() && cmd[0] == ':') {
                    cmd = cmd.substr(1);
                    leading_colon = true;
                }
                cmd = trim(cmd);
                if (cmd.find(' ') == std::string::npos && !cmd.empty()) {
                    std::vector<std::string> valid_cmds = {"quit", "write", "new", "todo", "delete", "edit", "sort"};
                    std::string closest_match = "";
                    for (const auto& vc : valid_cmds) {
                        if (vc.rfind(cmd, 0) == 0) {
                            closest_match = vc;
                            break;
                        }
                    }
                    if (!closest_match.empty()) {
                        std::string completed = closest_match;
                        if (completed == "new" || completed == "todo" || completed == "sort") {
                            completed += " ";
                        }
                        command_query = (leading_colon ? ":" : "") + completed;
                    }
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (!command_query.empty()) {
                    command_query.pop_back();
                } else {
                    active_mode = Mode::Normal;
                }
            } else if (ch >= 32 && ch <= 126) {
                if (command_query.length() < 80) {
                    command_query.push_back(ch);
                }
            }
            continue;
        }
        
        static bool pending_d = false;
        Action action = Action::None;
        if (active_focus != Focus::Search) {
            if (active_mode == Mode::Normal && ch == 'd') {
                if (!pending_d) {
                    pending_d = true;
                    Tui::draw_footer(footer_win, active_mode, active_focus, search_query);
                    continue;
                } else {
                    action = Action::Delete;
                    pending_d = false;
                }
            } else {
                pending_d = false;
                action = keymap.resolve(ch, active_mode);
            }
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
            if (action == Action::EnterNormalMode) {
                active_mode = Mode::Normal;
            } else if (action == Action::EnterCommandMode) {
                active_mode = Mode::Command;
                command_query.clear();
            } else if (action == Action::Search) {
                active_focus = Focus::Search;
            } else if (action == Action::Cancel) {
                if (!search_query.empty()) {
                    search_query.clear();
                }
            } else if (action == Action::Help) {
                Tui::show_help_dialog(keymap, active_mode);
                touchwin(lists_win);
                touchwin(tasks_win);
                touchwin(details_win);
                touchwin(footer_win);
                wnoutrefresh(lists_win);
                wnoutrefresh(tasks_win);
                wnoutrefresh(details_win);
                wnoutrefresh(footer_win);
            } else if (action == Action::Quit) {
                Storage::save_data(lists, DATA_FILE);
                running = false;
            } else if (action == Action::TogglePaneFocus) {
                if (active_focus == Focus::Lists) active_focus = Focus::Tasks;
                else if (active_focus == Focus::Tasks) active_focus = Focus::Details;
                else if (active_focus == Focus::Details) active_focus = Focus::Lists;
            } else if (action == Action::FocusRight) {
                if (active_focus == Focus::Lists) active_focus = Focus::Tasks;
                else if (active_focus == Focus::Tasks) active_focus = Focus::Details;
                else if (active_focus == Focus::Details) active_focus = Focus::Lists;
            } else if (action == Action::FocusLeft) {
                if (active_focus == Focus::Lists) active_focus = Focus::Details;
                else if (active_focus == Focus::Tasks) active_focus = Focus::Lists;
                else if (active_focus == Focus::Details) active_focus = Focus::Tasks;
            } else if (active_focus == Focus::Lists) {
                if (action == Action::MoveUp) {
                    if (selected_list_idx > 0) {
                        selected_list_idx--;
                        selected_task_idx = 0;
                    }
                } else if (action == Action::MoveDown) {
                    if (!lists.empty() && selected_list_idx < lists.size() - 1) {
                        selected_list_idx++;
                        selected_task_idx = 0;
                    }
                } else if (action == Action::Create) {
                    bool cancelled = false;
                    std::string list_name = Tui::show_list_input_dialog(cancelled, footer_win, &active_mode);
                    if (!cancelled && !list_name.empty()) {
                        List nl;
                        nl.name = list_name;
                        lists.push_back(nl);
                        selected_list_idx = lists.size() - 1;
                        selected_task_idx = 0;
                    }
                } else if (action == Action::Edit) {
                    if (!lists.empty()) {
                        bool cancelled = false;
                        std::string new_name = Tui::show_list_edit_dialog(lists[selected_list_idx].name, cancelled, footer_win, &active_mode);
                        if (!cancelled && !new_name.empty()) {
                            lists[selected_list_idx].name = new_name;
                        }
                    }
                } else if (action == Action::Delete) {
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
                } else if (action == Action::Sort) {
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
            } else if (active_focus == Focus::Tasks || active_focus == Focus::Details) {
                if (action == Action::MoveUp) {
                    if (active_focus == Focus::Tasks) {
                        if (selected_task_idx > 0) {
                            selected_task_idx--;
                        }
                    } else {
                        if (details_scroll_idx > 0) {
                            details_scroll_idx--;
                        }
                    }
                } else if (action == Action::MoveDown) {
                    if (active_focus == Focus::Tasks) {
                        if (!lists.empty() && selected_task_idx < pending_indices.size() + completed_indices.size() - 1) {
                            selected_task_idx++;
                        }
                    } else {
                        details_scroll_idx++;
                    }
                } else if (action == Action::ToggleComplete) {
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
                } else if (action == Action::Snooze) {
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
                } else if (action == Action::MoveToCategory) {
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
                } else if (action == Action::Create) {
                    if (lists.empty()) {
                        Tui::show_confirm_dialog("Error", "Create a list first!");
                    } else {
                        bool cancelled = false;
                        TaskType type = Tui::show_task_type_dialog(cancelled);
                        if (!cancelled) {
                            std::string title, desc, time_val;
                            RecurrenceRule recurrence = RecurrenceRule::None;
                            if (Tui::show_task_input_dialog(type, title, desc, time_val, recurrence, footer_win, &active_mode)) {
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
                } else if (action == Action::Edit) {
                    if (!lists.empty() && selected_list_idx < lists.size()) {
                        auto& active_list = lists[selected_list_idx];
                        if (!active_list.tasks.empty() && selected_task_idx < pending_indices.size() + completed_indices.size()) {
                            size_t actual_task_idx = (selected_task_idx < pending_indices.size())
                                                     ? pending_indices[selected_task_idx]
                                                     : completed_indices[selected_task_idx - pending_indices.size()];
                            auto& task = active_list.tasks[actual_task_idx];
                            std::string title, desc, time_val;
                            RecurrenceRule recurrence = RecurrenceRule::None;
                            if (Tui::show_task_edit_dialog(task.type, task.title, task.description, task.time_value, task.recurrence, title, desc, time_val, recurrence, footer_win, &active_mode)) {
                                task.title = title;
                                task.description = desc;
                                task.time_value = time_val;
                                task.recurrence = recurrence;
                            }
                        }
                    }
                } else if (action == Action::Delete) {
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
                } else if (action == Action::Sort) {
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
