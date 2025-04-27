///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	 //Movement speed that can be adjusted with mouse scroll
	 float gMovementSpeed = 2.5f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	//Add mouse scroll wheel callback for adjusting movement speed
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Wheel_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// When the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}
	// Calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // Reversed since y-coordinates go from bottom to top

	// Store current positions for next frame calculation
	gLastX = xMousePos;
	gLastY = yMousePos;

	// Apply a sensitivity factor to make mouse movement more responsive
	const float sensitivity = 9.0f; //*//
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	// Move the 3D camera according to the calculated offsets
	if (g_pCamera != nullptr)
	{
		g_pCamera->ProcessMouseMovement(xOffset, yOffset);
	}
}

/***********************************************************
 *  Mouse_Scroll_Wheel_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse wheel is scrolled within the active window.
 *  Used to adjust the camera movement speed.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Wheel_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// Use mouse wheel to adjust the movement speed
	if (yOffset > 0)
	{
		// Scroll up - increase movement speed
		gMovementSpeed += 0.5f;
	}
	else if (yOffset < 0)
	{
		// Scroll down - decrease movement speed but maintain a minimum
		gMovementSpeed = std::max(0.5f, gMovementSpeed - 0.5f);
	}

	// Print current speed to console for feedback
	std::cout << "Camera movement speed: " << gMovementSpeed << std::endl;
}


/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// If the camera object is null, then exit this method
	if (g_pCamera == nullptr)
	{
		return;
	}

	// Calculate actual movement speed based on deltaTime and adjusted speed
	float actualSpeed = gMovementSpeed * gDeltaTime;

	// WASD keys for forward, backward, left, and right movement
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		// Move forward (zoom in)
		g_pCamera->ProcessKeyboard(FORWARD, actualSpeed);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		// Move backward (zoom out)
		g_pCamera->ProcessKeyboard(BACKWARD, actualSpeed);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		// Move left
		g_pCamera->ProcessKeyboard(LEFT, actualSpeed);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		// Move right
		g_pCamera->ProcessKeyboard(RIGHT, actualSpeed);
	}

	// QE keys for vertical movement (up and down)
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		// Move upward
		g_pCamera->Position += glm::vec3(0.0f, actualSpeed, 0.0f);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		// Move downward
		g_pCamera->Position -= glm::vec3(0.0f, actualSpeed, 0.0f);
	}

	// Toggle between perspective and orthographic projection with P and O keys
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		bOrthographicProjection = false;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		bOrthographicProjection = true;
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix based on current mode(perspective or orthographic)
	if (!bOrthographicProjection)
	{
		// Perspective projection (3D view)
		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f
		);
	}
	 else
	 {
		 // Orthographic projection (2D view)
		 // For orthographic view, we want to look directly at the object
		 // Adjust these values to fit your scene
		 float orthoZoom = g_pCamera->Zoom / 10.0f;
		 float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

		 projection = glm::ortho(
			 -orthoZoom * aspectRatio,  // Left
			 orthoZoom * aspectRatio,   // Right
			 -orthoZoom,                // Bottom
			 orthoZoom,                 // Top
			 0.1f,                      // Near clipping plane
			 100.0f                     // Far clipping plane
		 );
	 }

	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}