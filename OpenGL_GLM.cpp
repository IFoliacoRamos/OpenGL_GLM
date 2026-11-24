
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstddef>
#include <cmath>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Origin.h"


// ============================================================
// CÁMARA: PROCESAR EL TECLADO
// ============================================================

void processInput(
    GLFWwindow* window,
    glm::vec3& cameraPos,
    const glm::vec3& cameraFront,
    const glm::vec3& cameraUp,
    float cameraSpeed
)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;

    glm::vec3 cameraRight = glm::normalize(
        glm::cross(cameraFront, cameraUp)
    );

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraRight;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraRight;

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraUp;

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraUp;
}


// ============================================================
// LEER ARCHIVOS DE SHADERS
// ============================================================

std::string readShaderFile(const char* path)
{
    std::ifstream file(path);

    if (!file.is_open())
    {
        std::cerr << "No se pudo abrir el shader: "
            << path << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}


// ============================================================
// MOSTRAR ERRORES DE SHADERS
// ============================================================

void printShaderLog(GLuint shader)
{
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    if (length > 0)
    {
        std::vector<char> log(length);
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        std::cerr << log.data() << std::endl;
    }
}

void printProgramLog(GLuint program)
{
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

    if (length > 0)
    {
        std::vector<char> log(length);
        glGetProgramInfoLog(program, length, nullptr, log.data());
        std::cerr << log.data() << std::endl;
    }
}


// ============================================================
// COMPILAR UN SHADER
// ============================================================

GLuint compileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* shaderCode = source.c_str();

    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        std::cerr << "Error al compilar el shader:" << std::endl;
        printShaderLog(shader);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}


// ============================================================
// CREAR EL PROGRAMA DE SHADERS DEL CREEPER
// ============================================================

GLuint createShaderProgram()
{
    std::string vertexSource =
        readShaderFile("shaders/vertex.vert");

    std::string fragmentSource =
        readShaderFile("shaders/fragment.frag");

    if (vertexSource.empty() || fragmentSource.empty())
        return 0;

    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexSource);

    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        std::cerr << "Error al enlazar el shader del Creeper."
            << std::endl;

        printProgramLog(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);

        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


// ============================================================
// SHADER INDEPENDIENTE PARA EL SUELO
// ============================================================

