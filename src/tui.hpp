#ifndef TUI_HPP
#define TUI_HPP

#include <ncurses.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <ctime>
#include <cstdio>
#include "list.hpp"
#include "task.hpp"
#include "keybinds.hpp"

enum class Focus {
    Lists,
    Tasks,
    Details,
    Search
};

namespace Tui {

// Color Pair constants
const int CP_BLUE = 1;
const int CP_CYAN = 2;
const int CP_GREEN = 3;
const int CP_YELLOW = 4;
const int CP_RED = 5;
const int CP_POPUP_HEADER = 6;
const int CP_POPUP_HIGHLIGHT = 7;

// Forward declarations
inline int show_selection_dialog(const std::string& title, const std::string& prompt, const std::vector<std::string>& options, bool& cancelled, int default_sel = 0);
inline void draw_footer(WINDOW* footer_win, Mode mode, Focus focus, const std::string& search_query, const std::string& command_query = "") {
    if (!footer_win) return;
    werase(footer_win);
    wattron(footer_win, COLOR_PAIR(CP_BLUE) | A_BOLD);
    box(footer_win, 0, 0);
    wattroff(footer_win, COLOR_PAIR(CP_BLUE) | A_BOLD);
    
    std::string mode_str = "";
    if (mode == Mode::Normal) mode_str = "-- NORMAL --";
    else if (mode == Mode::Insert) mode_str = "-- INSERT --";
    else if (mode == Mode::Visual) mode_str = "-- VISUAL --";
    else if (mode == Mode::Command) mode_str = "-- COMMAND --";
    
    wattron(footer_win, COLOR_PAIR(CP_CYAN) | A_BOLD);
    mvwprintw(footer_win, 1, 2, "%s", mode_str.c_str());
    wattroff(footer_win, COLOR_PAIR(CP_CYAN) | A_BOLD);
    
    int offset = mode_str.length() + 4;
    if (mode == Mode::Command) {
        mvwprintw(footer_win, 1, offset, ":%s", command_query.c_str());
        
        std::string cmd = command_query;
        if (!cmd.empty() && cmd[0] == ':') {
            cmd = cmd.substr(1);
        }
        size_t first = cmd.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = cmd.find_last_not_of(" \t\r\n");
            cmd = cmd.substr(first, (last - first + 1));
        }
        
        if (cmd.find(' ') == std::string::npos && !cmd.empty()) {
            std::vector<std::string> valid_cmds = {"quit", "write", "new", "todo", "delete", "edit", "sort"};
            std::string closest_match = "";
            for (const auto& vc : valid_cmds) {
                if (vc.rfind(cmd, 0) == 0) {
                    closest_match = vc;
                    break;
                }
            }
            if (!closest_match.empty() && closest_match != cmd) {
                std::string suggestion = closest_match.substr(cmd.length());
                wattron(footer_win, A_DIM);
                wprintw(footer_win, "%s", suggestion.c_str());
                wattroff(footer_win, A_DIM);
            }
        }
    } else if (focus == Focus::Search) {
        mvwprintw(footer_win, 1, offset, "Search: %s", search_query.c_str());
    } else if (!search_query.empty()) {
        mvwprintw(footer_win, 1, offset, "Search (Active): %s  |  Esc to clear", search_query.c_str());
    } else {
        std::string footer_text = "Tab: Pane | Space: Toggle | n: New | dd: Del | e: Edit | s: Sort | q: Exit | /: Search";
        mvwprintw(footer_win, 1, offset + (COLS - offset - footer_text.length()) / 2, "%s", footer_text.c_str());
    }
    wrefresh(footer_win);
}

inline int get_days_in_month(int year, int month) {
    if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
            return 29;
        return 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11)
        return 30;
    return 31;
}

inline int get_start_weekday(int year, int month) {
    std::tm tm = {};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = 1;
    std::mktime(&tm);
    return tm.tm_wday; // 0 = Sunday, 1 = Monday, ...
}

