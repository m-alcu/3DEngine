#pragma once
#include "../slib.hpp"
#include "../color.hpp"
#include "../textureSampler.hpp"

// solid color attribute not interpolated
class TexturedFlatEffect
{
public:
	// the vertex type that will be input into the pipeline
	class Vertex
	{
	public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec4 vp, slib::zvec2 _tex, bool _broken) :
    p_x(px), p_y(py), p_z(pz), ndc(vp), tex(_tex), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
        return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, ndc + v.ndc, tex + v.tex, true);
    }

    Vertex operator-(const Vertex &v) const {
        return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, ndc - v.ndc, tex - v.tex, true);
    }

    Vertex operator*(const float &rhs) const {
        return Vertex(p_x * rhs, p_y, p_z * rhs, ndc * rhs, tex * rhs, true);
    }

    Vertex& operator+=(const Vertex &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        ndc += v.ndc;
        tex += v.tex;
		broken = true;
        return *this;
    }

    Vertex& hraster(const Vertex& v) {
        p_z += v.p_z;
        tex += v.tex;
        return *this;
    }
        
	public:
        int32_t p_x;
        int32_t p_y;
        float p_z; 
        slib::vec3 world;
        slib::vec4 ndc;
        slib::zvec2 tex; // Texture coordinates
		slib::zvec2 texOverW; // tex divided by w for interpolation
		bool broken = false;
	};

	class VertexShader
	{
	public:
        Vertex operator()(const VertexData& vData, const slib::mat4& fullTransformMat, const slib::mat4& normalTransformMat, const Scene& scene) const
		{
            Vertex vertex;
            Projection<Vertex> projection;
            vertex.world = fullTransformMat * slib::vec4(vData.vertex, 1);
            vertex.ndc = slib::vec4(vertex.world, 1) * scene.viewMatrix * scene.projectionMatrix;
            vertex.tex = slib::zvec2(vData.texCoord.x, vData.texCoord.y, 1);
			projection.view(scene, vertex);
            return vertex;
		}
	};

    class GeometryShader
	{
	public:
    
        void operator()(Polygon<Vertex>& tri, const Scene& scene) const
		{

            const auto& Ka = tri.material.Ka; // vec3
            const auto& Kd = tri.material.Kd; // vec3
			const slib::vec3& luxDirection = scene.light.getDirection(tri.points[0].world); // any point aproximately the same
            tri.flatDiffuse = std::max(0.0f, smath::dot(tri.rotatedFaceNormal, luxDirection));
		}
	};

	class PixelShader
	{
	public:
		uint32_t operator()(Vertex& vRaster, const Scene& scene, Polygon<Vertex>& tri) const
		{
			TextureSampler<Vertex> sampler(vRaster, tri.material.map_Kd, tri.material.map_Kd.textureFilter);
            return sampler.sample(tri.flatDiffuse, 0, 0, 0).toBgra();
		}
	};
public:
    VertexShader vs;
    GeometryShader gs;
	PixelShader ps;
};