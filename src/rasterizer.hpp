#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include "scene.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "polygon.hpp"

enum class ClipPlane {
    Left, Right, Bottom, Top, Near, Far
};

template<class Effect>
class Rasterizer {
    public:
        Rasterizer() :  fullTransformMat(smath::identity()), 
                        normalTransformMat(smath::identity()),
                        viewMatrix(smath::identity())
          {}

        void drawRenderable(Solid& solid, Scene& scn) {
            setRenderable(&solid);
            scene = &scn;
            prepareRenderable();
            ProcessVertex();
            DrawFaces();
        }

        void setWireframe(bool wireframeMode) {
            wireframe = wireframeMode;
        }

    private:
        typedef typename Effect::Vertex vertex;
        std::vector<std::unique_ptr<vertex>> projectedPoints;
        Solid* solid;  // Pointer to the abstract Solid
        Scene* scene; // Pointer to the Scene
        slib::mat4 fullTransformMat;
        slib::mat4 normalTransformMat;
        slib::mat4 viewMatrix;
        Effect effect;
        bool wireframe = false;
        
        void setRenderable(Solid* solidPtr) {
            projectedPoints.clear();
            projectedPoints.resize(solidPtr->numVertices);
            solid = solidPtr;
        }

        void prepareRenderable() {
            slib::mat4 rotate = smath::rotation(slib::vec3({solid->position.xAngle, solid->position.yAngle, solid->position.zAngle}));
            slib::mat4 translate = smath::translation(slib::vec3({solid->position.x, solid->position.y, solid->position.z}));
            slib::mat4 scale = smath::scale(slib::vec3({solid->position.zoom, solid->position.zoom, solid->position.zoom}));
            //slib::mat4 viewMatrix = smath::view(scene->camera.eye, scene->camera.target, scene->camera.up);
            viewMatrix = smath::fpsview(scene->camera.pos, scene->camera.pitch, scene->camera.yaw, scene->camera.roll);

            float pitch = scene->camera.pitch * RAD;
            float yaw = scene->camera.yaw * RAD;
            float cosPitch = cos(pitch);
            float sinPitch = sin(pitch);
            float cosYaw = cos(yaw);
            float sinYaw = sin(yaw);
            slib::vec3 zaxis = {sinYaw * cosPitch, -sinPitch, cosPitch * cosYaw};

            scene->camera.forward = zaxis;

            fullTransformMat = translate * rotate * scale;
            normalTransformMat = rotate;
        }

        void ProcessVertex()
        {
            projectedPoints.resize(solid->numVertices);
        
            std::transform(
                solid->vertexData.begin(),
                solid->vertexData.end(),
                projectedPoints.begin(),
                [&](const auto& vData) {
                    return effect.vs(vData, fullTransformMat, viewMatrix, normalTransformMat, *scene);
                }
            );
        }

        void DrawFaces() {

            //#pragma omp parallel for
            for (int i = 0; i < static_cast<int>(solid->faceData.size()); ++i) {
                const auto& faceDataEntry = solid->faceData[i];
                const auto& face = faceDataEntry.face;
                slib::vec3 rotatedFaceNormal;
                rotatedFaceNormal = normalTransformMat * slib::vec4(faceDataEntry.faceNormal, 0);

                vertex* p1 = projectedPoints[face.vertexIndices[0]].get();

                if (wireframe || Visible(p1->world, rotatedFaceNormal)) {

                    const auto& idx = face.vertexIndices;

                    // Build the vertex list for the polygon
                    std::vector<vertex> polyVerts;
                    polyVerts.reserve(idx.size());

                    for (int i : idx) {
                        const vertex& v = *projectedPoints[i];
                        polyVerts.push_back(v);
                    }

                    Polygon<vertex> poly(
                        std::move(polyVerts),
                        face,
                        rotatedFaceNormal,
                        solid->materials.at(face.materialKey)
                    );

                    // For n-gons; rename your function if it isn't triangle-specific anymore
                    ClipCullDrawPolygonSutherlandHodgman(poly); // must be thread-safe
                }

            }
        }

        /*
        Check if triangle is visible.
        If the triangle is visible, we can proceed with the rasterization process.
        The calculation is based on the cross product of the edges of the triangle.
        - If the result is positive, the triangle is visible.
        - If the result is negative, the triangle is not visible.edeeee
        - If the result is zero, the triangle is coplanar with the screen.
        This is a simplified version of the backface culling algorithm.
        The backface culling algorithm is used to determine if a triangle is facing the camera or not.
        If the triangle is facing away from the camera, we can skip the rasterization process.
        */
        bool Visible(const slib::vec3& world, const slib::vec3& faceNormal) {

            slib::vec3 viewDir = scene->camera.pos - world;
            float dotResult = smath::dot(faceNormal, viewDir);
            // Return whether the triangle is facing the camera
            return dotResult > 0.0f;
        };

