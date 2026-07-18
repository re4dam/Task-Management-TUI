#ifndef PERSISTENCE_HPP
#define PERSISTENCE_HPP

#include <vector>
#include <string>
#include "core/list.hpp"
#include "core/theme.hpp"

class Keymap;

class IPersistence {
public:
    virtual ~IPersistence() = default;
    
    virtual std::vector<List> load_tasks() = 0;
    virtual bool save_tasks(const std::vector<List>& lists) = 0;

    virtual bool load_config(Keymap& keymap, std::string& active_theme) = 0;
    virtual bool save_config(const Keymap& keymap, const std::string& active_theme) = 0;

    virtual bool load_theme(const std::string& theme_name, ThemeColors& theme) = 0;
    virtual void generate_sample_theme_if_missing() = 0;
    
    virtual std::string get_config_path() = 0;
};

class FilePersistence : public IPersistence {
private:
    std::string tasks_filepath;
    
    std::string get_config_dir();
    void ensure_directory_exists(const std::string& filepath);
    std::string escape(const std::string& str);
    std::string unescape(const std::string& str);

public:
    FilePersistence();
    
    std::vector<List> load_tasks() override;
    bool save_tasks(const std::vector<List>& lists) override;

    bool load_config(Keymap& keymap, std::string& active_theme) override;
    bool save_config(const Keymap& keymap, const std::string& active_theme) override;

    bool load_theme(const std::string& theme_name, ThemeColors& theme) override;
    void generate_sample_theme_if_missing() override;
    
    std::string get_config_path() override;
};

#endif // PERSISTENCE_HPP
