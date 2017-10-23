#define HANDMADE_MATH_IMPLEMENTATION
#define HANDMADE_MATH_NO_SSE
#include "HandmadeMath.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "flextgl/flextGL.h"
#define SOKOL_IMPL
#define SOKOL_GLCORE33
#include "sokol_gfx.h"
#include <stdlib.h>
#include <stdio.h>
#include <sdf-shader.hpp>
#include <iostream>
#include <AstExternals.hpp>
#include <chrono>

void glfw_error(int error, const char* description) {
	fprintf(stderr, "Glfw error: %s", description);
}

typedef struct {
    float iGlobalTime;
    hmm_vec3 uCameraPos;
    hmm_vec3 uCameraDir;
    hmm_vec2 uViewport;
} params_t;

int main() {


    if (!glfwInit()) {
		exit(1);
	}
	glfwSetErrorCallback(glfw_error);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Shader-pg", NULL, NULL);
	if (!window) {
		exit(1);
	}
	glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    flextInit(window);

    // Setup sokol
    sg_desc desc = {};
    sg_setup(&desc);
    assert(sg_isvalid());

    float vertices[] = {
        -1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, -1.0f
    };
    sg_buffer_desc vbuf_desc = {};
    vbuf_desc.size = sizeof(vertices);
    vbuf_desc.content = vertices;
    sg_buffer vbuf = sg_make_buffer(&vbuf_desc);

    uint16_t indices[] = {
        0, 1, 2,
        0, 2, 3
    };
    sg_buffer_desc ibuf_desc = {};
    ibuf_desc.type = SG_BUFFERTYPE_INDEXBUFFER;
    ibuf_desc.size = sizeof(indices);
    ibuf_desc.content = indices;
    sg_buffer ibuf = sg_make_buffer(&ibuf_desc);

    std::string vertShader = getFileStr(std::string(PROJECT_ROOT) + "/../shader-things/sdf-vertex.glsl");
    AstNode *result = parseFile(std::string(PROJECT_ROOT) + "/../shader-things/sdf-fragment.glsl");
    auto extMap = new std::map<std::string, Expression*>();
    //(*extMap)[std::string("sdfDist")] = sphere3d(floatc(0.5), ref("point"));
    (*extMap)[std::string("sdfDist")] = unionSdf(
        sphere3d(floatWave(floatc(0.5), floatc(0.7), floatc(0.0)), ref("point")),
        box3d(floatc(0.5), floatc(0.5), floatc(0.5), ref("point"))
    );
    auto ext = new AstExternals(extMap);
    ext->visitSource(static_cast<Source*>(result));
	std::string fragShader; 
	result->toStringF(&fragShader, new FormatState());
    fprintf(stdout, fragShader.c_str());

    // Uniform block
    sg_shader_uniform_desc fs_iGlobalTime = {};
    fs_iGlobalTime.name = "iGlobalTime";
    fs_iGlobalTime.type = SG_UNIFORMTYPE_FLOAT;
    sg_shader_uniform_desc fs_uCameraPos = {};
    fs_uCameraPos.name = "uCameraPos";
    fs_uCameraPos.type = SG_UNIFORMTYPE_FLOAT3;
    sg_shader_uniform_desc fs_uCameraDir = {};
    fs_uCameraDir.name = "uCameraDir";
    fs_uCameraDir.type = SG_UNIFORMTYPE_FLOAT3;
    sg_shader_uniform_desc fs_uViewport = {};
    fs_uViewport.name = "uViewport";
    fs_uViewport.type = SG_UNIFORMTYPE_FLOAT2;
    sg_shader_uniform_block_desc fs_uniforms = {};
    fs_uniforms.size = sizeof(params_t);
    fs_uniforms.uniforms[0] = fs_iGlobalTime;
    fs_uniforms.uniforms[1] = fs_uCameraPos;
    fs_uniforms.uniforms[2] = fs_uCameraDir;
    fs_uniforms.uniforms[3] = fs_uViewport;

    // Shaders
    sg_shader_desc shader_desc = {};
    shader_desc.vs.source = vertShader.c_str();
    shader_desc.fs.source = fragShader.c_str();
    shader_desc.fs.uniform_blocks[0] = fs_uniforms;
    sg_shader shd = sg_make_shader(&shader_desc);

    // Vertex attribute
    sg_vertex_attr_desc pos_attr = {};
    pos_attr.offset = 0;
    pos_attr.format = SG_VERTEXFORMAT_FLOAT2;
    
    // Vertex layout
    sg_vertex_layout_desc vlayout_desc = {};
    vlayout_desc.stride = 8;
    vlayout_desc.attrs[0] = pos_attr;

    // Pipeline
    sg_pipeline_desc pipeline_desc = {};
    pipeline_desc.shader = shd;
    pipeline_desc.index_type = SG_INDEXTYPE_UINT16;
    pipeline_desc.vertex_layouts[0] = vlayout_desc;
    sg_pipeline pip = sg_make_pipeline(&pipeline_desc);

    // Draw state
    sg_draw_state draw_state = {};
    draw_state.pipeline = pip;
    draw_state.vertex_buffers[0] = vbuf;
    draw_state.index_buffer = ibuf;

    sg_pass_action pass_action = {};

    auto start_time = std::chrono::high_resolution_clock::now();
    params_t fs_params;
    fs_params.uCameraPos = HMM_Vec3(8.0f, 5.0f, 7.0f);
    fs_params.uCameraDir = HMM_NormalizeVec3(HMM_Vec3(-8.0f, -5.0f, -7.0f));
	while (!glfwWindowShouldClose(window)) {
        int currWidth, currHeight;
        glfwGetFramebufferSize(window, &currWidth, &currHeight);
        auto tick_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed_time = tick_time - start_time;
        typedef std::chrono::milliseconds ms;
        ms elapsed_time_ms = std::chrono::duration_cast<ms>(elapsed_time);
        float elapsed_time_float = elapsed_time_ms.count() / 1000.0f;
        fs_params.iGlobalTime = elapsed_time_float;
        fs_params.uViewport = HMM_Vec2((float)currWidth, (float)currHeight);
        sg_begin_default_pass(&pass_action, currWidth, currHeight);
        sg_apply_draw_state(&draw_state);
        sg_apply_uniform_block(SG_SHADERSTAGE_FS, 0, &fs_params, sizeof(fs_params));
        sg_draw(0, 6, 1);
        sg_end_pass();
        sg_commit();
		
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

    sg_shutdown();
	glfwDestroyWindow(window);
	glfwTerminate();

    //int c = getchar();
    return 0;
}