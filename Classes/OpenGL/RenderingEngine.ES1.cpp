//
//  RenderingEngine.ES1.cpp
//  WireframeSkeleton
//
//  Created by Aurelien Cobb on 22/02/2014.
//
//


#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"

namespace ES1 {
    
struct Drawable {
    GLuint VertexBuffer;
    GLuint IndexBuffer;
    int IndexCount;
};

class RenderingEngine : public IRenderingEngine {
public:
    RenderingEngine();
    void Initialize(const vector<ISurface*>& surfaces);
    void Render(const vector<Visual>& visuals) const;
private:
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    mat4 m_translation;
};

IRenderingEngine * CreateRenderingEngine() {
    return new RenderingEngine();
}

RenderingEngine::RenderingEngine() {
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
}

void RenderingEngine::Initialize(const vector<ISurface *> &surfaces) {
    vector<ISurface *>::const_iterator surface;
    for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {
        // Create VBO for vertices
        vector<float> vertices;
        (*surface)->GenerateVertices(vertices);
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
        
        // create VBO for indices (if needed)
        int indexCount = (*surface)->GetLineIndexCount();
        GLuint indexBuffer;
        if (!m_drawables.empty() && indexCount == m_drawables[0].IndexCount) {
            indexBuffer = m_drawables[0].IndexBuffer;
        } else {
            vector<GLushort> indices(indexCount);
            (*surface)->GenerateLineIndices(indices);
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);
        }
        Drawable drawable = { vertexBuffer, indexBuffer, indexCount };
        m_drawables.push_back(drawable);
    }
    // Create the framebuffer object
    GLuint frameBuffer;
    glGenFramebuffersOES(1, &frameBuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    m_translation = mat4::Translate(0, 0, -7);
}
    
void RenderingEngine::Render(const vector<Visual>& visuals) const {
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    vector<Visual>::const_iterator visual = visuals.begin();
    for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {
        
        // Viewport Transform
        ivec2 size = visual->ViewportSize;
        ivec2 lowerLeft = visual->LowerLeft;
        glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);
        
        // Modelview Transform
        mat4 rotation = visual->Orientation.ToMatrix();
        mat4 modelView = rotation * m_translation;
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(modelView.Pointer());
        
        // Projection Transform
        float h = 4.0 * size.y / size.x;
        mat4 projection = mat4::Frustum(-2, 2, -h/2, h/2, 5, 10);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection.Pointer());
        
        // set color
        vec3 color = visual->Color;
        glColor4f(color.x, color.y, color.z, 1.0);
        
        // Draw wireframe
        int stride = sizeof(vec3);
        const Drawable& drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexPointer(3, GL_FLOAT, stride, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_LINES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    }
}
    
}