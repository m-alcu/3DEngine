#pragma once
#include <algorithm>
#include "../slib.hpp"
#include "../color.hpp"

// solid color attribute not interpolated
class GouraudEffect
{
public:
	// the vertex type that will be input into the pipeline
	class Vertex
	{
	public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp, Color _color, bool _broken) :
    p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), color(_color), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
        return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal, ndc + v.ndc, color + v.color, true);
    }

    Vertex operator-(const Vertex &v) const {
        return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal, ndc - v.ndc, color - v.color, true);
    }

    Vertex operator*(const float &rhs) const {
        return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs, color * rhs, true);
    }


    Vertex& operator+=(const Vertex &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        ndc += v.ndc;
        color += v.color;
        return *this;
    }

    Vertex& hraster(const Vertex& v) {
        p_z += v.p_z;
        color += v.color;
        return *this;
    }
        
	public:
        int32_t p_x;
        int32_t p_y;
        float p_z; 
        slib::vec3 world;
        slib::vec3 normal;
        slib::vec4 ndc;
        Color color;
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
            vertex.normal = normalTransformMat * slib::vec4(vData.normal, 0);
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
            
        
            auto computeColor = [&](const slib::vec3& normal, const slib::vec3& world) -> Color {
                const slib::vec3& luxDirection = scene.light.getDirection(world);
                float ds = std::max(0.0f, smath::dot(normal, luxDirection)); // diffuse scalar
                return Color(Ka + Kd * ds); // assumes vec3 uses .r/g/b or [0]/[1]/[2]
            };

            for(auto& point : tri.points) {
                point.color = computeColor(point.normal, point.world);
            }        
        }
	};    

	class PixelShader
	{
	public:
		uint32_t operator()(Vertex& vRaster, const Scene& scene, Polygon<Vertex>& tri) const
		{
			return vRaster.color.toBgra();
		}
	};
public:
    VertexShader vs;
    GeometryShader gs;
	PixelShader ps;
};