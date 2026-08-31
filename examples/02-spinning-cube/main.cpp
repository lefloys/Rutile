#define RUTILE_IMPL
#include "cli.hpp"
#include "embedded_program.hpp"
#include "rt_glfw_swapchain.h"
#include "rt_swapchain.h"
#include "rutile.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

extern "C" const rt_example_program cube_rtslp;

struct Vertex {
	f32 position[3];
	f32 color[3];
	f32 normal[3];
};

struct Scene {
	f32 transform[16];
};

struct Camera {
	glm::vec3 position = glm::vec3(0.0f, 0.0f, 4.0f);
	f32 yaw = -glm::radians(90.0f);
	f32 pitch = 0.0f;
};

rt_swapchain Swapchain = RT_NULL_HANDLE;
rt_texture Depth = RT_NULL_HANDLE;
rt_texture_view DepthView = RT_NULL_HANDLE;
u32 FramebufferWidth = 1280;
u32 FramebufferHeight = 720;
f32 MouseDx = 0.0f;
f32 MouseDy = 0.0f;

constexpr rt_vertex_attribute Attributes[] = {
	{ "position", offsetof(Vertex, position), RT_RGB32_SFLOAT },
	{ "color", offsetof(Vertex, color), RT_RGB32_SFLOAT },
	{ "normal", offsetof(Vertex, normal), RT_RGB32_SFLOAT },
};

constexpr rt_vertex_input Inputs[] = {
	{ Attributes, 3, sizeof(Vertex), RT_VERTEX_RATE_VERTEX },
};

constexpr rt_vertex_layout Layout = { Inputs, 1 };