GLuint createGroundProgram()
{
    const std::string vertexSource = R"(
        #version 330 core

        layout (location = 0) in vec3 aPosition;
        layout (location = 1) in vec3 aColor;

        uniform mat4 uMVP;

        out vec3 vertexColor;

        void main()
        {
            gl_Position = uMVP * vec4(aPosition, 1.0);
            vertexColor = aColor;
        }
    )";

    const std::string fragmentSource = R"(
        #version 330 core

        in vec3 vertexColor;
        out vec4 FragColor;

        void main()
        {
            FragColor = vec4(vertexColor, 1.0);
        }
    )";

    GLuint vertexShader =
        compileShader(GL_VERTEX_SHADER, vertexSource);

    if (vertexShader == 0)
        return 0;

    GLuint fragmentShader =
        compileShader(GL_FRAGMENT_SHADER, fragmentSource);

    if (fragmentShader == 0)
    {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        std::cerr << "Error al enlazar el shader del suelo."
            << std::endl;

        printProgramLog(program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);

        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


// ============================================================
// ESTRUCTURAS DEL TERRENO
// ============================================================

struct GroundVertex
{
    glm::vec3 position;
    glm::vec3 color;
};

struct GroundMesh
{
    std::vector<GroundVertex> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;
};


// ============================================================
// AGREGAR UNA CARA CUADRADA AL TERRENO
// ============================================================

void addGroundQuad(
    GroundMesh& ground,
    glm::vec3 a,
    glm::vec3 b,
    glm::vec3 c,
    glm::vec3 d,
    glm::vec3 color
)
{
    unsigned int baseIndex =
        static_cast<unsigned int>(ground.vertices.size());

    ground.vertices.push_back({ a, color });
    ground.vertices.push_back({ b, color });
    ground.vertices.push_back({ c, color });
    ground.vertices.push_back({ d, color });

    ground.indices.push_back(baseIndex);
    ground.indices.push_back(baseIndex + 2);
    ground.indices.push_back(baseIndex + 1);

    ground.indices.push_back(baseIndex);
    ground.indices.push_back(baseIndex + 3);
    ground.indices.push_back(baseIndex + 2);
}


// ============================================================
// CREAR TERRENO ESTILO MINECRAFT
// ============================================================

void createGround(GroundMesh& ground)
{
    const float groundHeight = -5.5f;
    const float terrainSize = 34.0f;
    const float dirtDepth = 2.0f;

    const int divisions = 42;
    const int verticalDivisions = 8;

    const float pixelSize = terrainSize / divisions;
    const float verticalPixelSize = dirtDepth / verticalDivisions;

    const float start = -terrainSize / 2.0f;
    const float end = terrainSize / 2.0f;

    // SUPERFICIE SUPERIOR: CÉSPED

    for (int x = 0; x < divisions; x++)
    {
        for (int z = 0; z < divisions; z++)
        {
            float x0 = start + x * pixelSize;
            float x1 = x0 + pixelSize;

            float z0 = start + z * pixelSize;
            float z1 = z0 + pixelSize;

            int pattern = (x * 17 + z * 31 + x * z * 7) % 23;

            glm::vec3 grassColor;

            if (pattern == 0 || pattern == 1)
                grassColor = glm::vec3(0.38f, 0.70f, 0.20f);
            else if (pattern == 5 || pattern == 9)
                grassColor = glm::vec3(0.30f, 0.62f, 0.16f);
            else if (pattern == 14)
                grassColor = glm::vec3(0.42f, 0.74f, 0.24f);
            else
                grassColor = glm::vec3(0.34f, 0.66f, 0.18f);

            addGroundQuad(
                ground,
                glm::vec3(x0, groundHeight, z0),
                glm::vec3(x1, groundHeight, z0),
                glm::vec3(x1, groundHeight, z1),
                glm::vec3(x0, groundHeight, z1),
                grassColor
            );
        }
    }

    // LATERALES: TIERRA

    for (int side = 0; side < 4; side++)
    {
        for (int i = 0; i < divisions; i++)
        {
            for (int j = 0; j < verticalDivisions; j++)
            {
                float yTop =
                    groundHeight - j * verticalPixelSize;

                float yBottom =
                    yTop - verticalPixelSize;

                float position0 = start + i * pixelSize;
                float position1 = position0 + pixelSize;

                glm::vec3 sideColor;

                if (j == 0)
                {
                    sideColor = glm::vec3(0.30f, 0.62f, 0.16f);
                }
                else
                {
                    int pattern =
                        (i * 13 + j * 19 + side * 7) % 11;

                    if (pattern == 0 || pattern == 4)
                        sideColor = glm::vec3(0.42f, 0.25f, 0.12f);
                    else if (pattern == 2 || pattern == 7)
                        sideColor = glm::vec3(0.52f, 0.32f, 0.16f);
                    else
                        sideColor = glm::vec3(0.46f, 0.28f, 0.13f);
                }

                // Lateral frontal.
                if (side == 0)
                {
                    addGroundQuad(
                        ground,
                        glm::vec3(position0, yTop, start),
                        glm::vec3(position1, yTop, start),
                        glm::vec3(position1, yBottom, start),
                        glm::vec3(position0, yBottom, start),
                        sideColor
                    );
                }

                // Lateral trasero.
                else if (side == 1)
                {
                    addGroundQuad(
                        ground,
                        glm::vec3(position1, yTop, end),
                        glm::vec3(position0, yTop, end),
                        glm::vec3(position0, yBottom, end),
                        glm::vec3(position1, yBottom, end),
                        sideColor
                    );
                }

                // Lateral izquierdo.
                else if (side == 2)
                {
                    addGroundQuad(
                        ground,
                        glm::vec3(start, yTop, position1),
                        glm::vec3(start, yTop, position0),
                        glm::vec3(start, yBottom, position0),
                        glm::vec3(start, yBottom, position1),
                        sideColor
                    );
                }

                // Lateral derecho.
                else
                {
                    addGroundQuad(
                        ground,
                        glm::vec3(end, yTop, position0),
                        glm::vec3(end, yTop, position1),
                        glm::vec3(end, yBottom, position1),
                        glm::vec3(end, yBottom, position0),
                        sideColor
                    );
                }
            }
        }
    }
}


// ============================================================
// CURVA DE BÉZIER CÚBICA
// ============================================================

glm::vec3 bezierPoint(
    float t,
    const glm::vec3& P0,
    const glm::vec3& P1,
    const glm::vec3& P2,
    const glm::vec3& P3
)
{
    float u = 1.0f - t;

    return
        (u * u * u) * P0 +
        (3.0f * u * u * t) * P1 +
        (3.0f * u * t * t) * P2 +
        (t * t * t) * P3;
}


// ============================================================
// CALCULAR LA DIRECCIÓN DE LA CURVA
// ============================================================

glm::vec3 bezierDirection(
    float t,
    const glm::vec3& P0,
    const glm::vec3& P1,
    const glm::vec3& P2,
    const glm::vec3& P3
)
{
    float u = 1.0f - t;

    glm::vec3 direction =
        3.0f * u * u * (P1 - P0) +
        6.0f * u * t * (P2 - P1) +
        3.0f * t * t * (P3 - P2);

    return glm::normalize(direction);
}


// ============================================================
// ENVIAR EL TERRENO A LA GPU
// ============================================================

void setupGround(GroundMesh& ground)
{
    glGenVertexArrays(1, &ground.VAO);
    glGenBuffers(1, &ground.VBO);
    glGenBuffers(1, &ground.EBO);

    glBindVertexArray(ground.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, ground.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        ground.vertices.size() * sizeof(GroundVertex),
        ground.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ground.EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        ground.indices.size() * sizeof(unsigned int),
        ground.indices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GroundVertex),
        (void*)offsetof(GroundVertex, position)
    );

    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(GroundVertex),
        (void*)offsetof(GroundVertex, color)
    );

    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}


