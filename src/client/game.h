//
// Created by Jack Armstrong on 11/7/19.
//

#ifndef GLMINECRAFT3_GAME_H
#define GLMINECRAFT3_GAME_H

#include "gl/gl.h"
#include "gl/shader.h"
#include "gl/mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "block_rendering.h"
#include "block/block.h"
#include <csignal>

namespace client {

    class Game {
    public:

        std::shared_ptr<gl::Shader>shader;
        std::shared_ptr<gl::Mesh>mesh;
        std::shared_ptr<gl::Texture>texture;

        GLFWwindow* window;

        Game(GLFWwindow* window);

        void initialize();

        void loop();

        void end();

    };

}

#endif //GLMINECRAFT3_GAME_H