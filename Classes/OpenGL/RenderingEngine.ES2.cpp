//
//  RenderingEngine.ES1.cpp
//  WireframeSkeleton
//
//  Created by Aurelien Cobb on 22/02/2014.
//
//


#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#include <iostream>
#include <assert.h>
#include "Interfaces.hpp"
#include "Matrix.hpp"

#define STRINGIFY(A) #A
#include "../../Shaders/PixelLighting.vert"
#include "../../Shaders/PixelLighting.frag"

struct UniformHandles {
    GLuint ModelView;
    GLuint Projection;
    GLuint NormalMatrix;
    GLuint LightPosition;
    GLint Ambient;
    GLint Specular;
    GLint Shininess;
};

struct AttributeHandles {
    GLint Position;
    GLint Normal;
    GLint Diffuse;
    GLint TextureCoord;
};

namespace ES2 {
    
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
    void SetPngTexture(const string & name) const;
    GLuint BuildProgram(const char* vertexShaderSource, const char* fragmentShaderSource) const;
    GLuint BuildShader(const char* source, GLenum shaderType) const;
    vector<Drawable> m_drawables;
    GLuint m_colorRenderbuffer;
    GLuint m_depthRenderbuffer;
    GLuint m_gridTexture;
    IResourceManager * m_resourceManager;
    mat4 m_translation;
    UniformHandles m_uniforms;
    AttributeHandles m_attributes;
};

IRenderingEngine * CreateRenderingEngine(IResourceManager * resourceManager) {
    return new RenderingEngine(resourceManager);
}

RenderingEngine::RenderingEngine(IResourceManager * resourceManager) {
    m_resourceManager = resourceManager;
    glGenRenderbuffers(1, &m_colorRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
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
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
    
    glGenRenderbuffers(1, &m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
    
    // Create the framebuffer object
    GLuint frameBuffer;
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_colorRenderbuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthRenderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_colorRenderbuffer);
    
    GLuint program = BuildProgram(SimpleVertexShader, SimpleFragmentShader);
    glUseProgram(program);
    
    m_attributes.Position = glGetAttribLocation(program, "Position");
    m_attributes.Normal = glGetAttribLocation(program, "Normal");
    m_attributes.Diffuse = glGetAttribLocation(program, "DiffuseMaterial");
    m_attributes.TextureCoord = glGetAttribLocation(program, "TextureCoord");
    
    m_uniforms.Projection = glGetUniformLocation(program, "Projection");
    m_uniforms.ModelView = glGetUniformLocation(program, "Modelview");
    m_uniforms.NormalMatrix = glGetUniformLocation(program, "NormalMatrix");
    m_uniforms.LightPosition = glGetUniformLocation(program, "LightPosition");
    m_uniforms.Ambient = glGetUniformLocation(program, "AmbientMaterial");
    m_uniforms.Specular = glGetUniformLocation(program, "SpecularMaterial");
    m_uniforms.Shininess = glGetUniformLocation(program, "Shininess");
    
    // Load the texture
    glGenTextures(1, &m_gridTexture);
    glBindTexture(GL_TEXTURE_2D, m_gridTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    SetPngTexture("Grid16.png");
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // some default material parameters
    glVertexAttrib3f(m_uniforms.Ambient, 0.04, 0.04, 0.04);
    glVertexAttrib3f(m_uniforms.Specular, 0.5, 0.5, 0.5);
    glVertexAttrib1f(m_uniforms.Shininess, 50.0);
    
    glEnableVertexAttribArray(m_attributes.Position);
    glEnableVertexAttribArray(m_attributes.Normal);
    glEnableVertexAttribArray(m_attributes.TextureCoord);
    
    m_translation = mat4::Translate(0, 0, -7);
}

void RenderingEngine::Render(const vector<Visual>& visuals) const {
    glClearColor(0.0f, 0.125f, 0.25f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    vector<Visual>::const_iterator visual = visuals.begin();
    for (int visualIndex = 0; visual != visuals.end(); ++visual, ++visualIndex) {
        
        // Viewport Transform
        ivec2 size = visual->ViewportSize;
        ivec2 lowerLeft = visual->LowerLeft;
        glViewport(lowerLeft.x, lowerLeft.y, size.x, size.y);
        
        // Light Position
        vec4 lightPosition(0.25, 0.25, 1, 0);
        glUniform3fv(m_uniforms.LightPosition, 1, lightPosition.Pointer());
        
        // Model-View Transform
        mat4 rotation = visual->Orientation.ToMatrix();
        mat4 modelview = rotation * m_translation;
        glUniformMatrix4fv(m_uniforms.ModelView, 1, 0, modelview.Pointer());
        
        // Set Normal Matrix
        // (It is orthogonal, so its inverse transpose is itself)
        mat3 normalMatrix = modelview.ToMat3();
        glUniformMatrix3fv(m_uniforms.NormalMatrix, 1, 0, normalMatrix.Pointer());
        
        // Projection Transform
        float h = 4.0 * size.y / size.x;
        mat4 projectionMatrix = mat4::Frustum(-2, 2, -h / 2, h / 2, 5, 10);
        glUniformMatrix4fv(m_uniforms.Projection, 1, 0, projectionMatrix.Pointer());
        
        // Diffuse Color
        vec3 color = visual->Color * 0.75;
        glVertexAttrib4f(m_attributes.Diffuse, color.x, color.y, color.z, 1);
        
        // Draw the surface
        int stride = sizeof(vec3) * 2 + sizeof(vec2);
        const GLvoid * normalOffset = (const GLvoid *)sizeof(vec3);
        const GLvoid * texCoordOffset = (const GLvoid *)(sizeof(vec3) * 2);
        GLint position = m_attributes.Position;
        GLint normal = m_attributes.Normal;
        GLint texCoord = m_attributes.TextureCoord;
        
        const Drawable & drawable = m_drawables[visualIndex];
        glBindBuffer(GL_ARRAY_BUFFER, drawable.VertexBuffer);
        glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, stride, 0);
        glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, stride, normalOffset);
        glVertexAttribPointer(texCoord, 2, GL_FLOAT, GL_FALSE, stride, texCoordOffset);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, drawable.IndexBuffer);
        glDrawElements(GL_TRIANGLES, drawable.IndexCount, GL_UNSIGNED_SHORT, 0);
    }
}
    