std::vector<Vertex> make_cube() {
	struct Face {
		glm::vec3 normal;
		glm::vec3 color;
		std::array<glm::vec3, 4> corners;
	};
	const std::array<Face, 6> faces = {
		Face{ { 0, 0, 1 }, { 1.0f, 0.30f, 0.24f }, { { { -1, -1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { -1, 1, 1 } } } },
		Face{ { 0, 0, -1 }, { 0.25f, 0.52f, 1.0f }, { { { 1, -1, -1 }, { -1, -1, -1 }, { -1, 1, -1 }, { 1, 1, -1 } } } },
		Face{ { 1, 0, 0 }, { 0.32f, 1.0f, 0.48f }, { { { 1, -1, 1 }, { 1, -1, -1 }, { 1, 1, -1 }, { 1, 1, 1 } } } },
		Face{ { -1, 0, 0 }, { 0.78f, 0.34f, 1.0f }, { { { -1, -1, -1 }, { -1, -1, 1 }, { -1, 1, 1 }, { -1, 1, -1 } } } },
		Face{ { 0, 1, 0 }, { 1.0f, 0.78f, 0.22f }, { { { -1, 1, 1 }, { 1, 1, 1 }, { 1, 1, -1 }, { -1, 1, -1 } } } },
		Face{ { 0, -1, 0 }, { 0.20f, 0.88f, 0.94f }, { { { -1, -1, -1 }, { 1, -1, -1 }, { 1, -1, 1 }, { -1, -1, 1 } } } },
	};
	std::vector<Vertex> vertices;
	vertices.reserve(36);
	for (const Face& face : faces) {
		for (const u32 index : { 0u, 1u, 2u, 0u, 2u, 3u }) {
			const glm::vec3& point = face.corners[index];
			vertices.push_back({
				{ point.x, point.y, point.z },
				{ face.color.r, face.color.g, face.color.b },
				{ face.normal.x, face.normal.y, face.normal.z },
			});
		}
	}
	return vertices;
}

void framebuffer_resized(GLFWwindow*, int width, int height) {
	if (width <= 0 || height <= 0) {
		return;
	}
	FramebufferWidth = static_cast<u32>(width);
	FramebufferHeight = static_cast<u32>(height);
	rtSwapchainResize(Swapchain, FramebufferWidth, FramebufferHeight);
	if (Depth) {
		rtTextureResize(Depth, RT_TEXTURE_2D, RT_D32_SFLOAT, { FramebufferWidth, FramebufferHeight, 1 }, 1);
		rtTextureViewSetTexture(DepthView, Depth);
	}
}

void cursor_moved(GLFWwindow*, double x, double y) {
	static double previous_x = x;
	static double previous_y = y;
	MouseDx += static_cast<f32>(x - previous_x);
	MouseDy += static_cast<f32>(y - previous_y);
	previous_x = x;
	previous_y = y;
}

glm::vec3 camera_forward(const Camera& camera) {
	const f32 pitch_cosine = glm::cos(camera.pitch);
	return glm::normalize(glm::vec3(glm::cos(camera.yaw) * pitch_cosine, glm::sin(camera.pitch), glm::sin(camera.yaw) * pitch_cosine));
}

void update_camera(GLFWwindow* window, Camera& camera, f32 delta) {
	camera.yaw += MouseDx * 0.0022f;
	camera.pitch = glm::clamp(camera.pitch - MouseDy * 0.0022f, glm::radians(-85.0f), glm::radians(85.0f));
	MouseDx = 0.0f;
	MouseDy = 0.0f;
	const glm::vec3 forward = camera_forward(camera);
	const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
	glm::vec3 motion(0.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		motion += forward;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		motion -= forward;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		motion += right;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		motion -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		motion.y += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		motion.y -= 1.0f;
	}
	if (glm::dot(motion, motion) > 0.0f) {
		camera.position += glm::normalize(motion) * delta * 2.5f;
	}
}

int main(int argc, char** argv) {
	const ExampleOptions options = parse_cli(argc, argv);
	if (rtLoad("rt-vulkan", nullptr, 0) != RT_SUCCESS) {
		std::cerr << "rtLoad failed\n";
		return 1;
	}
	const char* features[] = { RT_FEATURE_PRESENTATION };
	rtInit(features, 1);
	rtLoadSwapchain();
	rtLoadGlfwSwapchain();

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Rutile 02 Spinning Cube", nullptr, nullptr);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, cursor_moved);
	glfwSetFramebufferSizeCallback(window, framebuffer_resized);

	Swapchain = rtSwapchainCreate();
	rtSwapchainBindGLFW(Swapchain, window);

	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);

	const std::vector<Vertex> vertices = make_cube();
	rt_buffer vertex_buffer = rtBufferCreate();
	rtBufferResize(vertex_buffer, RT_DEVICE_MEMORY, vertices.size() * sizeof(Vertex));

	Scene scene = {};
	rt_buffer scene_buffer = rtBufferCreate();
	rtBufferResize(scene_buffer, RT_DEVICE_MEMORY, sizeof(scene));

	rt_program program = rtProgramCreate();
	rtProgramSource(program, "main", cube_rtslp.data, cube_rtslp.size);
	rtProgramSetLayout(program, &Layout);
	rtProgramSetRasterState(program, RT_CULL_NONE, RT_FRONT_FACE_CCW, RT_FILL_SOLID);
	rtProgramFinalize(program);


	rt_location scene_location = rtProgramUniformLocation(program, "scene");
	rt_location vertex_location = rtProgramInputLocation(program, Attributes, 3);

	Depth = rtTextureCreate();
	rtTextureResize(Depth, RT_TEXTURE_2D, RT_D32_SFLOAT, { 1280, 720, 1 }, 1);
	DepthView = rtTextureViewCreate();
	rtTextureViewSetTexture(DepthView, Depth);

	rt_command_buffer secondary = rtCommandBufferCreate();
	rtCommandBufferBegin(secondary);
	rtCmdBufferData(secondary, vertex_buffer, { vertices.size() * sizeof(Vertex), 0 }, reinterpret_cast<const u08*>(vertices.data()));
	rtCommandBufferEnd(secondary);
	rtTimepointWait(rtQueueSubmit(queue, secondary));
	rtCommandBufferReset(secondary);

	rtCommandBufferContinueRendering(secondary);
	rtCmdUseProgram(secondary, program);
	rtCmdVertexBuffer(secondary, vertex_location, vertex_buffer, { vertices.size() * sizeof(Vertex), 0 });
	rtCmdDraw(secondary, vertices.size(), 0);
	rtCommandBufferEnd(secondary);

	rt_command_buffer primary = rtCommandBufferCreate();
	const auto start = std::chrono::steady_clock::now();
	auto previous = start;
	u32 rendered_frames = 0;

	Camera camera;
	while (!glfwWindowShouldClose(window) && (!options.frames || rendered_frames < options.frames)) {
		glfwPollEvents();
		const auto now = std::chrono::steady_clock::now();
		const f32 delta = std::chrono::duration<f32>(now - previous).count();
		const f32 time = std::chrono::duration<f32>(now - start).count();
		previous = now;

		update_camera(window, camera, delta);

		const glm::mat4 view = glm::lookAt(camera.position, camera.position + camera_forward(camera), glm::vec3(0, 1, 0));
		const glm::mat4 projection = glm::perspective(glm::radians(60.0f), static_cast<f32>(FramebufferWidth) / static_cast<f32>(FramebufferHeight), 0.1f, 100.0f);
		const glm::mat4 model = glm::rotate(glm::rotate(glm::mat4(1.0f), time, glm::vec3(0, 1, 0)), time, glm::vec3(1, 0, 0));
		const glm::mat4 transform = projection * view * model;
		std::memcpy(scene.transform, glm::value_ptr(transform), sizeof(scene.transform));
		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(Swapchain);
		if (!acquired.framebuffer) { continue; }
		rtFramebufferSetDepthView(acquired.framebuffer, DepthView);
		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(primary);


		rtCommandBufferBegin(primary);
		rtCmdBufferData(primary, scene_buffer, { sizeof(scene), 0 }, reinterpret_cast<const u08*>(&scene));
		rtCmdBufferBarrier(primary, scene_buffer, { sizeof(scene), 0 }, { RT_STAGE_TRANSFER, RT_ACCESS_WRITE }, { RT_STAGE_VERTEX, RT_ACCESS_READ });
		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, nullptr, 0.035f, 0.045f, 0.075f, 1.0f);
		rtCmdClearDepth(primary, 1.0f);
		rtCmdClear(primary, static_cast<enum rt_clear_flag>(RT_CLEAR_COLOR | RT_CLEAR_DEPTH));
		rtCmdBindBuffer(primary, scene_location, scene_buffer, { sizeof(scene), 0 });
		rtCmdExecute(primary, secondary);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);


		rt_timepoint rendered = rtQueueSubmit(queue, primary);
		rtFramebufferSetDepthView(acquired.framebuffer, RT_NULL_HANDLE);
		rtSwapchainPresent(Swapchain, rendered);
		rendered_frames++;
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(secondary);
	rtQueueDestroy(queue);
	rtTextureViewDestroy(DepthView);
	rtTextureDestroy(Depth);
	rtProgramDestroy(program);
	rtBufferDestroy(scene_buffer);
	rtBufferDestroy(vertex_buffer);
	rtSwapchainDestroy(Swapchain);
	glfwDestroyWindow(window);
	glfwTerminate();
	rtExit();
	rtUnload();
	return 0;
}
