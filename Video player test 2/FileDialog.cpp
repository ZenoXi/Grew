#include "FileDialog.h"

#include "ChiliWin.h"
#include <shobjidl.h>

std::vector<std::wstring> OpenFiles()
{
    std::vector<std::wstring> result;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
            IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            // Allow multiple file open
            FILEOPENDIALOGOPTIONS opt;
            pFileOpen->GetOptions(&opt);
            opt |= FOS_ALLOWMULTISELECT;
            hr = pFileOpen->SetOptions(opt);

            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box (by creating a stack of ugly 'if (SUCCEEDED(hr))' thanks winapi)
            if (SUCCEEDED(hr))
            {
                IShellItemArray* pItems;
                hr = pFileOpen->GetResults(&pItems);
                if (SUCCEEDED(hr))
                {
                    DWORD count;
                    hr = pItems->GetCount(&count);
                    if (SUCCEEDED(hr))
                    {
                        for (DWORD i = 0; i < count; i++)
                        {
                            IShellItem* pItem;
                            hr = pItems->GetItemAt(i, &pItem);
                            if (SUCCEEDED(hr))
                            {
                                PWSTR pszFilePath;
                                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                if (SUCCEEDED(hr))
                                {
                                    result.push_back(pszFilePath);
                                    CoTaskMemFree(pszFilePath);
                                }
                                pItem->Release();
                            }
                        }
                    }
                    pItems->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }

    return result;
}

std::vector<std::wstring> SaveFile()
{
    std::wstring result;

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileSaveDialog* pFileSave;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

        if (SUCCEEDED(hr))
        {
            // Show the Open dialog box.
            hr = pFileSave->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileSave->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr))
                    {
                        result = pszFilePath;
                        CoTaskMemFree(pszFilePath);
                    }
                    else
                    {
                        //std::cout << "[Err]Failed extract path" << std::endl;
                    }
                    pItem->Release();
                }
                else
                {
                    //std::cout << "[Err]Failed to get result" << std::endl;
                }
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
            {
                //std::cout << "[Err]Selection canceled" << std::endl;
            }
            else
            {
                //std::cout << "[Err]Unknown" << std::endl;
            }
            pFileSave->Release();
        }
        CoUninitialize();
    }

    return { result };
}