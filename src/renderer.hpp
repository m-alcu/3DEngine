#pragma once

#include <iostream>
#include <cstdint>
#include "objects/solid.hpp"
#include "rasterizer.hpp"
#include "effects/flatEffect.hpp"
#include "effects/gouraudEffect.hpp"
#include "effects/blinnPhongEffect.hpp"
#include "effects/phongEffect.hpp"
#include "effects/texturedFlatEffect.hpp"
#include "effects/texturedGouraudEffect.hpp"
#include "effects/texturedPhongEffect.hpp"
#include "effects/texturedBlinnPhongEffect.hpp"

class Renderer {

    public:

        void drawScene(Scene& scene, float zNear, float zFar, float viewAngle) {

            scene.drawBackground();

            prepareFrame(scene, zNear, zFar, viewAngle);
            for (auto& solidPtr : scene.solids) {
                switch (solidPtr->shading) {
                    case Shading::Flat: 
                        flatRasterizer.setWireframe(false);
                        flatRasterizer.drawRenderable(*solidPtr, scene);
                        break;   
                    case Shading::Wireframe: 
                        flatRasterizer.setWireframe(true);
                        flatRasterizer.drawRenderable(*solidPtr, scene);
                        break;                                              
                    case Shading::TexturedFlat: 
                        texturedFlatRasterizer.drawRenderable(*solidPtr, scene);
                        break;                             
                    case Shading::Gouraud: 
                        gouraudRasterizer.drawRenderable(*solidPtr, scene);
                        break;
                    case Shading::TexturedGouraud: 
                        texturedGouraudRasterizer.drawRenderable(*solidPtr, scene);
                        break;                        
                    case Shading::BlinnPhong:
                        blinnPhongRasterizer.drawRenderable(*solidPtr, scene);
                        break;  
                    case Shading::TexturedBlinnPhong:
                        texturedBlinnPhongRasterizer.drawRenderable(*solidPtr, scene);
                        break;                                                       
                    case Shading::Phong:
                        phongRasterizer.drawRenderable(*solidPtr, scene);
                        break;      
                    case Shading::TexturedPhong:
                        texturedPhongRasterizer.drawRenderable(*solidPtr, scene);
                        break;                                             
                    default: flatRasterizer.drawRenderable(*solidPtr, scene);
                }
            }
        }

        void prepareFrame(Scene& scene, float zNear, float zFar, float viewAngle) {

            //std::fill_n(scene.pixels, scene.screen.width * scene.screen.height, 0);
            std::copy(scene.backg, scene.backg + scene.screen.width * scene.screen.height, scene.pixels);
            scene.zBuffer->Clear(); // Clear the zBuffer

            zNear = 100.0f; // Near plane distance
            zFar = 10000.0f; // Far plane distance
            viewAngle = 45.0f; // Field of view angle in degrees   

            float aspectRatio = (float) scene.screen.width / scene.screen.height; // Width / Height ratio
            float fovRadians = viewAngle * (PI / 180.0f);
        
            scene.projectionMatrix = smath::perspective(zFar, zNear, aspectRatio, fovRadians);

            if (scene.orbiting) {
                scene.viewMatrix = smath::lookAt(scene.camera.pos, scene.camera.orbitTarget, { 0,1,0 });
            }
            else {
                scene.viewMatrix = smath::fpsview(scene.camera.pos, scene.camera.pitch, scene.camera.yaw, scene.camera.roll);
            }

            // Used in BlinnPhong shading
            // NOTE: For performance we approximate the per-fragment view vector V with -camera.forward.
            // This assumes all view rays are parallel (like an orthographic camera).
            // Works well when the camera is far away or objects are small on screen.
            // Not physically correct: highlights will "stick" to the camera instead of sliding across
            // surfaces when moving in perspective, but it’s often a good enough approximation.
            scene.halfwayVector = smath::normalize(scene.light.direction - scene.camera.forward);
			scene.forwardNeg = { -scene.camera.forward.x, -scene.camera.forward.y, -scene.camera.forward.z };
        }
        
        Rasterizer<FlatEffect> flatRasterizer;
        Rasterizer<GouraudEffect> gouraudRasterizer;
        Rasterizer<PhongEffect> phongRasterizer;
        Rasterizer<BlinnPhongEffect> blinnPhongRasterizer;
        Rasterizer<TexturedFlatEffect> texturedFlatRasterizer;
        Rasterizer<TexturedGouraudEffect> texturedGouraudRasterizer;
        Rasterizer<TexturedPhongEffect> texturedPhongRasterizer;
        Rasterizer<TexturedBlinnPhongEffect> texturedBlinnPhongRasterizer;
};