// ============================================================
// DIBUJAR EL TERRENO
// ============================================================

void drawGround(
    GroundMesh& ground,
    GLuint groundProgram,
    const glm::mat4& view,
    const glm::mat4& projection
)
{
    glUseProgram(groundProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    GLint mvpLocation =
        glGetUniformLocation(groundProgram, "uMVP");

    glUniformMatrix4fv(
        mvpLocation,
        1,
        GL_FALSE,
        glm::value_ptr(mvp)
    );

    glBindVertexArray(ground.VAO);

    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(ground.indices.size()),
        GL_UNSIGNED_INT,
        0
    );

    glBindVertexArray(0);
}


// ============================================================
// DIBUJAR LA CURVA DE BÉZIER
// ============================================================

void drawBezierCurve(
    GroundMesh& curve,
    GLuint groundProgram,
    const glm::mat4& view,
    const glm::mat4& projection
)
{
    glUseProgram(groundProgram);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 mvp = projection * view * model;

    GLint mvpLocation =
        glGetUniformLocation(groundProgram, "uMVP");

    glUniformMatrix4fv(
        mvpLocation,
        1,
        GL_FALSE,
        glm::value_ptr(mvp)
    );

    glBindVertexArray(curve.VAO);

    glLineWidth(3.0f);

    glDrawArrays(
        GL_LINE_STRIP,
        0,
        static_cast<GLsizei>(curve.vertices.size())
    );

    glBindVertexArray(0);
}


