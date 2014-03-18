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
    RenderingEngine(IResourceManager * resourceManager);
    void Initialize(const vector<ISurface*>& surfaces);
    void Render(const vector<Visual>& visuals) const;
private:
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    GLuint m_gridTexture;
    IResourceManager * m_resourceManager;
    mat4 m_translation;
};

IRenderingEngine * CreateRenderingEngine(IResourceManager * resourceManager) {
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager * resourceManager) {
    m_resourceManager = resourceManager;
    glGenRenderbuffersOES(1, &m_colorRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
}

void RenderingEngine::Initialize(const vector<ISurface *> &surfaces) {
    glEnable(GL_DEPTH_TEST);

    vector<ISurface *>::const_iterator surface;
    for (surface = surfaces.begin(); surface != surfaces.end(); ++surface) {
        // Create VBO for vertices
        vector<float> vertices;
        (*surface)->GenerateVertices(vertices, VertexFlagsNormal | VertexFlagsTexCoords);
        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);
        
        // create VBO for indices (if needed)
        int indexCount = (*surface)->GetTriangleIndexCount();
        GLuint indexBuffer;
        if (!m_drawables.empty() && indexCount == m_drawables[0].IndexCount) {
            indexBuffer = m_drawables[0].IndexBuffer;
        } else {
            vector<GLushort> indices(indexCount);
            (*surface)->GenerateTriangleIndices(indices);
            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);
        }
        Drawable drawable = { vertexBuffer, indexBuffer, indexCount };
        m_drawables.push_back(drawable);
    }
    
    // Depth Buffer
    int width, height;
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_WIDTH_OES, &width);
    glGetRenderbufferParameterivOES(GL_RENDERBUFFER_OES, GL_RENDERBUFFER_HEIGHT_OES, &height);
    
    glGenRenderbuffersOES(1, &m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT16_OES, width, height);
    
    // Create the framebuffer object
    GLuint frameBuffer;
    glGenFramebuffersOES(1, &frameBuffer);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, frameBuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, m_depthRenderbuffer);
    glBindRenderbufferOES(GL_RENDERBUFFER_OES, m_colorRenderbuffer);
    glEnableClientState(GL_VERTEX_ARRAY);
    
    // Load texture
    glGenTextures(1, &m_gridTexture);
    glBindTexture(GL_TEXTURE_2D, m_gridTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_resourceManager->LoadPngImage("Grid16.png");
    void * pixels = m_resourceManager->GetImageData();
    ivec2 imageSize = m_resourceManager->GetImageSize();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageSize.x, imageSize.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    m_resourceManager->UnloadImage();
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnable(GL_TEXTURE_2D);
    
    // Light
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // set up material properties
    vec4 specular(0.5, 0.5, 0.5, 1);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular.Pointer());
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50.0);
    
    m_translation = mat4::Translate(0, 0, -7);
}
    
void RenderingEngine::Render(const vector<Visual>& visuals) const {
    glClearColor(0.5f, 0.5f, 0.5f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vector<Visual>::const_iterator visual = visuals.begin();
    for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {
        
        // Viewport Transform
        ivec2 size = visual->ViewportSize;
        ivec2 lowerLeft = visual->LowerLeft;
        glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);
        
        // Set light position
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        vec4 lightPosition(0.25, 0.25, 1, 0);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition.Pointer());
        
        // Modelview Transform
        mat4 rotation = visual->Orientation.ToMatrix();
        mat4 modelView = rotation * m_translation;
        glLoadMatrixf(modelView.Pointer());
        
        // Projection Transform
        float h = 4.0 * size.y / size.x;
        mat4 projection = mat4::Frustum(-2, 2, -h/2, h/2, 5, 10);
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(projection.Pointer());
        
        // set diffuse color
        vec3 color = visual->Color * 0.75;
        vec4 diffuse(color.x, color.y, color.z, 1);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse.Pointer());
        
        // Draw the surface
        int stride = sizeof(vec3) * 2 + sizeof(vec2);
        const GLvoid * texCoordOffset = (const GLvoid *)(2* sizeof(vec3));
        const Drawable& drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexPointer(3, GL_FLOAT, stride, 0);
        const GLvoid * normalOffset = (const GLvoid *)sizeof(vec3);
        glNormalPointer(GL_FLOAT, stride, normalOffset);
        glTexCoordPointer(2, GL_FLOAT, stride, texCoordOffset);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    }
}
    
}