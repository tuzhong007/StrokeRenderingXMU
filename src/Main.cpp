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
using glm::vec4;
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


//-------------------Render Choices-----------------------


//-----------------global variables for interaction-------------------------
VBO* gVBO = nullptr; // VBO for storing vertex data of position, texture coordinates, and color
VBO* arcVBO = nullptr; // VBO for storing vertex data of accumulated arc length for piecewise quadratic curves
int ctlPtCnt = 0; // Count of control points for the current piecewise quadratic curve
int strokeWidth = 50; // Width of the stroke in pixels
int dash_l1 = 50; // Length of the dash segment in pixels
int dash_l2 = 10; // Gap length in pixels
Shader solidProgram, dashProgram, textureProgram; // Shader programs for different rendering tests
Shader *quadraticProgram = nullptr; // Pointer to the currently active shader program
Texture *currentTexture; // Texture for the quadratic curves
mat4 model, view, projection; // Transformation matrices for the rendering

#if INTERACTION_MODE == NO_INTERACTION
// Control vertex attributes for quadratic curves
vector<Vertex> vertices =
{ //     COORDINATES     /        TexCoords      /   Colors  //
	Vertex { vec3(200.f, 100.f, 0.0f),     	vec2(0.0f, 0.0f),		vec3(1.0f, 0.0f, 0.0f)},
	Vertex { vec3(400.f, 800.f, 0.0f),     	vec2(0.5f, 0.0f),		vec3(0.0f, 1.0f, 0.0f)},
	Vertex { vec3(600.f, 100.f, 0.0f),     	vec2(1.0f, 1.0f),		vec3(0.0f, 0.0f, 1.0f)},
};
// Vector to store accumulated arc length for piecewise quadratic curves
vector<float> accumulatedArcLength = { 0.0f, 0.0f, 0.0f }; // Initialize accumulated arc length for the three vertices
#elif INTERACTION_MODE == INTERACTION_ENABLED
// Start with an empty vector of vertices
vector<Vertex> vertices;
vector<float> accumulatedArcLength;
#endif
//-----------------global variables for interaction-------------------------


// Define the type of test to run
#define SOLID_TEST 0
#define DASH_TEST 1
#define TEXTURE_TEST 2
#define TEST_TYPE SOLID_TEST

// Get the analytic arclength of the quadratic curve
// https://stackoverflow.com/questions/11854907/calculate-the-length-of-a-segment-of-a-quadratic-bezier
float getArcLength(vec2 p0, vec2 p1, vec2 p2) {
	float x0 = p0.x, y0 = p0.y, x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y;
	float ax, ay, bx, by, A, B, C, b, c, u, k, L;
	// check if the quadratic curve is degenerate, or p0p1 and p1p2 are parallel
	if (abs((x1 - x0) * (y2 - y1) - (x2 - x1) * (y1 - y0)) < 1e-3) {
		return length(p2 - p0);
	}
	ax = x0 - x1 - x1 + x2;
	ay = y0 - y1 - y1 + y2;
	bx = x1 + x1 - x0 - x0;
	by = y1 + y1 - y0 - y0;
	A = 4.0 * ((ax * ax) + (ay * ay));
	B = 4.0 * ((ax * bx) + (ay * by));
	C = (bx * bx) + (by * by);
	b = B / (2.0 * A);
	c = C / A;
	u = 1.0f + b;
	k = c - (b * b);
	L = 0.5 * sqrt(A) *
		(
			(u * sqrt((u * u) + k))
			- (b * sqrt((b * b) + k))
			+ (k * log(abs((u + sqrt((u * u) + k)) / (b + sqrt((b * b) + k)))))
			);
	return L;
}

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

