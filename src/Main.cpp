#include<filesystem>
#include "VAO.h"
#include "EBO.h"
#include "Texture.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glad/glad.h"
#include <iostream>
namespace fs = std::filesystem;

const unsigned int width = 800;
const unsigned int height = 800;

using std::vector;
using glm::vec2;
using glm::vec3;
using glm::mat4;

// Control vertex attributes for quadratic curves
vector<Vertex> vertices =
{ //     COORDINATES     /        TexCoords      /   Colors  //
	Vertex { vec3( 200.f, 100.f, 0.0f),     	vec2(0.0f, 0.0f),		vec3(1.0f, 0.0f, 0.0f)},
	Vertex { vec3( 400.f, 800.f, 0.0f),     	vec2(0.5f, 0.0f),		vec3(0.0f, 1.0f, 0.0f)},
	Vertex { vec3( 600.f, 100.f, 0.0f),     	vec2(1.0f, 1.0f),		vec3(0.0f, 0.0f, 1.0f)},
}; 

#define SOLID_TEST 0
#define DASH_TEST 1
#define TEXTURE_TEST 2

#define TEST_TYPE TEXTURE_TEST


int main()
{

	std::cout << "Working directory: "
		<< std::filesystem::current_path() << std::endl;

	glfwInit();
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	glfwWindowHint(GLFW_SAMPLES, 8);  // Request 8x MSAA
	GLFWwindow* window = glfwCreateWindow(width, height, "GPU-Accelerated Rendering of Vector Strokes with Piecewise Quadratic Approximation", NULL, NULL);
	glfwSetWindowPos(window, 100, 100);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();
	glViewport(0, 0, width, height);

	VAO VAO1;
	VAO1.Bind();

	VBO VBO1(vertices);

	// Links VBO attributes such as coordinates and colors to VAO
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(VBO1, 2, 3, GL_FLOAT, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	VAO1.Unbind();
	VBO1.Unbind();

#if TEST_TYPE == SOLID_TEST
	// stroke rendering program without special effects
	Shader quadraticProgram("../shaders/quadratic.vert", "../shaders/quadratic.frag", "../shaders/quadratic.geom", "../shaders/quadratic.tesc", "../shaders/quadratic.tese");
#elif TEST_TYPE == DASH_TEST
	// dash rendering program
	Shader quadraticProgram("../shaders/quadratic.vert", "../shaders/quadratic.frag", "../shaders/dash_parallel.geom", "../shaders/dash_parallel.tesc", "../shaders/dash_parallel.tese");
#elif TEST_TYPE == TEXTURE_TEST
	// texture mapping program
	Shader quadraticProgram("../shaders/quadratic.vert", "../shaders/quadratic_texture.frag", "../shaders/quadratic_texture.geom", "../shaders/quadratic_texture.tesc", "../shaders/quadratic_texture.tese");
#endif

	std::string parentDir = (fs::current_path().parent_path()).string();
	std::string texPath = "./texture/";
	//std::cout << "parent directory: " << parentDir << std::endl;
	// Texture
	Texture brickWallTex((parentDir + texPath + "slim-bricks-in-cement-wall-512x512.png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	brickWallTex.texUnit(quadraticProgram, "tex0", 0);
	brickWallTex.Bind();


	quadraticProgram.Activate();
	mat4 model, view, projection;
	model = mat4(1.0f);
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	projection = glm::ortho(0.0f, width * 1.0f, 0.0f, height * 1.0f, 0.1f, 2.0f);
	glUniformMatrix4fv(glGetUniformLocation(quadraticProgram.ID, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(quadraticProgram.ID, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(quadraticProgram.ID, "projection"), 1, GL_FALSE, &projection[0][0]);

	glEnable(GL_MULTISAMPLE);
	int samples = 0;
	glGetIntegerv(GL_SAMPLES, &samples);
	printf("GL_SAMPLES = %d\n", samples);

	printf("Samples used by default framebuffer: %d\n", samples);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glfwSwapInterval(0);
#include <chrono>
	float avgTime = 0;
	int cnt = 0;
	while (!glfwWindowShouldClose(window))
	{
		auto start = std::chrono::high_resolution_clock::now();
		// Specify the color of the background
		glClearColor(1, 1, 1, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT);
		glUniform1f(glGetUniformLocation(quadraticProgram.ID, "w"), (float)100);
		glUniform1f(glGetUniformLocation(quadraticProgram.ID, "l1"), (float)50);
		glUniform1f(glGetUniformLocation(quadraticProgram.ID, "l2"), (float)0);
		VAO1.Bind();
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		//for (int i = 0; i < 2E3; ++i)
			glDrawArrays(GL_PATCHES, 0, vertices.size());
		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
		glFinish();
		auto end = std::chrono::high_resolution_clock::now();
		avgTime += std::chrono::duration<float, std::milli>(end - start).count();
		++cnt;
		if (cnt % 100 == 0)
		{
			std::cout << "Average time: " << avgTime / 100 << " ms" << std::endl;
			avgTime = 0;
		}
		/*std::chrono::duration<float, std::milli> duration = end - start;
		std::cout << "Time: " << duration.count() << " ms" << std::endl;*/
	}


	// Delete all the objects we've created
	quadraticProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}