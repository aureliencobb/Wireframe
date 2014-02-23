//
//  ParametricSurface.hpp
//  WireframeSkeleton
//
//  Created by Aurelien Cobb on 22/02/2014.
//
//

#ifndef WireframeSkeleton_ParametricSurface_h
#define WireframeSkeleton_ParametricSurface_h

#include "Interfaces.hpp"

struct ParametricInterval {
    ivec2 Divisions;
    vec2 UpperBound;
};

class ParametricSurface : public ISurface {
public:
    int GetVertexCount() const;
    int GetLineIndexCount() const;
    int GetTriangleIndexCount() const;
    void GenerateVertices(vector<float>& vertices, unsigned char flags) const;
    void GenerateLineIndices(vector<unsigned short>& indices) const;
    void GenerateTriangleIndices(vector<unsigned short>& indices) const;
protected:
    void SetInterval(const ParametricInterval& interval);
    virtual vec3 Evaluate(const vec2& domain) const = 0;
    virtual bool InvertNormal(const vec2& domain) const {return false;}
private:
    vec2 ComputeDomain(float i, float j) const;
    vec2 m_upperBound;
    ivec2 m_slices;
    ivec2 m_divisions;
};

#endif
