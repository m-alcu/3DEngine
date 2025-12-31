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

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp, float _diffuse, bool _broken) :
    p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), diffuse(_diffuse), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
        return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal, ndc + v.ndc, diffuse + v.diffuse, true);
    }

    Vertex operator-(const Vertex &v) const {
        return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal, ndc - v.ndc, diffuse - v.diffuse, true);
    }

    Vertex operator*(const float &rhs) const {
        return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs, diffuse * rhs, true);
    }


    Vertex& operator+=(const Vertex &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        ndc += v.ndc;
        diffuse += v.diffuse;
        return *this;
    }

    Vertex& hraster(const Vertex& v) {
        p_z += v.p_z;
        diffuse += v.diffuse;
        return *this;
    }
        
	public:
        int32_t p_x;
        int32_t p_y;
        float p_z; 
        slib::vec3 world;
        slib::vec3 normal;
        slib::vec4 ndc;
        float diffuse;
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
            projection.view(scene, vertex, true);
            return vertex;
		}      
	};
    
    class GeometryShader
	{
	public:
    
        void operator()(Polygon<Vertex>& poly, const Scene& scene) const
        {
            Projection<Vertex> projection;
            for(auto& point : poly.points) {
                const slib::vec3& luxDirection = scene.light.getDirection(point.world);
                point.diffuse = std::max(0.0f, smath::dot(point.normal, luxDirection)); // diffuse scalar
                projection.view(scene, point, false);
            }        
        }
	};    

	class PixelShader
	{
	public:
		uint32_t operator()(Vertex& vRaster, const Scene& scene, Polygon<Vertex>& poly) const
		{
			return Color(poly.material.Ka + poly.material.Kd * vRaster.diffuse).toBgra();
		}
	};
public:
    VertexShader vs;
    GeometryShader gs;
	PixelShader ps;
};