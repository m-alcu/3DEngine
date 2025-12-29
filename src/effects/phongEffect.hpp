#pragma once
#include <cmath>
#include "../slib.hpp"
#include "../color.hpp"

// solid color attribute not interpolated
class PhongEffect
{
public:
	// the vertex type that will be input into the pipeline
	class Vertex
	{
	public:
    Vertex() {}

    Vertex(int32_t px, int32_t py, float pz, slib::vec3 n, slib::vec4 vp) :
    p_x(px), p_y(py), p_z(pz), normal(n), ndc(vp) {}

    Vertex operator+(const Vertex &v) const {
        return Vertex(p_x + v.p_x, p_y + v.p_y, p_z + v.p_z, normal + v.normal, ndc + v.ndc);
    }

    Vertex operator-(const Vertex &v) const {
        return Vertex(p_x - v.p_x, p_y - v.p_y, p_z - v.p_z, normal - v.normal, ndc - v.ndc);
    }

    Vertex operator*(const float &rhs) const {
        return Vertex(p_x * rhs, p_y * rhs, p_z * rhs, normal * rhs, ndc * rhs);
    }


    Vertex& operator+=(const Vertex &v) {
        p_x += v.p_x;
        p_y += v.p_y;
        p_z += v.p_z;
        normal += v.normal;
        ndc += v.ndc;
        return *this;
    }

    Vertex& hraster(const Vertex& v) {
        p_z += v.p_z;
        normal += v.normal;
        return *this;
    }
        
	public:
        int32_t p_x;
        int32_t p_y;
        float p_z; 
        slib::vec3 world;
        slib::vec3 normal;
        slib::vec4 ndc;
	};

	class VertexShader
	{
	public:
        Vertex operator()(const VertexData& vData, const slib::mat4& fullTransformMat, const slib::mat4& normalTransformMat, const Scene& scene) const
		{
            Vertex vertex;
            vertex.world = fullTransformMat * slib::vec4(vData.vertex, 1);
            vertex.ndc = slib::vec4(vertex.world, 1) * scene.viewMatrix * scene.projectionMatrix;
            vertex.normal = normalTransformMat * slib::vec4(vData.normal, 0);
            return vertex;
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

            const auto& Ka = tri.material.Ka; // vec3
            const auto& Kd = tri.material.Kd; // vec3
            const auto& Ks = tri.material.Ks; // vec3
            const slib::vec3& luxDirection = scene.light.getDirection(vRaster.world);

            slib::vec3 normal = smath::normalize(vRaster.normal);
            float diff = std::max(0.0f, smath::dot(normal,luxDirection));
        
            slib::vec3 R = normal * 2.0f * smath::dot(normal,luxDirection) - luxDirection;
            // NOTE: For performance we approximate the per-fragment view vector V with -camera.forward.
            // This assumes all view rays are parallel (like an orthographic camera).
            // Works well when the camera is far away or objects are small on screen.
            // Not physically correct: highlights will "stick" to the camera instead of sliding across
            // surfaces when moving in perspective, but it’s often a good enough approximation.
            float specAngle = std::max(0.0f, smath::dot(R, scene.forwardNeg)); // viewer
            float spec = std::pow(specAngle, tri.material.Ns);
        
            slib::vec3 color = Ka + Kd * diff + Ks * spec;
            return Color(color).toBgra(); // assumes vec3 uses .r/g/b or [0]/[1]/[2]
		}
	};
public:
    VertexShader vs;
    GeometryShader gs;
	PixelShader ps;
};