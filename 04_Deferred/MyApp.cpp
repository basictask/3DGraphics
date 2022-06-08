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

	m_program.Init({			// Shader for drawing geometries
		{ GL_VERTEX_SHADER,   "Shaders/myVert.vert" },
		{ GL_FRAGMENT_SHADER, "Shaders/myFrag.frag" }
	});
	m_programSkybox.Init({
		{ GL_VERTEX_SHADER,			"Shaders/skybox.vert" },
		{ GL_FRAGMENT_SHADER,		"Shaders/skybox.frag" }
	});
	m_deferredPointlight.Init({ // A deferred shader for point lights
		{ GL_VERTEX_SHADER,		"Shaders/deferredPoint.vert" },
		{ GL_FRAGMENT_SHADER,	"Shaders/deferredPoint.frag" }
	});

	if (glGetError() != GL_NO_ERROR) { std::cout << "Error after shader compilation.\n"; exit(1); }

	// Defining geometry
	// 
	// Position of vertices:
	/*	The constructor of cube_positions has already created a GPU buffer identifier, and the following BufferData call will
		1. bind this to GL_ARRAY_BUFFER (because the type of cube_positions is ArrayBuffer) and
		2. upload the values of the container given in the argument to the GPU by calling glBufferData
	*/
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

	// Creating VBOs for the quad
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

	// Loading mesh
	m_textureMetal.FromFile("Assets/texture.png");

	// Loading texture
	m_mesh = ObjParser::parse("Assets/Suzanne.obj");

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

void CMyApp::DrawScene(const glm::mat4& viewProj, ProgramObject& program)
{
	program.Use();
	
	// Only texture information is needed, no lights.
	program.SetTexture("texImage", 0, m_textureMetal);
	
	// Drawing the plane underneath

	program.SetUniform("MVP", viewProj * glm::mat4(1));
	program.SetUniform("world", glm::mat4(1));
	program.SetUniform("worldIT", glm::mat4(1));

	m_vao.Bind();
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);	//Draws exactly the same but uses index buffer:
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
	m_vao.Unbind();

	// Suzanne wall

	float t = SDL_GetTicks() / 1000.f;
	for (int i = -1; i <= 1; ++i)
		for (int j = -1; j <= 1; ++j)
		{
			glm::mat4 suzanneWorld = glm::translate(glm::vec3(4 * i, 4 * (j + 1), sinf(t * 2 * M_PI * i * j)));
			program.SetUniform("MVP", viewProj * suzanneWorld);
			program.SetUniform("world", suzanneWorld);
			program.SetUniform("worldIT", glm::transpose(glm::inverse(suzanneWorld)));
			m_mesh->draw();
		}
	program.Unuse();
}

