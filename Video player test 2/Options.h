#pragma once

#include <map>
#include <string>
#include <mutex>

class Options
{
    std::map<std::string, std::string> _options;
    std::mutex _m_options;

private: // Singleton interface
    Options();
    static Options* _instance;
public:
    static void Init();
    static Options* Instance();

private:
    void _LoadFromFile();
    void _SaveToFile();

public:
    std::string GetValue(std::string name);
    void SetValue(std::string name, std::string value);
};