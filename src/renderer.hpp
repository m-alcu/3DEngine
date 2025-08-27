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
        
            //float zNear = 0.1f; // Near plane distance
            //float zFar  = 10000.0f; // Far plane distance
            float aspectRatio = (float) scene.screen.width / scene.screen.height; // Width / Height ratio
            float fovRadians = viewAngle * (PI / 180.0f);
        
            scene.projectionMatrix = smath::perspective(zFar, zNear, aspectRatio, fovRadians);

            if (scene.orbiting) {
                scene.viewMatrix = smath::view(scene.camera.pos, scene.camera.orbitTarget, { 0,1,0 });
            }
            else {
                scene.viewMatrix = smath::fpsview(scene.camera.pos, scene.camera.pitch, scene.camera.yaw, scene.camera.roll);
            }

            // Used in BlinnPhong shading
            scene.halfwayVector = smath::normalize(scene.lux + scene.camera.forward);
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


