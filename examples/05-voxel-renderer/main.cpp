#define RUTILE_IMPL
#include "cli.hpp"
#include "rt_ext_glfw.h"
#include "rt_ext_swapchain.h"
#include "rutile.h"
#include "world.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <rtsl/program.hpp>
#include <vector>

constexpr const char* Layers[] = { "rt-validation-layer" };
constexpr const char* Features[] = { RT_FEATURE_PRESENTATION };

extern "C" const rtsl::ProgramBytes terrain_rtslp;
extern "C" const rtsl::ProgramBytes water_rtslp;

constexpr rt_vertex_attribute VertexAttributes[] = {
	{ "position", offsetof(Vertex, position), RT_RGB32_SFLOAT },
	{ "color", offsetof(Vertex, color), RT_RGB32_SFLOAT },
	{ "normal", offsetof(Vertex, normal), RT_RGB32_SFLOAT },
	{ "ao", offsetof(Vertex, ao), RT_R32_SFLOAT },
	{ "pixel_uv", offsetof(Vertex, pixel_uv), RT_RG32_SFLOAT },
	{ "edge_mask", offsetof(Vertex, edge_mask), RT_R32_SFLOAT },
	{ "corner_mask", offsetof(Vertex, corner_mask), RT_R32_SFLOAT },
};

constexpr rt_vertex_input VertexInputs[] = {
	{ VertexAttributes, 7, sizeof(Vertex), RT_VERTEX_RATE_VERTEX },
};

constexpr rt_vertex_layout VertexLayout = { VertexInputs, 1 };

struct Camera {
	glm::vec3 position = glm::vec3(0.0f, 13.0f, 18.0f);
	f32 yaw = -glm::radians(90.0f);
	f32 pitch = -0.32f;
};

rt_swapchain Swapchain = RT_NULL_HANDLE;
rt_texture DepthTexture = RT_NULL_HANDLE;
rt_texture_view DepthView = RT_NULL_HANDLE;
u32 FramebufferWidth = 1280;
u32 FramebufferHeight = 720;
f32 MouseDx = 0.0f;
f32 MouseDy = 0.0f;

glm::vec3 camera_forward(const Camera& camera) {
	const f32 cp = glm::cos(camera.pitch);
	return glm::normalize(glm::vec3(glm::cos(camera.yaw) * cp, glm::sin(camera.pitch), glm::sin(camera.yaw) * cp));
}

glm::mat4 camera_matrix(const Camera& camera, f32 aspect) {
	const glm::vec3 forward = camera_forward(camera);
	const glm::mat4 view = glm::lookAt(camera.position, camera.position + forward, glm::vec3(0, 1, 0));
	const glm::mat4 projection = glm::perspective(glm::radians(70.0f), aspect, 0.05f, 180.0f);
	return projection * view;
}

void framebuffer_resized(GLFWwindow* window, int width, int height) {
	(void)window;
	if (width > 0 && height > 0) {
		FramebufferWidth = (u32)width;
		FramebufferHeight = (u32)height;
		if (Swapchain) {
			rtSwapchainResize(Swapchain, FramebufferWidth, FramebufferHeight);
		}
		if (DepthTexture) {
			rtTextureResize(DepthTexture, RT_TEXTURE_2D, RT_D32_SFLOAT, { FramebufferWidth, FramebufferHeight, 1 }, 1);
			rtTextureViewSetTexture(DepthView, DepthTexture);
		}
	}
}

void cursor_moved(GLFWwindow* window, double x, double y) {
	(void)window;

	static double previous_x = x;
	static double previous_y = y;
	MouseDx += (f32)(x - previous_x);
	MouseDy += (f32)(y - previous_y);
	previous_x = x;
	previous_y = y;
}

void update_camera(GLFWwindow* window, Camera* camera, f32 dt) {
	const f32 sensitivity = 0.0024f;
	camera->yaw += MouseDx * sensitivity;
	camera->pitch -= MouseDy * sensitivity;
	camera->pitch = glm::clamp(camera->pitch, -1.45f, 1.45f);
	MouseDx = 0.0f;
	MouseDy = 0.0f;

	const glm::vec3 forward = camera_forward(*camera);
	const glm::vec3 flat_forward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
	const glm::vec3 right = glm::normalize(glm::cross(flat_forward, glm::vec3(0, 1, 0)));
	const f32 speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ? 18.0f : 8.0f;
	glm::vec3 velocity(0.0f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		velocity += flat_forward;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		velocity -= flat_forward;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		velocity += right;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		velocity -= right;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		velocity.y += 1.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		velocity.y -= 1.0f;
	}
	if (glm::dot(velocity, velocity) > 0.0f) {
		camera->position += glm::normalize(velocity) * speed * dt;
	}
}

