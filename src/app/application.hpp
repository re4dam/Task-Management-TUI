#ifndef APP_APPLICATION_HPP
#define APP_APPLICATION_HPP

#include <vector>
#include <string>
#include <ncurses.h>
#include "core/list.hpp"
#include "persistence/persistence.hpp"
#include "input/keybinds.hpp"
#include "ui/tui.hpp"

class Application {
private:
    FilePersistence persistence;
    Keymap keymap;
    std::string active_theme;
    Mode active_mode;
    
    std::vector<List> lists;
    size_t selected_list_idx;
    size_t selected_task_idx;
    int details_scroll_idx;
    Focus active_focus;
    std::string search_query;
    std::string command_query;
    
    WINDOW* header_win;
    WINDOW* lists_win;
    WINDOW* tasks_win;
    WINDOW* details_win;
    WINDOW* footer_win;
    
    bool running;

    void setup_windows();
    void execute_command(const std::string& cmd_query, const std::vector<size_t>& pending_indices, const std::vector<size_t>& completed_indices);
    
    // Helpers
    bool contains_substring(const std::string& str, const std::string& sub);
    std::string trim(const std::string& str);
    int levenshtein_distance(const std::string& s1, const std::string& s2);

public:
    Application();
    ~Application();
    
    int run();
};

#endif // APP_APPLICATION_HPP
