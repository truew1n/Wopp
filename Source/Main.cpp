#include <iostream>
#include <windows.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "AudioController.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
#include <chrono>

#define PERIODIC_TIMER 50U

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertex shader Source code
const char* vertexShaderSource = R"(
    #version 460 core
    layout(location = 0) in vec3 aPos;
    layout(location = 1) in vec3 aNormal;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    out vec3 FragPos;
    out vec3 Normal;

    void main()
    {
        FragPos = vec3(model * vec4(aPos, 1.0));
        Normal = mat3(transpose(inverse(model))) * aNormal;  
        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

// Fragment shader Source code
const char* fragmentShaderSource = R"(
    #version 460 core
    out vec4 FragColor;

    in vec3 FragPos;
    in vec3 Normal;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;

    void main()
    {
        // Ambient
        float ambientStrength = 0.1;
        vec3 ambient = ambientStrength * lightColor;

        // Diffuse
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        // Specular
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;  

        vec3 result = (ambient + diffuse + specular) * objectColor;
        FragColor = vec4(result, 1.0);
    }
)";

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

void GenerateSphere(float radius, int sectorCount, int stackCount, 
                    std::vector<float>& vertices, std::vector<float>& normals, 
                    std::vector<unsigned int>& indices) {
    float x, y, z, xy;                              // vertex position
    float nx, ny, nz, lengthInv = 1.0f / radius;    // normal
    float s, t;                                     // texCoord

    float sectorStep = 2 * M_PI / sectorCount;
    float stackStep = M_PI / stackCount;
    float sectorAngle, stackAngle;

    for (int i = 0; i <= stackCount; ++i) {
        stackAngle = M_PI / 2 - i * stackStep;       // starting from pi/2 to -pi/2
        xy = radius * cosf(stackAngle);              // r * cos(u)
        z = radius * sinf(stackAngle);               // r * sin(u)

        for (int j = 0; j <= sectorCount; ++j) {
            sectorAngle = j * sectorStep;             // starting from 0 to 2pi

            x = xy * cosf(sectorAngle);               // x = r * cos(u) * cos(v)
            y = xy * sinf(sectorAngle);               // y = r * cos(u) * sin(v)

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            nx = x * lengthInv;
            ny = y * lengthInv;
            nz = z * lengthInv;
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);

            s = (float)j / sectorCount;
            t = (float)i / stackCount;
        }
    }

    unsigned int k1, k2;
    for (int i = 0; i < stackCount; ++i) {
        k1 = i * (sectorCount + 1);     // beginning of current stack
        k2 = k1 + sectorCount + 1;      // beginning of next stack

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != (stackCount - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void RenderSphere(AudioController& Controller, GLuint VAO, GLuint VBO, GLuint EBO, GLuint ShaderProgram, uint64_t CurrentMillisecond, std::vector<unsigned int> SphereIndices, uint32_t Width, uint32_t Height, float deltaTime) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(ShaderProgram);

    // Constants
    const int SphereCount = 40; // Number of spheres
    const float SPHERE_RADIUS = 1.0f; // Adjust sphere radius based on your needs
    const float SPACING = 2.0f * SPHERE_RADIUS; // Adjust spacing to be proportional to sphere size
    const float MAX_HEIGHT = 25.0f; // Maximum height of spheres

    // Calculate total width and adjust camera position
    float totalWidth = (SphereCount - 1) * SPACING; // Total width considering spacing between spheres
    float fieldOfView = glm::radians(90.0f); // Field of view in radians
    float aspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    float cameraDistance = (totalWidth / 2.0f) / tan(fieldOfView / 2.0f);

    // Adjust camera position to fit all spheres
    glm::vec3 cameraPosition(0.0f, 0.0f, cameraDistance);
    glm::mat4 view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Adjust the projection matrix
    glm::mat4 projection = glm::perspective(fieldOfView, aspectRatio, 0.1f, 100.0f);

    GLint viewLoc = glGetUniformLocation(ShaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    GLint projLoc = glGetUniformLocation(ShaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    GLint lightPosLoc = glGetUniformLocation(ShaderProgram, "lightPos");
    glUniform3f(lightPosLoc, 1.2f, 1.0f, 2.0f);

    GLint viewPosLoc = glGetUniformLocation(ShaderProgram, "viewPos");
    glUniform3f(viewPosLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    GLint lightColorLoc = glGetUniformLocation(ShaderProgram, "lightColor");
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

    GLint objectColorLoc = glGetUniformLocation(ShaderProgram, "objectColor");
    glUniform3f(objectColorLoc, 0.3f, 0.7f, 0.9f);

    glBindVertexArray(VAO);

    // Get audio data
    wave_t* Wave = Controller.GetWaveData();
    if (Wave->data_chunk.data == nullptr || Wave->data_chunk.subchunk_size == 0) {
        std::cerr << "No audio data!" << std::endl;
        return;
    }

    int16_t* Samples = (int16_t*)Wave->data_chunk.data;
    size_t SampleCount = Wave->data_chunk.subchunk_size / (Wave->fmt_chunk.bits_per_sample / 8);
    int Channels = Wave->fmt_chunk.num_channels;

    int64_t StartSample = CurrentMillisecond * Wave->fmt_chunk.sample_rate / 1000;
    int64_t EndSample = (CurrentMillisecond + PERIODIC_TIMER) * Wave->fmt_chunk.sample_rate / 1000;

    if (StartSample >= SampleCount || EndSample > SampleCount || StartSample >= EndSample) {
        std::cerr << "Invalid sample range." << std::endl;
        return;
    }

    std::vector<int> FrequencyCounts(SphereCount, 0);
    int FrequencyRange = 65536 / SphereCount;

    for (int64_t i = StartSample; i < EndSample; i++) {
        for (int ch = 0; ch < Channels; ++ch) {
            int16_t Sample = Samples[i * Channels + ch];
            int FrequencyIndex = (Sample + 32768) / FrequencyRange;
            if (FrequencyIndex >= 0 && FrequencyIndex < SphereCount) {
                FrequencyCounts[FrequencyIndex]++;
            }
        }
    }

    int maxFrequencyCount = *std::max_element(FrequencyCounts.begin(), FrequencyCounts.end());

    for (int i = 0; i < SphereCount; ++i) {
        float height = std::min((float)FrequencyCounts[i] / maxFrequencyCount * MAX_HEIGHT, MAX_HEIGHT);
        float xPos = (i - (SphereCount / 2)) * SPACING; // Center spheres horizontally
        float yPos = height / 2.0f - (MAX_HEIGHT / 2.0f); // Center the height

        static std::vector<glm::vec3> lastPositions(SphereCount, glm::vec3(0.0f));
        glm::vec3 currentPosition(xPos, yPos, 0.0f);

        glm::vec3 newPosition = glm::mix(lastPositions[i], currentPosition, deltaTime * 16.0f); // Adjust the 2.0f factor to control the speed of the transition
        lastPositions[i] = newPosition;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, newPosition);

        GLint modelLoc = glGetUniformLocation(ShaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        glDrawElements(GL_TRIANGLES, SphereIndices.size(), GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
}



int main(void) {
    if (!glfwInit()) {
        return -1;
    }

    uint32_t Width = 1920;
    uint32_t Height = 1080;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string WindowTitle = "Wopp - Wave Audio Player";

    GLFWwindow *Window = glfwCreateWindow(Width, Height, WindowTitle.c_str(), NULL, NULL);
    if (!Window) {
        fprintf(stderr, "Failed to create GLFW Window!\n");
        glfwTerminate();
        return -1;
    }

    glfwSetFramebufferSizeCallback(Window, ResizeCallback);
    glfwMakeContextCurrent(Window);
    glfwSwapInterval(0);

    if (glewInit() != GLEW_OK) {
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
    if (hFind == INVALID_HANDLE_VALUE) {
        std::wcerr << L"Error finding files in directory" << std::endl;
        return 1;
    }

    do {
        Controller.Add(L"Assets\\Audio\\" + std::wstring(findFileData.cFileName));
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);

    uint64_t CurrentMillisecond = 0;
    TimerParam Parameter = { PeriodicTimerCallback, &CurrentMillisecond };
    Timer AudioTimer(PERIODIC_TIMER, 0U, &Parameter, TimerType::PERIODIC);
    Controller.BindTimer(AudioTimer, true);
    Controller.Start();

    glEnable(GL_DEPTH_TEST);

    // Sphere data
    std::vector<float> SphereVertices;
    std::vector<float> SphereNormals;
    std::vector<unsigned int> SphereIndices;
    GenerateSphere(1.0f, 36, 18, SphereVertices, SphereNormals, SphereIndices);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, SphereVertices.size() * sizeof(float), SphereVertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, SphereIndices.size() * sizeof(unsigned int), SphereIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    
    // Track the time
    auto lastTime = std::chrono::high_resolution_clock::now();
    float deltaTime = 0.0f;

    while (!glfwWindowShouldClose(Window)) {
        // Calculate DeltaTime
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> duration = currentTime - lastTime;
        deltaTime = duration.count();
        lastTime = currentTime;

        if (Controller.GetAudioStreamState() == EAudioStreamState::PLAYING) {
            RenderSphere(Controller, VAO, VBO, EBO, ShaderProgram, CurrentMillisecond, SphereIndices, Width, Height, deltaTime);
        } else {
            CurrentMillisecond = 0U;
        }

        glfwSwapBuffers(Window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
