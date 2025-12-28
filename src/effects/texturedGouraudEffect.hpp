#pragma once
#include <algorithm>
#include "../slib.hpp"
#include "../color.hpp"
#include "../textureSampler.hpp"

// solid color attribute not interpolated
class TexturedGouraudEffect
{
public:
	// the vertex type that will be input into the pipeline
	class Vertex
	{
	public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::zvec2 _tex, float _diffuse) :
    p_x(px), p_y(py), p_z(pz), ndc(vp), tex(_tex), diffuse(_diffuse) {}

    Vertex operator+(const Vertex &v) const {
        return Vertex(p_x + v.p_x, p_y + v.p_y, p_z + v.p_z, ndc + v.ndc, tex + v.tex, diffuse + v.diffuse);
    }

    Vertex operator-(const Vertex &v) const {
        return Vertex(p_x - v.p_x, p_y - v.p_y, p_z - v.p_z, ndc - v.ndc, tex - v.tex, diffuse - v.diffuse);
    }

    Vertex operator*(const float &rhs) const {
        return Vertex(p_x * rhs, p_y * rhs, p_z * rhs, ndc * rhs, tex * rhs, diffuse * rhs);
    }

    Vertex& operator+=(const Vertex &v) {
        p_x += v.p_x;
        p_y += v.p_y;
        p_z += v.p_z;
        ndc += v.ndc;
        tex += v.tex;
        diffuse += v.diffuse;
        return *this;
    }

    Vertex& hraster(const Vertex& v) {
        p_z += v.p_z;
        tex += v.tex;
        diffuse += v.diffuse;
        return *this;
    }
        
	public:
        int32_t p_x;
        int32_t p_y;
        float p_z; 
        slib::vec3 world;
        slib::vec3 point;
        slib::vec4 ndc;
        slib::zvec2 tex; // Texture coordinates
        float diffuse; // Diffuse color
	};

    class VertexShader
	{
	public:
        std::unique_ptr<Vertex> operator()(const VertexData& vData, const slib::mat4& fullTransformMat, const slib::mat4& normalTransformMat, const Scene& scene) const
		{
            Vertex screenPoint;
            slib::vec3 normal;
            screenPoint.world = fullTransformMat * slib::vec4(vData.vertex, 1);
            screenPoint.point =  slib::vec4(screenPoint.world, 1) * scene.viewMatrix;
            screenPoint.ndc = slib::vec4(screenPoint.point, 1) * scene.projectionMatrix;
            screenPoint.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
            normal = normalTransformMat * slib::vec4(vData.normal, 0);
            screenPoint.diffuse = std::max(0.0f, smath::dot(normal, scene.light.getDirection(screenPoint.world)));
            return std::make_unique<Vertex>(screenPoint);
		} 
	};
    
    class GeometryShader
	{
	public:
    
        void operator()(Polygon<Vertex>& tri, const Scene& scene) const
        {
        }
	};    

	class PixelShader
	{
	public:
		uint32_t operator()(Vertex& vRaster, const Scene& scene, Polygon<Vertex>& tri) const
		{

            TextureSampler<Vertex> sampler(vRaster, tri.material.map_Kd, tri.material.map_Kd.textureFilter);
            return sampler.sample(vRaster.diffuse, 0, 0, 0).toBgra();
		}
	};
public:
    VertexShader vs;
    GeometryShader gs;
	PixelShader ps;
};