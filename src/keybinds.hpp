#ifndef KEYBINDS_HPP
#define KEYBINDS_HPP

#include <ncurses.h>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "toml.hpp"

enum class Mode {
    Normal,
    Insert,
    Visual,
    Command
};

inline std::string mode_to_string(Mode mode) {
    switch (mode) {
        case Mode::Normal: return "normal";
        case Mode::Insert: return "insert";
        case Mode::Visual: return "visual";
        case Mode::Command: return "command";
    }
    return "normal";
}

inline Mode string_to_mode(const std::string& str) {
    if (str == "normal") return Mode::Normal;
    if (str == "insert") return Mode::Insert;
    if (str == "visual") return Mode::Visual;
    if (str == "command") return Mode::Command;
    return Mode::Normal;
}

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

inline std::string action_to_string(Action action) {
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

inline Action string_to_action(const std::string& str) {
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

inline int string_to_keycode(const std::string& str) {
    if (str.length() == 1) {
        return (int)str[0];
    }
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    if (s == "up") return KEY_UP;
    if (s == "down") return KEY_DOWN;
    if (s == "left") return KEY_LEFT;
    if (s == "right") return KEY_RIGHT;
    if (s == "tab") return 9;
    if (s == "shift+tab") return KEY_BTAB;
    if (s == "enter" || s == "return") return 10;
    if (s == "esc" || s == "escape") return 27;
    if (s == "space") return ' ';
    if (s == "backspace") return KEY_BACKSPACE;
    return -1;
}

inline std::string keycode_to_string(int key) {
    if (key >= 32 && key <= 126) {
        return std::string(1, (char)key);
    }
    switch (key) {
        case KEY_UP: return "up";
        case KEY_DOWN: return "down";
        case KEY_LEFT: return "left";
        case KEY_RIGHT: return "right";
        case 9: return "tab";
        case KEY_BTAB: return "shift+tab";
        case 10: return "enter";
        case 27: return "esc";
        case KEY_BACKSPACE: return "backspace";
        case 127: return "backspace";
        case 8: return "backspace";
    }
    return "unknown";
}

class Keymap {
public:
    std::map<Mode, std::map<int, Action>> mappings;

    Keymap() {
        load_defaults();
    }

    void load_defaults() {
        mappings.clear();
        
        // Default Normal mode keys
        mappings[Mode::Normal][KEY_UP] = Action::MoveUp;
        mappings[Mode::Normal][KEY_DOWN] = Action::MoveDown;
        mappings[Mode::Normal][KEY_LEFT] = Action::FocusLeft;
        mappings[Mode::Normal][KEY_RIGHT] = Action::FocusRight;
        mappings[Mode::Normal]['k'] = Action::MoveUp;
        mappings[Mode::Normal]['j'] = Action::MoveDown;
        mappings[Mode::Normal]['h'] = Action::FocusLeft;
        mappings[Mode::Normal]['l'] = Action::FocusRight;
        mappings[Mode::Normal][9] = Action::TogglePaneFocus; // Tab
        mappings[Mode::Normal][KEY_BTAB] = Action::FocusLeft; // Shift+Tab
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
        mappings[Mode::Normal]['q'] = Action::Quit;

        // Default Insert mode keys (minimal for now)
        mappings[Mode::Insert][27] = Action::EnterNormalMode;

        // Default Visual mode keys (minimal for now)
        mappings[Mode::Visual][27] = Action::EnterNormalMode;

        // Default Command mode keys (minimal for now)
        mappings[Mode::Command][27] = Action::EnterNormalMode;
    }

    std::string get_config_path() const {
        std::string path;
        const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
        if (xdg_config && *xdg_config != '\0') {
            path = std::string(xdg_config) + "/dotui/config.toml";
        } else {
            const char* home = std::getenv("HOME");
            if (home) {
                path = std::string(home) + "/.config/dotui/config.toml";
            } else {
                path = ".config/dotui/config.toml";
            }
        }
        return path;
    }

    void ensure_config_dir_exists(const std::string& filepath) {
        std::filesystem::path fp(filepath);
        std::filesystem::path parent = fp.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
    }

    bool load_from_file() {
        std::string path = get_config_path();
        if (!std::filesystem::exists(path)) {
            save_to_file(); // Save defaults on first run
            return true;
        }

        try {
            auto config = toml::parse_file(path);
            auto keybinds = config["keybinds"].as_table();
            if (keybinds) {
                mappings.clear();
                
                for (auto&& [mode_name, mode_table_node] : *keybinds) {
                    if (mode_table_node.is_table()) {
                        Mode mode = string_to_mode(std::string(mode_name.str()));
                        auto mode_table = mode_table_node.as_table();
                        for (auto&& [key_str, val_node] : *mode_table) {
                            if (val_node.is_string()) {
                                int keycode = string_to_keycode(std::string(key_str.str()));
                                Action action = string_to_action(val_node.as_string()->get());
                                if (keycode != -1 && action != Action::None) {
                                    mappings[mode][keycode] = action;
                                }
                            }
                        }
                    }
                }
                
                // Fallback for unconfigured modes to have default keys
                if (mappings.find(Mode::Normal) == mappings.end()) {
                    load_defaults();
                    save_to_file();
                }
                return true;
            }
        } catch (const toml::parse_error& err) {
            std::cerr << "TOML parse error: " << err.description() << " at " << err.source().begin << "\n";
            load_defaults();
            return false;
        } catch (...) {
            load_defaults();
            return false;
        }
        return false;
    }

    bool save_to_file() {
        std::string path = get_config_path();
        try {
            ensure_config_dir_exists(path);
            
            toml::table tbl;
            toml::table keybinds_tbl;

            for (const auto& [mode, mode_mappings] : mappings) {
                toml::table mode_tbl;
                for (const auto& [keycode, action] : mode_mappings) {
                    std::string key_str = keycode_to_string(keycode);
                    if (key_str != "unknown") {
                        mode_tbl.insert_or_assign(key_str, action_to_string(action));
                    }
                }
                keybinds_tbl.insert_or_assign(mode_to_string(mode), mode_tbl);
            }
            tbl.insert_or_assign("keybinds", keybinds_tbl);

            std::ofstream out(path);
            if (out.is_open()) {
                out << tbl << "\n";
                return true;
            }
        } catch (...) {
            return false;
        }
        return false;
    }

    Action resolve(int keycode, Mode mode = Mode::Normal) const {
        auto mode_it = mappings.find(mode);
        if (mode_it != mappings.end()) {
            auto key_it = mode_it->second.find(keycode);
            if (key_it != mode_it->second.end()) {
                return key_it->second;
            }
        }
        return Action::None;
    }
};

#endif // KEYBINDS_HPP