// ============================================================
// VÉRTICES DEL MODELO DEL CREEPER
// ============================================================

struct ModelVertex
{
    glm::vec3 position;
    glm::vec4 color;
    glm::vec2 texCoords;
    glm::vec3 normal;
};

struct ModelMesh
{
    std::vector<ModelVertex> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLuint textureID = 0;
};


// ============================================================
// CARGAR LA TEXTURA INTEGRADA EN EL GLB
// ============================================================

bool loadEmbeddedTexture(
    const aiScene* scene,
    const aiMaterial* material,
    GLuint& textureID
)
{
    aiString texturePath;

    aiReturn result = material->GetTexture(
        aiTextureType_DIFFUSE,
        0,
        &texturePath
    );

    if (result != AI_SUCCESS)
    {
        result = material->GetTexture(
            aiTextureType_BASE_COLOR,
            0,
            &texturePath
        );
    }

    if (result != AI_SUCCESS)
    {
        std::cerr << "No se encontró una textura." << std::endl;
        return false;
    }

    if (texturePath.C_Str()[0] != '*')
    {
        std::cerr << "La textura no está integrada en el GLB."
            << std::endl;
        return false;
    }

    int textureIndex = -1;

    try
    {
        textureIndex = std::stoi(texturePath.C_Str() + 1);
    }
    catch (...)
    {
        std::cerr << "Índice de textura no válido." << std::endl;
        return false;
    }

    if (textureIndex < 0 ||
        textureIndex >= static_cast<int>(scene->mNumTextures))
    {
        std::cerr << "Índice de textura fuera de rango."
            << std::endl;
        return false;
    }

    const aiTexture* embeddedTexture =
        scene->mTextures[textureIndex];

    if (embeddedTexture->mHeight != 0)
    {
        std::cerr << "La textura integrada no está comprimida."
            << std::endl;
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_set_flip_vertically_on_load(true);

    unsigned char* imageData = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc*>(
            embeddedTexture->pcData
            ),
        static_cast<int>(embeddedTexture->mWidth),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha
    );

    if (imageData == nullptr)
    {
        std::cerr << "No se pudo cargar la textura: "
            << stbi_failure_reason() << std::endl;
        return false;
    }

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        imageData
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    stbi_image_free(imageData);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Textura del Creeper cargada: "
        << width << "x" << height << std::endl;

    return true;
}


// ============================================================
// CARGAR EL MODELO 3D
// ============================================================

bool loadModel(const std::string& path, ModelMesh& mesh)
{
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals
    );

    if (!scene ||
        !scene->mRootNode ||
        scene->mNumMeshes == 0)
    {
        std::cerr << "Error al cargar el modelo: "
            << importer.GetErrorString() << std::endl;
        return false;
    }

    const aiMesh* aiMeshData = scene->mMeshes[0];

    if (aiMeshData->mMaterialIndex >= scene->mNumMaterials)
    {
        std::cerr << "Material no válido." << std::endl;
        return false;
    }

    const aiMaterial* material =
        scene->mMaterials[aiMeshData->mMaterialIndex];

    if (!loadEmbeddedTexture(scene, material, mesh.textureID))
    {
        std::cerr << "No se pudo cargar la textura del Creeper."
            << std::endl;
        return false;
    }

    // Extraer los vértices.
    for (unsigned int i = 0;
        i < aiMeshData->mNumVertices;
        i++)
    {
        ModelVertex vertex;

        // Posición del vértice.
        vertex.position = glm::vec3(
            aiMeshData->mVertices[i].x,
            aiMeshData->mVertices[i].y,
            aiMeshData->mVertices[i].z
        );

        vertex.color = glm::vec4(1.0f);

        // Extraer las normales para la iluminación Phong.
        if (aiMeshData->HasNormals())
        {
            vertex.normal = glm::vec3(
                aiMeshData->mNormals[i].x,
                aiMeshData->mNormals[i].y,
                aiMeshData->mNormals[i].z
            );
        }
        else
        {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }

        // Coordenadas de textura.
        if (aiMeshData->HasTextureCoords(0))
        {
            vertex.texCoords = glm::vec2(
                aiMeshData->mTextureCoords[0][i].x,
                aiMeshData->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.texCoords = glm::vec2(0.0f);
        }

        mesh.vertices.push_back(vertex);
    }

    // Extraer los índices.
    for (unsigned int i = 0;
        i < aiMeshData->mNumFaces;
        i++)
    {
        const aiFace& face = aiMeshData->mFaces[i];

        for (unsigned int j = 0;
            j < face.mNumIndices;
            j++)
        {
            mesh.indices.push_back(face.mIndices[j]);
        }
    }

    std::cout << "Modelo cargado correctamente." << std::endl;
    std::cout << "Vértices: " << mesh.vertices.size() << std::endl;
    std::cout << "Índices: " << mesh.indices.size() << std::endl;

    return true;
}


