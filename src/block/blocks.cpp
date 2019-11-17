//
// Created by Jack Armstrong on 11/16/19.
//

#include "blocks.h"

namespace block {

    const Block *fromID(int id) {

        switch (id) {
            case BLOCK_ID_GRASS:
                return &GRASS;
            case BLOCK_ID_DIRT:
                return &DIRT;
            case BLOCK_ID_STONE:
                return &STONE;
        }

//        printf("le wot %i\n", id);
//        std::raise(11);
        return &NONE;

    }

}