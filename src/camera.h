#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <cmath>
#include <iostream>

#define M_2PI 2 * M_PI
#define ROT_EPSILON 0.25

class Camera {
    private:
        float move_delta = 0.15;
        float tBob = 0.0;
        float bobAmp = 0.2;
        bool noclip = false;
        glm::vec3 position;
        glm::vec3 fowards = glm::vec3(1, 0, 0);
        glm::vec3 up;
        glm::vec3 right;
        const glm::vec3 global_up = glm::vec3(0, 1, 0);


        void recalculate_vectors() {
            fowards = glm::normalize(fowards);
            //fowards = glm::normalize(glm::vec3(cos(theta)*cos(phi), sin(theta)*cos(phi), sin(phi)));
            right = glm::normalize(glm::cross(fowards, global_up));
            up = glm::normalize(glm::cross(right, fowards));
        }

        void rotate_foward(float dtheta, float dphi) {
            float s = fowards.x > 0 ? 1 : -1;
            glm::mat4 hrot = glm::rotate(-glm::degrees(dtheta / 8), global_up);
            hrot = glm::rotate(hrot, glm::degrees(dphi / 10), right);
            fowards = hrot * glm::vec4(fowards, 1.0);
            recalculate_vectors();
        }
        
        void vec3_to_array(float a[], glm::vec3 v) {
            a[0] = v.x; 
            a[1] = v.y;
            a[2] = v.z;
        }
    public:
        Camera (glm::vec3 init_position) {
            position = init_position;
            recalculate_vectors();
        }

        void update(glm::vec2 mouse, glm::vec2 mouse_last) {
            glm::vec2 delta = mouse - mouse_last;
            float dtheta = atan(delta.x);
            float dphi = atan2(-delta.y,  sqrt(delta.x * delta.x + 1));
            rotate_foward(dtheta, dphi);
        }

        void toggle_noclip() {
            noclip = !noclip;
        }

        void move(glm::vec3 direction) {
            // direction = (foward coeff, up coeff, right coeff)
            if (noclip && (direction.x != 0 || direction.y != 0 || direction.z != 0)) {
                position += move_delta * glm::normalize(
                    direction.x * fowards +
                    direction.y * up +
                    direction.z * right
                );
            } else if (direction.x != 0 || direction.z != 0) {
                glm::vec3 fproj(fowards.x, 0, fowards.z); // Projection onto horozontal
                position += move_delta * glm::normalize(
                    direction.x * fproj +
                    direction.z * right
                );
                position.y = bobAmp * sin(tBob);
                tBob += move_delta;
            }
        }

        void to_shader(ComputeShader* shader) {
            shader->use();
            float p[3], f[3], u[3], r[3];
            vec3_to_array(p, position);
            vec3_to_array(f, fowards);
            vec3_to_array(u, up);
            vec3_to_array(r, right);
            shader->setVec3("viewer.position", p);
            shader->setVec3("viewer.fowards", f);
            shader->setVec3("viewer.up", u);
            shader->setVec3("viewer.right", r);
        }

    
};

#endif