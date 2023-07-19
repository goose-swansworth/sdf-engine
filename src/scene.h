#ifndef SCENE_H
#define SCENE_H

#include <glm/glm.hpp>
#include <random>

#include "computeshader.h"
#include "stb_image.h"

#define BYTES_PER_BOX 12


class Scene {

    private:

        void init_boxes(float boxData[]) {
            //box layout (l, w, h, _) (x, y, z, _) (r, g, b, a)
            std::default_random_engine generator;
            std::uniform_int_distribution<int> dims_pool(1, 2);
            std::uniform_int_distribution<int> pos_pool(-20, 20);
            std::uniform_int_distribution<int> col_pool(0, 255);
            for (int i = 0; i < num_boxes; i++) {
                float w = (float)dims_pool(generator);
                boxData[i * BYTES_PER_BOX] = w;
                boxData[i * BYTES_PER_BOX + 1] = w;
                boxData[i * BYTES_PER_BOX + 2] = w;
                boxData[i * BYTES_PER_BOX + 3] = 0.0f;

                boxData[i * BYTES_PER_BOX + 4] = pos_pool(generator);
                boxData[i * BYTES_PER_BOX + 5] = w - 1;
                boxData[i * BYTES_PER_BOX + 6] = pos_pool(generator);
                boxData[i * BYTES_PER_BOX + 7] = 0.0f;

                boxData[i * BYTES_PER_BOX + 8] = (float)col_pool(generator)/255;
                boxData[i * BYTES_PER_BOX + 9] = (float)col_pool(generator)/255;
                boxData[i * BYTES_PER_BOX + 10] = (float)col_pool(generator)/255;
                boxData[i * BYTES_PER_BOX + 11] = 1.0f;
            }
        }

        void create_box_buffer() {
            float boxData[num_boxes * BYTES_PER_BOX];
            init_boxes(boxData);
            glGenBuffers(1, &boxSSB);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, boxSSB);
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(boxData), boxData, GL_DYNAMIC_READ);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, boxSSB);
        }

        void loadTextureLump() {
            glGenTextures(1, &textureID);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            
            // load the lumped textures
            unsigned char* lump = stbi_load("../textures/lumped.png", &txtLumpWidth, &txtLumpHeight, &numChannels, 0);
            if (lump) {
                std::cout << "Lumped texture size: " << txtLumpWidth << ", " << txtLumpHeight << std::endl;
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, txtLumpWidth, txtLumpHeight, 0, GL_RGBA, 
                        GL_UNSIGNED_BYTE, lump);
                glBindImageTexture(2, textureID, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
            } else {
                std::cout << "Failed to load texture" << std::endl;
            }
            

        }

    public:
        float light[3];
        float floor_height = 1;
        uint num_boxes;
        unsigned int boxSSB;
        float floor1[4] = {0.8, 0.8, 0.8, 1.0};
        float floor2[4] = {0.7, 0.7, 0.7, 1.0};
        float sky_color[4] = {0.392, 0.583, 0.929, 1.0};

        int txtLumpWidth;
        int txtLumpHeight;
        int numChannels;
        unsigned int textureID;

        Scene(glm::vec3 init_light, uint n_boxes) {
            light[0] = init_light.x; 
            light[1] = init_light.y;
            light[2] = init_light.z;
            num_boxes = n_boxes;
        }

        // Send scene data to the shader
        void prepare(ComputeShader* shader) {
            shader->use();
            shader->setVec3("light", light);
            shader->setFloat("FLOOR", floor_height);
            shader->setVec4("FLOOR1", floor1);
            shader->setVec4("FLOOR2", floor2);
            shader->setVec4("sky_color", sky_color);
            shader->setInt("num_boxes", num_boxes);

            create_box_buffer();
            loadTextureLump();
        }

        void set_light(glm::vec3 new_light, ComputeShader* shader) {
            light[0] = new_light.x; 
            light[1] = new_light.y;
            light[2] = new_light.z;
            shader->setVec3("light", light);
        }

};

#endif