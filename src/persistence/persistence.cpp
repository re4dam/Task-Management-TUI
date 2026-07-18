#include "persistence.hpp"
#include "../input/keybinds.hpp"
#include "../toml.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

FilePersistence::FilePersistence() {
    // Determine the default tasks_data filepath
    std::string path;
    std::string old_path;
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config != '\0') {
        path = std::string(xdg_config) + "/dotui/tasks.txt";
        old_path = std::string(xdg_config) + "/task-tui/tasks.txt";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.config/dotui/tasks.txt";
            old_path = std::string(home) + "/.config/task-tui/tasks.txt";
        } else {
            path = ".config/dotui/tasks.txt";
            old_path = ".config/task-tui/tasks.txt";
        }
    }

    try {
        if (!std::filesystem::exists(path) && std::filesystem::exists(old_path)) {
            ensure_directory_exists(path);
            std::filesystem::copy_file(old_path, path);
        }
    } catch (...) {
        // Fall back gracefully
    }
    
    tasks_filepath = path;
}

std::string FilePersistence::get_config_dir() {
    std::string path;
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config != '\0') {
        path = std::string(xdg_config) + "/dotui";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.config/dotui";
        } else {
            path = ".config/dotui";
        }
    }
    return path;
}

std::string FilePersistence::get_config_path() {
    return get_config_dir() + "/config.toml";
}

void FilePersistence::ensure_directory_exists(const std::string& filepath) {
    std::filesystem::path fp(filepath);
    std::filesystem::path parent = fp.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

std::string FilePersistence::escape(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (c == '\n') {
            result += "\\n";
        } else if (c == '\\') {
            result += "\\\\";
        } else if (c == '|') {
            result += "\\p";
        } else {
            result += c;
        }
    }
    return result;
}

