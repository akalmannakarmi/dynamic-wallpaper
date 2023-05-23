#define SDL_MAIN_HANDLED
#include <iostream>
#include <cstdlib>
#include <SDL2/SDL.h>  // Make sure to have SDL2 library installed

int main() {
    std::string videoPath = "video.mp4";  // Replace "input_video.mp4" with the path to your video file
    std::string outputDirectory = "frames/";  // Replace "frames/" with the path to the output directory

    SDL_Init(SDL_INIT_VIDEO);
    SDL_DisplayMode dm;
    if (SDL_GetDesktopDisplayMode(0, &dm) != 0) {
        std::cerr << "Failed to retrieve desktop display mode." << std::endl;
        return 1;
    }

    int desktopWidth = dm.w;
    int desktopHeight = dm.h;

    SDL_Quit();

    std::string command = "ffmpeg -i " + videoPath + " -vf \"scale=" + std::to_string(desktopWidth) + ":" + std::to_string(desktopHeight) + "\" " + outputDirectory + "frame-%d.jpg";
    // std::string command = "ffmpeg -i " + videoPath + " " + outputDirectory + "frame-%d.jpg";
    
    int result = std::system(command.c_str());

    if (result == 0) {
        std::cout << "Image files created and resized successfully!" << std::endl;
    } else {
        std::cerr << "Failed to create or resize image files." << std::endl;
    }

    return 0;
}