inline std::string show_calendar_picker(bool& cancelled, const std::string& initial_date) {
    int cur_year = 2026;
    int cur_month = 7;
    int cur_day = 18;
    
    std::time_t t = std::time(nullptr);
    std::tm* now = std::localtime(&t);
    int today_year = now->tm_year + 1900;
    int today_month = now->tm_mon + 1;
    int today_day = now->tm_mday;
    
    bool parsed = false;
    if (!initial_date.empty()) {
        if (initial_date.length() >= 10 && initial_date[4] == '-' && initial_date[7] == '-') {
            try {
                cur_year = std::stoi(initial_date.substr(0, 4));
                cur_month = std::stoi(initial_date.substr(5, 2));
                cur_day = std::stoi(initial_date.substr(8, 2));
                parsed = true;
            } catch (...) {}
        }
    }
    
    if (!parsed) {
        cur_year = today_year;
        cur_month = today_month;
        cur_day = today_day;
    }
    
    int width = 34;
    int height = 13;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    
    std::vector<std::string> month_names = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    
    bool done = false;
    cancelled = false;
    
    while (!done) {
        werase(win);
        wattron(win, COLOR_PAIR(CP_BLUE));
        box(win, 0, 0);
        wattroff(win, COLOR_PAIR(CP_BLUE));
        
        wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
        mvwprintw(win, 0, (width - 15) / 2, " Select Date ");
        wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
        
        std::string month_year_str = month_names[cur_month - 1] + " " + std::to_string(cur_year);
        mvwprintw(win, 1, 2, "<");
        mvwprintw(win, 1, width - 3, ">");
        mvwprintw(win, 1, (width - month_year_str.length()) / 2, "%s", month_year_str.c_str());
        
        wattron(win, A_UNDERLINE);
        mvwprintw(win, 3, 3, "Su Mo Tu We Th Fr Sa");
        wattroff(win, A_UNDERLINE);
        
        int days_in_month = get_days_in_month(cur_year, cur_month);
        int start_weekday = get_start_weekday(cur_year, cur_month);
        
        if (cur_day > days_in_month) cur_day = days_in_month;
        if (cur_day < 1) cur_day = 1;
        
        int current_draw_day = 1;
        for (int row = 0; row < 6; ++row) {
            for (int col = 0; col < 7; ++col) {
                int index = row * 7 + col;
                if (index < start_weekday) {
                    continue;
                }
                if (current_draw_day > days_in_month) {
                    break;
                }
                
                int draw_y = 4 + row;
                int draw_x = 3 + col * 4;
                
                bool is_selected = (current_draw_day == cur_day);
                bool is_today = (cur_year == today_year && cur_month == today_month && current_draw_day == today_day);
                
                if (is_selected) {
                    wattron(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
                    mvwprintw(win, draw_y, draw_x, "%2d", current_draw_day);
                    wattroff(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
                } else if (is_today) {
                    wattron(win, COLOR_PAIR(CP_GREEN) | A_BOLD);
                    mvwprintw(win, draw_y, draw_x, "%2d", current_draw_day);
                    wattroff(win, COLOR_PAIR(CP_GREEN) | A_BOLD);
                } else {
                    mvwprintw(win, draw_y, draw_x, "%2d", current_draw_day);
                }
                current_draw_day++;
            }
            if (current_draw_day > days_in_month) {
                break;
            }
        }
        
        mvwprintw(win, height - 2, 2, "hjkl/arrows: Move  Enter: Select");
        
        wrefresh(win);
        
        int ch = wgetch(win);
        
        if (ch == 27) { // ESC
            cancelled = true;
            done = true;
        } else if (ch == '\n' || ch == '\r') {
            done = true;
        } else if (ch == KEY_LEFT || ch == 'h') {
            if (cur_day > 1) {
                cur_day--;
            } else {
                if (cur_month > 1) {
                    cur_month--;
                } else {
                    cur_month = 12;
                    cur_year--;
                }
                cur_day = get_days_in_month(cur_year, cur_month);
            }
        } else if (ch == KEY_RIGHT || ch == 'l') {
            if (cur_day < days_in_month) {
                cur_day++;
            } else {
                if (cur_month < 12) {
                    cur_month++;
                } else {
                    cur_month = 1;
                    cur_year++;
                }
                cur_day = 1;
            }
        } else if (ch == KEY_UP || ch == 'k') {
            if (cur_day > 7) {
                cur_day -= 7;
            }
        } else if (ch == KEY_DOWN || ch == 'j') {
            if (cur_day + 7 <= days_in_month) {
                cur_day += 7;
            }
        } else if (ch == '<' || ch == ',' || ch == '-') {
            if (cur_month > 1) {
                cur_month--;
            } else {
                cur_month = 12;
                cur_year--;
            }
        } else if (ch == '>' || ch == '.' || ch == '+') {
            if (cur_month < 12) {
                cur_month++;
            } else {
                cur_month = 1;
                cur_year++;
            }
        }
    }
    
    delwin(win);
    
    if (cancelled) return "";
    
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", cur_year, cur_month, cur_day);
    return std::string(buf);
}

inline void init_tui() {
    initscr();
    set_escdelay(25);
    raw();
    keypad(stdscr, TRUE);
    noecho();
    curs_set(0);
    start_color();
    use_default_colors();
    
    // Initialize color pairs (Foreground, Background=-1 for default terminal background)
    init_pair(CP_BLUE, COLOR_BLUE, -1);
    init_pair(CP_CYAN, COLOR_CYAN, -1);
    init_pair(CP_GREEN, COLOR_GREEN, -1);
    init_pair(CP_YELLOW, COLOR_YELLOW, -1);
    init_pair(CP_RED, COLOR_RED, -1);
    init_pair(CP_POPUP_HEADER, COLOR_WHITE, COLOR_BLUE);
    init_pair(CP_POPUP_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);
}

inline void end_tui() {
    endwin();
}

inline std::vector<std::string> wrap_text(const std::string& text, int width) {
    std::vector<std::string> wrapped;
    if (width <= 0) return { text };
    std::stringstream paragraph_stream(text);
    std::string paragraph;
    while (std::getline(paragraph_stream, paragraph, '\n')) {
        if (paragraph.empty()) {
            wrapped.push_back("");
            continue;
        }
        std::stringstream word_stream(paragraph);
        std::string word;
        std::string current_line;
        while (word_stream >> word) {
            if (current_line.empty()) {
                current_line = word;
            } else if (current_line.length() + 1 + word.length() <= (size_t)width) {
                current_line += " " + word;
            } else {
                wrapped.push_back(current_line);
                current_line = word;
            }
        }
        if (!current_line.empty()) {
            wrapped.push_back(current_line);
        }
    }
    return wrapped;
}

// Prompts user for a text input line inside a centered popup dialog.
// ESC cancels the dialog.
inline void redraw_input_field(WINDOW* win, int y, int x, int width, const std::string& input, int cursor_pos) {
    wattron(win, A_UNDERLINE);
    for (int i = 0; i < width; ++i) {
        mvwaddch(win, y, x + i, ' ');
    }
    
    std::string visible_part = "";
    int scroll_offset = 0;
    if (input.length() < (size_t)width) {
        visible_part = input;
        scroll_offset = 0;
    } else {
        if (cursor_pos < width - 5) {
            visible_part = input.substr(0, width);
            scroll_offset = 0;
        } else {
            scroll_offset = cursor_pos - width + 5;
            if (scroll_offset + width > (int)input.length()) {
                scroll_offset = (int)input.length() - width;
            }
            if (scroll_offset < 0) scroll_offset = 0;
            visible_part = input.substr(scroll_offset, width);
        }
    }
    
    mvwprintw(win, y, x, "%s", visible_part.c_str());
    wattroff(win, A_UNDERLINE);
    
    int relative_cursor = cursor_pos - scroll_offset;
    if (relative_cursor < 0) relative_cursor = 0;
    if (relative_cursor >= width) relative_cursor = width - 1;
    
    wmove(win, y, x + relative_cursor);
    wrefresh(win);
}

inline std::string get_input_field(WINDOW* win, int y, int x, int width, bool& cancelled, const std::string& initial_val = "", WINDOW* footer_win = nullptr, Mode* global_mode = nullptr) {
    std::string input = initial_val;
    int cursor_pos = input.length();
    
    Mode local_mode = Mode::Insert;
    if (global_mode) {
        *global_mode = Mode::Insert;
    }
    
    if (footer_win) {
        draw_footer(footer_win, local_mode, Focus::Tasks, "");
    }
    
    curs_set(1);
    noecho();
    keypad(win, TRUE);
    
    redraw_input_field(win, y, x, width, input, cursor_pos);
    
    while (true) {
        int ch = wgetch(win);
        
        if (local_mode == Mode::Insert) {
            if (ch == 27) { // ESC key or Alt key sequence
                nodelay(win, TRUE);
                int next_ch = wgetch(win);
                nodelay(win, FALSE);
                if (next_ch == KEY_BACKSPACE || next_ch == 127 || next_ch == 8) {
                    while (cursor_pos > 0 && input[cursor_pos - 1] == ' ') {
                        input.erase(cursor_pos - 1, 1);
                        cursor_pos--;
                    }
                    while (cursor_pos > 0 && input[cursor_pos - 1] != ' ') {
                        input.erase(cursor_pos - 1, 1);
                        cursor_pos--;
                    }
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                } else if (next_ch == ERR) {
                    local_mode = Mode::Normal;
                    if (global_mode) {
                        *global_mode = Mode::Normal;
                    }
                    if (footer_win) {
                        draw_footer(footer_win, local_mode, Focus::Tasks, "");
                    }
                    if (cursor_pos > 0 && cursor_pos == (int)input.length()) {
                        cursor_pos--;
                    }
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            } else if (ch == '\n' || ch == '\r') {
                break;
            } else if (ch == KEY_LEFT) {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            } else if (ch == KEY_RIGHT) {
                if (cursor_pos < (int)input.length()) {
                    cursor_pos++;
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (cursor_pos > 0) {
                    input.erase(cursor_pos - 1, 1);
                    cursor_pos--;
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            } else if (ch >= 32 && ch <= 126) {
                if (input.length() < 100) {
                    input.insert(cursor_pos, 1, ch);
                    cursor_pos++;
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            }
        } else { // Normal Mode
            if (ch == 27) { // ESC in Normal mode cancels the dialog
                cancelled = true;
                curs_set(0);
                if (global_mode) {
                    *global_mode = Mode::Normal;
                }
                return "";
            } else if (ch == '\n' || ch == '\r') {
                break;
            } else if (ch == 'i') {
                local_mode = Mode::Insert;
                if (global_mode) {
                    *global_mode = Mode::Insert;
                }
                if (footer_win) {
                    draw_footer(footer_win, local_mode, Focus::Tasks, "");
                }
                redraw_input_field(win, y, x, width, input, cursor_pos);
            } else if (ch == 'a') {
                local_mode = Mode::Insert;
                if (global_mode) {
                    *global_mode = Mode::Insert;
                }
                if (footer_win) {
                    draw_footer(footer_win, local_mode, Focus::Tasks, "");
                }
                if (cursor_pos < (int)input.length()) {
                    cursor_pos++;
                }
                redraw_input_field(win, y, x, width, input, cursor_pos);
            } else if (ch == 'h' || ch == KEY_LEFT) {
                if (cursor_pos > 0) {
                    cursor_pos--;
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            } else if (ch == 'l' || ch == KEY_RIGHT) {
                if (cursor_pos < (int)input.length() - 1) {
                    cursor_pos++;
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            } else if (ch == 'x') {
                if (!input.empty() && cursor_pos >= 0 && cursor_pos < (int)input.length()) {
                    input.erase(cursor_pos, 1);
                    if (cursor_pos >= (int)input.length() && cursor_pos > 0) {
                        cursor_pos--;
                    }
                    redraw_input_field(win, y, x, width, input, cursor_pos);
                }
            }
        }
    }
    curs_set(0);
    cancelled = false;
    if (global_mode) {
        *global_mode = Mode::Normal;
    }
    return input;
}

// Shows a confirmation modal dialog (Yes/No). Returns true if Yes is selected.
inline bool show_confirm_dialog(const std::string& title, const std::string& message) {
    int width = 60;
    std::vector<std::string> wrapped_msg = wrap_text(message, width - 8);
    int height = 7 + wrapped_msg.size();
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    // Draw header
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - title.length() - 2) / 2, " %s ", title.c_str());
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    for (size_t i = 0; i < wrapped_msg.size(); ++i) {
        mvwprintw(win, 2 + i, (width - wrapped_msg[i].length()) / 2, "%s", wrapped_msg[i].c_str());
    }
    
    int button_y = 3 + wrapped_msg.size();
    bool selection = true; // true = Yes, false = No
    while (true) {
        // Render buttons
        if (selection) {
            wattron(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, button_y, width / 2 - 8, "[ Yes ]");
            wattroff(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            
            mvwprintw(win, button_y, width / 2 + 2, "[ No  ]");
        } else {
            mvwprintw(win, button_y, width / 2 - 8, "[ Yes ]");
            
            wattron(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, button_y, width / 2 + 2, "[ No  ]");
            wattroff(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
        }
        
        wrefresh(win);
        int ch = wgetch(win);
        if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == '\t') {
            selection = !selection;
        } else if (ch == 'y' || ch == 'Y') {
            delwin(win);
            return true;
        } else if (ch == 'n' || ch == 'N') {
            delwin(win);
            return false;
        } else if (ch == '\n' || ch == '\r') {
            delwin(win);
            return selection;
        } else if (ch == 27) { // ESC
            delwin(win);
            return false;
        }
    }
}

// Shows input modal dialog for creating a list.
inline std::string show_list_input_dialog(bool& cancelled, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr) {
    int height = 8;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - 16) / 2, " New Category ");
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    mvwprintw(win, 2, 4, "Enter list name:");
    mvwprintw(win, 5, (width - 38) / 2, "(Press Enter to submit, ESC to cancel)");
    
    std::string name = get_input_field(win, 3, 4, width - 8, cancelled, "", footer_win, global_mode);
    
    delwin(win);
    return name;
}

// Shows input modal dialog for renaming a list.
inline std::string show_list_edit_dialog(const std::string& current_name, bool& cancelled, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr) {
    int height = 8;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - 17) / 2, " Rename Category ");
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    mvwprintw(win, 2, 4, "Edit list name:");
    mvwprintw(win, 5, (width - 38) / 2, "(Press Enter to submit, ESC to cancel)");
    
    std::string name = get_input_field(win, 3, 4, width - 8, cancelled, current_name, footer_win, global_mode);
    
    delwin(win);
    return name;
}

// Shows selection dialog to choose task type (Activity vs Assignment)
inline TaskType show_task_type_dialog(bool& cancelled) {
    int height = 9;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - 15) / 2, " Choose Type ");
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    mvwprintw(win, 2, 4, "Select the type of task to create:");
    
    int selection = 0; // 0 = Activity, 1 = Assignment
    while (true) {
        if (selection == 0) {
            wattron(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, 4, 6, " > Activity   (Starts at: Date/Time)     ");
            wattroff(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            
            mvwprintw(win, 5, 6, "   Assignment (Due by: Date/Time)        ");
        } else {
            mvwprintw(win, 4, 6, "   Activity   (Starts at: Date/Time)     ");
            
            wattron(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            mvwprintw(win, 5, 6, " > Assignment (Due by: Date/Time)        ");
            wattroff(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
        }
        
        mvwprintw(win, 7, (width - 58) / 2, "(Up/Down or j/k to select, Enter to choose, ESC to cancel)");
        wrefresh(win);
        
        int ch = wgetch(win);
        if (ch == KEY_UP || ch == KEY_DOWN || ch == 'j' || ch == 'k' || ch == '\t') {
            selection = (selection == 0) ? 1 : 0;
        } else if (ch == '\n' || ch == '\r') {
            delwin(win);
            cancelled = false;
            return (selection == 0) ? TaskType::Activity : TaskType::Assignment;
        } else if (ch == 27) { // ESC
            delwin(win);
            cancelled = true;
            return TaskType::Activity;
        }
    }
}

inline std::string get_preset_date(int days_offset) {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    tm->tm_mday += days_offset;
    std::time_t result_time = std::mktime(tm);
    if (result_time == -1) return "";
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
    return std::string(buf);
}

inline std::string show_datetime_picker_dialog(TaskType type, bool& cancelled, const std::string& initial_val = "", WINDOW* footer_win = nullptr, Mode* global_mode = nullptr) {
    std::vector<std::string> options = {
        "Today",
        "Tomorrow",
        "Next Week",
        "Custom (Calendar Picker)",
        "None (No date/time)"
    };
    
    std::string prompt = (type == TaskType::Activity) ? "Select start date preset:" : "Select deadline preset:";
    bool sel_cancelled = false;
    int choice = show_selection_dialog("Date/Time Preset", prompt, options, sel_cancelled);
    if (sel_cancelled || choice == -1) {
        cancelled = true;
        return "";
    }
    
    cancelled = false;
    if (choice == 4) { // None
        return "";
    }
    
    std::string date_str = "";
    if (choice == 0) date_str = get_preset_date(0);
    else if (choice == 1) date_str = get_preset_date(1);
    else if (choice == 2) date_str = get_preset_date(7);
    else { // Custom
        bool cal_cancelled = false;
        date_str = show_calendar_picker(cal_cancelled, initial_val);
        if (cal_cancelled || date_str.empty()) {
            cancelled = true;
            return "";
        }
    }
    
    int height = 9;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - 14) / 2, " Enter Time ");
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    mvwprintw(win, 2, 4, "Selected Date: %s", date_str.c_str());
    mvwprintw(win, 4, 4, "Enter time (HH:MM, default 12:00):");
    mvwprintw(win, 6, (width - 38) / 2, "(Press Enter to submit, ESC to cancel)");
    
    std::string time_val = get_input_field(win, 5, 4, width - 8, cancelled, "12:00", footer_win, global_mode);
    delwin(win);
    
    if (cancelled) {
        return "";
    }
    if (time_val.empty()) {
        time_val = "12:00";
    }
    return date_str + " " + time_val;
}
inline std::string show_multiline_editor_dialog(const std::string& title, const std::string& prompt, bool& cancelled, const std::string& initial_val = "") {
    int height = 15;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - title.length() - 2) / 2, " %s ", title.c_str());
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    mvwprintw(win, 1, 4, "%s", prompt.c_str());
    mvwprintw(win, height - 2, (width - 44) / 2, "[Ctrl+X / F2] Save  |  [ESC] Cancel");
    
    std::string input = initial_val;
    curs_set(1);
    noecho();
    
    while (true) {
        std::vector<std::string> lines;
        std::string current_para = "";
        for (char c : input) {
            if (c == '\n') {
                auto wrapped_para = wrap_text(current_para, width - 8);
                if (wrapped_para.empty()) {
                    lines.push_back("");
                } else {
                    lines.insert(lines.end(), wrapped_para.begin(), wrapped_para.end());
                }
                current_para = "";
            } else {
                current_para += c;
            }
        }
        auto wrapped_para = wrap_text(current_para, width - 8);
        if (wrapped_para.empty()) {
            lines.push_back("");
        } else {
            lines.insert(lines.end(), wrapped_para.begin(), wrapped_para.end());
        }
        
        for (int r = 2; r < height - 2; ++r) {
            wmove(win, r, 4);
            wclrtoeol(win);
            mvwaddch(win, r, width - 1, ACS_VLINE);
        }
        
        int max_lines_to_draw = height - 5;
        for (int i = 0; i < (int)lines.size() && i < max_lines_to_draw; ++i) {
            mvwprintw(win, 2 + i, 4, "%s", lines[i].c_str());
        }
        
        int cursor_y = 2 + (int)lines.size() - 1;
        if (cursor_y >= height - 3) {
            cursor_y = height - 3;
        }
        int cursor_x = 4 + (int)lines.back().length();
        if (cursor_x >= width - 4) {
            cursor_x = width - 4;
        }
        
        wmove(win, cursor_y, cursor_x);
        wrefresh(win);
        
        int ch = wgetch(win);
        if (ch == 27) { // ESC key or Alt key sequence
            nodelay(win, TRUE);
            int next_ch = wgetch(win);
            nodelay(win, FALSE);
            if (next_ch == KEY_BACKSPACE || next_ch == 127 || next_ch == 8) {
                while (!input.empty() && (input.back() == ' ' || input.back() == '\n')) {
                    input.pop_back();
                }
                while (!input.empty() && input.back() != ' ' && input.back() != '\n') {
                    input.pop_back();
                }
            } else if (next_ch == ERR) {
                delwin(win);
                cancelled = true;
                curs_set(0);
                return "";
            }
        }
        if (ch == 24 || ch == KEY_F(2) || ch == 4) { // Ctrl+X, F2, Ctrl+D
            delwin(win);
            cancelled = false;
            curs_set(0);
            return input;
        }
        if (ch == '\n' || ch == '\r') {
            if (lines.size() < (size_t)max_lines_to_draw) {
                input.push_back('\n');
            }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!input.empty()) {
                input.pop_back();
            }
        } else if (ch >= 32 && ch <= 126) {
            if (input.length() < 400 && lines.size() <= (size_t)max_lines_to_draw) {
                if (lines.back().length() < (size_t)width - 8 || lines.size() < (size_t)max_lines_to_draw) {
                    input.push_back(ch);
                }
            }
        }
    }
}

