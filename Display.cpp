#include "Display.h"
#include <iostream>
#include <sstream>

using namespace std;

namespace videocave {

    Display::Display(int id, int w, int h, int num, bool yuv): 
                        mId(id), mNumDisplay(num), mWidth(w), mHeight(h), mInitialized(false),
                        mTextureY(0), mTextureU(0), mTextureV(0), mMaterial(0), mQuad(0),
                        mIsYUV(yuv)
    {

    }

    Display::~Display()
    {
        if(mTextureY) delete mTextureY;
        if(mTextureU) delete mTextureU;
        if(mTextureV) delete mTextureV;
        if(mMaterial) delete mMaterial;
        if(mQuad) delete mQuad;
    }

    void Display::update(const uint8_t* rgb_pixels) {
        if(mIsYUV || !mInitialized)
            return;
        mTextureY->updateTexture(rgb_pixels);
    }

    void Display::update(  const uint8_t* pixels_y, int stride_y, const uint8_t* pixels_u, int stride_u,
                            const uint8_t* pixels_v, int stride_v) {

        if(!mIsYUV)
            return;
        mTextureY->updateTexture(pixels_y, stride_y);
        mTextureU->updateTexture(pixels_u, stride_u);
        mTextureV->updateTexture(pixels_v, stride_v);
    }

    int Display::initWindow(int width, int height) {
         // Initialise GLFW
        if( !glfwInit() ) {
            fprintf( stderr, "Failed to initialize GLFW\n" );
            exit(1);
        }

        // OpenGL 4.1
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

        // Open a window and create its OpenGL context
        std::ostringstream ss;
        if(mId == 0)
            ss << "Master (" << mId << ")";
        else 
            ss << "Video viewer client (" << mId << ")";
        window = glfwCreateWindow( width, height, ss.str().c_str(), NULL, NULL);
        if( window == NULL ){
            cout << mId << ": Failed to open GLFW window: " << stderr << endl;
            glfwTerminate();
            exit(1);
        }
        glfwMakeContextCurrent(window);
        //glfwSetKeyCallback(window, key_callback);
        //glfwSetWindowSizeCallback(window, window_size_callback);

        // Initialize GLEW
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            cout << mId << ": Failed to initialize GLEW: " << stderr << endl;
            exit(1);
        }

        return 0;
     }
            
    void Display::setup() {

        if(mInitialized)
            return;

        if(!mIsYUV) {
            cout << mId << ": Display::setup RGB() " << mWidth << " " << mHeight << endl;
            mTextureY = new Texture(mWidth, mHeight, 3, 0);
            mTextureY->initTexture();
            mMaterial = new ImageRGBMaterial();
        }
        else {
            cout << mId << ": Display::setup YUV() " << mWidth << " " << mHeight << endl;
            mTextureY = new Texture(mWidth, mHeight, 1, 0);
            mTextureY->initTexture();

            mTextureU = new Texture(mWidth/2, mHeight/2, 1, 1);
            mTextureU->initTexture();

            mTextureV = new Texture(mWidth/2, mHeight/2, 1, 2);
            mTextureV->initTexture();

            mMaterial = new ImageYUVMaterial();
        }

        mQuad = new SSQuad();
        mQuad->init(mId, mNumDisplay);

        mInitialized = true;
    }   

    void Display::render() {
        setup();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f,0.0f,0.0f,0.0f);
        
        GLSLProgram* shader = mMaterial->getShader();
        shader->bind();

        mTextureY->bind();
        shader->setUniform("uTexY", (int)0);
        if(mIsYUV) {
            mTextureU->bind();
            mTextureV->bind();
            shader->setUniform("uTexU", (int)1);
            shader->setUniform("uTexV", (int)2);
        }

        mQuad->draw();
        
        shader->unbind();

        glfwSwapBuffers(window);
    }

}; // namespace