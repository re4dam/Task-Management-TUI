#ifndef STORAGE_HPP
#define STORAGE_HPP

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>
#include "list.hpp"

namespace Storage {

inline std::string get_data_filepath() {
    std::string path;
    const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config != '\0') {
        path = std::string(xdg_config) + "/task-tui/tasks.txt";
    } else {
        const char* home = std::getenv("HOME");
        if (home) {
            path = std::string(home) + "/.config/task-tui/tasks.txt";
        } else {
            path = ".config/task-tui/tasks.txt";
        }
    }
    return path;
}

inline void ensure_directory_exists(const std::string& filepath) {
    std::filesystem::path fp(filepath);
    std::filesystem::path parent = fp.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

inline std::string escape(const std::string& str) {
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

inline std::string unescape(const std::string& str) {
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

inline bool save_data(const std::vector<List>& lists, const std::string& filename) {
    ensure_directory_exists(filename);
    std::ofstream out(filename);
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

inline std::vector<List> load_data(const std::string& filename) {
    std::vector<List> lists;
    std::ifstream in(filename);
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

} // namespace Storage

#endif // STORAGE_HPP