inline bool show_task_input_dialog(TaskType type, std::string& title_out, std::string& desc_out, std::string& time_out, RecurrenceRule& recurrence_out, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr) {
    int height = 14;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    std::string type_label = (type == TaskType::Activity) ? " New Activity Task " : " New Assignment Task ";
    mvwprintw(win, 0, (width - type_label.length()) / 2, "%s", type_label.c_str());
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    bool cancelled = false;
    
    // Prompt 1: Title
    mvwprintw(win, 2, 4, "1. Task Title:");
    std::string title = get_input_field(win, 3, 4, width - 8, cancelled, "", footer_win, global_mode);
    if (cancelled || title.empty()) {
        delwin(win);
        return false;
    }
    
    // Prompt 2: Description
    mvwprintw(win, 5, 4, "2. Description (optional):");
    mvwprintw(win, 6, 4, "(Opening Description Editor...)");
    wrefresh(win);
    std::string desc = show_multiline_editor_dialog("Task Description", "Type description below:", cancelled);
    touchwin(win);
    wrefresh(win);
    if (cancelled) {
        delwin(win);
        return false;
    }
    std::string desc_snippet = desc;
    if (desc_snippet.length() > (size_t)width - 12) {
        desc_snippet = desc_snippet.substr(0, width - 15) + "...";
    }
    for (char& c : desc_snippet) {
        if (c == '\n') c = ' ';
    }
    mvwprintw(win, 6, 4, "%-52s", desc_snippet.c_str());
    
    // Prompt 3: Date/Time Preset Selector
    std::string time_prompt = (type == TaskType::Activity) ? "3. Choose Start Date/Time:" : "3. Choose Deadline Date/Time:";
    mvwprintw(win, 8, 4, "%s", time_prompt.c_str());
    mvwprintw(win, 9, 4, "(Opening Preset Selector...)");
    wrefresh(win);
    std::string time_val = show_datetime_picker_dialog(type, cancelled, "", footer_win, global_mode);
    touchwin(win);
    wrefresh(win);
    if (cancelled) {
        delwin(win);
        return false;
    }
    
    // Prompt 4: Recurrence Rule Selection
    bool sel_cancelled = false;
    std::vector<std::string> rec_options = { "None", "Daily", "Weekly", "Monthly" };
    int choice = show_selection_dialog("Task Recurrence", "Select recurrence interval:", rec_options, sel_cancelled);
    if (sel_cancelled || choice == -1) {
        delwin(win);
        return false;
    }
    recurrence_out = (RecurrenceRule)choice;
    
    title_out = title;
    desc_out = desc;
    time_out = time_val;
    
    delwin(win);
    return true;
}

