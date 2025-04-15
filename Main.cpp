//------- Ignore this ----------
#include<filesystem>
namespace fs = std::filesystem;
//------------------------------

//#include"Model.h"
#include"VAO.h"
#include"EBO.h"
#include"Texture.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glad/glad.h"
#include <iostream>

const unsigned int width = 800;
const unsigned int height = 800;

using std::vector;
using glm::vec2;
using glm::vec3;
using glm::mat4;

vector<Vertex> vertices =
{ //     COORDINATES     /        TexCoords      /   Colors  //
	Vertex { vec3( 200.f, 100.f, 0.0f),     	vec2(0.0f, 0.0f),		vec3(1.0f, 0.0f, 0.0f)},
	Vertex { vec3( 400.f, 500.f, 0.0f),     	vec2(0.5f, 0.0f),		vec3(0.0f, 1.0f, 0.0f)},
	Vertex { vec3( 600.f, 100.f, 0.0f),     	vec2(1.0f, 1.0f),		vec3(0.0f, 0.0f, 1.0f)},
}; 

vector<GLuint> indices =
{
	0, 1, 2, 
};

int main()
{

	std::cout << "Working directory: "
		<< std::filesystem::current_path() << std::endl;

	// Initialize GLFW
	glfwInit();

	// Tell GLFW what version of OpenGL we are using 
	// In this case we are using OpenGL 4.5
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	// Tell GLFW we are using the CORE profile
	// So that means we only have the modern functions
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	
	// Create a GLFWwindow object of 800 by 800 pixels, naming it "YoutubeOpenGL"
	GLFWwindow* window = glfwCreateWindow(width, height, "Curvature Guided Adaptive Tessellation", NULL, NULL);
	// Error check if the window fails to create
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	// Introduce the window into the current context
	glfwMakeContextCurrent(window);

	//Load GLAD so it configures OpenGL
	gladLoadGL();
	// Specify the viewport of OpenGL in the Window
	// In this case the viewport goes from x = 0, y = 0, to x = 800, y = 800
	glViewport(0, 0, width, height);

	// Generates Vertex Array Object and binds it
	VAO VAO1;
	VAO1.Bind();

	// Generates Vertex Buffer Object and links it to vertices
	VBO VBO1(vertices);
	// Generates Element Buffer Object and links it to indices
	EBO EBO1(indices);

	// Links VBO attributes such as coordinates and colors to VAO
	VAO1.LinkAttrib(VBO1, 0, 3, GL_FLOAT, 8 * sizeof(float), (void*)0);
	VAO1.LinkAttrib(VBO1, 1, 2, GL_FLOAT, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	VAO1.LinkAttrib(VBO1, 2, 3, GL_FLOAT, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	// Unbind all to prevent accidentally modifying them
	VAO1.Unbind();
	VBO1.Unbind();
	EBO1.Unbind();



	// Generates Shader object using shaders default.vert and default.frag
	//Shader quadraticProgram("quadratic.vert", "quadratic.frag", "quadratic.geom", "quadratic.tesc", "quadratic.tese");
	// dash rendering program
	Shader quadraticProgram("quadratic.vert", "quadratic.frag", "dash.geom", "dash.tesc", "dash.tese");
	std::string parentDir = (fs::current_path().fs::path::parent_path()).string();
	std::string texPath = "/Resources/YoutubeOpenGL 6 - Textures/";

	// Texture
	Texture popCat((parentDir + texPath + "pop_cat.png").c_str(), GL_TEXTURE_2D, GL_TEXTURE0, GL_RGBA, GL_UNSIGNED_BYTE);
	popCat.texUnit(quadraticProgram, "tex0", 0);
	popCat.Bind();


	quadraticProgram.Activate();
	mat4 model, view, projection;
	model = mat4(1.0f);
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	projection = glm::ortho(0.0f, width * 1.0f, 0.0f, height * 1.0f, 0.1f, 2.0f);
	glUniformMatrix4fv(glGetUniformLocation(quadraticProgram.ID, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(quadraticProgram.ID, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(quadraticProgram.ID, "projection"), 1, GL_FALSE, &projection[0][0]);


	glDisable(GL_DEPTH_TEST);
	// Enables the Stencil Buffer
	//.glEnable(GL_STENCIL_TEST);
	glDisable(GL_STENCIL_TEST);
	// Sets rules for outcomes of stecil tests
	glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);

	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (!glfwWindowShouldClose(window))
	{
		// Specify the color of the background
		glClearColor(1, 1, 1, 1.0f);
		// Clean the back buffer and depth buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		//// Make it so the stencil test always passes
		//glStencilFunc(GL_ALWAYS, 1, 0xFF);
		//// Enable modifying of the stencil buffer
		//glStencilMask(0xFF);
		//glColorMask(0x00, 0x00, 0x00, 0x00);
		//// Draw the normal model
		////model.Draw(shaderProgram, camera);
		

		//glUniform1i(glGetUniformLocation(quadraticProgram.ID, "isStencil"), (int)1);
		//VAO1.Bind();
		//glPatchParameteri(GL_PATCH_VERTICES, 3);
		//glDrawArrays(GL_PATCHES, 0, indices.size());

		//// Make it so only the pixels without the value 1 pass the test
		//glStencilFunc(GL_EQUAL, 255, 0xFF);
		//// Disable modifying of the stencil buffer
		//glStencilMask(0x00);
		//glColorMask(0xFF, 0xFF, 0xFF, 0xFF);
		
		//glUniform1i(glGetUniformLocation(quadraticProgram.ID, "isStencil"), (int)0);
		glUniform1f(glGetUniformLocation(quadraticProgram.ID, "w"), (float)100);
		glUniform1f(glGetUniformLocation(quadraticProgram.ID, "l1"), (float)200);
		glUniform1f(glGetUniformLocation(quadraticProgram.ID, "l2"), (float)50);
		VAO1.Bind();
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		glDrawArrays(GL_PATCHES, 0, indices.size());


		//// Enable modifying of the stencil buffer
		//glStencilMask(0xFF);
		//// Clear stencil buffer
		//glStencilFunc(GL_ALWAYS, 0, 0xFF);


		////debug
		//glUniform1i(glGetUniformLocation(quadraticProgram.ID, "isStencil"), (int)0);
		//VAO1.Bind();
		//glPatchParameteri(GL_PATCH_VERTICES, 3);
		//glDrawArrays(GL_PATCHES, 0, indices.size());


		// Swap the back buffer with the front buffer
		glfwSwapBuffers(window);
		// Take care of all GLFW events
		glfwPollEvents();
	}



	// Delete all the objects we've created
	quadraticProgram.Delete();
	// Delete window before ending the program
	glfwDestroyWindow(window);
	// Terminate GLFW before ending the program
	glfwTerminate();
	return 0;
}