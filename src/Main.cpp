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

const unsigned int width = 1600;
const unsigned int height = 900;

using std::vector;
using glm::vec2;
using glm::vec3;
using glm::mat4;

//-------------------Render Choices-----------------------
// Define whether to display FPS or not
#define NO_FPS_DISPLAY 0
#define FPS_DISPLAY 1
#define FPS_MODE NO_FPS_DISPLAY

// Define the interaction mode
#define NO_INTERACTION 0
#define INTERACTION_ENABLED 1
#define INTERACTION_MODE INTERACTION_ENABLED

#if INTERACTION_MODE == INTERACTION_ENABLED
// Control vertex attributes for quadratic curves
vector<Vertex> vertices =
{ //     COORDINATES     /        TexCoords      /   Colors  //
	Vertex { vec3(200.f, 100.f, 0.0f),     	vec2(0.0f, 0.0f),		vec3(1.0f, 0.0f, 0.0f)},
	Vertex { vec3(400.f, 800.f, 0.0f),     	vec2(0.5f, 0.0f),		vec3(0.0f, 1.0f, 0.0f)},
	Vertex { vec3(600.f, 100.f, 0.0f),     	vec2(1.0f, 1.0f),		vec3(0.0f, 0.0f, 1.0f)},
};
#elif INTERACTION_MODE == INTERACTION_ENABLED
// Start with an empty vector of vertices
vector<Vertex> vertices;
#endif
//-------------------Render Choices-----------------------


//-----------------global variables for interaction-------------------------
VBO* gVBO = nullptr;
Shader* gShader = nullptr;
int ctlPtCnt = 0; // Count of control points for quadratic curves
int strokeWidth = 100; // Width of the stroke in pixels
int dash_l1 = 50; // Length of the dash segment in pixels
int dash_l2 = 10; // Gap length in pixels
Shader solidProgram, dashProgram, textureProgram; // Shader programs for different rendering tests
Shader *quadraticProgram = nullptr; // Pointer to the currently active shader program
Texture *currentTexture; // Texture for the quadratic curves
mat4 model, view, projection;
//-----------------global variables for interaction-------------------------


// Define the type of test to run
#define SOLID_TEST 0
#define DASH_TEST 1
#define TEXTURE_TEST 2
#define TEST_TYPE SOLID_TEST

void insertNewVertex(float x, float y)
{
	++ctlPtCnt; // Increment control point count
	Vertex newVertex;
	newVertex.position = vec3(x, y, 0.0f);
	switch (ctlPtCnt % 3) // Cycle through texUV for each vertex
	{
	case 0:
		newVertex.texUV = vec2(1.0f, 1.0f);
		break;
	case 1:
		newVertex.texUV = vec2(0.0f, 0.0f);
		break;
	case 2:
		newVertex.texUV = vec2(0.5f, 0.0f);
		break;
	default:
		break;
	}
	newVertex.color = vec3(1.0f, 0.0f, 0.0f);
	vertices.push_back(newVertex);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Convert y to match OpenGL coords (origin at bottom-left)
		ypos = height - ypos;
		if (ctlPtCnt < 3)
		{ 
			// If we have less than 3 vertices, insert a new vertex at the clicked position
			insertNewVertex(static_cast<float>(xpos), static_cast<float>(ypos));
			// Create a new vertex at clicked position
		}
		else
		{
			Vertex lastVertex = vertices.back();
			Vertex secondLastVertex = vertices[vertices.size() - 2];
			insertNewVertex(lastVertex.position.x, lastVertex.position.y);
			// Determine the position of the control point based on the last two vertices (C1 continuity at joints)
			float newX = 2 * lastVertex.position.x - secondLastVertex.position.x;
			float newY = 2 * lastVertex.position.y - secondLastVertex.position.y;
			// Insert a new vertex at the calculated position
			// This will create a new control point that is a reflection of the last vertex, ensuring C1 continuity at the joint
			insertNewVertex(newX, newY);
			// Insert a new vertex at the clicked position
			insertNewVertex(static_cast<float>(xpos), static_cast<float>(ypos));
		}

		// If we have 3 vertices, we can start rendering the quadratic curve
		if ((ctlPtCnt % 3) == 0)
		{
			// Update VBO data
			gVBO->Bind();
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
			gVBO->Unbind();
		}
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)  // Only trigger on press
	{
		bool shiftPressed = (mods & GLFW_MOD_SHIFT);
		switch (key)
		{
		case GLFW_KEY_C:

			break;
			

		case GLFW_KEY_G:
			if (shiftPressed)
				dash_l2 -= 1;
			else
				dash_l2 += 1;
			dash_l2 = std::clamp(dash_l2, 0, 100);
			break;

		case GLFW_KEY_S:
			if (shiftPressed)
				dash_l1 -= 1;
			else
				dash_l1 += 1;
			dash_l1 = std::clamp(dash_l1, 0, 100);
			break;

		case GLFW_KEY_W:
			if (shiftPressed)
				strokeWidth -= 1;
			else
				strokeWidth += 1;
			strokeWidth = std::clamp(strokeWidth, 0, 100);
			break;
			

		case GLFW_KEY_1:
			std::cout << "Switched to SOLID_TEST.\n";
			quadraticProgram = &solidProgram; // Set the shader program to solid rendering
			quadraticProgram->Activate();
			break;

		case GLFW_KEY_2:
			std::cout << "Switched to DASH_TEST.\n";
			quadraticProgram = &dashProgram; // Set the shader program to dash rendering
			quadraticProgram->Activate();
			break;

		case GLFW_KEY_3:
			std::cout << "Switched to TEXTURE_TEST.\n";
			quadraticProgram = &textureProgram; // Set the shader program to texture mapping
			quadraticProgram->Activate();
			currentTexture->texUnit(*quadraticProgram, "tex0", 0);
			currentTexture->Bind();
			break;

		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, true);
			break;
		}
	}
	// key continuation
	if (action == GLFW_REPEAT) // Only trigger on repeat
	{
		bool shiftPressed = (mods & GLFW_MOD_SHIFT);

		switch (key)
		{
		case GLFW_KEY_G:
			if (shiftPressed)
				dash_l2 -= 1;
			else
				dash_l2 += 1;
			dash_l2 = std::clamp(dash_l2, 0, 100);
			break;

		case GLFW_KEY_S:
			if (shiftPressed)
				dash_l1 -= 1;
			else
				dash_l1 += 1;
			dash_l1 = std::clamp(dash_l1, 0, 100);
			break;

		case GLFW_KEY_W:
			if (shiftPressed)
				strokeWidth -= 1;
			else
				strokeWidth += 1;
			strokeWidth = std::clamp(strokeWidth, 0, 100);
			break;
		}
	}
}