inline bool show_task_edit_dialog(TaskType type, const std::string& init_title, const std::string& init_desc, const std::string& init_time, RecurrenceRule init_recurrence, std::string& title_out, std::string& desc_out, std::string& time_out, RecurrenceRule& recurrence_out, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr) {
    int height = 14;
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    std::string type_label = (type == TaskType::Activity) ? " Edit Activity Task " : " Edit Assignment Task ";
    mvwprintw(win, 0, (width - type_label.length()) / 2, "%s", type_label.c_str());
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    bool cancelled = false;
    
    // Draw initial description snippet
    std::string init_desc_snippet = init_desc;
    if (init_desc_snippet.length() > (size_t)width - 12) {
        init_desc_snippet = init_desc_snippet.substr(0, width - 15) + "...";
    }
    for (char& c : init_desc_snippet) {
        if (c == '\n') c = ' ';
    }
    mvwprintw(win, 5, 4, "2. Description (optional):");
    mvwprintw(win, 6, 4, "%-52s", init_desc_snippet.c_str());
    wrefresh(win);
    
    // Prompt 1: Title
    mvwprintw(win, 2, 4, "1. Task Title:");
    std::string title = get_input_field(win, 3, 4, width - 8, cancelled, init_title, footer_win, global_mode);
    if (cancelled || title.empty()) {
        delwin(win);
        return false;
    }
    
    // Prompt 2: Description
    mvwprintw(win, 5, 4, "2. Description (optional):");
    mvwprintw(win, 6, 4, "(Opening Description Editor...)");
    wrefresh(win);
    std::string desc = show_multiline_editor_dialog("Task Description", "Type description below:", cancelled, init_desc);
    touchwin(win);
    wrefresh(win);
    if (cancelled) {
        delwin(win);
        return false;
    }
    std::string desc_snippet = desc;
    if (desc_snippet.length() > (size_t)width - 12) {
        desc_snippet = desc_snippet.substr(0, width - 15) + "...";
    }
    for (char& c : desc_snippet) {
        if (c == '\n') c = ' ';
    }
    mvwprintw(win, 6, 4, "%-52s", desc_snippet.c_str());
    
    // Prompt 3: Date/Time Preset Selector
    std::string time_prompt = (type == TaskType::Activity) ? "3. Choose Start Date/Time:" : "3. Choose Deadline Date/Time:";
    mvwprintw(win, 8, 4, "%s", time_prompt.c_str());
    mvwprintw(win, 9, 4, "(Opening Preset Selector...)");
    wrefresh(win);
    std::string time_val = show_datetime_picker_dialog(type, cancelled, init_time, footer_win, global_mode);
    touchwin(win);
    wrefresh(win);
    if (cancelled) {
        delwin(win);
        return false;
    }
    
    // Prompt 4: Recurrence Rule Selection
    bool sel_cancelled = false;
    std::vector<std::string> rec_options = { "None", "Daily", "Weekly", "Monthly" };
    int choice = show_selection_dialog("Task Recurrence", "Select recurrence interval:", rec_options, sel_cancelled, (int)init_recurrence);
    if (sel_cancelled || choice == -1) {
        delwin(win);
        return false;
    }
    recurrence_out = (RecurrenceRule)choice;
    
    title_out = title;
    desc_out = desc;
    time_out = time_val;
    
    delwin(win);
    return true;
}

