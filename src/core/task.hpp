#ifndef TASK_HPP
#define TASK_HPP

#include <string>
#include <ctime>

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

std::string recurrence_to_string(RecurrenceRule rule);
RecurrenceRule string_to_recurrence(const std::string& str);
std::tm string_to_tm(const std::string& datetime_str, bool& success);
std::string tm_to_string(const std::tm& tm);
std::string get_relative_time_string(const std::string& datetime_str);

struct Task {
    std::string title;
    std::string description;
    bool completed = false;
    TaskType type = TaskType::Activity;
    std::string time_value; // Start time for Activity, Deadline for Assignment
    RecurrenceRule recurrence = RecurrenceRule::None;
};

#endif // TASK_HPP