// ============================================================
// PREPARAR LA MALLA DEL CREEPER
// ============================================================

void setupMesh(ModelMesh& mesh)
{
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);

    glBufferData(
        GL_ARRAY_BUFFER,
        mesh.vertices.size() * sizeof(ModelVertex),
        mesh.vertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);

    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        mesh.indices.size() * sizeof(unsigned int),
        mesh.indices.data(),
        GL_STATIC_DRAW
    );

    // Atributo 0: posición.
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE,
        sizeof(ModelVertex),
        (void*)offsetof(ModelVertex, position)
    );

    glEnableVertexAttribArray(0);

    // Atributo 1: color.
    glVertexAttribPointer(
        1, 4, GL_FLOAT, GL_FALSE,
        sizeof(ModelVertex),
        (void*)offsetof(ModelVertex, color)
    );

    glEnableVertexAttribArray(1);

    // Atributo 2: coordenadas de textura.
    glVertexAttribPointer(
        2, 2, GL_FLOAT, GL_FALSE,
        sizeof(ModelVertex),
        (void*)offsetof(ModelVertex, texCoords)
    );

    glEnableVertexAttribArray(2);

    // Atributo 3: normales para Phong.
    glVertexAttribPointer(
        3, 3, GL_FLOAT, GL_FALSE,
        sizeof(ModelVertex),
        (void*)offsetof(ModelVertex, normal)
    );

    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}


// ============================================================
// FUNCIÓN PRINCIPAL
// ============================================================