std::string FilePersistence::unescape(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            if (str[i+1] == 'n') {
                result += '\n';
                i++;
            } else if (str[i+1] == '\\') {
                result += '\\';
                i++;
            } else if (str[i+1] == 'p') {
                result += '|';
                i++;
            } else {
                result += str[i];
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

std::vector<List> FilePersistence::load_tasks() {
    std::vector<List> lists;
    std::ifstream in(tasks_filepath);
    if (!in.is_open()) return lists;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = line.find('|');
        while (end != std::string::npos) {
            tokens.push_back(line.substr(start, end - start));
            start = end + 1;
            end = line.find('|', start);
        }
        tokens.push_back(line.substr(start));

        if (tokens.empty()) continue;
        if (tokens[0] == "L" && tokens.size() >= 2) {
            List list;
            list.name = unescape(tokens[1]);
            lists.push_back(list);
        } else if (tokens[0] == "T" && tokens.size() >= 6 && !lists.empty()) {
            Task task;
            task.completed = (tokens[1] == "1");
            task.type = (tokens[2] == "ACT") ? TaskType::Activity : TaskType::Assignment;
            task.title = unescape(tokens[3]);
            task.description = unescape(tokens[4]);
            task.time_value = unescape(tokens[5]);
            if (tokens.size() >= 7) {
                task.recurrence = string_to_recurrence(tokens[6]);
            } else {
                task.recurrence = RecurrenceRule::None;
            }
            lists.back().tasks.push_back(task);
        }
    }
    return lists;
}

bool FilePersistence::save_tasks(const std::vector<List>& lists) {
    ensure_directory_exists(tasks_filepath);
    std::ofstream out(tasks_filepath);
    if (!out.is_open()) return false;
    for (const auto& list : lists) {
        out << "L|" << escape(list.name) << "\n";
        for (const auto& task : list.tasks) {
            out << "T|" << (task.completed ? "1" : "0") << "|"
                << (task.type == TaskType::Activity ? "ACT" : "ASM") << "|"
                << escape(task.title) << "|"
                << escape(task.description) << "|"
                << escape(task.time_value) << "|"
                << recurrence_to_string(task.recurrence) << "\n";
        }
    }
    return true;
}

bool FilePersistence::load_config(Keymap& keymap, std::string& active_theme) {
    std::string path = get_config_path();
    if (!std::filesystem::exists(path)) {
        save_config(keymap, active_theme); // Save defaults on first run
        return true;
    }

    try {
        auto config = toml::parse_file(path);
        
        // Load active theme
        auto theme_tbl = config["theme"].as_table();
        if (theme_tbl) {
            auto active_val = theme_tbl->get("active");
            if (active_val && active_val->is_string()) {
                active_theme = active_val->as_string()->get();
            }
        }

        // Load keybinds
        auto keybinds = config["keybinds"].as_table();
        if (keybinds) {
            // Reset keymap to be populated
            keymap.mappings.clear();
            
            for (auto&& [mode_name, mode_table_node] : *keybinds) {
                if (mode_table_node.is_table()) {
                    Mode mode = string_to_mode(std::string(mode_name.str()));
                    auto mode_table = mode_table_node.as_table();
                    for (auto&& [key_str, val_node] : *mode_table) {
                        if (val_node.is_string()) {
                            int keycode = string_to_keycode(std::string(key_str.str()));
                            Action action = string_to_action(val_node.as_string()->get());
                            if (keycode != -1 && action != Action::None) {
                                keymap.mappings[mode][keycode] = action;
                            }
                        }
                    }
                }
            }
            
            // Fallback for unconfigured normal mode
            if (keymap.mappings.find(Mode::Normal) == keymap.mappings.end()) {
                keymap.load_defaults();
                save_config(keymap, active_theme);
            }
            return true;
        }
    } catch (...) {
        keymap.load_defaults();
        return false;
    }
    return false;
}

bool FilePersistence::save_config(const Keymap& keymap, const std::string& active_theme) {
    std::string path = get_config_path();
    try {
        ensure_directory_exists(path);
        
        toml::table tbl;
        
        // Theme section
        toml::table theme_tbl;
        theme_tbl.insert_or_assign("active", active_theme);
        tbl.insert_or_assign("theme", theme_tbl);

        // Keybinds section
        toml::table keybinds_tbl;
        for (const auto& [mode, mode_mappings] : keymap.mappings) {
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

bool FilePersistence::load_theme(const std::string& theme_name, ThemeColors& theme) {
    theme.name = theme_name;
    if (theme_name == "default" || theme_name.empty()) {
        theme = ThemeColors(); // Use default constructor values
        return true;
    }

    std::string theme_path = get_config_dir() + "/themes/" + theme_name + ".toml";
    if (!std::filesystem::exists(theme_path)) {
        return false;
    }

    try {
        auto tbl = toml::parse_file(theme_path);
        auto colors = tbl["colors"].as_table();
        if (colors) {
            auto parse_color = [&](const char* key, std::string& target) {
                auto val = colors->get(key);
                if (val && val->is_string()) {
                    target = val->as_string()->get();
                }
            };
            
            parse_color("border_unfocused_fg", theme.border_unfocused_fg);
            parse_color("border_unfocused_bg", theme.border_unfocused_bg);
            parse_color("border_focused_fg", theme.border_focused_fg);
            parse_color("border_focused_bg", theme.border_focused_bg);
            parse_color("success_fg", theme.success_fg);
            parse_color("success_bg", theme.success_bg);
            parse_color("warning_fg", theme.warning_fg);
            parse_color("warning_bg", theme.warning_bg);
            parse_color("danger_fg", theme.danger_fg);
            parse_color("danger_bg", theme.danger_bg);
            parse_color("popup_header_fg", theme.popup_header_fg);
            parse_color("popup_header_bg", theme.popup_header_bg);
            parse_color("popup_highlight_fg", theme.popup_highlight_fg);
            parse_color("popup_highlight_bg", theme.popup_highlight_bg);
            return true;
        }
    } catch (...) {
        return false;
    }
    return false;
}

void FilePersistence::generate_sample_theme_if_missing() {
    std::string themes_dir = get_config_dir() + "/themes";
    std::string path = themes_dir + "/dracula.toml";
    try {
        if (!std::filesystem::exists(themes_dir)) {
            std::filesystem::create_directories(themes_dir);
        }
        if (!std::filesystem::exists(path)) {
            std::ofstream out(path);
            if (out.is_open()) {
                out << "[colors]\n"
                    << "border_unfocused_fg = \"blue\"\n"
                    << "border_unfocused_bg = \"default\"\n"
                    << "border_focused_fg = \"magenta\"\n"
                    << "border_focused_bg = \"default\"\n"
                    << "success_fg = \"green\"\n"
                    << "success_bg = \"default\"\n"
                    << "warning_fg = \"yellow\"\n"
                    << "warning_bg = \"default\"\n"
                    << "danger_fg = \"red\"\n"
                    << "danger_bg = \"default\"\n"
                    << "popup_header_fg = \"white\"\n"
                    << "popup_header_bg = \"magenta\"\n"
                    << "popup_highlight_fg = \"black\"\n"
                    << "popup_highlight_bg = \"magenta\"\n";
            }
        }
    } catch (...) {}
}
