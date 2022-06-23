#include "MyApp.h"

#include <math.h>
#include <vector>

#include <array>
#include <list>
#include <tuple>

#include "imgui/imgui.h"
#include "Includes/ObjParser_OGL3.h"

CMyApp::CMyApp(void){}

CMyApp::~CMyApp(void){}

bool CMyApp::Init()
{
	glClearColor(0.2, 0.4, 0.7, 1);	// Clear color is bluish
	glEnable(GL_CULL_FACE);			// Drop faces looking backwards
	glEnable(GL_DEPTH_TEST);		// Enable depth test
	glEnable(GL_DEPTH_CLAMP);		// Enable depth clamp

	// Initialize shaders
	m_program.Init({				// Default shader
		{ GL_VERTEX_SHADER,			"Shaders/myVert.vert" },
		{ GL_FRAGMENT_SHADER,		"Shaders/myFrag.frag" }
	},{
		{ 0, "vs_in_pos" },
		{ 1, "vs_in_normal" },
		{ 2, "vs_out_tex0" },
	});
	m_programSkybox.Init({			// Skybox shader
		{ GL_VERTEX_SHADER,			"Shaders/skybox.vert" },
		{ GL_FRAGMENT_SHADER,		"Shaders/skybox.frag" }
	});
	m_shadowProgram.Init({		// Shadowmap shader
		{ GL_VERTEX_SHADER,		"Shaders/shadow_map.vert" },
		{ GL_FRAGMENT_SHADER,	"Shaders/shadow_map.frag" }
	});

	//-------------
	// Skybox stuff
	//-------------
	if (glGetError() != GL_NO_ERROR) { std::cout << "Error after shader compilation.\n"; exit(1); }

	// Defining geometry
	// Position of vertices:
	static ArrayBuffer cube_positions(std::vector<glm::vec3>{
		/*back face:*/	glm::vec3(-1, -1, -1), glm::vec3(1, -1, -1), glm::vec3(1, 1, -1), glm::vec3(-1, 1, -1),
		/*front face:*/	glm::vec3(-1, -1, 1),  glm::vec3(1, -1,  1), glm::vec3(1, 1,  1), glm::vec3(-1, 1,  1),
	});

	// And the indices which the primitives are constructed by (from the arrays defined above) - prepared to draw them as a triangle list
	static IndexBuffer cube_indices(std::vector<uint16_t>{
		/*back: */ 0, 1, 2, 2, 3, 0, /*front:*/ 4, 6, 5, 6, 4, 7,
		/*left: */ 0, 3, 4, 4, 3, 7, /*right:*/ 1, 5, 2, 5, 6, 2,
		/*bottom:*/1, 0, 4, 1, 4, 5, /*top:  */ 3, 2, 6, 3, 6, 7,
	});

	// Registering geometry in VAO
	m_cube_vao.Init({ {CreateAttribute<0, glm::vec3, 0, sizeof(glm::vec3)>, cube_positions} }, cube_indices);

	// Skybox cubemap
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	m_skyboxTexture.AttachFromFile("Assets/xpos.png", false, GL_TEXTURE_CUBE_MAP_POSITIVE_X);
	m_skyboxTexture.AttachFromFile("Assets/xneg.png", false, GL_TEXTURE_CUBE_MAP_NEGATIVE_X);
	m_skyboxTexture.AttachFromFile("Assets/ypos.png", false, GL_TEXTURE_CUBE_MAP_POSITIVE_Y);
	m_skyboxTexture.AttachFromFile("Assets/yneg.png", false, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y);
	m_skyboxTexture.AttachFromFile("Assets/zpos.png", false, GL_TEXTURE_CUBE_MAP_POSITIVE_Z);
	m_skyboxTexture.AttachFromFile("Assets/zneg.png", true, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	ArrayBuffer positions(std::vector<glm::vec3>{glm::vec3(-20, 0, -20), glm::vec3(-20, 0, 20), glm::vec3(20, 0, -20), glm::vec3(20, 0, 20)});
	ArrayBuffer normals(std::vector<glm::vec3>{glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0)});
	ArrayBuffer texture(std::vector<glm::vec2>{glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(1, 0), glm::vec2(1, 1)});
	IndexBuffer indices(std::vector<uint16_t>{0, 1, 2, 2, 1, 3});

	m_vao.Bind();
	m_vao.AddAttribute(CreateAttribute<  0, glm::vec3, 0, sizeof(glm::vec3)>, positions);
	m_vao.AddAttribute(CreateAttribute<  1, glm::vec3, 0, sizeof(glm::vec3)>, normals);
	m_vao.AddAttribute(CreateAttribute<  2, glm::vec2, 0, sizeof(glm::vec2)>, texture);
	m_vao.SetIndices(indices);
	m_vao.Unbind();

	//-------------
	// Mesh and tex
	//-------------
	// Loading mesh
	m_textureMetal.FromFile("Assets/texture.png");

	// Loading texture
	m_mesh = ObjParser::parse("Assets/Suzanne.obj");

	// Camera
	m_camera.SetProj(45.0f, 640.0f / 480.0f, 0.01f, 1000.0f);

	// FBO - initial
	CreateFrameBuffer(640, 480);

	return true;
}