void CMyApp::Render()
{

	// 1.
	// Render to the framebuffer

	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	DrawScene(m_camera.GetViewProj(), m_program);

	// 2.
	// Draw Lights by additions
	
	//2.1. Setting up the blending
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0, 0, 0, 1);		//Clear to black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);		// Depth test is not performed -- all fragments should add color
	glDepthMask(GL_FALSE);			// Depth values are not need to be written

	glEnable(GL_BLEND);				// Instead of overwriting pixels in the FBO we
	glBlendEquation(GL_FUNC_ADD);	// perform addition for each pixel and thus
	glBlendFunc(GL_ONE, GL_ONE);	// summing the contribution of each light source

	// 2.2. Light program setup

	m_deferredPointlight.Use();
	m_deferredPointlight.SetTexture("diffuseTexture" , 0, m_diffuseBuffer);
	m_deferredPointlight.SetTexture("normalTexture"  , 1, m_normalBuffer);
	m_deferredPointlight.SetTexture("positionTexture", 2, m_position_Buffer);
	
	// 2.3. Draw point lights

	m_deferredPointlight.SetUniform("lightPos", m_light_pos);
	m_deferredPointlight.SetUniform("Ld", glm::vec4(1,0.8,0.5,1));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // First light
	
	float t = SDL_GetTicks() / 1000.f;
	m_deferredPointlight.SetUniform("lightPos", 10.f*glm::vec3(cosf(t),0.5,sinf(t)));
	m_deferredPointlight.SetUniform("Ld", glm::vec4(0.5, 1, 1, 1));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // Second light

	// 2.4. Undo the blending options

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// Skybox
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

	// The last 3 rows <=> m_programSkybox.SetCubeTexture("skyboxTexture", 0, m_skyboxTexture);

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, 0);

	// Finally set it back
	glDepthFunc(prevDepthFnc);

	// 3.
	// User Interface
	ImGui::ShowTestWindow(); // Demo of all ImGui commands. See its implementation for details.
		// It's worth browsing imgui.h, as well as reading the FAQ at the beginning of imgui.cpp.
		// There is no regular documentation, but the things mentioned above should be sufficient.

	ImGui::SetNextWindowPos(ImVec2(300, 400), ImGuiSetCond_FirstUseEver);
	if(ImGui::Begin("Test window")) // Note that ImGui returns false when window is collapsed so we can early-out
	{
		ImGui::SliderFloat3("light_pos", &m_light_pos.x, -10.f, 10.f);
		ImGui::Image((ImTextureID)m_diffuseBuffer  , ImVec2(256, 256), ImVec2(0,1), ImVec2(1,0));
		ImGui::Image((ImTextureID)m_normalBuffer   , ImVec2(256, 256), ImVec2(0,1), ImVec2(1,0));
		ImGui::Image((ImTextureID)m_position_Buffer, ImVec2(256, 256), ImVec2(0,1), ImVec2(1,0));
	}
	ImGui::End(); // In either case, ImGui::End() needs to be called for ImGui::Begin().
		// Note that other commands may work differently and may not need an End* if Begin* returned false.
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
	// Clear if the function is not being called for the first time
	if (m_frameBufferCreated)
	{
		glDeleteTextures(1, &m_diffuseBuffer);
		glDeleteTextures(1, &m_normalBuffer);
		glDeleteTextures(1, &m_position_Buffer);
		glDeleteRenderbuffers(1, &m_depthBuffer);
		glDeleteFramebuffers(1, &m_frameBuffer);
	}

	glGenFramebuffers(1, &m_frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_frameBuffer);

	// 1.  Diffuse colors
	glGenTextures(1, &m_diffuseBuffer);
	glBindTexture(GL_TEXTURE_2D, m_diffuseBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	setTexture2DParameters(GL_NEAREST, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 0, GL_TEXTURE_2D, m_diffuseBuffer, 0);
	if (glGetError() != GL_NO_ERROR) {
		std::cout << "Error creating color attachment 0" << std::endl;		
		exit(1);
	}

	// 2.  Normal vectors
	glGenTextures(1, &m_normalBuffer);
	glBindTexture(GL_TEXTURE_2D, m_normalBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	setTexture2DParameters(GL_NEAREST, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 1, GL_TEXTURE_2D, m_normalBuffer, 0);
	if (glGetError() != GL_NO_ERROR) {
		std::cout << "Error creating color attachment 1" << std::endl;
		exit(1);
	}

	// 3.  Word-space positions
	glGenTextures(1, &m_position_Buffer);
	glBindTexture(GL_TEXTURE_2D, m_position_Buffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
	setTexture2DParameters(GL_NEAREST, GL_NEAREST);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + 2, GL_TEXTURE_2D, m_position_Buffer, 0);
	if (glGetError() != GL_NO_ERROR) {
		std::cout << "Error creating color attachment 2" << std::endl;
		exit(1);
	}

	// 4. Depth renderbuffer
	glGenRenderbuffers(1, &m_depthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer);
	if (glGetError() != GL_NO_ERROR) {
		std::cout << "Error creating depth attachment" << std::endl;
		exit(1);
	}

	//Specifying which color outputs are active
	GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0,
							 GL_COLOR_ATTACHMENT1,
							 GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(3, drawBuffers);

	// -- Completeness check
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Incomplete framebuffer (";
		switch (status) {
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			std::cout << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT";		break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	std::cout << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
		case GL_FRAMEBUFFER_UNSUPPORTED:					std::cout << "GL_FRAMEBUFFER_UNSUPPORTED";					break;
		}
		std::cout << ")" << std::endl;
		exit(1);
	}

	// -- Unbind framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	m_frameBufferCreated = true;
}