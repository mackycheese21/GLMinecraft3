//
// Created by Jack Armstrong on 12/29/19.
//

#ifndef GLMINECRAFT3_GAME_ROOM_H
#define GLMINECRAFT3_GAME_ROOM_H

#include <set>
#include <map>
#include "world/world.h"
#include "server_player.h"
#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>            // uuid class
#include <boost/uuid/uuid_generators.hpp> // generators
#include <boost/uuid/uuid_io.hpp>         // streaming operators etc.

namespace server {

    struct game_room {
        block::world world;
        std::map<std::string, std::shared_ptr<nbt::nbt>> entities;
        std::set<server_player_ptr> players;

        boost::asio::deadline_timer timer;

        entity_type *et_base;
        entity_type *et_player;
        entity_type *et_zombie;

        std::mutex protect_game_state;
        std::string queued_chat;

        std::shared_ptr<nbt::nbt> get_entity_list() {
            auto nbt_entities = new nbt::nbt_compound();
            for (const auto &e:entities) {
                nbt_entities->value[e.first] = e.second;
            }
            return std::shared_ptr<nbt::nbt>(nbt_entities);
        }

        entity_type *get_type(const std::shared_ptr<nbt::nbt> &e) {
            int id = nbt::cast_int(nbt::cast_compound(e)->value["entity_type_id"])->value;
            if (id == 1)return et_player;
            if (id == 2)return et_zombie;
            return et_base;
        }

        void frame_handler(boost::system::error_code err) {
            {
                std::lock_guard<std::mutex> guard(protect_game_state);

                for (const auto &e:entities) {
                    get_type(e.second)->update(e.second, this);
                }

                broadcast_to_all(nbt::make_compound({
                                                            {"entities", get_entity_list()},
                                                            {"chat",     nbt::make_string(queued_chat)}
                                                    }));
                queued_chat = "";
            }
            //NO LOGIC OUTSIDE OF THIS
            int FPS = 30;
            timer.expires_at(timer.expires_at() + boost::posix_time::milliseconds((int) (1000 / FPS)));
            timer.async_wait([this](boost::system::error_code err) {
                frame_handler(err);
            });
        }

        explicit game_room(boost::asio::io_context &io_context) : timer(io_context,
                                                                        boost::posix_time::milliseconds(0)) {
            world.generate_world();
            frame_handler(boost::system::error_code());

            et_base = new entity_type_base();
            et_player = new entity_type_player();
            et_zombie = new entity_type_zombie();
        }

        ~game_room() {
            delete et_base;
            delete et_player;
            delete et_zombie;
        }

        std::string get_next_entity_id() {
            std::stringstream str;
            str << boost::uuids::random_generator()();
            return str.str();
        }

        std::string spawn_entity(const std::shared_ptr<nbt::nbt> &e, glm::vec3 position) {
            std::string i = get_next_entity_id();
            nbt::cast_compound(e)->value["id"] = nbt::make_string(i);
            nbt::cast_compound(e)->value["position"] = nbt::make_list(
                    {nbt::make_float(position.x), nbt::make_float(position.y), nbt::make_float(position.z)});
            entities[i] = e;
            return i;
        }

        void join(const server_player_ptr &ptr) {
            std::lock_guard<std::mutex> guard(protect_game_state);
            spawn_entity(et_zombie->initialize(), {16.1F, 150, 16.1F});
            ptr->send_world(world);
            std::shared_ptr<nbt::nbt> entity = et_player->initialize();
            std::string id = spawn_entity(entity, {24, 150, 24});
            nbt::cast_compound(entity)->value["name"] = nbt::make_string(id);
            ptr->deliver(nbt::make_compound({
                                                    {"player_id", nbt::make_string(id)},
                                                    {"entities",  get_entity_list()}
                                            }));
            players.insert(ptr);
            ptr->entity_id = id;
            std::cout << "PLAYER " << ptr->entity_id << " JOINED\n";
            queued_chat = ptr->entity_id + " joined the game";
        }

