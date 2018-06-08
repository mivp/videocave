#ifndef SSQUAD_H
#define SSQUAD_H

namespace videocave {

    class SSQuad
    {
    public:
        SSQuad();

        void init(int id, int numdisplay);
        void draw();

    private:
        static const float vertices[8];
        static float texcoors[8];
        static const unsigned int indices[6];
        unsigned int vvertices;
        unsigned int vindices;
        unsigned int vtexcoors;
        unsigned int vaoHandle;
    };
    
}; // namespace

#endif // SSQUAD_H
