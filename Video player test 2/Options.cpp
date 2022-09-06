#include "Options.h"

#include "Functions.h"

#include <ShlObj.h>
#include <fstream>
#include <filesystem>

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

    // Get %appdata% folder
    wchar_t* path_c;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_c) != S_OK) {
        CoTaskMemFree(path_c);
        return;
    }
    std::filesystem::path path(path_c);
    CoTaskMemFree(path_c);

    path /= L"Grew";
    path /= L"options";
    std::wifstream fin(path.string());
    if (!fin)
        return;

    while (!fin.eof())
    {
        std::wstring line;
        std::getline(fin, line);
        
        std::array<std::wstring, 2> parts;
        split_wstr(line, parts, '=');

        if (parts[0].empty())
            continue;

        _options[parts[0]] = parts[1];
    }
}

void Options::_SaveToFile()
{
    // Get %appdata% folder
    wchar_t* path_c;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path_c) != S_OK) {
        CoTaskMemFree(path_c);
        return;
    }
    std::filesystem::path path(path_c);
    CoTaskMemFree(path_c);

    path /= L"Grew";
    path /= L"options";
    std::wofstream fout(path.string());
    if (!fout)
        return;

    for (auto& option : _options)
    {
        fout << option.first << '=' << option.second << std::endl;
    }
}

std::wstring Options::GetValue(std::wstring name)
{
    std::lock_guard<std::mutex> lock(_m_options);
    return _options[name];
}

void Options::SetValue(std::wstring name, std::wstring value)
{
    std::lock_guard<std::mutex> lock(_m_options);
    _options[name] = value;
    _SaveToFile();
}