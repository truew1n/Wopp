#include <iostream>
#include <windows.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "AudioController.hpp"


void ResizeCallback(GLFWwindow *Window, int32_t FrameBufferWidth, int32_t FrameBufferHeight)
{
    glViewport(0, 0, FrameBufferWidth, FrameBufferHeight);
}

int main(void)
{
    if(!glfwInit()) {
        return -1;
    }

    uint32_t Width = 1920;
    uint32_t Height = 1080;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    std::string WindowTitle = "Wopp - Wave Audio Player";

    GLFWwindow *Window = glfwCreateWindow(Width, Height, WindowTitle.c_str(), NULL, NULL);
    if(!Window) {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(Window, ResizeCallback);
    
    glfwMakeContextCurrent(Window);

    glfwSwapInterval(1);
    if(glewInit() != GLEW_OK) {
        fprintf(stderr, "Glew failed to initialize!\n");
        return -1;
    }

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glFrontFace(GL_CCW);

    double CurrentTime = 0.0;
    double PreviouseTime = 0.0;
    double TimeDifference = 0.0;
    uint32_t FrameCounter = 0;

    std::string FPS;
    std::string MS;
    std::string NewWindowTitle;

    AudioController Controller = AudioController();

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind;

    LPCWSTR searchPath = L"Assets\\Audio\\*.wav";
    hFind = FindFirstFileW(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error finding files in directory" << std::endl;
        return 1;
    }

    do {
        Controller.Add(L"Assets\\Audio\\" + std::wstring(findFileData.cFileName));
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);

    Controller.Start();

    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    while(!glfwWindowShouldClose(Window)) {
        CurrentTime = glfwGetTime();
        TimeDifference = CurrentTime - PreviouseTime;
        FrameCounter++;
        if(TimeDifference >= 1.0 / 30.0) {
            FPS = std::to_string((1.0 / TimeDifference) * FrameCounter);
            MS = std::to_string((TimeDifference / FrameCounter) * 1000.0);
            NewWindowTitle = WindowTitle + " - " + FPS + "FPS / " + MS + "ms";
            glfwSetWindowTitle(Window, NewWindowTitle.c_str());
            PreviouseTime = CurrentTime;
            FrameCounter = 0;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        
        glfwSwapBuffers(Window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(Window);
    glfwTerminate();

    Controller.Free();
    return 0;
}