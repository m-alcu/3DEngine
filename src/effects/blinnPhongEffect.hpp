#pragma once
#include <cmath>
#include "../slib.hpp"
#include "../color.hpp"

// solid color attribute not interpolated
class BlinnPhongEffect
{
public:
	// the vertex type that will be input into the pipeline
	class Vertex
	{
	public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp, slib::vec3 _world, bool _broken) :
    p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp), world(_world), broken(_broken) {}

    Vertex operator+(const Vertex &v) const {
        return Vertex(p_x + v.p_x, p_y, p_z + v.p_z, normal + v.normal, ndc + v.ndc, world + v.world, true);
    }

    Vertex operator-(const Vertex &v) const {
        return Vertex(p_x - v.p_x, p_y, p_z - v.p_z, normal - v.normal, ndc - v.ndc, world - v.world, true);
    }

    Vertex operator*(const float &rhs) const {
        return Vertex(p_x * rhs, p_y, p_z * rhs, normal * rhs, ndc * rhs, world * rhs, true);
    }

    Vertex& operator+=(const Vertex &v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
		world += v.world;
        ndc += v.ndc;
        return *this;
    }

    Vertex& vraster(const Vertex& v) {
        p_x += v.p_x;
        p_z += v.p_z;
        normal += v.normal;
        world += v.world;
        return *this;
    }

    Vertex& hraster(const Vertex& v) {
        p_z += v.p_z;
        normal += v.normal;
		world += v.world;
        return *this;
    }
        
	public:
        int32_t p_x;
        int32_t p_y;
        float p_z; 
        slib::vec3 world;
        slib::vec3 normal;
        slib::vec4 ndc;
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
            for (auto& point : poly.points) {
                projection.view(scene, point, false);
            }
		}
	};      

	class PixelShader
	{
	public:
		uint32_t operator()(Vertex& vRaster, const Scene& scene, Polygon<Vertex>& poly) const
		{

            const auto& Ka = poly.material.Ka; // vec3
            const auto& Kd = poly.material.Kd; // vec3
            const auto& Ks = poly.material.Ks; // vec3
            const slib::vec3& luxDirection = scene.light.getDirection(vRaster.world);
            // Normalize vectors
            slib::vec3 N = smath::normalize(vRaster.normal); // Normal at the fragment
            slib::vec3 L = luxDirection; // Light direction
            //slib::vec3 V = scene.eye; // Viewer direction (you may want to define this differently later)
        
            // Diffuse component
            float diff = std::max(0.0f, smath::dot(N,L)) * scene.light.intensity;
        
            // Halfway vector H = normalize(L + V)
            //slib::vec3 H = smath::normalize(L + V);
            const slib::vec3& halfwayVector = smath::normalize(luxDirection - scene.camera.forward);
        
            // Specular component: spec = (N · H)^shininess
            float specAngle = std::max(0.0f, smath::dot(N,halfwayVector)); // viewer
            float spec = std::pow(specAngle, poly.material.Ns); // Blinn Phong shininess needs *4 to be like Phong
        
            slib::vec3 color = Ka + Kd * diff + Ks * spec;
            return Color(color).toBgra();
        }
	};
public:
    VertexShader vs;
    GeometryShader gs;
    PixelShader ps;
};