        /*
        Clipping is done using the Sutherland-Hodgman algorithm (1974) in the ndc space.
        The Sutherland-Hodgman algorithm is a polygon clipping algorithm that clips a polygon against a convex clipping region.
        The algorithm works by iterating through each edge of the polygon and checking if the vertices are inside or outside the clipping plane.
        If a vertex is inside, it is added to the output polygon. If a vertex is outside, the algorithm checks if the previous vertex was inside. If it was, the edge between the two vertices is clipped and the intersection point is added to the output polygon.
        The algorithm continues until all edges have been processed.
        https://en.wikipedia.org/wiki/Sutherland%E2%80%93Hodgman_algorithm
        */

        void ClipCullDrawPolygonSutherlandHodgman(const Polygon<vertex>& t) {
            std::vector<vertex> polygon = t.points;

            for (ClipPlane plane : {ClipPlane::Left, ClipPlane::Right, ClipPlane::Bottom, 
                                    ClipPlane::Top, ClipPlane::Near, ClipPlane::Far}) {
                polygon = ClipAgainstPlane(polygon, plane);
                if (polygon.empty()) return; // Completely outside
            }

            Polygon<vertex> tri(polygon, t.face, t.faceNormal, t.material);
            draw(tri);
        }

        std::vector<vertex> ClipAgainstPlane(const std::vector<vertex>& poly, ClipPlane plane) {
            std::vector<vertex> output;
            if (poly.empty()) return output;
        
            vertex prev = poly.back();
            bool prevInside = IsInside(prev, plane);
        
            for (const auto& curr : poly) {
                bool currInside = IsInside(curr, plane);
        
                if (currInside != prevInside) {
                    // from inside to outside, we need to clip the edge always this way
                    if (prevInside) {
                        float alpha = ComputeAlpha(prev, curr, plane);
                        output.push_back(prev + (curr - prev) * alpha);
                    } else {
                        float alpha = ComputeAlpha(curr, prev, plane);
                        output.push_back(curr + (prev - curr) * alpha);
                    }
                }
                if (currInside)
                    output.push_back(curr);
                prev = curr;
                prevInside = currInside;
            }
        
            return output;
        } 
        
        bool IsInside(const vertex& v, ClipPlane plane) {
            const auto& p = v.ndc;
            switch (plane) {
                case ClipPlane::Left:   return p.x >= -p.w;
                case ClipPlane::Right:  return p.x <=  p.w;
                case ClipPlane::Bottom: return p.y >= -p.w;
                case ClipPlane::Top:    return p.y <=  p.w;
                case ClipPlane::Near:   return p.z >= -p.w;
                case ClipPlane::Far:    return p.z <=  p.w;
            }
            return false;
        }
        
        float ComputeAlpha(const vertex& a, const vertex& b, ClipPlane plane) {
            const auto& pa = a.ndc;
            const auto& pb = b.ndc;
            float num, denom;
        
            switch (plane) {
                case ClipPlane::Left:
                    num = pa.x + pa.w; denom = (pa.x + pa.w) - (pb.x + pb.w); break;
                case ClipPlane::Right:
                    num = pa.x - pa.w; denom = (pa.x - pa.w) - (pb.x - pb.w); break;
                case ClipPlane::Bottom:
                    num = pa.y + pa.w; denom = (pa.y + pa.w) - (pb.y + pb.w); break;
                case ClipPlane::Top:
                    num = pa.y - pa.w; denom = (pa.y - pa.w) - (pb.y - pb.w); break;
                case ClipPlane::Near:
                    num = pa.z + pa.w; denom = (pa.z + pa.w) - (pb.z + pb.w); break;
                case ClipPlane::Far:
                    num = pa.z - pa.w; denom = (pa.z - pa.w) - (pb.z - pb.w); break;
            }
        
            return denom != 0.0f ? num / denom : 0.0f;
        }
        
        /*
        Drawing a triangle with scanline rasterization.
        The algorithm works by iterating through each scanline of the triangle and determining the left and right edges of the triangle at that scanline.
        For each scanline, the algorithm calculates the x-coordinates of the left and right edges of the triangle and fills in the pixels between them.
        The algorithm uses a slope to determine the x-coordinates of the left and right edges of the triangle at each scanline.
        */

