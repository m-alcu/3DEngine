#include "ShadowSystem.hpp"
#include "../scene.hpp"
#include "../rasterizer.hpp"
#include "../effects/shadowEffect.hpp"

namespace ShadowSystem {

    void renderShadowPass(Scene &scene, Rasterizer<ShadowEffect> &shadowRasterizer) {
        for (auto &lightSource : scene.lightSources()) {
            if (!lightSource->shadowComponent || !lightSource->shadowComponent->shadowMap) {
                continue;
            }
            lightSource->shadowComponent->shadowMap->clear();
            buildLightMatrices(*lightSource->shadowComponent,
                               lightSource->lightComponent->light,
                               scene.sceneCenter,
                               scene.sceneRadius);
            for (auto &solidPtr : scene.renderables()) {
                shadowRasterizer.drawRenderable(*solidPtr->transform,
                                                *solidPtr->mesh,
                                                *solidPtr->materialComponent,
                                                solidPtr->render->shading,
                                                &scene,
                                                lightSource->lightComponent,
                                                lightSource->shadowComponent);
            }
        }
    }

} // namespace ShadowSystem
