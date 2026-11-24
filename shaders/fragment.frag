
#version 330 core

in vec2 vTexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

// Textura del Creeper.
uniform sampler2D uTexture;

// Datos de iluminación.
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    // Obtener el color original de la textura.
    vec3 objectColor = texture(uTexture, vTexCoord).rgb;

    // Normalizar la normal de la superficie.
    vec3 norm = normalize(Normal);

    // Calcular la dirección hacia la luz.
    vec3 lightDir = normalize(lightPos - FragPos);

    // -----------------------------------------
    // 1. ILUMINACIÓN AMBIENTAL
    // -----------------------------------------

    vec3 ambient = 0.25 * lightColor;

    // -----------------------------------------
    // 2. ILUMINACIÓN DIFUSA
    // -----------------------------------------

    float diff = max(dot(norm, lightDir), 0.0);

    vec3 diffuse = diff * lightColor;

    // -----------------------------------------
    // 3. ILUMINACIÓN ESPECULAR
    // -----------------------------------------

    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(
        max(dot(viewDir, reflectDir), 0.0),
        32.0
    );

    vec3 specular = 0.25 * spec * lightColor;

    // -----------------------------------------
    // COMBINAR LOS TRES COMPONENTES
    // -----------------------------------------

    vec3 result =
        (ambient + diffuse + specular) * objectColor;

    FragColor = vec4(result, 1.0);
}
