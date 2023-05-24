#include "wallpaper.h"
#include <windows.h>

// int main()
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    HANDLE hMutex = CreateMutex(NULL, TRUE, "dynamicWallpaperSC");

    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        // An instance of the program is already running
        CloseHandle(hMutex);
        return 0;
    }

    Wallpaper wp;
    wp.changeWallpaper();

    CloseHandle(hMutex);
    return 0;
}
