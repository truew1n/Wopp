#include <iostream>
#include <windows.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "AudioController.hpp"
#include <vector>
#include <algorithm>
#include <cmath>

#define PERIODIC_TIMER 50U

// Vertex shader Source code
const char* vertexShaderSource = R"(
    #version 460 core
    layout(location = 0) in vec3 aPos;
    void main()
    {
        gl_Position = vec4(aPos, 1.0);
    }
)";

// Fragment shader Source code
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
    *CParameter += PERIODIC_TIMER;
}

void RenderWaveform(AudioController& Controller, GLuint VAO, GLuint VBO, GLuint ShaderProgram, uint64_t CurrentMillisecond)
{
    wave_t* Wave = Controller.GetWaveData();

    if (Wave->data_chunk.data == nullptr || Wave->data_chunk.subchunk_size == 0) {
        return;
    }

    int16_t* Samples = (int16_t*)Wave->data_chunk.data;
    size_t SampleCount = Wave->data_chunk.subchunk_size / (Wave->fmt_chunk.bits_per_sample / 8);
    int Channels = Wave->fmt_chunk.num_channels;

    int64_t StartSample = CurrentMillisecond * Wave->fmt_chunk.sample_rate / 1000;
    int64_t EndSample = (CurrentMillisecond + PERIODIC_TIMER) * Wave->fmt_chunk.sample_rate / 1000;

    if(StartSample >= SampleCount || EndSample > SampleCount || StartSample >= EndSample) {
        std::cerr << "Invalid sample range." << std::endl;
        return;
    }

    std::vector<float> Vertices((EndSample - StartSample) * Channels * 6);

    for(int64_t i = StartSample, idx = 0; i < EndSample; ++i) {
        for(int ch = 0; ch < Channels; ++ch, idx += 6) {
            float X = (float)(i - StartSample) / (EndSample - StartSample) * 2.0f - 1.0f;
            float YOffset = 2.0f / Channels * ch - 1.0f + 1.0f / Channels;
            float Y = (Samples[i * Channels + ch] / 32768.0f) + YOffset;

            Vertices[idx] = X;
            Vertices[idx + 1] = Y;
            Vertices[idx + 2] = 0.0f;

            Vertices[idx + 3] = X;
            Vertices[idx + 4] = YOffset;
            Vertices[idx + 5] = 0.0f;
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(float), Vertices.data(), GL_DYNAMIC_DRAW);

    glUseProgram(ShaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, Vertices.size() / 3);
}

void RenderEqualizer(AudioController& Controller, GLuint VAO, GLuint VBO, GLuint ShaderProgram, uint64_t CurrentMillisecond)
{
    wave_t* Wave = Controller.GetWaveData();

    if (Wave->data_chunk.data == nullptr || Wave->data_chunk.subchunk_size == 0) {
        return;
    }

    int16_t* Samples = (int16_t*)Wave->data_chunk.data;
    size_t SampleCount = Wave->data_chunk.subchunk_size / (Wave->fmt_chunk.bits_per_sample / 8);
    int Channels = Wave->fmt_chunk.num_channels;

    int64_t StartSample = CurrentMillisecond * Wave->fmt_chunk.sample_rate / 1000;
    int64_t EndSample = (CurrentMillisecond + PERIODIC_TIMER) * Wave->fmt_chunk.sample_rate / 1000;

    if(StartSample >= SampleCount || EndSample > SampleCount || StartSample >= EndSample) {
        std::cerr << "Invalid sample range." << std::endl;
        return;
    }

    int BarCount = 200;
    int FrequencyRange = 65536 / BarCount;

    std::vector<float> Vertices(BarCount * 6 * 6);
    std::vector<int> FrequencyCounts(BarCount, 0);

    for (int64_t i = StartSample; i < EndSample; i++) {
        for (int ch = 0; ch < Channels; ++ch) {
            int16_t Sample = Samples[i * Channels + ch];
            int FrequencyIndex = (Sample + 32768) / FrequencyRange;
            if (FrequencyIndex >= 0 && FrequencyIndex < BarCount) {
                FrequencyCounts[FrequencyIndex]++;
            }
        }
    }

    int MaxCount = *std::max_element(FrequencyCounts.begin(), FrequencyCounts.end());

    for (int Bar = 0; Bar < BarCount; ++Bar) {
        float X = (float)Bar / BarCount * 2.0f - 1.0f;
        float Width = 2.0f / BarCount * 0.8f; // Bar width (80% of allocated space)
        float Height = (float)FrequencyCounts[Bar] / MaxCount * 2.0f;

        int idx = Bar * 6 * 6;

        Vertices[idx]     = X;
        Vertices[idx + 1] = -1.0f;
        Vertices[idx + 2] = 0.0f;

        Vertices[idx + 3] = X + Width;
        Vertices[idx + 4] = -1.0f;
        Vertices[idx + 5] = 0.0f;

        Vertices[idx + 6] = X + Width;
        Vertices[idx + 7] = -1.0f + Height;
        Vertices[idx + 8] = 0.0f;

        Vertices[idx + 9] = X;
        Vertices[idx + 10] = -1.0f;
        Vertices[idx + 11] = 0.0f;

        Vertices[idx + 12] = X + Width;
        Vertices[idx + 13] = -1.0f + Height;
        Vertices[idx + 14] = 0.0f;

        Vertices[idx + 15] = X;
        Vertices[idx + 16] = -1.0f + Height;
        Vertices[idx + 17] = 0.0f;
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(float), Vertices.data(), GL_DYNAMIC_DRAW);

    glUseProgram(ShaderProgram);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, Vertices.size() / 3);
}

