#include "keybinds.hpp"
#include "keycodes.hpp"
#include <algorithm>

std::string mode_to_string(Mode mode) {
    switch (mode) {
        case Mode::Normal: return "normal";
        case Mode::Insert: return "insert";
        case Mode::Visual: return "visual";
        case Mode::Command: return "command";
    }
    return "normal";
}

Mode string_to_mode(const std::string& str) {
    if (str == "normal") return Mode::Normal;
    if (str == "insert") return Mode::Insert;
    if (str == "visual") return Mode::Visual;
    if (str == "command") return Mode::Command;
    return Mode::Normal;
}

std::string action_to_string(Action action) {
    switch (action) {
        case Action::MoveUp: return "MoveUp";
        case Action::MoveDown: return "MoveDown";
        case Action::FocusLeft: return "FocusLeft";
        case Action::FocusRight: return "FocusRight";
        case Action::TogglePaneFocus: return "TogglePaneFocus";
        case Action::Create: return "Create";
        case Action::Edit: return "Edit";
        case Action::Delete: return "Delete";
        case Action::Sort: return "Sort";
        case Action::ToggleComplete: return "ToggleComplete";
        case Action::Snooze: return "Snooze";
        case Action::MoveToCategory: return "MoveToCategory";
        case Action::Search: return "Search";
        case Action::Help: return "Help";
        case Action::Quit: return "Quit";
        case Action::EnterNormalMode: return "EnterNormalMode";
        case Action::EnterInsertMode: return "EnterInsertMode";
        case Action::EnterVisualMode: return "EnterVisualMode";
        case Action::EnterCommandMode: return "EnterCommandMode";
        case Action::OpenDatePicker: return "OpenDatePicker";
        case Action::CycleTheme: return "CycleTheme";
        case Action::Confirm: return "Confirm";
        case Action::Cancel: return "Cancel";
        default: return "None";
    }
}

Action string_to_action(const std::string& str) {
    if (str == "MoveUp") return Action::MoveUp;
    if (str == "MoveDown") return Action::MoveDown;
    if (str == "FocusLeft") return Action::FocusLeft;
    if (str == "FocusRight") return Action::FocusRight;
    if (str == "TogglePaneFocus") return Action::TogglePaneFocus;
    if (str == "Create") return Action::Create;
    if (str == "Edit") return Action::Edit;
    if (str == "Delete") return Action::Delete;
    if (str == "Sort") return Action::Sort;
    if (str == "ToggleComplete") return Action::ToggleComplete;
    if (str == "Snooze") return Action::Snooze;
    if (str == "MoveToCategory") return Action::MoveToCategory;
    if (str == "Search") return Action::Search;
    if (str == "Help") return Action::Help;
    if (str == "Quit") return Action::Quit;
    if (str == "EnterNormalMode") return Action::EnterNormalMode;
    if (str == "EnterInsertMode") return Action::EnterInsertMode;
    if (str == "EnterVisualMode") return Action::EnterVisualMode;
    if (str == "EnterCommandMode") return Action::EnterCommandMode;
    if (str == "OpenDatePicker") return Action::OpenDatePicker;
    if (str == "CycleTheme") return Action::CycleTheme;
    if (str == "Confirm") return Action::Confirm;
    if (str == "Cancel") return Action::Cancel;
    return Action::None;
}

int string_to_keycode(const std::string& str) {
    if (str.length() == 1) {
        return (int)str[0];
    }
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    if (s == "up") return DOTUI_KEY_UP;
    if (s == "down") return DOTUI_KEY_DOWN;
    if (s == "left") return DOTUI_KEY_LEFT;
    if (s == "right") return DOTUI_KEY_RIGHT;
    if (s == "tab") return 9;
    if (s == "shift+tab") return DOTUI_KEY_BTAB;
    if (s == "enter" || s == "return") return 10;
    if (s == "esc" || s == "escape") return 27;
    if (s == "space") return ' ';
    if (s == "backspace") return DOTUI_KEY_BACKSPACE;
    return -1;
}

