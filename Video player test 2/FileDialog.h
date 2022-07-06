#pragma once

#include <string>
#include <vector>
#include <thread>

struct FileDialogOptions
{
    // Empty vector allows everything
    // First pair item is the group name
    // Second pair item are the extensions belonging to the group
    // Refer to https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifiledialog-setfiletypes
    std::vector<std::pair<std::wstring, std::wstring>> allowedExtensions;
    // File dialog will be used to select a folder
    bool openFolders = false;
};

std::vector<std::wstring> OpenFiles(FileDialogOptions);
std::vector<std::wstring> SaveFile(FileDialogOptions);

class AsyncFileDialog
{
    std::thread _fileDialogThread;
    bool _done = false;
    std::vector<std::wstring> _paths;

public:
    void Open(FileDialogOptions options = FileDialogOptions{})
    {
        _done = false;
        _paths.clear();
        _fileDialogThread = std::thread(&AsyncFileDialog::_ShowDialog, this, static_cast<std::vector<std::wstring>(*)(FileDialogOptions)>(&OpenFiles), options);
        _fileDialogThread.detach();
    }

    void Save(FileDialogOptions options = FileDialogOptions{})
    {
        _done = false;
        _paths.clear();
        _fileDialogThread = std::thread(&AsyncFileDialog::_ShowDialog, this, static_cast<std::vector<std::wstring>(*)(FileDialogOptions)>(&SaveFile), options);
        _fileDialogThread.detach();
    }

    bool Done() const
    {
        return _done;
    }

    std::wstring Result() const
    {
        return _paths.empty() ? L"" : _paths.front();
    }

    std::vector<std::wstring> ParsedResult() const
    {
        return _paths;

        //std::vector<std::wstring> filepaths;

        //size_t index = 0;
        //while (index < _path.length())
        //{
        //    size_t indices[2]{ -1, -1 };
        //    size_t count = 0;

        //    // Find 2 separate double quote marks
        //    while (index < _path.length())
        //    {
        //        if (_path[index] != L'"')
        //            continue;

        //        indices[count++] = index;
        //        index++;

        //        if (count == 2)
        //            break;
        //    }

        //    if (count != 2)
        //        break;

        //    // Add substring to file path list
        //    size_t length = indices[1] - indices[0] - 1;
        //    filepaths.push_back(_path.substr(indices[0] + 1, length));
        //}

        //return filepaths;
    }

private:
    void _ShowDialog(std::vector<std::wstring>(*dialogFunc)(FileDialogOptions), FileDialogOptions options)
    {
        _paths = dialogFunc(options);
        _done = true;
    }
};