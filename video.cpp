#include <iostream>
#include <cstdlib>

int main() {
    std::string videoPath = "video.mp4";  // Replace "input_video.mp4" with the path to your video file
    std::string outputDirectory = "frames/";  // Replace "frames/" with the path to the output directory

    std::string command = "ffmpeg -i " + videoPath + " " + outputDirectory + "frame-%d.jpg";
    int result = std::system(command.c_str());

    if (result == 0) {
        std::cout << "Image files created successfully!" << std::endl;
    } else {
        std::cerr << "Failed to create image files." << std::endl;
    }

    return 0;
}