inline int show_selection_dialog(const std::string& title, const std::string& prompt, const std::vector<std::string>& options, bool& cancelled, int default_sel) {
    int height = 7 + options.size();
    int width = 60;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - title.length() - 2) / 2, " %s ", title.c_str());
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    mvwprintw(win, 2, 4, "%s", prompt.c_str());
    
    int selection = default_sel;
    while (true) {
        for (size_t i = 0; i < options.size(); ++i) {
            std::string display_opt = options[i];
            if (display_opt.length() > 46) {
                display_opt = display_opt.substr(0, 43) + "...";
            }
            if ((int)i == selection) {
                wattron(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
                mvwprintw(win, 4 + i, 6, " > %-46s ", display_opt.c_str());
                wattroff(win, COLOR_PAIR(CP_POPUP_HIGHLIGHT) | A_BOLD);
            } else {
                mvwprintw(win, 4 + i, 6, "   %-46s ", display_opt.c_str());
            }
        }
        
        mvwprintw(win, height - 2, (width - 51) / 2, "(Up/Down to select, Enter to choose, ESC to cancel)");
        wrefresh(win);
        
        int ch = wgetch(win);
        if (ch == KEY_UP || ch == 'k') {
            if (selection > 0) selection--;
        } else if (ch == KEY_DOWN || ch == 'j') {
            if (selection < (int)options.size() - 1) selection++;
        } else if (ch == '\n' || ch == '\r') {
            delwin(win);
            cancelled = false;
            return selection;
        } else if (ch == 27) { // ESC
            delwin(win);
            cancelled = true;
            return -1;
        }
    }
}