int main()
{
    // INICIALIZAR GLFW.

    if (!glfwInit())
    {
        std::cerr << "No se pudo inicializar GLFW." << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        800,
        800,
        "OpenGL - Creeper 3D",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cerr << "No se pudo crear la ventana." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // INICIALIZAR GLAD.

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "No se pudo inicializar GLAD." << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // CREAR EL SHADER DEL CREEPER.

    GLuint shaderProgram = createShaderProgram();

    if (shaderProgram == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // CREAR EL SHADER DEL SUELO.

    GLuint groundProgram = createGroundProgram();

    if (groundProgram == 0)
    {
        glDeleteProgram(shaderProgram);
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // CARGAR EL CREEPER.

    ModelMesh creeperMesh;

    if (!loadModel("models/minecraft_-_creeper.glb", creeperMesh))
    {
        glDeleteProgram(groundProgram);
        glDeleteProgram(shaderProgram);

        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    setupMesh(creeperMesh);

    // CREAR EL TERRENO.

    GroundMesh ground;

    createGround(ground);
    setupGround(ground);

    // CREAR LA CURVA DE BÉZIER.

    glm::vec3 P0(-6.0f, 0.0f, -5.0f);
    glm::vec3 P1(-6.0f, 0.0f, 5.0f);
    glm::vec3 P2(6.0f, 0.0f, 5.0f);
    glm::vec3 P3(6.0f, 0.0f, -5.0f);

    GroundMesh bezierCurve;

    for (int i = 0; i <= 100; i++)
    {
        float t = static_cast<float>(i) / 100.0f;

        glm::vec3 point = bezierPoint(t, P0, P1, P2, P3);

        // La curva queda un poco por encima de la superficie.
        point.y = -5.48f;

        bezierCurve.vertices.push_back({
            point,
            glm::vec3(1.0f, 1.0f, 0.0f)
            });
    }

    setupGround(bezierCurve);

    // OBTENER LOS UNIFORMS DEL CREEPER.

    GLint mvpLoc =
        glGetUniformLocation(shaderProgram, "uMVP");

    GLint textureLoc =
        glGetUniformLocation(shaderProgram, "uTexture");

    // Uniforms de iluminación Phong.
    GLint modelLoc =
        glGetUniformLocation(shaderProgram, "uModel");

    GLint lightPosLoc =
        glGetUniformLocation(shaderProgram, "lightPos");

    GLint lightColorLoc =
        glGetUniformLocation(shaderProgram, "lightColor");

    GLint viewPosLoc =
        glGetUniformLocation(shaderProgram, "viewPos");

    // Comprobar que todos los uniforms existen.
    if (
        mvpLoc == -1 ||
        textureLoc == -1 ||
        modelLoc == -1 ||
        lightPosLoc == -1 ||
        lightColorLoc == -1 ||
        viewPosLoc == -1
        )
    {
        std::cerr << "No se encontraron los uniforms del Creeper."
            << std::endl;

        glDeleteVertexArrays(1, &ground.VAO);
        glDeleteBuffers(1, &ground.VBO);
        glDeleteBuffers(1, &ground.EBO);

        glDeleteVertexArrays(1, &bezierCurve.VAO);
        glDeleteBuffers(1, &bezierCurve.VBO);
        glDeleteBuffers(1, &bezierCurve.EBO);

        glDeleteProgram(groundProgram);

        glDeleteVertexArrays(1, &creeperMesh.VAO);
        glDeleteBuffers(1, &creeperMesh.VBO);
        glDeleteBuffers(1, &creeperMesh.EBO);
        glDeleteTextures(1, &creeperMesh.textureID);

        glDeleteProgram(shaderProgram);

        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // CONFIGURACIÓN DE OPENGL.

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // CREAR LOS EJES DE COORDENADAS.

    Origin origin;

    // CONFIGURACIÓN DE LA CÁMARA.

    glm::vec3 cameraPos(0.0f, 0.0f, 12.0f);

    glm::vec3 cameraFront = glm::normalize(
        glm::vec3(0.0f, -0.45f, -1.0f)
    );

    glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

    float cameraSpeed = 0.02f;

    // BUCLE PRINCIPAL.

    while (!glfwWindowShouldClose(window))
    {
        // Procesar el teclado.
        processInput(
            window,
            cameraPos,
            cameraFront,
            cameraUp,
            cameraSpeed
        );

        int width = 0;
        int height = 0;

        glfwGetFramebufferSize(window, &width, &height);

        if (width == 0 || height == 0)
        {
            glfwPollEvents();
            continue;
        }

        glViewport(0, 0, width, height);

        // CIELO AZUL.

        glClearColor(0.40f, 0.70f, 1.00f, 1.00f);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT
        );

        // MATRIZ DE PROYECCIÓN.

        glm::mat4 projection = glm::perspective(
            glm::radians(60.0f),
            static_cast<float>(width) / static_cast<float>(height),
            0.1f,
            100.0f
        );

        // MATRIZ VIEW.

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        // DIBUJAR EL SUELO.

        drawGround(
            ground,
            groundProgram,
            view,
            projection
        );

        // DIBUJAR LA TRAYECTORIA DE BÉZIER.

        drawBezierCurve(
            bezierCurve,
            groundProgram,
            view,
            projection
        );

        // MOVIMIENTO DEL CREEPER SOBRE LA CURVA.

        float currentTime = static_cast<float>(glfwGetTime());

        // Recorrido de ida y vuelta.
        float cycle = std::fmod(currentTime * 0.08f, 2.0f);

        float t;
        bool movingForward;

        if (cycle <= 1.0f)
        {
            t = cycle;
            movingForward = true;
        }
        else
        {
            t = 2.0f - cycle;
            movingForward = false;
        }

        // Calcular la posición del Creeper.
        glm::vec3 creeperPosition = bezierPoint(
            t, P0, P1, P2, P3
        );

        // Calcular la dirección de movimiento.
        glm::vec3 direction = bezierDirection(
            t, P0, P1, P2, P3
        );

        // Invertir la dirección cuando regresa.
        if (!movingForward)
            direction = -direction;

        // Calcular el ángulo para orientar el Creeper.
        float angle = std::atan2(
            direction.x,
            direction.z
        );

        // MATRIZ MODEL DEL CREEPER.

        glm::mat4 model = glm::mat4(1.0f);

        // Trasladar el personaje a su posición en la curva.
        model = glm::translate(
            model,
            creeperPosition
        );

        // Orientarlo hacia la dirección de desplazamiento.
        model = glm::rotate(
            model,
            angle,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // Corregir la orientación original del modelo.
        model = glm::rotate(
            model,
            glm::radians(-90.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        // MATRIZ MVP DEL CREEPER.

        glm::mat4 mvp = projection * view * model;

        // DIBUJAR LOS EJES.

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(
            mvpLoc,
            1,
            GL_FALSE,
            glm::value_ptr(mvp)
        );

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            creeperMesh.textureID
        );

        glUniform1i(textureLoc, 0);

        origin.draw();

        // DIBUJAR EL CREEPER CON ILUMINACIÓN PHONG.

        glUseProgram(shaderProgram);

        // Posición de la fuente de luz.
        glUniform3f(
            lightPosLoc,
            0.0f, 10.0f, 5.0f
        );

        // Color blanco de la luz.
        glUniform3f(
            lightColorLoc,
            1.0f, 1.0f, 1.0f
        );

        // Posición actual de la cámara.
        glUniform3f(
            viewPosLoc,
            cameraPos.x,
            cameraPos.y,
            cameraPos.z
        );

        // Enviar la matriz Model al vertex shader.
        glUniformMatrix4fv(
            modelLoc,
            1,
            GL_FALSE,
            glm::value_ptr(model)
        );

        // Enviar la matriz MVP.
        glUniformMatrix4fv(
            mvpLoc,
            1,
            GL_FALSE,
            glm::value_ptr(mvp)
        );

        // Activar la textura.
        glActiveTexture(GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            creeperMesh.textureID
        );

        glUniform1i(textureLoc, 0);

        // Dibujar la malla.
        glBindVertexArray(creeperMesh.VAO);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(creeperMesh.indices.size()),
            GL_UNSIGNED_INT,
            0
        );

        glBindVertexArray(0);

        // MOSTRAR EL RESULTADO.

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // LIBERAR RECURSOS DEL SUELO.

    glDeleteVertexArrays(1, &ground.VAO);
    glDeleteBuffers(1, &ground.VBO);
    glDeleteBuffers(1, &ground.EBO);

    glDeleteProgram(groundProgram);

    // LIBERAR RECURSOS DE LA CURVA.

    glDeleteVertexArrays(1, &bezierCurve.VAO);
    glDeleteBuffers(1, &bezierCurve.VBO);
    glDeleteBuffers(1, &bezierCurve.EBO);

    // LIBERAR RECURSOS DEL CREEPER.

    glDeleteVertexArrays(1, &creeperMesh.VAO);
    glDeleteBuffers(1, &creeperMesh.VBO);
    glDeleteBuffers(1, &creeperMesh.EBO);

    glDeleteTextures(1, &creeperMesh.textureID);

    glDeleteProgram(shaderProgram);

    // CERRAR GLFW.

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