int main(int argc, char** argv) {
	const ExampleOptions options = parse_cli(argc, argv);
	if (rtLoad("rt-d3d12", Layers, 1) != RT_SUCCESS) {
		std::cerr << "rtLoad failed\n";
		return 1;
	}

	rtInit(Features, 1);
	rtLoad_RT_EXT_SWAPCHAIN();
	rtLoad_RT_EXT_GLFW();

	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "Rutile 05 Voxel Renderer", nullptr, nullptr);

	glfwSetFramebufferSizeCallback(window, framebuffer_resized);
	glfwSetCursorPosCallback(window, cursor_moved);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	if (glfwRawMouseMotionSupported()) {
		glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
	}

	int framebuffer_width = 0;
	int framebuffer_height = 0;
	glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
	if (framebuffer_width > 0 && framebuffer_height > 0) {
		FramebufferWidth = (u32)framebuffer_width;
		FramebufferHeight = (u32)framebuffer_height;
	}

	rt_swapchain swapchain = rtSwapchainCreate();
	rtSwapchainBindWindowGLFW(swapchain, window);
	Swapchain = swapchain;
	rt_queue queue = rtQueueCreate(RT_QUEUE_GRAPHICS);

	std::vector<Vertex> vertices = build_world_mesh();
	rt_buffer vertex_buffer = rtBufferCreate();
	const usize vertex_size = vertices.size() * sizeof(vertices[0]);
	rtBufferResize(vertex_buffer, RT_DEVICE_MEMORY, vertex_size);

	std::vector<Vertex> water_vertices = build_water_mesh();
	rt_buffer water_vertex_buffer = rtBufferCreate();
	const usize water_vertex_size = water_vertices.size() * sizeof(water_vertices[0]);
	rtBufferResize(water_vertex_buffer, RT_DEVICE_MEMORY, water_vertex_size);

	glm::mat4 transform{ 1.0f };
	rt_buffer transform_buffer = rtBufferCreate();
	rtBufferResize(transform_buffer, RT_DEVICE_MEMORY, sizeof(transform));

	glm::mat4 water_transform{ 1.0f };
	rt_buffer water_transform_buffer = rtBufferCreate();
	rtBufferResize(water_transform_buffer, RT_DEVICE_MEMORY, sizeof(water_transform));

	rt_graphics_program graphics_program = rtGraphicsProgramCreate();
	rtGraphicsProgramSetLayout(graphics_program, &VertexLayout);
	rtGraphicsProgramSetSource(graphics_program, terrain_rtslp.data, terrain_rtslp.size);
	rtGraphicsProgramSetRasterState(graphics_program, RT_CULL_BACK, RT_FRONT_FACE_CCW, RT_FILL_SOLID);
	rtGraphicsProgramFinalize(graphics_program);
	rt_location transform_location = rtGraphicsProgramUniformLocation(graphics_program, "scene");
	rt_location vertex_location = rtGraphicsProgramInputLocation(graphics_program, VertexAttributes, 7);

	rt_graphics_program water_program = rtGraphicsProgramCreate();
	rtGraphicsProgramSetLayout(water_program, &VertexLayout);
	rtGraphicsProgramSetSource(water_program, water_rtslp.data, water_rtslp.size);
	rtGraphicsProgramSetRasterState(water_program, RT_CULL_BACK, RT_FRONT_FACE_CCW, RT_FILL_SOLID);
	rtGraphicsProgramSetBlendState(water_program, true, RT_BLEND_SRC_ALPHA, RT_BLEND_ONE_MINUS_SRC_ALPHA, RT_BLEND_OP_ADD, RT_BLEND_ONE, RT_BLEND_ONE_MINUS_SRC_ALPHA, RT_BLEND_OP_ADD);
	rtGraphicsProgramFinalize(water_program);
	rt_location water_transform_location = rtGraphicsProgramUniformLocation(water_program, "scene");
	rt_location water_vertex_location = rtGraphicsProgramInputLocation(water_program, VertexAttributes, 7);

	rt_command_buffer cmd = rtCommandBufferCreate();
	DepthTexture = rtTextureCreate();
	DepthView = rtTextureViewCreate();
	rtTextureResize(DepthTexture, RT_TEXTURE_2D, RT_D32_SFLOAT, { FramebufferWidth, FramebufferHeight, 1 }, 1);
	rtTextureViewSetTexture(DepthView, DepthTexture);
	rtCommandBufferBegin(cmd);
	rtCmdBufferData(cmd, vertex_buffer, { vertex_size, 0 }, reinterpret_cast<const u08*>(vertices.data()));
	rtCmdBufferData(cmd, water_vertex_buffer, { water_vertex_size, 0 }, reinterpret_cast<const u08*>(water_vertices.data()));
	rtCommandBufferEnd(cmd);
	rtTimepointWait(rtQueueSubmit(queue, cmd));
	rtCommandBufferReset(cmd);
	rtCommandBufferContinueRendering(cmd);
	rtCmdUseGraphicsProgram(cmd, graphics_program);
	rtCmdVertexBuffer(cmd, vertex_location, vertex_buffer, { vertex_size, 0 });
	rtCmdDraw(cmd, vertices.size(), 0);
	rtCmdUseGraphicsProgram(cmd, water_program);
	rtCmdVertexBuffer(cmd, water_vertex_location, water_vertex_buffer, { water_vertex_size, 0 });
	rtCmdDraw(cmd, water_vertices.size(), 0);
	rtCommandBufferEnd(cmd);
	rt_command_buffer primary = rtCommandBufferCreate();

	Camera camera;
	auto start_time = std::chrono::steady_clock::now();
	auto previous_time = start_time;
	auto fps_time = start_time;
	u32 fps_frames = 0;
	u32 rendered_frames = 0;

	while (!glfwWindowShouldClose(window) && (!options.frames || rendered_frames < options.frames)) {
		glfwPollEvents();
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(window, GLFW_TRUE);
		}

		auto now = std::chrono::steady_clock::now();
		const std::chrono::duration<f32> delta = now - previous_time;
		previous_time = now;
		update_camera(window, &camera, delta.count());

		const u32 current_width = FramebufferWidth;
		const u32 current_height = FramebufferHeight;
		rt_swapchain_acquire_result acquired = rtSwapchainAcquire(swapchain);
		if (!acquired.framebuffer) {
			continue;
		}

		const f32 aspect = current_height ? (f32)current_width / (f32)current_height : 1.0f;
		const glm::mat4 view_projection = camera_matrix(camera, aspect);
		const std::chrono::duration<f32> elapsed = now - start_time;
		transform = view_projection;
		water_transform = view_projection * glm::translate(glm::mat4{ 1.0f }, glm::vec3{ 0.0f, glm::sin(elapsed.count() * 1.8f) * 0.035f, 0.0f });
		rtFramebufferSetDepthView(acquired.framebuffer, DepthView);
		rtQueueWait(queue, acquired.timepoint);
		rtCommandBufferReset(primary);
		rtCommandBufferBegin(primary);
		rtCmdBufferData(primary, transform_buffer, { sizeof(transform), 0 }, reinterpret_cast<const u08*>(&transform));
		rtCmdBufferData(primary, water_transform_buffer, { sizeof(water_transform), 0 }, reinterpret_cast<const u08*>(&water_transform));
		rtCmdBufferBarrier(primary, transform_buffer, { sizeof(transform), 0 }, { RT_STAGE_TRANSFER, RT_ACCESS_WRITE }, { RT_STAGE_VERTEX, RT_ACCESS_READ });
		rtCmdBufferBarrier(primary, water_transform_buffer, { sizeof(water_transform), 0 }, { RT_STAGE_TRANSFER, RT_ACCESS_WRITE }, { RT_STAGE_VERTEX, RT_ACCESS_READ });
		rtCmdBeginRendering(primary, acquired.framebuffer);
		rtCmdClearColor(primary, RT_LOCATION_ZERO, 0.54f, 0.72f, 0.94f, 1.0f);
		rtCmdClearDepth(primary, 1.0f);
		rtCmdClear(primary, static_cast<enum rt_clear_flag>(RT_CLEAR_COLOR | RT_CLEAR_DEPTH));
		rtCmdBindBuffer(primary, transform_location, transform_buffer, { sizeof(transform), 0 });
		rtCmdBindBuffer(primary, water_transform_location, water_transform_buffer, { sizeof(water_transform), 0 });
		rtCmdExecute(primary, cmd);
		rtCmdEndRendering(primary);
		rtCommandBufferEnd(primary);

		rt_timepoint rendered = rtQueueSubmit(queue, primary);
		rtSwapchainPresent(swapchain, rendered);
		rendered_frames++;

		fps_frames++;
		const auto fps_now = std::chrono::steady_clock::now();
		const std::chrono::duration<f32> fps_delta = fps_now - fps_time;
		if (fps_delta.count() >= 0.5f) {
			char title[96];
			const f32 fps = (f32)fps_frames / fps_delta.count();
			std::snprintf(title, sizeof(title), "Rutile 05 Voxel Renderer - %.0f FPS", fps);
			glfwSetWindowTitle(window, title);
			fps_time = fps_now;
			fps_frames = 0;
		}
	}

	rtTimepointWait(rtQueueFlush(queue));
	rtCommandBufferDestroy(primary);
	rtCommandBufferDestroy(cmd);
	rtQueueDestroy(queue);
	rtGraphicsProgramDestroy(water_program);
	rtGraphicsProgramDestroy(graphics_program);
	rtBufferDestroy(water_transform_buffer);
	rtBufferDestroy(transform_buffer);
	rtBufferDestroy(water_vertex_buffer);
	rtBufferDestroy(vertex_buffer);
	rtTextureViewDestroy(DepthView);
	rtTextureDestroy(DepthTexture);
	rtSwapchainDestroy(swapchain);
	Swapchain = RT_NULL_HANDLE;
	glfwDestroyWindow(window);
	glfwTerminate();
	rtExit();
	rtUnload();
	return 0;
}