GLuint CompileShader(GLenum Type, const char* Source)
{
    GLuint shader = glCreateShader(Type);
    glShaderSource(shader, 1, &Source, NULL);
    glCompileShader(shader);

    int Success;
    char InfoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &Success);
    if(!Success) {
        glGetShaderInfoLog(shader, 512, NULL, InfoLog);
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << InfoLog << std::endl;
    }

    return shader;
}

GLuint CreateShaderProgram(const char* VertexSource, const char* FragmentSource)
{
    GLuint VertexShader = CompileShader(GL_VERTEX_SHADER, VertexSource);
    GLuint FragmentShader = CompileShader(GL_FRAGMENT_SHADER, FragmentSource);

    GLuint ShaderProgram = glCreateProgram();
    glAttachShader(ShaderProgram, VertexShader);
    glAttachShader(ShaderProgram, FragmentShader);
    glLinkProgram(ShaderProgram);

    int Success;
    char InfoLog[512];
    glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
    if(!Success) {
        glGetProgramInfoLog(ShaderProgram, 512, NULL, InfoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << InfoLog << std::endl;
    }

    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);

    return ShaderProgram;
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
    glfwSwapInterval(0);

    if(glewInit() != GLEW_OK) {
        fprintf(stderr, "Glew failed to initialize!\n");
        return -1;
    }

    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));

    GLuint ShaderProgram = CreateShaderProgram(vertexShaderSource, fragmentShaderSource);

    AudioController Controller;

    WIN32_FIND_DATAW findFileData;
    HANDLE hFind;

    LPCWSTR searchPath = L"Assets\\Audio\\*.wav";
    hFind = FindFirstFileW(searchPath, &findFileData);
    if(hFind == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error finding files in directory" << std::endl;
        return 1;
    }

    do {
        Controller.Add(L"Assets\\Audio\\" + std::wstring(findFileData.cFileName));
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);

    uint64_t CurrentMillisecond = 0;
    TimerParam Parameter = {PeriodicTimerCallback, &CurrentMillisecond};
    Timer AudioTimer(PERIODIC_TIMER, 0U, &Parameter, TimerType::PERIODIC);
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

    while(!glfwWindowShouldClose(Window)) {
        CurrentTime = glfwGetTime();
        TimeDifference = CurrentTime - PreviouseTime;
        FrameCounter++;
        if (TimeDifference >= 1.0 / 30.0) {
            FPS = std::to_string((1.0 / TimeDifference) * FrameCounter);
            MS = std::to_string((TimeDifference / FrameCounter) * 1000.0);
            NewWindowTitle = WindowTitle + " - " + FPS + "FPS / " + MS + "ms";
            glfwSetWindowTitle(Window, NewWindowTitle.c_str());
            PreviouseTime = CurrentTime;
            FrameCounter = 0;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(Controller.GetAudioStreamState() == EAudioStreamState::PLAYING) {
            // RenderWaveform(Controller, VAO, VBO, ShaderProgram, CurrentMillisecond);
            RenderEqualizer(Controller, VAO, VBO, ShaderProgram, CurrentMillisecond);
        } else {
            CurrentMillisecond = 0U;
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(ShaderProgram);

    glfwDestroyWindow(Window);
    glfwTerminate();

    Controller.Free();
    return 0;
}