void CMyApp::Clean()
{
	if (m_frameBufferCreated)
	{
		glDeleteTextures(1, &m_diffuseBuffer);
		glDeleteTextures(1, &m_normalBuffer);
		glDeleteTextures(1, &m_position_Buffer);
		glDeleteRenderbuffers(1, &m_depthBuffer);
		glDeleteFramebuffers(1, &m_frameBuffer);
	}
}

void CMyApp::Update()
{
	static Uint32 last_time = SDL_GetTicks();
	float delta_time = (SDL_GetTicks() - last_time) / 1000.0f;

	m_camera.Update(delta_time);

	last_time = SDL_GetTicks();
}

void CMyApp::DrawSkyBox(const glm::mat4& viewProj, ProgramObject& program)
{
	// Save the last Z-test, namely the relation by which we update the pixel.
	GLint prevDepthFnc;
	glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFnc);

	// Now we use less-then-or-equal, because we push everything to the far clipping plane
	glDepthFunc(GL_LEQUAL);
	m_cube_vao.Bind();
	m_programSkybox.Use();
	m_programSkybox.SetUniform("MVP", m_camera.GetViewProj() * glm::translate(m_camera.GetEye()));

	// Set the cube map texture to the 0th sampler (in the shader too)
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_skyboxTexture);
	glUniform1i(m_programSkybox.GetLocation("skyboxTexture"), 0);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);
	glDepthFunc(prevDepthFnc); // Finally set it back
}

void CMyApp::DrawScene(const glm::mat4& viewProj, ProgramObject& program, bool shadow = false)
{
	program.Use();

	if (!shadow) {
		program.SetTexture("textureShadow", 1, m_shadow_texture); // depth values
		program.SetTexture("texImage", 0, m_textureMetal);
		program.SetUniform("shadowVP", m_light_mvp); //so we can read the shadow map
		program.SetUniform("toLight", -m_light_dir);
	}

	// Drawing the plane underneath
	program.SetUniform("MVP", viewProj * glm::mat4(1));
	if (!shadow) {
		program.SetUniform("world", glm::mat4(1));
		program.SetUniform("worldIT", glm::mat4(1));
		program.SetUniform("Kd", glm::vec4(0.1, 0.9, 0.3, 1));
	}

	m_vao.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	//Draws exactly the same but uses index buffer:
	m_vao.Unbind();

	// Suzanne wall
	float t = SDL_GetTicks() / 1000.f;
	for (int i = -1; i <= 1; ++i) 
	{
		for (int j = -1; j <= 1; ++j)
		{
			glm::mat4 suzanneWorld = glm::translate(glm::vec3(4 * i, 4 * (j + 1), sinf(t * 2 * M_PI * i * j)));
			program.SetUniform("MVP", viewProj * suzanneWorld);
			program.SetUniform("world", suzanneWorld);
			program.SetUniform("worldIT", glm::transpose(glm::inverse(suzanneWorld)));
			m_mesh->draw();
		}
	}

	program.Unuse();
}

