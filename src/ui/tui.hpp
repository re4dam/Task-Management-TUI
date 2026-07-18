#ifndef UI_TUI_HPP
#define UI_TUI_HPP

#include <ncurses.h>
#include <string>
#include <vector>
#include "core/list.hpp"
#include "core/task.hpp"
#include "input/keybinds.hpp"
#include "core/theme.hpp"

enum class Focus {
    Lists,
    Tasks,
    Details,
    Search
};

namespace Tui {

// Color Pair constants
extern const int CP_BLUE;
extern const int CP_CYAN;
extern const int CP_GREEN;
extern const int CP_YELLOW;
extern const int CP_RED;
extern const int CP_POPUP_HEADER;
extern const int CP_POPUP_HIGHLIGHT;

short string_to_ncurses_color(const std::string& str);
void apply_theme(const ThemeColors& theme);
void draw_footer(WINDOW* footer_win, Mode mode, Focus focus, const std::string& search_query, const std::string& command_query = "");
int get_days_in_month(int year, int month);
int get_start_weekday(int year, int month);
std::string show_calendar_picker(bool& cancelled, const std::string& initial_date);
void init_tui();
void end_tui();
std::vector<std::string> wrap_text(const std::string& text, int width);
void redraw_input_field(WINDOW* win, int y, int x, int width, const std::string& input, int cursor_pos);
std::string get_input_field(WINDOW* win, int y, int x, int width, bool& cancelled, const std::string& initial_val = "", WINDOW* footer_win = nullptr, Mode* global_mode = nullptr);
bool show_confirm_dialog(const std::string& title, const std::string& message);
std::string show_list_input_dialog(bool& cancelled, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr);
std::string show_list_edit_dialog(const std::string& current_name, bool& cancelled, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr);
TaskType show_task_type_dialog(bool& cancelled);
std::string get_preset_date(int days_offset);
std::string show_datetime_picker_dialog(TaskType type, bool& cancelled, const std::string& initial_val = "", WINDOW* footer_win = nullptr, Mode* global_mode = nullptr);
std::string show_multiline_editor_dialog(const std::string& title, const std::string& prompt, bool& cancelled, const std::string& initial_val = "");
bool show_task_input_dialog(TaskType type, std::string& title_out, std::string& desc_out, std::string& time_out, RecurrenceRule& recurrence_out, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr);
bool show_task_edit_dialog(TaskType type, const std::string& init_title, const std::string& init_desc, const std::string& init_time, RecurrenceRule init_recurrence, std::string& title_out, std::string& desc_out, std::string& time_out, RecurrenceRule& recurrence_out, WINDOW* footer_win = nullptr, Mode* global_mode = nullptr);
int show_selection_dialog(const std::string& title, const std::string& prompt, const std::vector<std::string>& options, bool& cancelled, int default_sel = 0);
int show_snooze_dialog(bool& cancelled);
std::string action_to_description(Action action);
void show_help_dialog(const Keymap& keymap, Mode current_mode);

// Main Layout Draw Functions
void draw_lists(WINDOW* win, const std::vector<List>& lists, size_t selected_list_idx, Focus active_focus);
void draw_tasks(WINDOW* win, const std::vector<List>& lists, size_t selected_list_idx, size_t selected_task_idx, Focus active_focus, const std::vector<size_t>& pending_indices, const std::vector<size_t>& completed_indices, const Keymap& keymap);
void draw_details(WINDOW* win, bool has_selection, const Task& task, int& details_scroll_idx, Focus active_focus);

} // namespace Tui

#endif // UI_TUI_HPP
