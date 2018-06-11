#ifndef DISPLAY_H__
#define DISPLAY_H__

#include "Texture.h"
#include "Material.h"
#include "SSQuad.h"

namespace videocave {

    class Display {

    public:
        Display(int id, int w=300, int h=600, int num=1, bool yuv=true);
        ~Display();
        int initWindow(int width, int height);
        void update(const uint8_t* rgb_pixels);
        void setup();
        void render();

        GLFWwindow* window;

    private:
        

    private:
        int mId, mNumDisplay;
        int mWidth, mHeight; // image width and image height
        bool mInitialized;
        bool mIsYUV;
        
        Texture *mTexture;
        Material* mMaterial;
        SSQuad* mQuad;
    };

}; // namespace

#endif