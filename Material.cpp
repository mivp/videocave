#include "Material.h"
#include <iostream>

namespace videocave {

    Material::Material(): m_shader(0) {
        m_shaderDir = SHADER_DIR;
        std::cout << "Shader dir: " << SHADER_DIR << std::endl;

    }

    Material::~Material() {
        if(m_shader)
            delete m_shader;
    }

    /**
     */
    ImageRGBMaterial::ImageRGBMaterial(): Material() {
        m_shader = new GLSLProgram();
        string filename;
        filename = m_shaderDir; filename.append("image_rgb.vert");
        m_shader->compileShader(filename.c_str());
        filename = m_shaderDir; filename.append("image_rgb.frag");
        m_shader->compileShader(filename.c_str());
        m_shader->link();
    }

    ImageRGBMaterial::~ImageRGBMaterial() {

    }

    /**
     */
    ImageYUVMaterial::ImageYUVMaterial(): Material() {
        m_shader = new GLSLProgram();
        string filename;
        filename = m_shaderDir; filename.append("image_yuv.vert");
        m_shader->compileShader(filename.c_str());
        filename = m_shaderDir; filename.append("image_yuv.frag");
        m_shader->compileShader(filename.c_str());
        m_shader->link();
    }

    ImageYUVMaterial::~ImageYUVMaterial() {

    }

}; //namespace videocave