void init()
{

}


int main()
{
	std::cout << "Interaction Notes:" << std::endl;
	std::cout << "1. Use mouse to add new control points. The first three points define the first curve. Then, every click will generate a new curve that connects to the clicked position in a C1 continuous way at the joint of previous curve."<< std::endl << std::endl;
	std::cout << "2. Press key `1', `2', `3' to switch between solid stroke, dashed stroke and texture stroke." << std::endl << std::endl;;
	std::cout << "3. Press key `w', `s', `g' to increase stroke width, dashed segment length and gap length. If you want to decrease them, press `shift' at the same time with the keys." << std::endl;
	glfwInit();
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	glfwWindowHint(GLFW_SAMPLES, 8);  // Request 8x MSAA
	GLFWwindow* window = glfwCreateWindow(width, height, "GPU-Accelerated Rendering of Vector Strokes with Piecewise Quadratic Approximation", NULL, NULL);
	glfwSetWindowPos(window, 100, 100);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);

	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	gladLoadGL();
	glViewport(0, 0, width, height);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_MULTISAMPLE);
	int samples = 0;
	glGetIntegerv(GL_SAMPLES, &samples);
	printf("\nGL_SAMPLES = %d\n", samples);
	printf("Samples used by default framebuffer: %d\n", samples);

	glfwSwapInterval(0);


	VAO VAO1;
	VAO1.Bind();

	VBO VBO1(vertices);
	gVBO = &VBO1; // Store the VBO pointer globally for mouse interaction
	// Links VBO attributes such as coordinates and colors to VAO
	VAO1.LinkAttrib(*gVBO, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(*gVBO, 1, 2, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(*gVBO, 2, 3, GL_FLOAT, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	VAO1.Unbind();
	gVBO->Unbind();

// Shader programs for different rendering tests
	solidProgram = Shader("../shaders/quadratic.vert", "../shaders/quadratic.frag", "../shaders/quadratic.geom", "../shaders/quadratic.tesc", "../shaders/quadratic.tese");
	// dash rendering program
	dashProgram = Shader("../shaders/quadratic.vert", "../shaders/quadratic.frag", "../shaders/dash_parallel.geom", "../shaders/dash_parallel.tesc", "../shaders/dash_parallel.tese");
	// texture mapping program
	textureProgram = Shader("../shaders/quadratic.vert", "../shaders/quadratic_texture.frag", "../shaders/quadratic_texture.geom", "../shaders/quadratic_texture.tesc", "../shaders/quadratic_texture.tese");
	quadraticProgram = &dashProgram; // Set the default shader program to solid rendering
// Texture loading
	std::string parentDir = (fs::current_path().parent_path()).string();
	std::string texPath = "./texture/";
	Texture brickWallTex((parentDir + texPath + "slim-bricks-in-cement-wall-512x512.png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	currentTexture = &brickWallTex; // Set the current texture to the brick wall texture
	currentTexture->texUnit(*quadraticProgram, "tex0", 0);
	currentTexture->Bind();
	quadraticProgram->Activate();
// Transformation matrices
	model = mat4(1.0f);
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	projection = glm::ortho(0.0f, width * 1.0f, 0.0f, height * 1.0f, 0.1f, 2.0f);
	
	
#include <chrono>
	float avgTime = 0;
	int cnt = 0;
	while (!glfwWindowShouldClose(window))
	{
		auto start = std::chrono::high_resolution_clock::now();
		glClearColor(1, 1, 1, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT);

		glUniformMatrix4fv(glGetUniformLocation(quadraticProgram->ID, "model"), 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(quadraticProgram->ID, "view"), 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(quadraticProgram->ID, "projection"), 1, GL_FALSE, &projection[0][0]);
		glUniform1f(glGetUniformLocation(quadraticProgram->ID, "w"), (float)strokeWidth);
		glUniform1f(glGetUniformLocation(quadraticProgram->ID, "l1"), (float)dash_l1);
		glUniform1f(glGetUniformLocation(quadraticProgram->ID, "l2"), (float)dash_l2);

		VAO1.Bind();
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		//for (int i = 0; i < 2E3; ++i)
			glDrawArrays(GL_PATCHES, 0, vertices.size() - (vertices.size() % 3));
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
			if (FPS_MODE == FPS_DISPLAY)
			{
				std::cout << "FPS: " << 1000.0f / (avgTime / 100) << std::endl;
				std::cout << "Average time: " << avgTime / 100 << " ms" << std::endl;
			}
			
			avgTime = 0;
		}
		/*std::chrono::duration<float, std::milli> duration = end - start;
		std::cout << "Time: " << duration.count() << " ms" << std::endl;*/
	}

	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}