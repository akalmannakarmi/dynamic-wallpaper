#ifndef WALLPAPER_H
#define WALLPAPER_H

#include <vector>
#include <string>
class Wallpaper{
    private:
    bool running = true;
    bool autoStart = true;
    bool checkConfig = false;
    int change = 1;
    int frameRate = 5;
    int quality = 5;
    std::string videoPath;
    std::wstring imagesPath;
    int frameDurationMilliseconds = 1000 / frameRate;
    const std::wstring configPath = L"config.txt";
    std::vector<std::wstring> imagePaths;

    public:
    Wallpaper();
    ~Wallpaper();

    void init();
    bool addToStartup();
    bool removeFromStartup();
    bool loadConfig();
    bool loadImages();
    void changeWp(int &threads,const std::wstring &imagePath);
    void changeWallpaper();
    bool setVideo();
};

#endif