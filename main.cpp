// #include <iostream>
#include <fstream>
#include <windows.h>
#include <chrono>
#include <vector>
#include <filesystem>
#include <algorithm>


bool AddToStartup(){
    // Get the full path to the executable
    wchar_t executablePath_[MAX_PATH];
    GetModuleFileNameW(nullptr, executablePath_, MAX_PATH);
    std::wstring executablePath = executablePath_;
    // Extract the directory path from the executable path
    std::wstring executableDirectory = std::filesystem::path(executablePath).parent_path();

    // Set the current working directory to the executable directory
    if (!SetCurrentDirectoryW(executableDirectory.c_str()))
    {
        // std::cout << "Failed to set the current working directory." << std::endl;
        return false;
    }

    HKEY hkey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey);
    if (result != ERROR_SUCCESS)
    {
        // std::cout << "Failed to open registry key." << std::endl;
        return false;
    }

    result = RegSetValueExW(hkey, L"dynamicWallpaper", 0, REG_SZ, reinterpret_cast<const BYTE*>(executablePath.c_str()), (executablePath.size() + 1) * sizeof(wchar_t));
    if (result != ERROR_SUCCESS)
    {
        // std::cout << "Failed to set registry value." << std::endl;
        RegCloseKey(hkey);
        return false;
    }

    RegCloseKey(hkey);
    return true;
}

bool LoadConfig(const std::wstring& configPath, int& change, int& frameRate, std::wstring& imagePath){
    std::string narrowConfigPath(configPath.begin(), configPath.end());
    std::ifstream configFile(narrowConfigPath);
    if (!configFile)
    {
        // std::cout << "Failed to open config file." << std::endl;
        return false;
    }

    configFile >> change >> frameRate;

    std::string narrowImagePath;
    std::getline(configFile >> std::ws, narrowImagePath);
    imagePath = std::wstring(narrowImagePath.begin(), narrowImagePath.end());

    configFile.close();
    return true;
}



// int main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Add the executable to startup
    if (!AddToStartup())
    {
        // std::cout << "Failed to add to startup." << std::endl;
        return -1;
    }

    // Load configuration from file
    int change = 1;
    int frameRate = 5;
    std::wstring imagePath;

    if (!LoadConfig(L"config.txt", change, frameRate, imagePath))
    {
        // std::cout << "Failed to load configuration." << std::endl;
        return -1;
    }

    size_t i = 0;
    const int frameDurationMilliseconds = 1000 / frameRate;

    std::vector<std::wstring> imagePaths;

    std::wstring currentDirectory = std::filesystem::current_path();
    std::wstring framesFolderPath = currentDirectory + L"\\" + imagePath;

    // Iterate over the files in the frames folder
    for (const auto& entry : std::filesystem::directory_iterator(framesFolderPath))
    {
        if (entry.path().extension() == L".jpg")
        {
            imagePaths.push_back(entry.path().wstring());
        }
    }

    // Sort the image paths numerically
    std::sort(imagePaths.begin(), imagePaths.end(), [](const std::wstring& a, const std::wstring& b) {
        return std::stoi(a.substr(a.find_last_of(L"-") + 1, a.find_last_of(L".") - a.find_last_of(L"-") - 1)) <
               std::stoi(b.substr(b.find_last_of(L"-") + 1, b.find_last_of(L".") - b.find_last_of(L"-") - 1));
    });

    while (true)
    {
        auto frameStartTime = std::chrono::steady_clock::now();

        // Set the desktop wallpaper
        if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)imagePaths[i].c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) == 0)
        {
            // Error handling
            return -1;
        }

        i += change;
        if (i >= imagePaths.size())
        {
            i = 0;
        }

        auto frameEndTime = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - frameStartTime).count();

        // Delay to achieve the desired frame rate
        if (frameDuration < frameDurationMilliseconds)
        {
            Sleep(frameDurationMilliseconds - frameDuration);
        }
    }

    return 0;
}