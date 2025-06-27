#include <iostream>
#include <string>
#include <assert.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace std;

struct Geometry
{
	GLuint VAO;
	GLuint vertexCount;
	GLuint textureID = 0;
	string textureFilePath;
};


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
Geometry setupGeometry(const char* filepath);
bool loadObject(
    const char* path,
    std::vector<glm::vec3>& out_vertices,
    std::vector<glm::vec2>& out_uvs,
    std::vector<glm::vec3>& out_normals);
int loadTexture(const string& path);
string loadMTL(const string& path);

const GLchar *vertexShaderSource = R"(
	#version 400
	layout (location = 0) in vec3 position;
	layout (location = 1) in vec3 color;
	layout (location = 2) in vec2 tex_coord;
	out vec4 vertexColor;
	out vec2 texCoord;
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;
	void main()
	{
		gl_Position = projection * view * model * vec4(position, 1.0);
		vertexColor = vec4(color, 1.0);
		texCoord = vec2(tex_coord.x, 1 - tex_coord.y);
	}
)";


const GLchar *fragmentShaderSource = R"(
#version 400
in vec4 vertexColor;
in vec2 texCoord;
out vec4 color;
uniform sampler2D tex_buffer;
void main()
{
		color = texture(tex_buffer, texCoord);
}
)";

const GLuint WIDTH = 600, HEIGHT = 600;
bool rotateX=false, rotateY=false, rotateZ=false;
glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
string mtlFilePath = "";
 
float posX = 0.0f;
float posZ = 0.0f;
float posY = 0.0f;

glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 15.0f); 
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f); 
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f); 

float accumulatedAngleX = 0.0f;
float accumulatedAngleY = 0.0f;
float accumulatedAngleZ = 0.0f;

int main()
{
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Ola 3D -- Rodrigo!", nullptr, nullptr);
		glfwMakeContextCurrent(window);
		glfwSetKeyCallback(window, key_callback);
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
				std::cout << "Failed to initialize GLAD" << std::endl;
				return -1;
		}
		std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
		std::cout << "OpenGL version supported: " << glGetString(GL_VERSION) << std::endl;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		GLuint shaderID = setupShader();
		Geometry geometry = setupGeometry("../assets/Modelos3D/Suzanne.obj");
		glUseProgram(shaderID);
		glm::mat4 model = glm::mat4(1);
		GLint modelLoc = glGetUniformLocation(shaderID, "model");
		GLint viewLoc  = glGetUniformLocation(shaderID, "view");
		GLint projLoc  = glGetUniformLocation(shaderID, "projection");
		model = glm::rotate(model,  glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(model));
    glEnable(GL_DEPTH_TEST);
		while (!glfwWindowShouldClose(window))
	  {
			glfwPollEvents();
			glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			static float lastTime = glfwGetTime();
            float currentTime = glfwGetTime();
            float deltaTime = currentTime - lastTime;
            lastTime = currentTime;

            if (rotateX) accumulatedAngleX += deltaTime;
            if (rotateY) accumulatedAngleY += deltaTime;
            if (rotateZ) accumulatedAngleZ += deltaTime;

			glm::mat4 view = glm::lookAt(
					cameraPos,
					cameraPos + cameraFront,
					cameraUp
			);
			glm::mat4 projection = glm::perspective(
					glm::radians(45.0f),
					(float)width / height,
					0.1f,
					100.0f
			);
			glLineWidth(10);
			glPointSize(20);
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(posX, posY, posZ));
			if (rotateX || accumulatedAngleX > 0) model = glm::rotate(model, accumulatedAngleX, glm::vec3(1.0f, 0.0f, 0.0f));
            if (rotateY || accumulatedAngleY > 0) model = glm::rotate(model, accumulatedAngleY, glm::vec3(0.0f, 1.0f, 0.0f));
            if (rotateZ || accumulatedAngleZ > 0) model = glm::rotate(model, accumulatedAngleZ, glm::vec3(0.0f, 0.0f, 1.0f));
			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
			glActiveTexture(GL_TEXTURE0);
			if (geometry.textureID > 0) glBindTexture(GL_TEXTURE_2D, geometry.textureID);
			glUniform1i(glGetUniformLocation(shaderID, "texture1"), 0); 
			glBindVertexArray(geometry.VAO);
			glDrawArrays(GL_TRIANGLES, 0, geometry.vertexCount);
			glBindVertexArray(0);
			glfwSwapBuffers(window);
	}
		glDeleteVertexArrays(1, &geometry.VAO);
		glfwTerminate();
		return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
		const float cameraSpeed = 0.1f;
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
		{
				if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
				if (key == GLFW_KEY_X) { rotateX = !rotateX; rotateY = false; rotateZ = false; }
				if (key == GLFW_KEY_Y) { rotateX = false; rotateY = !rotateY; rotateZ = false; }
				if (key == GLFW_KEY_Z) { rotateX = false; rotateY = false; rotateZ = !rotateZ; }
				if (key == GLFW_KEY_UP) cameraPos -= cameraSpeed * cameraUp;
				if (key == GLFW_KEY_DOWN) cameraPos += cameraSpeed * cameraUp;
				if (key == GLFW_KEY_LEFT) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
				if (key == GLFW_KEY_RIGHT) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
				if (key == GLFW_KEY_PAGE_UP) cameraPos += cameraSpeed * cameraFront;
				if (key == GLFW_KEY_PAGE_DOWN) cameraPos -= cameraSpeed * cameraFront;
		}
}


int setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int loadTexture(const string& path)
{
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        cerr << "Failed to load texture: " << path << endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texID;
}

bool loadObject(
	const char* path,
	std::vector<glm::vec3>& out_vertices,
	std::vector<glm::vec2>& out_uvs,
	std::vector<glm::vec3>& out_normals)
{
	std::ifstream file(path);

	if (!file)
	{
			std::cerr << "Failed to open file: " << path << std::endl;
			return false;
	}

	std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
	std::vector<glm::vec3> temp_vertices;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::string line;

	while (std::getline(file, line))
	{
			std::istringstream iss(line);
			std::string type;
			iss >> type;

			if (type == "v") 
			{
					glm::vec3 vertex;
					iss >> vertex.x >> vertex.y >> vertex.z;
					temp_vertices.push_back(vertex);
			}
			else if (type == "vt") 
			{
					glm::vec2 uv;
					iss >> uv.x >> uv.y;
					temp_uvs.push_back(uv);
			}
			else if (type == "vn") 
			{
					glm::vec3 normal;
					iss >> normal.x >> normal.y >> normal.z;
					temp_normals.push_back(normal);
			}
			else if (type == "f")
			{
					unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
					char slash;

					for (int i = 0; i < 3; ++i)
					{
							iss >> vertexIndex[i] >> slash >> uvIndex[i] >> slash >> normalIndex[i];
							vertexIndices.push_back(vertexIndex[i]);
							uvIndices.push_back(uvIndex[i]);
							normalIndices.push_back(normalIndex[i]);
					}
			}
			else if (type == "mtllib")
			{
					iss >> mtlFilePath;
			}
	}

	for (unsigned int i = 0; i < vertexIndices.size(); ++i)
	{
			unsigned int vertexIndex = vertexIndices[i];
			unsigned int uvIndex = uvIndices[i];
			unsigned int normalIndex = normalIndices[i];

			glm::vec3 vertex = temp_vertices[vertexIndex - 1];
			glm::vec2 uv = temp_uvs[uvIndex - 1];
			glm::vec3 normal = temp_normals[normalIndex - 1];

			out_vertices.push_back(vertex);
			out_uvs.push_back(uv);
			out_normals.push_back(normal);
	}

	file.close();

	return true;
}

Geometry setupGeometry(const char* filepath)
{
    std::vector<GLfloat> vertices;
    std::vector<glm::vec3> vert;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    loadObject(filepath, vert, uvs, normals);

    vertices.reserve(vert.size() * 8); 
    for (size_t i = 0; i < vert.size(); ++i)
    {
        vertices.insert(vertices.end(), {vert[i].x, vert[i].y, vert[i].z, 1.0f, 0.0f, 0.0f, uvs[i].x, uvs[i].y});
    }

    GLuint VBO, VAO;

    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);

    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Geometry geom;
    geom.VAO = VAO;
    geom.vertexCount = vertices.size() / 6;

    string basePath = string(filepath).substr(0, string(filepath).find_last_of("/"));
    string mtlPath = basePath + "/" + mtlFilePath;
    string textureFile = loadMTL(mtlPath);

    if (!textureFile.empty())
    {
        string fullTexturePath = basePath + "/" + textureFile;
        geom.textureID = loadTexture(fullTexturePath);
        geom.textureFilePath = fullTexturePath;
    }

    return geom;
}

string loadMTL(const string& path)
{
    ifstream mtlFile(path);
    if (!mtlFile)
    {
        cerr << "Failed to open MTL file: " << path << endl;
        return "";
    }

    string line, texturePath;
    while (getline(mtlFile, line))
    {
        istringstream iss(line);
        string keyword;
        iss >> keyword;

        if (keyword == "map_Kd")
        {
            iss >> texturePath;
            break;
        }
    }
    mtlFile.close();

    if (texturePath.empty())
    {
        cerr << "No diffuse texture found in MTL file: " << path << endl;
    }
    return texturePath;
}