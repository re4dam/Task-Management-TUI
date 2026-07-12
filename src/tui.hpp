#ifndef TUI_HPP
#define TUI_HPP

#include <ncurses.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include "list.hpp"
#include "task.hpp"

enum class Focus {
    Lists,
    Tasks,
    Search
};

namespace Tui {

// Forward declarations
inline int show_selection_dialog(const std::string& title, const std::string& prompt, const std::vector<std::string>& options, bool& cancelled, int default_sel = 0);

// Color Pair constants
const int CP_BLUE = 1;
const int CP_CYAN = 2;
const int CP_GREEN = 3;
const int CP_YELLOW = 4;
const int CP_RED = 5;
const int CP_POPUP_HEADER = 6;
const int CP_POPUP_HIGHLIGHT = 7;

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
inline std::string get_input_field(WINDOW* win, int y, int x, int width, bool& cancelled, const std::string& initial_val = "") {
    std::string input = initial_val;
    curs_set(1);
    noecho();
    wattron(win, A_UNDERLINE);
    for (int i = 0; i < width; ++i) {
        mvwaddch(win, y, x + i, ' ');
    }
    if (!initial_val.empty()) {
        mvwprintw(win, y, x, "%s", initial_val.c_str());
    }
    wattroff(win, A_UNDERLINE);
    wmove(win, y, x + input.length());
    wrefresh(win);
    
    int curr_x = x + input.length();
    while (true) {
        int ch = wgetch(win);
        if (ch == 27) { // ESC key
            cancelled = true;
            curs_set(0);
            return "";
        }
        if (ch == '\n' || ch == '\r') {
            break;
        }
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (!input.empty()) {
                input.pop_back();
                curr_x--;
                wattron(win, A_UNDERLINE);
                mvwaddch(win, y, curr_x, ' ');
                wattroff(win, A_UNDERLINE);
                wmove(win, y, curr_x);
                wrefresh(win);
            }
        } else if (ch >= 32 && ch <= 126) {
            if (input.length() < (size_t)width - 1) {
                input.push_back(ch);
                mvwaddch(win, y, curr_x, ch);
                curr_x++;
                wrefresh(win);
            }
        }
    }
    curs_set(0);
    cancelled = false;
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
inline std::string show_list_input_dialog(bool& cancelled) {
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
    
    std::string name = get_input_field(win, 3, 4, width - 8, cancelled);
    
    delwin(win);
    return name;
}

// Shows input modal dialog for renaming a list.
inline std::string show_list_edit_dialog(const std::string& current_name, bool& cancelled) {
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
    
    std::string name = get_input_field(win, 3, 4, width - 8, cancelled, current_name);
    
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
        
        mvwprintw(win, 7, (width - 51) / 2, "(Up/Down to select, Enter to choose, ESC to cancel)");
        wrefresh(win);
        
        int ch = wgetch(win);
        if (ch == KEY_UP || ch == KEY_DOWN || ch == '\t') {
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

inline std::string show_datetime_picker_dialog(TaskType type, bool& cancelled, const std::string& initial_val = "") {
    std::vector<std::string> options = {
        "Today",
        "Tomorrow",
        "Next Week",
        "Custom (Type manually)",
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
        int height = 8;
        int width = 60;
        int start_y = (LINES - height) / 2;
        int start_x = (COLS - width) / 2;
        WINDOW* win = newwin(height, width, start_y, start_x);
        keypad(win, TRUE);
        box(win, 0, 0);
        
        wattron(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
        mvwprintw(win, 0, (width - 15) / 2, " Custom Date ");
        wattroff(win, COLOR_PAIR(CP_POPUP_HEADER) | A_BOLD);
        
        mvwprintw(win, 2, 4, "Enter Date/Time (YYYY-MM-DD HH:MM):");
        mvwprintw(win, 5, (width - 38) / 2, "(Press Enter to submit, ESC to cancel)");
        
        std::string typed_val = get_input_field(win, 3, 4, width - 8, cancelled, initial_val);
        delwin(win);
        return typed_val;
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
    
    std::string time_val = get_input_field(win, 5, 4, width - 8, cancelled, "12:00");
    delwin(win);
    
    if (cancelled) {
        return "";
    }
    if (time_val.empty()) {
        time_val = "12:00";
    }
    return date_str + " " + time_val;
}

inline bool show_task_input_dialog(TaskType type, std::string& title_out, std::string& desc_out, std::string& time_out, RecurrenceRule& recurrence_out) {
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
    std::string title = get_input_field(win, 3, 4, width - 8, cancelled);
    if (cancelled || title.empty()) {
        delwin(win);
        return false;
    }
    
    // Prompt 2: Description
    mvwprintw(win, 5, 4, "2. Description (optional):");
    std::string desc = get_input_field(win, 6, 4, width - 8, cancelled);
    if (cancelled) {
        delwin(win);
        return false;
    }
    
    // Prompt 3: Date/Time Preset Selector
    std::string time_prompt = (type == TaskType::Activity) ? "3. Choose Start Date/Time:" : "3. Choose Deadline Date/Time:";
    mvwprintw(win, 8, 4, "%s", time_prompt.c_str());
    mvwprintw(win, 9, 4, "(Opening Preset Selector...)");
    wrefresh(win);
    std::string time_val = show_datetime_picker_dialog(type, cancelled);
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

inline bool show_task_edit_dialog(TaskType type, const std::string& init_title, const std::string& init_desc, const std::string& init_time, RecurrenceRule init_recurrence, std::string& title_out, std::string& desc_out, std::string& time_out, RecurrenceRule& recurrence_out) {
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
    
    // Prompt 1: Title
    mvwprintw(win, 2, 4, "1. Task Title:");
    std::string title = get_input_field(win, 3, 4, width - 8, cancelled, init_title);
    if (cancelled || title.empty()) {
        delwin(win);
        return false;
    }
    
    // Prompt 2: Description
    mvwprintw(win, 5, 4, "2. Description (optional):");
    std::string desc = get_input_field(win, 6, 4, width - 8, cancelled, init_desc);
    if (cancelled) {
        delwin(win);
        return false;
    }
    
    // Prompt 3: Date/Time Preset Selector
    std::string time_prompt = (type == TaskType::Activity) ? "3. Choose Start Date/Time:" : "3. Choose Deadline Date/Time:";
    mvwprintw(win, 8, 4, "%s", time_prompt.c_str());
    mvwprintw(win, 9, 4, "(Opening Preset Selector...)");
    wrefresh(win);
    std::string time_val = show_datetime_picker_dialog(type, cancelled, init_time);
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


} // namespace Tui

#endif // TUI_HPP
