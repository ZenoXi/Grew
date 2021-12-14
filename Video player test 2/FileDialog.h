#pragma once

#include <string>
#include <thread>

std::wstring OpenFile();
std::wstring SaveFile();

class AsyncFileDialog
{
    std::thread _fileDialogThread;
    bool _done = false;
    std::wstring _path = L"";

public:
    void Open()
    {
        _done = false;
        _path = L"";
        _fileDialogThread = std::thread(&AsyncFileDialog::_ShowDialog, this, static_cast<std::wstring(*)()>(&OpenFile));
        _fileDialogThread.detach();
    }

    void Save()
    {
        _done = false;
        _path = L"";
        _fileDialogThread = std::thread(&AsyncFileDialog::_ShowDialog, this, static_cast<std::wstring(*)()>(&SaveFile));
        _fileDialogThread.detach();
    }

    bool Done() const
    {
        return _done;
    }

    std::wstring Result() const
    {
        return _path;
    }

private:
    void _ShowDialog(std::wstring(*dialogFunc)())
    {
        _path = dialogFunc();
        _done = true;
    }
};