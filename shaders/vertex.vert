
#version 330 core

// Datos que llegan desde la malla.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;

// Datos que enviaremos al fragment shader.
out vec2 vTexCoord;
out vec3 FragPos;
out vec3 Normal;

// Matrices.
uniform mat4 uMVP;
uniform mat4 uModel;

void main()
{
    // Posición final del vértice en pantalla.
    gl_Position = uMVP * vec4(aPos, 1.0);

    // Posición del vértice en el espacio del mundo.
    FragPos = vec3(uModel * vec4(aPos, 1.0));

    // Transformar correctamente la normal.
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    Normal = normalize(normalMatrix * aNormal);

    // Conservar las coordenadas de textura.
    vTexCoord = aTexCoord;
}