// Shadow map content (partly) from: https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
void CMyApp::Render()
{
	//------------------------
	// Render from light's POV
	//------------------------
	// Draw scene to shadow map
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);	// FBO that has the shadow map attached to it
	glViewport(0, 0, 1024, 1024);						// Set which pixels to render to within this fbo.
	glClear(GL_DEPTH_BUFFER_BIT);						// Clear depth values 

	// Light properties
	glm::vec3 lightInvDir = glm::vec3(0.5f, 7, 7);
	glm::vec3 lightPos(0, 15, 50);
	glm::mat4 depthProjectionMatrix = glm::perspective<float>(glm::radians(45.0f), 1.0f, 2.0f, 50.0f);
	glm::mat4 depthViewMatrix = glm::lookAt(lightPos, lightPos - lightInvDir, glm::vec3(0, 1, 0));

	// Model and projection matrix
	glm::mat4 depthModelMatrix = glm::mat4(1.0);
	m_light_mvp = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;

	// Bias term 
	glm::mat4 biasMatrix(
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0);

	glm::mat4 depthBiasMVP = biasMatrix * m_light_mvp;
	DrawScene(depthBiasMVP, m_shadowProgram, true);		// Render

	//-------------------------
	// Render from camera's POV
	//-------------------------
	// Draw mesh to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);				// default framebuffer (the backbuffer)
	glViewport(0, 0, m_width, m_height);				// We need to set the render area back
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// clearing the default fbo
	DrawScene(m_camera.GetViewProj(), m_program, false);
	
	//------------------------
	//    Render the Skybox 
	//------------------------
	glEnable(GL_DEPTH_TEST);
	DrawSkyBox(m_camera.GetViewProj(), m_program);

	//------------------------
	//     Render the UI
	//------------------------
	ImGui::ShowTestWindow();
	ImGui::SetNextWindowPos(ImVec2(300, 400), ImGuiSetCond_FirstUseEver);
	ImGui::Begin("Test window");
	{
		ImGui::SliderFloat3("light_dir", &m_light_dir.x, -1.f, 1.f);
		m_light_dir = glm::normalize(m_light_dir);
		ImGui::Image((ImTextureID)m_shadow_texture, ImVec2(256, 256));
	}
	ImGui::End();
}

void CMyApp::KeyboardDown(SDL_KeyboardEvent& key)
{
	m_camera.KeyboardDown(key);
}

void CMyApp::KeyboardUp(SDL_KeyboardEvent& key)
{
	m_camera.KeyboardUp(key);
}

void CMyApp::MouseMove(SDL_MouseMotionEvent& mouse)
{
	m_camera.MouseMove(mouse);
}

void CMyApp::MouseDown(SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseUp(SDL_MouseButtonEvent& mouse)
{
}

void CMyApp::MouseWheel(SDL_MouseWheelEvent& wheel)
{
}

// _w and _h are the width and height of the window's size
void CMyApp::Resize(int _w, int _h)
{
	glViewport(0, 0, _w, _h );

	m_camera.Resize(_w, _h);

	CreateFrameBuffer(_w, _h);
}

inline void setTexture2DParameters(GLenum magfilter = GL_LINEAR, GLenum minfilter = GL_LINEAR, GLenum wrap_s = GL_CLAMP_TO_EDGE, GLenum wrap_t = GL_CLAMP_TO_EDGE)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minfilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
}

void CMyApp::CreateFrameBuffer(int width, int height)
{
	// Framebuffer creation code from: 03 - Shadows project
	// Clear if the function is not being called for the first time
	if (m_frameBufferCreated) {
		glDeleteTextures(1, &m_shadow_texture);
		glDeleteFramebuffers(1, &m_frameBuffer);
	}

	glGenFramebuffers(1, &m_frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

	glGenTextures(1, &m_shadow_texture); // Create texture holding the depth components
	glBindTexture(GL_TEXTURE_2D, m_shadow_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	setTexture2DParameters(GL_NEAREST, GL_NEAREST); //so its shorter

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadow_texture, 0);
	if (glGetError() != GL_NO_ERROR) {
		std::cout << "Error creating depth attachment" << std::endl;
		exit(1);
	}

	glDrawBuffer(GL_NONE); // No need for any color output!

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER); // -- Completeness check
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Incomplete framebuffer (";
		switch (status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";		break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
		case GL_FRAMEBUFFER_UNSUPPORTED:					std::cout << "GL_FRAMEBUFFER_UNSUPPORTED";					break;
		}
		std::cout << ")" << std::endl;
		char ch; std::cin >> ch;
		exit(1);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);	// -- Unbind framebuffer
	m_frameBufferCreated = true;
}