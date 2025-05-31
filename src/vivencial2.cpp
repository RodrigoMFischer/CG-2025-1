/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// LoadSimpleObj
#include "../include/LoadSimpleObj.h"

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

// Protótipos das funções
int setupShader();

// Dimensões da janela (pode ser alterado em tempo de execução)
const GLuint WIDTH = 600, HEIGHT = 600;

// Código fonte do Vertex Shader (em GLSL): ainda hardcoded
const GLchar *vertexShaderSource = R"(
#version 460

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragPos = model * vec4(position, 1.0);
    vNormal = mat3(transpose(inverse(model))) * normal;
    texCoord = texc;
    vColor = vec4(color, 1.0);
}
)";

//Códifo fonte do Fragment Shader (em GLSL): ainda hardcoded
const GLchar *fragmentShaderSource = R"(
#version 460

in vec2 texCoord;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;

uniform sampler2D texBuff;
uniform vec3 camPos;

uniform vec3 keyPos, fillPos, backPos;
uniform vec3 keyColor, fillColor, backColor;
uniform bool lightOn[3];

uniform float ka, kd, ks, q;

out vec4 color;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vec3(fragPos));
    vec3 ambient = ka * vec3(1.0);
    vec3 result = ambient * vec3(vColor);

    vec3 lights[3] = vec3[3](keyPos, fillPos, backPos);
    vec3 lightColors[3] = vec3[3](keyColor, fillColor, backColor);

    for (int i = 0; i < 3; i++) {
        if (lightOn[i]) {
            vec3 L = normalize(lights[i] - vec3(fragPos));
            float diff = max(dot(N, L), 0.0);
            vec3 R = normalize(reflect(-L, N));
            float spec = pow(max(dot(R, V), 0.0), q);
            float d = length(lights[i] - vec3(fragPos));
            float attenuation = 1.0 / (d * d + 0.1);
            result += (kd * diff + ks * spec) * lightColors[i] * attenuation * vec3(vColor);
        }
    }

    color = vec4(result, 1.0);
}
)";


// Variaveis globais para coordenada atual do cubo
float posX = 0.0f;
float posZ = 0.0f;
float posY = 0.0f;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 10.0f); // Posição inicial da câmera
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); // Direção para onde a câmera está olhando
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f); // Vetor para cima

int nVertices = 0;
GLuint VAO;

// Transformações separadas (posição no eixo X como exemplo)
bool rotateX = false, rotateY = false, rotateZ = false;

// Posições das luzes (valores exemplo, ajuste conforme necessário)
glm::vec3 keyPos = glm::vec3(1.5f, 1.0f, 1.0f);     // Luz principal
glm::vec3 fillPos = glm::vec3(-1.5f, 0.5f, 1.0f);   // Luz de preenchimento
glm::vec3 backPos = glm::vec3(0.0f, 1.5f, -1.0f);   // Luz de fundo

// Intensidades (valores sugeridos)
glm::vec3 keyColor = glm::vec3(1.0f, 1.0f, 1.0f) * 1.0f;
glm::vec3 fillColor = glm::vec3(1.0f, 1.0f, 1.0f) * 0.5f;
glm::vec3 backColor = glm::vec3(1.0f, 1.0f, 1.0f) * 0.3f;

// Estados das luzes (1=ligada, 0=desligada)
int lightOn[3] = {1, 1, 1};
int selectedLight = 0; // 0 = Key, 1 = Fill, 2 = Back

// Função MAIN
int main()
{
    // Inicialização da GLFW
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW." << std::endl;
        return -1;
    }

    // Configurações do contexto OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criação da janela
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vivencial 2", nullptr, nullptr);
    if (!window) {
        std::cerr << "Falha ao criar a janela GLFW." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Inicializa GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Falha ao inicializar GLAD." << std::endl;
        return -1;
    }

    // Informações da GPU
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL version supported: " << glGetString(GL_VERSION) << std::endl;

    // Configuração da viewport
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // Callback de teclado
    glfwSetKeyCallback(window, key_callback);

    // Compilação dos shaders
    GLuint shaderID = setupShader();
    glUseProgram(shaderID);

    // Carregar modelo 3D
    VAO = loadSimpleOBJ("../assets/Modelos3D/Suzanne.obj", nVertices);

    // Localizações dos uniforms
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    GLint viewLoc = glGetUniformLocation(shaderID, "view");
    GLint projLoc = glGetUniformLocation(shaderID, "projection");

    // Uniforms de luzes
    glUniform3fv(glGetUniformLocation(shaderID, "keyPos"), 1, glm::value_ptr(keyPos));
    glUniform3fv(glGetUniformLocation(shaderID, "fillPos"), 1, glm::value_ptr(fillPos));
    glUniform3fv(glGetUniformLocation(shaderID, "backPos"), 1, glm::value_ptr(backPos));
    glUniform3fv(glGetUniformLocation(shaderID, "keyColor"), 1, glm::value_ptr(keyColor));
    glUniform3fv(glGetUniformLocation(shaderID, "fillColor"), 1, glm::value_ptr(fillColor));
    glUniform3fv(glGetUniformLocation(shaderID, "backColor"), 1, glm::value_ptr(backColor));

    // LightOn como bool (GL_TRUE/GL_FALSE)
    GLint lightOnUniform = glGetUniformLocation(shaderID, "lightOn");
    GLboolean lightsOn[3] = {GL_TRUE, GL_TRUE, GL_TRUE};
    glUniform1iv(lightOnUniform, 3, reinterpret_cast<GLint*>(lightsOn));

    // Uniform da posição da câmera (camPos)
    GLint camPosLoc = glGetUniformLocation(shaderID, "camPos");

    // Uniforms materiais (ka, kd, ks, q) - valores default para começar
    glUniform1f(glGetUniformLocation(shaderID, "ka"), 0.1f);
    glUniform1f(glGetUniformLocation(shaderID, "kd"), 0.7f);
    glUniform1f(glGetUniformLocation(shaderID, "ks"), 0.3f);
    glUniform1f(glGetUniformLocation(shaderID, "q"), 32.0f);

    // Habilitar Z-buffer
    glEnable(GL_DEPTH_TEST);

    // Loop principal
    while (!glfwWindowShouldClose(window))
    {
        // Processa eventos
        glfwPollEvents();

        // Limpa a tela
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Tempo para animação
        float angle = static_cast<float>(glfwGetTime());

        // MATRIZ VIEW (câmera)
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // MATRIZ PROJECTION
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / height, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // MATRIZ MODEL
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(posX, posY, posZ));
        if (rotateX) model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
        else if (rotateY) model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
        else if (rotateZ) model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

        // Atualiza posição da câmera no shader
        glUniform3fv(camPosLoc, 1, glm::value_ptr(cameraPos));

        // Desenha o modelo
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        // Troca os buffers
        glfwSwapBuffers(window);
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(shaderID);
    glfwTerminate();

    return 0;
}


// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    const float moveStep = 0.1f;
	const float step = 0.2f; // Passo de movimento


    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_ESCAPE)
            glfwSetWindowShouldClose(window, true);

        // Movimentação do objeto selecionado
        if (key == GLFW_KEY_LEFT) {
            posX -= moveStep;
        }
        if (key == GLFW_KEY_RIGHT) {
            posX += moveStep;
        }
        if (key == GLFW_KEY_UP) {
            posY += moveStep;
        }
        if (key == GLFW_KEY_DOWN) {
            posY -= moveStep;
        }

        // Zoom individual (movimento no eixo Z do objeto)
        if (key == GLFW_KEY_I) {
            posZ += moveStep;
        }
        if (key == GLFW_KEY_J) {
            posZ -= moveStep;
        }

        // Rotação do objeto selecionado
        if (key == GLFW_KEY_X) {
			rotateX = true; rotateY = false; rotateZ = false;
        }
        if (key == GLFW_KEY_Y) {
			rotateX = false; rotateY = true; rotateZ = false;
        }
        if (key == GLFW_KEY_Z) {
			rotateX = false; rotateY = false; rotateZ = true;
        }

        // Movimento da câmera global (continua funcionando normalmente)
        if (key == GLFW_KEY_W)
            cameraPos -= moveStep * cameraUp;
        if (key == GLFW_KEY_S)
            cameraPos += moveStep * cameraUp;
        if (key == GLFW_KEY_A)
            cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * moveStep;
        if (key == GLFW_KEY_D)
            cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * moveStep;


		// Seleciona a luz
        if (key == GLFW_KEY_1) selectedLight = 0;
        if (key == GLFW_KEY_2) selectedLight = 1;
        if (key == GLFW_KEY_3) selectedLight = 2;

        // Liga/desliga a luz com L
        if (key == GLFW_KEY_L)
        {
            lightOn[selectedLight] = !lightOn[selectedLight];
            std::cout << "Luz " << selectedLight << (lightOn[selectedLight] ? " ligada" : " desligada") << std::endl;

            // Atualiza o shader
            GLint shaderID;
            glGetIntegerv(GL_CURRENT_PROGRAM, &shaderID);
            GLint lightOnLoc = glGetUniformLocation(shaderID, "lightOn");
            glUniform1iv(lightOnLoc, 3, lightOn);
        }

        // Mover luz selecionada
        glm::vec3* selectedPos = nullptr;
        if (selectedLight == 0) selectedPos = &keyPos;
        else if (selectedLight == 1) selectedPos = &fillPos;
        else if (selectedLight == 2) selectedPos = &backPos;

		if (selectedPos)
        {
            if (key == GLFW_KEY_W) selectedPos->y += step;
            if (key == GLFW_KEY_S) selectedPos->y -= step;
            if (key == GLFW_KEY_A) selectedPos->x -= step;
            if (key == GLFW_KEY_D) selectedPos->x += step;
            if (key == GLFW_KEY_Q) selectedPos->z -= step;
            if (key == GLFW_KEY_E) selectedPos->z += step;

            std::cout << "Luz " << selectedLight << " movida para: "
                      << selectedPos->x << ", " << selectedPos->y << ", " << selectedPos->z << std::endl;

            // Atualiza posições no shader
            GLint shaderID;
            glGetIntegerv(GL_CURRENT_PROGRAM, &shaderID);

            glUniform3fv(glGetUniformLocation(shaderID, "keyPos"), 1, glm::value_ptr(keyPos));
            glUniform3fv(glGetUniformLocation(shaderID, "fillPos"), 1, glm::value_ptr(fillPos));
            glUniform3fv(glGetUniformLocation(shaderID, "backPos"), 1, glm::value_ptr(backPos));
        }
    }
}

// Esta função está basntante hardcoded - objetivo é compilar e "buildar" um programa de
// shader simples e único neste exemplo de código
// O código fonte do vertex e fragment shader está nos arrays vertexShaderSource e
// fragmentShader source no iniçio deste arquivo
// A função retorna o identificador do programa de shader
int setupShader()
{
	// Vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// Checando erros de compilação (exibição via log no terminal)
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Fragment shader
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// Checando erros de compilação (exibição via log no terminal)
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// Linkando os shaders e criando o identificador do programa de shader
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// Checando por erros de linkagem
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}