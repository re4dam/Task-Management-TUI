#ifndef INPUT_KEYBINDS_HPP
#define INPUT_KEYBINDS_HPP

#include <string>
#include <map>
#include <vector>

enum class Mode {
    Normal,
    Insert,
    Visual,
    Command
};

std::string mode_to_string(Mode mode);
Mode string_to_mode(const std::string& str);

enum class Action {
    None,
    MoveUp,
    MoveDown,
    FocusLeft,
    FocusRight,
    TogglePaneFocus, // Tab
    Create,          // n / a
    Edit,            // e
    Delete,          // d / x
    Sort,            // s
    ToggleComplete,  // Space
    Snooze,          // p
    MoveToCategory,  // m
    Search,          // /
    Help,            // ?
    Quit,            // q
    
    // Mode transitions & command bar / date picker
    EnterNormalMode,
    EnterInsertMode,
    EnterVisualMode,
    EnterCommandMode,
    OpenDatePicker,
    CycleTheme,
    Confirm,
    Cancel
};

std::string action_to_string(Action action);
Action string_to_action(const std::string& str);

int string_to_keycode(const std::string& str);
std::string keycode_to_string(int key);

class Keymap {
public:
    std::map<Mode, std::map<int, Action>> mappings;

    Keymap();

    void load_defaults();
    std::vector<std::string> get_keys_for_action(Action action, Mode mode = Mode::Normal) const;
    Action resolve(int keycode, Mode mode = Mode::Normal) const;
};

#endif // INPUT_KEYBINDS_HPP
