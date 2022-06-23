#pragma once

// C++ includes
#include <memory>

// GLEW
#include <GL/glew.h>

// SDL
#include <SDL.h>
#include <SDL_opengl.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform2.hpp>

#include "Includes/ProgramObject.h"
#include "Includes/BufferObject.h"
#include "Includes/VertexArrayObject.h"
#include "Includes/TextureObject.h"

#include "Includes/Mesh_OGL3.h"
#include "Includes/gCamera.h"

class CMyApp
{
public:
	CMyApp(void);
	~CMyApp(void);

	bool Init();
	void Clean();

	void Update();
	void Render();

	void KeyboardDown(SDL_KeyboardEvent&);
	void KeyboardUp(SDL_KeyboardEvent&);
	void MouseMove(SDL_MouseMotionEvent&);
	void MouseDown(SDL_MouseButtonEvent&);
	void MouseUp(SDL_MouseButtonEvent&);
	void MouseWheel(SDL_MouseWheelEvent&);
	void Resize(int, int);
protected:
	// FBO creating function
	void CreateFrameBuffer(int width, int height);
	void DrawScene(const glm::mat4& viewProj, ProgramObject& program, bool shadow);
	void DrawSkyBox(const glm::mat4& viewProj, ProgramObject& program);

	// variables for shaders
	ProgramObject		m_program;			// basic program for shaders
	ProgramObject		m_programSkybox;	// skybox shaders
	ProgramObject		m_shadowProgram;	// A deffered shader program to draw point lightsources

	Texture2D			m_textureMetal;
	Texture2D			m_texture;
	TextureCubeMap		m_skyboxTexture;

	VertexArrayObject	m_vao;
	VertexArrayObject	m_cube_vao;				
	gCamera				m_camera;

	std::unique_ptr<Mesh>	m_mesh;

	int	m_width  = 1024; 
	int m_height = 1024;

	glm::vec3 m_light_dir = glm::normalize(glm::vec3(0, -1, -1));
	glm::vec3 m_light_pos = glm::vec3(0, 10, 0);
	glm::mat4 m_light_mvp;
	float	m_filterWeight{};

	// stuffs for the FBO
	bool m_frameBufferCreated{ false };
	GLuint m_shadow_texture;
	GLuint m_frameBuffer;
	GLuint m_diffuseBuffer;
	GLuint m_normalBuffer;
	GLuint m_position_Buffer;
	GLuint m_depthBuffer;
};

