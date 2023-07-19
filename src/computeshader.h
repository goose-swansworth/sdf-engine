

#ifndef COMPUTESHADER_H
#define COMPUTESHADER_H

#include "glad.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class ComputeShader {
    public:
        unsigned int ID;

        ComputeShader(const char* computePath) {
            std::ifstream computeFile;
            std::string computeCode_s;
            computeFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
            try {
                //open shader file
                computeFile.open(computePath);
                std::stringstream computeStream;
                // read file's buffer contents into stream
                computeStream << computeFile.rdbuf();
                //close shader file
                computeFile.close();
                //conver stream into string
                computeCode_s = computeStream.str();
            } catch (std::ifstream::failure e) {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
            }
            unsigned int compute;
            const char* computeCode = computeCode_s.c_str();
            // compute shader
            compute = glCreateShader(GL_COMPUTE_SHADER);
            glShaderSource(compute, 1, &computeCode, NULL);
            glCompileShader(compute);
            checkCompileErrors(compute, "COMPUTE");

            // shader Program
            ID = glCreateProgram();
            glAttachShader(ID, compute);
            glLinkProgram(ID);
            checkCompileErrors(ID, "PROGRAM");

            glDeleteShader(compute);
        }

        void use() {
            glUseProgram(ID);
        }
        // utility uniform functions
        // ------------------------------------------------------------------------
        void setBool(const std::string &name, bool value) const
        {         
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
        }
        // ------------------------------------------------------------------------
        void setInt(const std::string &name, int value) const
        { 
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
        }
        // ------------------------------------------------------------------------
        void setFloat(const std::string &name, float value) const
        { 
            glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
        }
        void setVec3(const std::string &name, float vec[]) {
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, vec);
        }
        void setVec4(const std::string &name, float vec[]) {
            glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, vec);
        }
    private:
    // utility function for checking shader compilation/linking errors.
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};
#endif