        class Slope
        {
            vertex begin, step;
        public:
            Slope() {}
            Slope(vertex from, vertex to, int num_steps)
            {
                float inv_step = 1.f / num_steps;
                begin = from;                   // Begin here
                step  = (to - from) * inv_step; // Stepsize = (end-begin) / num_steps
            }
            vertex get() const { return begin; }
            int getx() const { return begin.p_x >> 16; }
            void advance()    { begin += step; }
        };

        void draw(Polygon<vertex>& tri) {

            auto* pixels = static_cast<uint32_t*>(scene->pixels);

            for (auto& point : tri.points) {
                effect.vs.viewProjection(*scene, point);
            }

            if (wireframe) {
                drawWireframe(tri, 0xffffffff, pixels);
				return; // No rasterization in wireframe mode
            }

            auto begin = std::begin(tri.points), end = std::end(tri.points);

            // Find the point that is topleft-most. Begin both slopes (left & right) from there.
            // Also find the bottomright-most vertex; that’s where the rendering ends.
            auto cmp_top_left = [&](const vertex& a, const vertex& b) {
                return std::tie(a.p_y, a.p_x) < std::tie(b.p_y, b.p_x); // top-left is "smaller"
            };
            auto [first, last] = std::minmax_element(begin, end, cmp_top_left);

            std::array cur { first, first };
            auto gety = [&](int side) -> int { return cur[side]->p_y >> 16; };

            effect.gs(tri, *scene);
            
            int forwards = 1;
            Slope slopes[2] {};
            for(int side = 0, cury = gety(side), nexty[2] = {cury,cury}, hy = cury * scene->screen.width; cur[side] != last; )
            {
                // We have reached a bend on either side (or both). "side" indicates which side the next bend is.
                // In the beginning of the loop, both sides have a bend (top-left corner of the polygon).
                // In that situation, we first process the left side, then without rendering anything, process the right side.
                // Now check whether to go forwards or backwards in the circular chain of points.
                auto prev = std::move(cur[side]);

                if(side == forwards) cur[side] = (std::next(prev) == end) ? begin : std::next(prev);
                else                 cur[side] = std::prev(prev == begin ? end : prev);

                nexty[side]  = gety(side);
                slopes[side] = Slope(*prev, *cur[side], nexty[side] - cury);

                // Identify which side the next bend is going to be, by choosing the smaller Y coordinate.
                side = (nexty[0] <= nexty[1]) ? 0 : 1;
                // Process scanlines until the next bend.
                for(int limit = nexty[side]; cury < limit; ++cury, hy+= scene->screen.width)
                    DrawScanline(hy, slopes[0], slopes[1], tri, pixels);

            }                   

        };

       
        inline void DrawScanline(const int& hy, Slope& left, Slope& right, Polygon<vertex>& tri, uint32_t* pixels) {
            
            int xStart = left.getx();
            int xEnd = right.getx();
            int dx = xEnd - xStart;
        
            if (dx != 0) {
                float invDx = 1.0f / dx;
                vertex vStart = left.get();
                vertex vStep = (right.get() - vStart) * invDx;
        
                for (int x = xStart; x < xEnd; ++x) {
                    int index = hy + x;
                    if (scene->zBuffer->TestAndSet(index, vStart.p_z)) {
                        pixels[index] = effect.ps(vStart, *scene, tri);
                    }
                    vStart += vStep;
                }
            }
        
            left.advance();
            right.advance();
        } 

        void drawWireframe(Polygon<vertex> tri, uint32_t color, uint32_t* pixels) {
            int width = scene->screen.width;
            int height = scene->screen.height;

            // Loop through vertices and chain to the next, including wrap-around
            for (size_t i = 0; i < tri.points.size(); i++) {
                auto& v0 = tri.points[i];
                auto& v1 = tri.points[(i + 1) % tri.points.size()]; // wrap back to first

                drawBresenhamLine(v0.p_x >> 16, v0.p_y >> 16, v1.p_x >> 16, v1.p_y >> 16, pixels, width, height, color);
            }
        }

        void drawBresenhamLine(int x0, int y0, int x1, int y1, uint32_t* pixels, int width, int height, uint32_t color) {
            int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
            int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
            int err = dx + dy, e2;

            while (true) {
                if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
                    pixels[y0 * width + x0] = color;

                if (x0 == x1 && y0 == y1) break;
                e2 = 2 * err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
            }
        }

    };
    

