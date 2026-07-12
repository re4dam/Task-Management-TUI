#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

enum class TaskType {
    Activity,
    Assignment
};

enum class RecurrenceRule {
    None,
    Daily,
    Weekly,
    Monthly
};

inline std::string recurrence_to_string(RecurrenceRule rule) {
    switch (rule) {
        case RecurrenceRule::Daily: return "DAILY";
        case RecurrenceRule::Weekly: return "WEEKLY";
        case RecurrenceRule::Monthly: return "MONTHLY";
        default: return "NONE";
    }
}

inline RecurrenceRule string_to_recurrence(const std::string& str) {
    if (str == "DAILY") return RecurrenceRule::Daily;
    if (str == "WEEKLY") return RecurrenceRule::Weekly;
    if (str == "MONTHLY") return RecurrenceRule::Monthly;
    return RecurrenceRule::None;
}

inline std::tm string_to_tm(const std::string& datetime_str, bool& success) {
    std::tm tm = {};
    std::istringstream ss(datetime_str);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
    success = !ss.fail();
    return tm;
}

inline std::string tm_to_string(const std::tm& tm) {
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return ss.str();
}

inline std::string get_relative_time_string(const std::string& datetime_str) {
    if (datetime_str.empty()) return "";
    bool success = false;
    std::tm tm = string_to_tm(datetime_str, success);
    if (!success) return "";
    std::time_t task_time = std::mktime(&tm);
    if (task_time == -1) return "";
    std::time_t now = std::time(nullptr);
    double diff_seconds = std::difftime(task_time, now);
    bool in_future = (diff_seconds >= 0);
    long long diff = std::abs((long long)diff_seconds);
    long long minutes = (diff / 60) % 60;
    long long hours = (diff / 3600) % 24;
    long long days = diff / 86400;

    std::string time_part = "";
    if (days > 0) {
        time_part += std::to_string(days) + (days == 1 ? " day" : " days");
        if (hours > 0) {
            time_part += ", " + std::to_string(hours) + (hours == 1 ? " hour" : " hours");
        }
    } else if (hours > 0) {
        time_part += std::to_string(hours) + (hours == 1 ? " hour" : " hours");
        if (minutes > 0) {
            time_part += ", " + std::to_string(minutes) + (minutes == 1 ? " minute" : " minutes");
        }
    } else {
        time_part += std::to_string(minutes) + (minutes == 1 ? " minute" : " minutes");
    }

    if (in_future) {
        return " (In " + time_part + ")";
    } else {
        return " (Overdue by " + time_part + ")";
    }
}

struct Task {
    std::string title;
    std::string description;
    bool completed = false;
    TaskType type = TaskType::Activity;
    std::string time_value; // Start time for Activity, Deadline for Assignment
    RecurrenceRule recurrence = RecurrenceRule::None;
};

#endif // TASK_HPP
