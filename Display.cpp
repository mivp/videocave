#include "Display.h"
#include <iostream>
#include <sstream>

using namespace std;

namespace videocave {

    Display::Display(int id, int w, int h, int num, bool yuv): 
                        mId(id), mNumDisplay(num), mWidth(w), mHeight(h), mInitialized(false),
                        mTexture(0), mMaterial(0), mQuad(0),
                        mIsYUV(yuv)
    {

    }

    Display::~Display()
    {
        if(mTexture) delete mTexture;
        if(mMaterial) delete mMaterial;
        if(mQuad) delete mQuad;
    }

    void Display::update(const uint8_t* pixels) {
        if(!mInitialized)
            return;
        mTexture->updateTexture(pixels);
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
            mTexture = new Texture(mWidth, mHeight, 3, 0);
            mTexture->initTexture();
            mMaterial = new ImageRGBMaterial();
        }
        else {
            cout << mId << ": Display::setup YUV() " << mWidth << " " << mHeight << endl;
            mTexture = new Texture(mWidth/2, mHeight, 4, 0);
            mTexture->initTexture();

            mMaterial = new ImageYUVMaterial();
        }

        mQuad = new SSQuad();
        //mQuad->init(mId, mNumDisplay);
        mQuad->init(1, 1);

        mInitialized = true;
    }   

    void Display::render() {
        setup();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f,0.0f,0.0f,0.0f);
        
        GLSLProgram* shader = mMaterial->getShader();
        shader->bind();

        mTexture->bind();
        shader->setUniform("uTexY", (int)0);

        mQuad->draw();
        
        shader->unbind();

        glfwSwapBuffers(window);
    }

}; // namespace