// Function to get the world position of the cursor based on mouse coordinates to address the issue of the cursor position being in screen space rather than world space
vec3 getCursorWorldPosition(double xpos, double ypos)
{
	// Convert to Normalized Device Coordinates (NDC) [-1, 1]
	float ndcX = (2.0f * float(xpos)) / width - 1.0f;
	float ndcY = 1.0f - (2.0f * float(ypos)) / height;  // flip Y axis for OpenGL
	float ndcZ = 0.0f; 

	vec4 ndcPos(ndcX, ndcY, ndcZ, 1.0f);

	// Compute inverse of combined projection * view matrix
	mat4 invPV = glm::inverse(projection * view);

	// Unproject to world space
	vec4 worldPos = invPV * ndcPos;
	if (worldPos.w != 0.0f)
		worldPos /= worldPos.w;

	// Compute inverse model matrix to convert to object local space (undo translation, rotation, scale)
	mat4 invModel = glm::inverse(model);
	vec4 localPos = invModel * worldPos;

	return vec3(localPos);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		vec3 localPos = getCursorWorldPosition(xpos, ypos);
		xpos = localPos.x; // Get the x coordinate in world space
		ypos = localPos.y; // Get the y coordinate in world space
		if (ctlPtCnt < 3)
		{ 
			// If we have less than 3 vertices, insert a new vertex at the clicked position
			insertNewVertex(static_cast<float>(xpos), static_cast<float>(ypos));
			// Create a new vertex at clicked position
			if (ctlPtCnt == 3)
			{
				accumulatedArcLength.push_back(0.0f); // Initialize accumulated arc length for the first three vertices
				accumulatedArcLength.push_back(0.0f);
				accumulatedArcLength.push_back(0.0f);
				
			}
		}
		else
		{
			Vertex lastVertex = vertices[vertices.size() - 1];
			Vertex secondLastVertex = vertices[vertices.size() - 2];
			Vertex thirdLastVertex = vertices[vertices.size() - 3];
			// Calculate the arc length for the last three vertices
			float arcLength = getArcLength(
				vec2(thirdLastVertex.position.x, thirdLastVertex.position.y),
				vec2(secondLastVertex.position.x, secondLastVertex.position.y),
				vec2(lastVertex.position.x, lastVertex.position.y)
			) + accumulatedArcLength.back();
			// Update the accumulated arc length for the vertices
			accumulatedArcLength.push_back(arcLength);
			accumulatedArcLength.push_back(arcLength);
			accumulatedArcLength.push_back(arcLength);

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

		if ((ctlPtCnt % 3) == 0)
		{
			// Update VBO data for position, texture coordinates, and color
			gVBO->Bind();
			glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
			gVBO->Unbind();
			// Update VBO data for accumulated arc length
			arcVBO->Bind();
			glBufferData(GL_ARRAY_BUFFER, accumulatedArcLength.size() * sizeof(float), accumulatedArcLength.data(), GL_DYNAMIC_DRAW);
			arcVBO->Unbind();
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		// Right mouse button pressed - start with a new piecewise quadratic curve
		ctlPtCnt = 0;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)  // Only trigger on press
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
		case GLFW_KEY_C:
			// Clear the last segment of piecewise quadratic curve
			ctlPtCnt -= 3;
			if (ctlPtCnt < 0)
				ctlPtCnt = 0; // Ensure ctlPtCnt does not go negative
			// Remove the last three vertices and their corresponding accumulated arc length
			if (vertices.size() >= 3)
			{
				vertices.erase(vertices.end() - 3, vertices.end());
				accumulatedArcLength.erase(accumulatedArcLength.end() - 3, accumulatedArcLength.end());
			}
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

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Example: scale model up/down
	float scaleChange = 1.0f + (float)yoffset * 0.1f;
	model = glm::scale(model, vec3(scaleChange, scaleChange, scaleChange));
}



int main()
{
// OpenGL initialization
	std::cout << "GPU-Accelerated Rendering of Vector Strokes with Piecewise Quadratic Approximation, by Xuhai Chen, Guangze Zhang, Wanyi Wang, Prof. Juan Cao and Prof. Zhonggui Chen from Xiamen University." << std::endl << std::endl;
	std::cout << "Interaction Notes:" << std::endl;
	std::cout << "1. Left-click to add new control points to the current piecewise quadratic curve. The first three points define the initial curve segment. Each additional click adds a new segment that connects smoothly (C1 continuity) to the previous curve."<< std::endl << std::endl;
	std::cout << "2. Right-click to start a new piecewise quadratic curve." << std::endl << std::endl;
	std::cout << "3. Use the scroll wheel to zoom in or zoom out." << std::endl << std::endl;
	std::cout << "4. Scroll to zoom in or zoom out." << std::endl << std::endl;
	std::cout << "5. Press keys 1, 2, or 3 to switch between solid stroke, dashed stroke, and textured stroke modes." << std::endl << std::endl;;
	std::cout << "6. Press w, s, or g to increase stroke width, dash segment length, and gap length, respectively. Hold Shift while pressing these keys to decrease the corresponding values." << std::endl;
	glfwInit();
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	glfwWindowHint(GLFW_SAMPLES, 8);  // Request 8x MSAA
	GLFWwindow* window = glfwCreateWindow(width, height, "GPU-Accelerated Rendering of Vector Strokes with Piecewise Quadratic Approximation", NULL, NULL);
	glfwSetWindowPos(window, 100, 100);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);

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

// Create a VAO and VBO for the control points of the quadratic curves
	VAO VAO1;
	VAO1.Bind();

	VBO VBO1(vertices), VBO2(accumulatedArcLength);
	gVBO = &VBO1; // Store the VBO pointer globally for mouse interaction
	arcVBO = &VBO2; // Store the arc length VBO pointer globally for mouse interaction
	// Links VBO attributes such as coordinates and colors to VAO
	VAO1.LinkAttrib(*gVBO, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(*gVBO, 1, 2, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(*gVBO, 2, 3, GL_FLOAT, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	VAO1.LinkAttrib(*arcVBO, 3, 1, GL_FLOAT, sizeof(float), (void*)0);
	VAO1.Unbind();
	gVBO->Unbind();

// Shader programs for different rendering tests
	solidProgram = Shader("../shaders/quadratic.vert", "../shaders/quadratic.frag", "../shaders/quadratic.geom", "../shaders/quadratic.tesc", "../shaders/quadratic.tese");
	// dash rendering program
	dashProgram = Shader("../shaders/quadratic.vert", "../shaders/quadratic.frag", "../shaders/dash_parallel.geom", "../shaders/dash_parallel.tesc", "../shaders/dash_parallel.tese");
	// texture mapping program
	textureProgram = Shader("../shaders/quadratic.vert", "../shaders/quadratic_texture.frag", "../shaders/quadratic_texture.geom", "../shaders/quadratic_texture.tesc", "../shaders/quadratic_texture.tese");
	quadraticProgram = &solidProgram; // Set the default shader program to solid rendering
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