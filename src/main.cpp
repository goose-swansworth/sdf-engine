#include "glad.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <chrono>

#include "shader.h"
#include "computeshader.h"
#include "camera.h"
#include "scene.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 900

#define TEXTURE_WIDTH 1200
#define TEXTURE_HEIGHT 900

unsigned int quadVAO = 0;
unsigned int quadVBO;

using namespace std;

glm::vec2 mpos_last;
bool fix_camera = false;

void print_vec(glm::vec3 v) {
    cout << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")" << endl;
}





glm::vec2 get_mouse_pos(GLFWwindow* window) {
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    float screen_x = (2 * xpos - WINDOW_WIDTH) / WINDOW_WIDTH;
    float screen_y = (2 * ypos - WINDOW_HEIGHT) / WINDOW_HEIGHT;
    return glm::vec2(screen_x, screen_y);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window, Camera* viewer, bool* AA) {
    glm::vec3 dir(0.0);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        dir.x = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        dir.x = -1;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        dir.z = -1;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        dir.z = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        dir.y = 1;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        dir.y = -1;
    }
    if (glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS) {
        *AA = !*AA;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
        fix_camera = !fix_camera;
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
        viewer->toggle_noclip();
    }
    viewer->move(dir);
    if (!fix_camera) {
        glm::vec2 mpos = get_mouse_pos(window);
        viewer->update(mpos, mpos_last);
        mpos_last = mpos;
    }
    
}

void renderQuad()
{
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}



int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Compute-Shader", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    //glfwSwapInterval(0);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    //load and complie shaders
    ComputeShader cShader("../src/computeshader.glsl");
    Shader vfShader("../src/quad.vert", "../src/quad.frag");

    vfShader.use();
    vfShader.setInt("tex", 0);

    //create the texture
    unsigned int texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, TEXTURE_WIDTH, TEXTURE_HEIGHT, 0, GL_RGBA, 
                GL_FLOAT, NULL);

    glBindImageTexture(0, texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Initalise scene
    Scene scene(
        glm::vec3(-10, 10, 0), // Light's postion
        9
    );
    scene.prepare(&cShader);
    // Initalise camera
    Camera viewer(glm::vec3(-5, 0, 0));
    // AA option
    bool AA = true;
    // get mouse postion
    mpos_last = get_mouse_pos(window);
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;
    int fCounter = 0;

    while(!glfwWindowShouldClose(window)) {
        //Inputs
        processInput(window, &viewer, &AA);
        viewer.to_shader(&cShader);
        //scene.set_light(viewer.position, &cShader);
        cShader.setBool("AA", AA);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        //-------Render loop-------//
        cShader.use();
        glDispatchCompute((unsigned int)TEXTURE_WIDTH / 8, (unsigned int)TEXTURE_HEIGHT / 8, 1);
        // make sure writing to image has finished before read
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        // render image to quad
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glActiveTexture(GL_TEXTURE0);
        vfShader.use();
        renderQuad();
        //-------Render end-------//

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (fCounter > 500) {
            cout << "FPS: " << 1 / deltaTime << endl;
            fCounter = 0;
        } else {
            fCounter++;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();


    } 
    glfwTerminate();
    return 0;
}