// Shows snooze selection modal.
inline int show_snooze_dialog(bool& cancelled) {
    std::vector<std::string> options = {
        "+1 Hour",
        "+1 Day",
        "+1 Week"
    };
    return show_selection_dialog("Snooze Task", "Postpone task deadline/starts by:", options, cancelled);
}


// Shows help and keybindings modal dialog.
inline void show_help_dialog() {
    int height = 18;
    int width = 72;
    int start_y = (LINES - height) / 2;
    int start_x = (COLS - width) / 2;
    
    WINDOW* win = newwin(height, width, start_y, start_x);
    keypad(win, TRUE);
    box(win, 0, 0);
    
    wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    mvwprintw(win, 0, (width - 20) / 2, " Help & Keybindings ");
    wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
    
    int r = 1;
    wattron(win, COLOR_PAIR(CP_YELLOW) | A_BOLD);
    mvwprintw(win, r++, 2, "General Navigation:");
    wattroff(win, COLOR_PAIR(CP_YELLOW) | A_BOLD);
    mvwprintw(win, r++, 4, "Tab / h / l  - Switch active pane");
    mvwprintw(win, r++, 4, "/            - Filter tasks (Search)");
    mvwprintw(win, r++, 4, "?            - Show this help tooltip");
    mvwprintw(win, r++, 4, "q            - Save and Exit application");
    
    r++;
    wattron(win, COLOR_PAIR(CP_CYAN) | A_BOLD);
    mvwprintw(win, r++, 2, "Category Actions:");
    wattroff(win, COLOR_PAIR(CP_CYAN) | A_BOLD);
    mvwprintw(win, r++, 4, "j / k        - Scroll list");
    mvwprintw(win, r++, 4, "n / a        - Create new category");
    mvwprintw(win, r++, 4, "e            - Rename category");
    mvwprintw(win, r++, 4, "d / x        - Delete category");
    mvwprintw(win, r++, 4, "s            - Sort or shift categories");
    
    r++;
    wattron(win, COLOR_PAIR(CP_GREEN) | A_BOLD);
    mvwprintw(win, r++, 2, "Task Actions:");
    wattroff(win, COLOR_PAIR(CP_GREEN) | A_BOLD);
    mvwprintw(win, r++, 4, "j / k        - Scroll list");
    mvwprintw(win, r++, 4, "Space        - Toggle completion (recurrence)");
    mvwprintw(win, r++, 4, "n / a / e    - Add / Edit task details");
    mvwprintw(win, r++, 4, "d / x        - Delete task");
    mvwprintw(win, r++, 4, "p / m        - Snooze task / Move to category");
    mvwprintw(win, r++, 4, "s            - Sort or shift task positions");
    
    mvwprintw(win, height - 2, (width - 24) / 2, "(Press any key to close)");
    wrefresh(win);
    
    wgetch(win);
    delwin(win);
}

} // namespace Tui

#endif // TUI_HPP