        void handle_player_interaction_packet(const server_player_ptr &player, const std::shared_ptr<nbt::nbt> data) {
            std::shared_ptr<nbt::nbt_compound> compound = nbt::cast_compound(data);
            std::lock_guard<std::mutex> guard(protect_game_state);
            std::shared_ptr<nbt::nbt_compound> ent = nbt::cast_compound(entities[player->entity_id]);
            {
                std::shared_ptr<nbt::nbt_compound> movement = nbt::cast_compound(compound->value["movement"]);
                bool left = nbt::cast_short(movement->value["left"])->value;
                bool right = nbt::cast_short(movement->value["right"])->value;
                bool front = nbt::cast_short(movement->value["front"])->value;
                bool back = nbt::cast_short(movement->value["back"])->value;
                bool sprint = nbt::cast_short(movement->value["sprint"])->value;
                bool jump = nbt::cast_short(movement->value["jump"])->value;

                std::shared_ptr<nbt::nbt_list> pos = nbt::cast_list(ent->value["position"]);
                std::shared_ptr<nbt::nbt_list> look = nbt::cast_list(ent->value["lookdir"]);
                std::shared_ptr<nbt::nbt_list> vel = nbt::cast_list(ent->value["velocity"]);
                float d = 1;
                glm::vec3 newMotion{0, 0, 0};
                glm::vec3 curLook{nbt::cast_float(look->value[0])->value, nbt::cast_float(look->value[1])->value,
                                  nbt::cast_float(look->value[2])->value};
                glm::vec3 curVel{nbt::cast_float(vel->value[0])->value, nbt::cast_float(vel->value[1])->value,
                                 nbt::cast_float(vel->value[2])->value};
                glm::vec3 leftdir = glm::cross(curLook, glm::vec3{0, -1, 0});
                leftdir.y = 0;
                leftdir = glm::normalize(leftdir);
                glm::vec3 forward = curLook;
                forward.y = 0;
                forward = glm::normalize(forward);
                if (front)newMotion += forward * d * (float) (1 + sprint);
                if (back)newMotion -= forward * d;
                if (left)newMotion += leftdir * d;
                if (right)newMotion -= leftdir * d;
                if (jump)curVel.y += 2.5;
                ent->value["motion"] = nbt::make_list(
                        {nbt::make_float(newMotion.x), nbt::make_float(newMotion.y), nbt::make_float(newMotion.z)});
                ent->value["velocity"] = nbt::make_list(
                        {nbt::make_float(curVel.x), nbt::make_float(curVel.y), nbt::make_float(curVel.z)});
            }
            {
                std::shared_ptr<nbt::nbt_list> nlook = nbt::cast_list(compound->value["lookdir"]);
                std::shared_ptr<nbt::nbt_list> look = nbt::cast_list(ent->value["lookdir"]);
                nbt::cast_float(look->value[0])->value = nbt::cast_float(nlook->value[0])->value;
                nbt::cast_float(look->value[1])->value = nbt::cast_float(nlook->value[1])->value;
                nbt::cast_float(look->value[2])->value = nbt::cast_float(nlook->value[2])->value;
            }
            {
                std::string chat = nbt::cast_string(compound->value["chat"])->value;
                //TODO: commands will be parsed here
                if (!chat.empty())queued_chat = "<" + player->entity_id + "> " + chat;
            }
        }

        void kill_entity(const std::string &id) {

            entities.erase(id);
        }

        void leave(const server_player_ptr &ptr) {
            players.erase(ptr);
            kill_entity(ptr->entity_id);
            std::cout << "PLAYER " << ptr->entity_id << " LEFT\n";
            queued_chat = ptr->entity_id + " left the game";
        }

        void broadcast_to_all(const std::shared_ptr<nbt::nbt> &msg) {
            for (const auto &p:players)p->deliver(msg);
        }
    };

}

#endif //GLMINECRAFT3_GAME_ROOM_H
