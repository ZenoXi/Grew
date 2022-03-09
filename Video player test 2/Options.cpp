#include "Options.h"

#include "Functions.h"

#include <fstream>

Options* Options::_instance = nullptr;

Options::Options()
{
    std::lock_guard<std::mutex> lock(_m_options);
    _LoadFromFile();
}

void Options::Init()
{
    if (!_instance)
    {
        _instance = new Options();
    }
}

Options* Options::Instance()
{
    return _instance;
}


void Options::_LoadFromFile()
{
    _options.clear();

    std::ifstream fin("options");
    if (!fin)
        return;

    while (!fin.eof())
    {
        std::string line;
        std::getline(fin, line);
        
        std::array<std::string, 2> parts;
        split_str(line, parts, '=');

        if (parts[0].empty())
            continue;

        _options[parts[0]] = parts[1];
    }
}

void Options::_SaveToFile()
{
    std::ofstream fout("options");
    if (!fout)
        return;

    for (auto& option : _options)
    {
        fout << option.first << '=' << option.second << std::endl;
    }
}

std::string Options::GetValue(std::string name)
{
    std::lock_guard<std::mutex> lock(_m_options);
    return _options[name];
}

void Options::SetValue(std::string name, std::string value)
{
    std::lock_guard<std::mutex> lock(_m_options);
    _options[name] = value;
    _SaveToFile();
}