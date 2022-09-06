#pragma once

#include <map>
#include <string>
#include <mutex>

class Options
{
    std::map<std::wstring, std::wstring> _options;
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
    std::wstring GetValue(std::wstring name);
    void SetValue(std::wstring name, std::wstring value);
};