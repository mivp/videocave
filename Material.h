#ifndef MATERIAL_H__
#define MATERIAL_H__

#include <glm/gtx/transform.hpp>
#include "GLSLProgram.h"

namespace videocave {

    class Material {

    protected:
        GLSLProgram* m_shader;
        string m_shaderDir;

    public:
        Material();
        ~Material();

        GLSLProgram* getShader() { return m_shader; }
    };

    /**
     */
    class ImageRGBMaterial: public Material {

    protected:

    public:
        ImageRGBMaterial();
        ~ImageRGBMaterial();
    };

    /**
     */
    class ImageYUVMaterial: public Material {

    protected:

    public:
        ImageYUVMaterial();
        ~ImageYUVMaterial();
    };

}; //namespace videocave

#endif
