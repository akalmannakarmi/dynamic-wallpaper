#include "wallpaper.h"

#include <iostream>
#include <fstream>
#include <SDL2/SDL.h>
#include <filesystem>
#include <windows.h>
#include <thread>
#include <cmath>

Wallpaper::Wallpaper(){
    loadConfig();
    setVideo();
    loadImages();
    if(autoStart){
        addToStartup();
    }else{
        removeFromStartup();
    }
}
Wallpaper::~Wallpaper(){}

void Wallpaper::init(){
    loadConfig();
    if (setVideo()){
        loadImages();
    }
    if(autoStart){
        addToStartup();
    }else{
        removeFromStartup();
    }
}
bool Wallpaper::loadConfig() {
    std::string narrowConfigPath(configPath.begin(), configPath.end());
    std::ifstream configFile(narrowConfigPath);
    if (!configFile) {
        std::cout << "Failed to open config file." << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(configFile, line)) {
        // Skip empty lines or lines starting with a '#' (comment)
        if (line.empty() || line[0] == '#')
            continue;

        // Find the position of the '=' sign
        std::size_t delimiterPos = line.find('=');
        if (delimiterPos == std::string::npos)
            continue; // Invalid line format, skip

        // Extract the key and value from the line
        std::string key = line.substr(0, delimiterPos);
        std::string value = line.substr(delimiterPos + 1);

        // Trim whitespace from the key and value
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        // Set the corresponding values based on the key
        if (key == "change") {
            change = std::stoi(value);
        } else if (key == "frameRate") {
            frameRate = std::stoi(value);
            frameDurationMilliseconds = 1000 / frameRate;
        } else if (key == "videoPath") {
            videoPath = std::string(value.begin(), value.end());
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            
            size_t lastSlashIndex = videoPath.find_last_of("\\");
            size_t lastSlashIndex2 = videoPath.find_last_of("/");
            size_t lastDotIndex = videoPath.find_last_of(".");
            std::string path;
            if (lastSlashIndex != std::string::npos) {
                path =videoPath.substr(lastSlashIndex + 1, lastDotIndex - lastSlashIndex - 1);
            }else if(lastSlashIndex2 != std::string::npos){
                path =videoPath.substr(lastSlashIndex2 + 1, lastDotIndex - lastSlashIndex2 - 1);
            }else {
                path = videoPath.substr(0, lastDotIndex);
            }
            imagesPath =  converter.from_bytes(path);
        } else if (key == "autoStart") {
            autoStart = (value == "true");
        }else if (key == "checkConfig") {
            checkConfig = (value == "true");
        } else if (key == "quality") {
            quality = std::stoi(value)%32;
        } else if (key == "maxThreads") {
            maxThreads = std::stoi(value);
        }
    }

    configFile.close();
    return true;
}

bool Wallpaper::addToStartup(){
    // Get the full path to the executable
    wchar_t executablePath_[MAX_PATH];
    GetModuleFileNameW(nullptr, executablePath_, MAX_PATH);
    std::wstring executablePath = executablePath_;
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
bool Wallpaper::removeFromStartup() {
    HKEY hkey;
    LONG result = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hkey);
    if (result != ERROR_SUCCESS) {
        // std::cout << "Failed to open registry key." << std::endl;
        return false;
    }

    result = RegDeleteValueW(hkey, L"dynamicWallpaper");
    if (result != ERROR_SUCCESS) {
        // std::cout << "Failed to delete registry value." << std::endl;
        RegCloseKey(hkey);
        return false;
    }

    RegCloseKey(hkey);
    return true;
}


void Wallpaper::changeWp(int& threads, const std::wstring& imagePath){
    if (SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)imagePath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE) == 0){
        std::cerr << "Failed to change" << std::endl;
    }
    threads--;
}
void Wallpaper::changeWallpaper(){
    int i = 0;
    int threads = 0;
    while (running){
        auto frameStartTime = std::chrono::steady_clock::now();

        // Set the desktop wallpaper
        if (threads < maxThreads){
            threads++;
            std::thread change(&Wallpaper::changeWp, this, std::ref(threads), std::cref(imagePaths[i]));
            change.detach();
        } else {
            std::cerr << "Threads full" << std::endl;
        }

        if (checkConfig && fmod(i/change/(double)frameRate, 10.0) == 0){
            init();
        }
        i += change;
        if (i >= imagePaths.size()){
            i = 0;
        }

        auto frameEndTime = std::chrono::steady_clock::now();
        auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(frameEndTime - frameStartTime).count();

        // Delay to achieve the desired frame rate
        if (frameDuration < frameDurationMilliseconds){
            Sleep(frameDurationMilliseconds - frameDuration);
        }
    }
}


bool Wallpaper::loadImages(){
    imagePaths.clear();
    std::wstring currentDirectory = std::filesystem::current_path();
    std::wstring framesFolderPath = currentDirectory + L"\\" + imagesPath;

    // Iterate over the files in the frames folder
    for (const auto& entry : std::filesystem::directory_iterator(framesFolderPath)){
        if (entry.path().extension() == L".jpg"){
            imagePaths.push_back(entry.path().wstring());
        }
    }
    return true;
}
bool Wallpaper::setVideo(){
    std::string outputDirectory;
    size_t lastSlashIndex = videoPath.find_last_of("\\");
    size_t lastSlashIndex2 = videoPath.find_last_of("/");
    size_t lastDotIndex = videoPath.find_last_of(".");
    std::string path;
    if (lastSlashIndex != std::string::npos) {
        outputDirectory =videoPath.substr(lastSlashIndex + 1, lastDotIndex - lastSlashIndex - 1);
    }else if(lastSlashIndex2 != std::string::npos){
        outputDirectory =videoPath.substr(lastSlashIndex2 + 1, lastDotIndex - lastSlashIndex2 - 1);
    }else {
        outputDirectory = videoPath.substr(0, lastDotIndex);
    }

    if (std::filesystem::is_directory(outputDirectory)) {
        std::cout << "Output directory already exists." << std::endl;
        return false;
    } else {
        std::filesystem::create_directory(outputDirectory);
        std::cout << "Output directory created." << std::endl;
    }

    // Get desktop Width and Height
    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        std::cerr << "Failed to retrieve desktop display mode." << std::endl;
        return 1;
    }
    int desktopWidth = dm.w;
    int desktopHeight = dm.h;
    SDL_Quit();

    std::string command = "ffmpeg -i " + videoPath + " -q:v " + std::to_string(quality) + " -vf \"scale=" + std::to_string(desktopWidth) + ":" + std::to_string(desktopHeight) + ":force_original_aspect_ratio=decrease:flags=lanczos\" " + outputDirectory + "/frame-%09d.jpg";
    // std::string command = "ffmpeg -i " + videoPath + " -q:v 2 " + outputDirectory + "/frame-%09d.jpg";
    
    int result = std::system(command.c_str());

    if (result == 0) {
        std::cout << "Image files created and resized successfully!" << std::endl;
        return true;
    } else {
        std::cerr << "Failed to create or resize image files." << std::endl;
        return false;
    }
}