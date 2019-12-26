//
// Created by paladin on 12/14/19.
//

#include <cstdlib>
#include <deque>
#include <boost/asio.hpp>
#include "gl/gl.h"
#include "client/game.h"


namespace networking {

#define SIZE 100


    static void on_glfw_error(int error, const char* description) {
        fprintf(stderr, "Error: %i,%s\n", error, description);
    }


    int client(std::string host) {
        glfwSetErrorCallback(on_glfw_error);
        if (!glfwInit()) {
            printf("GLFW NOT INITIALIZED\n");
            exit(EXIT_FAILURE);
        }
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        GLFWwindow* window = glfwCreateWindow(1000, 1000, "GLMinecraft3", nullptr, nullptr);

        glfwMakeContextCurrent(window);
        gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
        glfwSwapInterval(1);

        gl_check_error();

        bool is_first_frame = true;

        client::game game(window);
        game.initialize();
        game.download_world(host);

        while (!glfwWindowShouldClose(window)) {
            glGetError();

            game.loop();

            glfwSwapBuffers(window);
            glfwPollEvents();

            if (is_first_frame) {
                glfwSetWindowPos(window, 150, 150);
                is_first_frame = false;
            }

            gl_check_error();
        }
        game.end();
        glfwTerminate();

        return 0;
    }
}