void RenderingEngine::SetPngTexture(const string &name) const {
    TextureDescription description = m_resourceManager->LoadPngImage(name);
    GLenum format;
    switch (description.Format) {
        case TextureFormatGrey:
            format = GL_LUMINANCE;
            break;
        case TextureFormatGreyAlpha:
            format = GL_LUMINANCE_ALPHA;
            break;
        case TextureFormatRGB:
            format = GL_RGB;
            break;
        case TextureFormatRGBA:
            format = GL_RGBA;
            break;
    }
    GLenum type;
    switch (description.BitsPerComponent) {
        case 8:
            type = GL_UNSIGNED_BYTE;
            break;
        case 4:
            if (format == GL_RGBA) {
                type = GL_UNSIGNED_SHORT_4_4_4_4;
                break;
            }
            // intentionally fall through
        default:
            assert(!"Unsupported Format");
            break;
    }
    void * data = m_resourceManager->GetImageData();
    ivec2 size = description.size;
    glTexImage2D(GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, type, data);
    m_resourceManager->UnloadImage();
}
    
GLuint RenderingEngine::BuildProgram(const char* vertexShaderSource, const char* fragmentShaderSource) const {
    GLuint vertexShader = BuildShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = BuildShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vertexShader);
    glAttachShader(programHandle, fragmentShader);
    glLinkProgram(programHandle);
    GLint linkSuccess;
    glGetProgramiv(programHandle, GL_LINK_STATUS, &linkSuccess);
    if (linkSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(programHandle, sizeof(messages), 0, &messages[0]); std::cout << messages;
        exit(1);
    }
    return programHandle;
}
    
GLuint RenderingEngine::BuildShader(const char* source, GLenum shaderType) const {
    GLuint shaderHandle = glCreateShader(shaderType);
    glShaderSource(shaderHandle, 1, &source, 0);
    glCompileShader(shaderHandle);
    GLint compileSuccess;
    glGetShaderiv(shaderHandle, GL_COMPILE_STATUS, &compileSuccess);
    if (compileSuccess == GL_FALSE) {
        GLchar messages[256];
        glGetShaderInfoLog(shaderHandle, sizeof(messages), 0, &messages[0]); std::cout << messages;
        exit(1);
    }
    return shaderHandle;
}
    
}