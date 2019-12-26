//
// Created by Jack Armstrong on 11/7/19.
//

#include "game.h"
#include <thread>
#include <boost/asio.hpp>

namespace client {

    game::game(GLFWwindow* window) {
        this->window = window;
    }

    void game::initialize() {
        world=std::make_shared<block::world>();
        shader = std::make_shared<gl::shader>("test", "test");
        texture = std::make_shared<gl::texture>("1.8_textures_0.png");
        basic_shader=std::make_shared<gl::shader>("basic","basic");
        gl::mesh_data cube_data;
        cube_data.buffers.push_back({3,{
            // 000 001 010 011 100 101 110 111

            0,0,0, 1,0,0,
            0,0,0, 0,1,0,
            0,0,0, 0,0,1,

            0,0,1, 1,0,0,
            0,0,1, 0,1,0,
            0,0,1, 0,0,0,

            0,1,0, 1,1,0,
            0,1,0, 0,0,0,
            0,1,0, 0,1,1,

            0,1,1, 1,1,1,
            0,1,1, 0,0,1,
            0,1,1, 0,1,0,

            1,0,0, 0,0,0,
            1,0,0, 1,1,0,
            1,0,0, 1,0,1,

            1,1,0, 0,1,0,
            1,1,0, 1,0,0,
            1,1,0, 1,1,1,

            1,1,1, 0,1,1,
            1,1,1, 1,0,1,
            1,1,1, 1,1,0,

        }});
        cube_data.tri={
            0,1,
            2,3,
            4,5,
            6,7,
            8,9,
            10,11,
            12,13,
            14,15,
            16,17,
            18,19,
            20,21,
            22,23,
            24,25,
            26,27,
            28,29,
            30,31,
            32,33,
            34,35,
            36,37,
            38,39,
            40,41
        };
        basic_cube=std::make_shared<gl::mesh>(&cube_data);
    }

    void game::loop() {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClearColor(0.5, 0.7, 0.9, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 p = glm::perspective(80.0F, 1.0F, 0.01F, 1000.0F);
        glm::mat4 v = glm::lookAt(glm::vec3(cos(glfwGetTime() * 0.25) * 16 * WORLD_SIZE / 2 + 16 * WORLD_SIZE / 2,
                                            sin(glfwGetTime() * 0.25) * 32 + 64,
                                            sin(glfwGetTime() * 0.25) * 16 * WORLD_SIZE / 2 + 16 * WORLD_SIZE / 2),
                                  glm::vec3(WORLD_SIZE * 8, 60, WORLD_SIZE * 8), glm::vec3(0, -1, 0));

        shader->bind();
        shader->uniform4x4("perspective", p);
        shader->uniform4x4("view", v);
        shader->texture("tex", texture, 0);

        for (int x = 0; x < WORLD_SIZE; x++) {
            for (int z = 0; z < WORLD_SIZE; z++) {
                rendered_world[x][z]->render(shader);
            }
        }

        basic_shader->bind();
        basic_shader->uniform4x4("perspective", p);
        basic_shader->uniform4x4("view", v);
        for(int x=0;x<WORLD_SIZE;x++){
            for(int y=0;y<16;y++){
                for(int z=0;z<WORLD_SIZE;z++){
                    basic_shader->uniform4x4("mode",glm::translate(glm::mat4(1),{x*16,y*16,z*16}));
                    basic_cube->render_lines();
                }
            }
        }

        if (glfwGetKey(window, GLFW_KEY_Q)) {
            std::raise(11);
        }
    }

    void game::end() {

    }

    void game::download_world(std::string host) {
        try {
            boost::asio::io_context io_context;

            boost::asio::ip::tcp::resolver resolver(io_context);
            boost::asio::ip::tcp::resolver::results_type endpoints=resolver.resolve(host,"daytime");

            boost::asio::ip::tcp::socket socket(io_context);
            boost::asio::connect(socket,endpoints);

            boost::system::error_code err;
            for(int x=0;x<WORLD_SIZE;x++) {
                for (int y = 0; y < 16; y++) {
                    for (int z = 0; z < WORLD_SIZE; z++) {
                        boost::array<long, 4096>arr{};
                        size_t len = boost::asio::read(socket, boost::asio::buffer(arr));
//                        printf("%zu %zu\n",len,sizeof(long),sizeof(long)*4096);
//                        boost::asio::read(socket,arr);
                        if(err==boost::asio::error::eof){
                            printf("UNEXPECTED EOF %i %i %i\n",x,y,z);
                            std::raise(11);
                        }else if(err){
                            throw boost::system::system_error(err);
                        }
                        world->map[x][z]->read(y,arr);
                    }
                }
            }



            for (int x = 0; x < WORLD_SIZE; x++) {
                for (int z = 0; z < WORLD_SIZE; z++) {
                    rendered_world[x][z]=std::make_shared<rendered_chunk>(glm::ivec2{x,z});
                    rendered_world[x][z]->take_chunk(world,world->map[x][z]);
                    rendered_world[x][z]->render_chunk();
                }
            }
        }catch(std::exception&e){
            std::cerr<<"client ex caught: "<<e.what()<<"\n";
            std::raise(11);
        }
    }

}