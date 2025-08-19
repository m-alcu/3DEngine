#pragma once
#include <iostream>
#include <cstdint>
#include <cmath>
#include "scene.hpp"
#include "slib.hpp"
#include "smath.hpp"
#include "polygon.hpp"

template<class Effect>
class AmigaRasterizer {
    public:
        AmigaRasterizer() :  fullTransformMat(smath::identity()), 
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

        struct FaceDepthInfo {
            int index;           // index of the face
            float averageZ;      // used for sorting
            slib::vec3 rotatedFaceNormal;  // transformed face normal
        };

        std::vector<FaceDepthInfo> SortFaces(std::vector<FaceData>& faceData) {

            std::vector<FaceDepthInfo> faceIndicesWithDepth;
            // This does not change the size of the vector, just reserves space for future entries, this is more efficient
            faceIndicesWithDepth.reserve(faceData.size());

            for (int i = 0; i < static_cast<int>(faceData.size()); ++i) {
                const auto& faceDataEntry = faceData[i];
                const auto& face = faceDataEntry.face;

                slib::vec3 rotatedFaceNormal;
                rotatedFaceNormal = normalTransformMat * slib::vec4(faceDataEntry.faceNormal, 0);
                if (Visible(projectedPoints[face.vertexIndices[0]]->world, rotatedFaceNormal) || wireframe) {

                    float z1 = projectedPoints[face.vertexIndices[0]]->p_z;
                    float z2 = projectedPoints[face.vertexIndices[1]]->p_z;
                    float z3 = projectedPoints[face.vertexIndices[2]]->p_z;
                    float averageZ = z1 + z2 + z3;
                    faceIndicesWithDepth.emplace_back(i, averageZ, rotatedFaceNormal);
                }
            }
            
            if (!wireframe) {
                // If wireframe mode is not enabled, we need to cull back faces
                std::sort(faceIndicesWithDepth.begin(), faceIndicesWithDepth.end(),
                    [](const auto& a, const auto& b) {
                        return a.averageZ > b.averageZ;
                    });
            }

            return faceIndicesWithDepth;
        }


        void DrawFaces() {

            std::vector<FaceDepthInfo> faceIndicesWithDepth = SortFaces(solid->faceData);

            for (int i = 0; i < static_cast<int>(faceIndicesWithDepth.size()); ++i) {
                const auto& faceDataEntry = solid->faceData[faceIndicesWithDepth[i].index];
                const auto& face = faceDataEntry.face;

                const auto& idx = face.vertexIndices;
                if (idx.size() < 3) return; // nothing to draw

                // Build the vertex list for the polygon
                std::vector<vertex> polyVerts;
                polyVerts.reserve(idx.size());

                for (int i : idx) {
                    // (Optional) bounds/null checks:
                    if (i < 0 || static_cast<size_t>(i) >= projectedPoints.size() || !projectedPoints[i])
                        return; // or continue / handle error

                    const vertex& v = *projectedPoints[i];
                    polyVerts.push_back(v);
                }

                Polygon<vertex> poly(
                    std::move(polyVerts),
                    face,
                    faceIndicesWithDepth[i].rotatedFaceNormal,
                    solid->materials.at(face.materialKey)
                );

                ClipCullDrawPolygonSutherlandHodgman(poly); // Must be thread-safe!
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

            Polygon<vertex> tri(polygon , t.face, t.faceNormal, t.material);
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
                        vertex clippedVertex = prev + (curr - prev) * alpha;
                        effect.vs.viewProjection(clippedVertex, *scene);
                        output.push_back(clippedVertex);
                    } else {
                        float alpha = ComputeAlpha(curr, prev, plane);
                        vertex clippedVertex = curr + (prev - curr) * alpha;
                        effect.vs.viewProjection(clippedVertex, *scene);
                        output.push_back(clippedVertex);
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

        void draw(Polygon<vertex>& tri) {

            auto* pixels = static_cast<uint32_t*>(scene->sdlSurface->pixels);

            effect.gs(tri, *scene);

            if (wireframe) 
                drawWireframeAmiga(tri, 0xffffffff, pixels);
            else 
                drawAmiga(tri, tri.flatColor, pixels);
        };
        
        void drawAmiga(Polygon<vertex> tri, uint32_t color, uint32_t* pixels) {
            int x,y;

            std::vector<int16_t> line(2 * scene->screen.height);
            for(y=0;y<scene->screen.height;y++) { line[2*y+0] = scene->screen.width+1; line[2*y+1] = -1; }

            // Loop through vertices and chain to the next, including wrap-around
            for (size_t i = 0; i < tri.points.size(); i++) {
                auto& v0 = tri.points[i];
                auto& v1 = tri.points[(i + 1) % tri.points.size()]; // wrap back to first

                rasterizeAmiga(v0.p_x, v0.p_y, v1.p_x, v1.p_y, line.data());
            }

            for(y=0;y<scene->screen.height;y++)
                if(line[2*y+1]>line[2*y+0]) for(x=line[2*y+0]; x<=line[2*y+1]; x++) pixels[scene->screen.width*y+x] = color;
        }

        /*
        Bresenham's line algorithm for rasterizing lines.
        This algorithm is used to draw lines on a raster display.
        No floating point operations are used, only integer arithmetic.
        It works by calculating the difference in x and y coordinates between the two endpoints of the line and then incrementally drawing pixels along the line.
        */

        void rasterizeAmiga(int x0, int y0, int x1, int y1, int16_t *line) 
        {
            if((y0<0 && y1<0) || (y0>=scene->screen.height && y1>=scene->screen.height)) return; // exit if line outside rasterized area
            int dx = std::abs(x1 - x0);
            int dy = std::abs(y1 - y0);
            int err2,err = dx-dy;
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            
            while(1)  {
                if(y0>=0 && y0<scene->screen.height) {
                    if(x0<line[2*(y0)+0]) line[2*(y0)+0] = x0>0 ? x0 : 0;
                    if(x0>line[2*(y0)+1]) line[2*(y0)+1] = x0<scene->screen.width ? x0 : scene->screen.width-1;
                }

                if(x0==x1 && y0==y1)  return;
                err2 = err+err;
                if(err2 > -dy) { err -= dy; x0 += sx; }
                if(err2 < dx)  { err += dx; y0 += sy; }
            }
        }

        void drawWireframeAmiga(Polygon<vertex> tri, uint32_t color, uint32_t* pixels) {
            int width = scene->screen.width;
            int height = scene->screen.height;

            // Loop through vertices and chain to the next, including wrap-around
            for (size_t i = 0; i < tri.points.size(); i++) {
                auto& v0 = tri.points[i];
                auto& v1 = tri.points[(i + 1) % tri.points.size()]; // wrap back to first

                drawLine(v0.p_x, v0.p_y, v1.p_x, v1.p_y, pixels, width, height, color);
            }
        }

        void drawLine(int x0, int y0, int x1, int y1, uint32_t* pixels, int width, int height, uint32_t color) {
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
    