std::string keycode_to_string(int key) {
    if (key >= 32 && key <= 126) {
        return std::string(1, (char)key);
    }
    switch (key) {
        case DOTUI_KEY_UP: return "up";
        case DOTUI_KEY_DOWN: return "down";
        case DOTUI_KEY_LEFT: return "left";
        case DOTUI_KEY_RIGHT: return "right";
        case 9: return "tab";
        case DOTUI_KEY_BTAB: return "shift+tab";
        case 10: return "enter";
        case 27: return "esc";
        case DOTUI_KEY_BACKSPACE: return "backspace";
        case 127: return "backspace";
        case 8: return "backspace";
    }
    return "unknown";
}

Keymap::Keymap() {
    load_defaults();
}

void Keymap::load_defaults() {
    mappings.clear();
    
    // Default Normal mode keys
    mappings[Mode::Normal][DOTUI_KEY_UP] = Action::MoveUp;
    mappings[Mode::Normal][DOTUI_KEY_DOWN] = Action::MoveDown;
    mappings[Mode::Normal][DOTUI_KEY_LEFT] = Action::FocusLeft;
    mappings[Mode::Normal][DOTUI_KEY_RIGHT] = Action::FocusRight;
    mappings[Mode::Normal]['k'] = Action::MoveUp;
    mappings[Mode::Normal]['j'] = Action::MoveDown;
    mappings[Mode::Normal]['h'] = Action::FocusLeft;
    mappings[Mode::Normal]['l'] = Action::FocusRight;
    mappings[Mode::Normal][9] = Action::TogglePaneFocus; // Tab
    mappings[Mode::Normal][DOTUI_KEY_BTAB] = Action::FocusLeft; // Shift+Tab
    mappings[Mode::Normal]['n'] = Action::Create;
    mappings[Mode::Normal]['a'] = Action::Create;
    mappings[Mode::Normal]['e'] = Action::Edit;
    mappings[Mode::Normal]['d'] = Action::Delete;
    mappings[Mode::Normal]['x'] = Action::Delete;
    mappings[Mode::Normal]['s'] = Action::Sort;
    mappings[Mode::Normal][' '] = Action::ToggleComplete;
    mappings[Mode::Normal]['p'] = Action::Snooze;
    mappings[Mode::Normal]['m'] = Action::MoveToCategory;
    mappings[Mode::Normal]['/'] = Action::Search;
    mappings[Mode::Normal]['?'] = Action::Help;
    mappings[Mode::Normal][':'] = Action::EnterCommandMode;
    mappings[Mode::Normal]['q'] = Action::Quit;

    // Default Insert mode keys (minimal for now)
    mappings[Mode::Insert][27] = Action::EnterNormalMode;

    // Default Visual mode keys (minimal for now)
    mappings[Mode::Visual][27] = Action::EnterNormalMode;

    // Default Command mode keys (minimal for now)
    mappings[Mode::Command][27] = Action::EnterNormalMode;
}

std::vector<std::string> Keymap::get_keys_for_action(Action action, Mode mode) const {
    std::vector<std::string> keys;
    auto mode_it = mappings.find(mode);
    if (mode_it != mappings.end()) {
        for (const auto& [keycode, act] : mode_it->second) {
            if (act == action) {
                std::string k_str = keycode_to_string(keycode);
                if (k_str != "unknown") {
                    keys.push_back(k_str);
                }
            }
        }
    }
    return keys;
}

Action Keymap::resolve(int keycode, Mode mode) const {
    auto mode_it = mappings.find(mode);
    if (mode_it != mappings.end()) {
        auto key_it = mode_it->second.find(keycode);
        if (key_it != mode_it->second.end()) {
            return key_it->second;
        }
    }
    return Action::None;
}
