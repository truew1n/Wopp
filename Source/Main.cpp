#include <iostream>
#include <windows.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "AudioController.hpp"

// Vertex shader source code
const char* vertexShaderSource = R"(
    #version 460 core
    layout(location = 0) in vec3 aPos;
    void main()
    {
        gl_Position = vec4(aPos, 1.0);
    }
)";

// Fragment shader source code
const char* fragmentShaderSource = R"(
    #version 460 core
    out vec4 FragColor;
    void main()
    {
        FragColor = vec4(0.0, 1.0, 0.0, 1.0);
    }
)";

void ResizeCallback(GLFWwindow *Window, int32_t FrameBufferWidth, int32_t FrameBufferHeight)
{
    glViewport(0, 0, FrameBufferWidth, FrameBufferHeight);
}

void PeriodicTimerCallback(void *Parameter)
{
    uint64_t *CParameter = (uint64_t *)Parameter;
    *CParameter += 100;
}

void renderWaveform(AudioController& controller, GLuint VAO, GLuint VBO, GLuint shaderProgram, uint64_t currentMillisecond)
{
    wave_t* wave = controller.GetWaveData();

    if (wave->data_chunk.data == nullptr || wave->data_chunk.subchunk_size == 0) {
        return;
    }

    int16_t* samples = (int16_t*)wave->data_chunk.data;
    size_t sampleCount = wave->data_chunk.subchunk_size / (wave->fmt_chunk.bits_per_sample / 8);
    int channels = wave->fmt_chunk.num_channels;

    int64_t startSample = currentMillisecond * wave->fmt_chunk.sample_rate / 1000;
    int64_t endSample = (currentMillisecond + 100) * wave->fmt_chunk.sample_rate / 1000; // 100 milliseconds

    if (startSample >= sampleCount || endSample > sampleCount || startSample >= endSample) {
        std::cerr << "Invalid sample range." << std::endl;
        return;
    }

    std::vector<float> vertices((endSample - startSample) * channels * 6); // Preallocate memory

    for (int64_t i = startSample, idx = 0; i < endSample; i++) {
        for (int ch = 0; ch < channels; ++ch, idx += 6) {
            float x = (float)(i - startSample) / (endSample - startSample) * 2.0f - 1.0f;
            float yOffset = 2.0f / channels * ch - 1.0f + 1.0f / channels;
            float y = (samples[i * channels + ch] / 32768.0f) + yOffset;

            vertices[idx] = x;
            vertices[idx + 1] = y;
            vertices[idx + 2] = 0.0f;
            vertices[idx + 3] = x;
            vertices[idx + 4] = yOffset;
            vertices[idx + 5] = 0.0f;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, vertices.size() / 3);
}

GLuint CompileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint CreateShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

int main(void)
{
    if (!glfwInit())
    {
        return -1;
    }

    uint32_t Width = 1920;
    uint32_t Height = 1080;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string WindowTitle = "Wopp - Wave Audio Player";

    GLFWwindow *Window = glfwCreateWindow(Width, Height, WindowTitle.c_str(), NULL, NULL);
    if (!Window)
    {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(Window, ResizeCallback);
    glfwMakeContextCurrent(Window);
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "Glew failed to initialize!\n");
        return -1;
    }

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    GLuint shaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    AudioController Controller;

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind;

    LPCWSTR searchPath = L"Assets\\Audio\\*.wav";
    hFind = FindFirstFileW(searchPath, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        std::wcerr << L"Error finding files in directory" << std::endl;
        return 1;
    }

    do
    {
        Controller.Add(L"Assets\\Audio\\" + std::wstring(findFileData.cFileName));
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);

    uint64_t CurrentMillisecond = 0;
    TimerParam Parameter = {PeriodicTimerCallback, &CurrentMillisecond};
    Timer AudioTimer(100U, 0U, &Parameter, TimerType::PERIODIC);
    Controller.BindTimer(AudioTimer, true);
    Controller.Start();

    glEnable(GL_DEPTH_TEST);

    double CurrentTime = 0.0;
    double PreviouseTime = 0.0;
    double TimeDifference = 0.0;
    uint32_t FrameCounter = 0;

    std::string FPS;
    std::string MS;
    std::string NewWindowTitle;

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    while (!glfwWindowShouldClose(Window))
    {
        CurrentTime = glfwGetTime();
        TimeDifference = CurrentTime - PreviouseTime;
        FrameCounter++;
        if (TimeDifference >= 1.0 / 30.0)
        {
            FPS = std::to_string((1.0 / TimeDifference) * FrameCounter);
            MS = std::to_string((TimeDifference / FrameCounter) * 1000.0);
            NewWindowTitle = WindowTitle + " - " + FPS + "FPS / " + MS + "ms";
            glfwSetWindowTitle(Window, NewWindowTitle.c_str());
            PreviouseTime = CurrentTime;
            FrameCounter = 0;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(Controller.GetAudioStreamState() == EAudioStreamState::PLAYING) {
            renderWaveform(Controller, VAO, VBO, shaderProgram, CurrentMillisecond);
        } else {
            CurrentMillisecond = 0U;
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(Window);
    glfwTerminate();

    Controller.Free();
    return 0;
}