#define STB_IMAGE_IMPLEMENTATION

#include "renderer.h"

#include <filesystem>
namespace fs = std::filesystem;

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "logger.h"

#include "Helpers/openglobject.h"

#include "Loaders/stb_image.h"

#include "UI/uisystem.h"

namespace pg
{
    namespace
    {
        constexpr static const char * const DOM = "Renderer";
    }

    void RenderCall::processUiComponent(UiComponent *component)
    {
        setVisibility(component->isVisible());

        if (not component->isWindowClipped())
        {
            state.scissorEnabled = true;
            float tx = component->clipTopLeft.horizontalAnchor;
            float ty = component->clipTopLeft.verticalAnchor;

            float bx = component->clipBottomRight.horizontalAnchor;
            float by = component->clipBottomRight.verticalAnchor;

            // To get width and height you need to subtract bottom corner to the top corner
            state.scissorBound = constant::Vector4D{tx, ty, bx - tx, by - ty};
        }

        setDepth(component->pos.z);
    }

    BaseAbstractRenderer::BaseAbstractRenderer(MasterRenderer *masterRenderer, const RenderStage& stage) : masterRenderer(masterRenderer), renderStage(stage)
    {
        LOG_THIS_MEMBER("Base Abstract Renderer");

        masterRenderer->addRenderer(this);
    }

    MasterRenderer::MasterRenderer()
    {
        LOG_THIS_MEMBER(DOM);

        initializeParameters();
    }

    MasterRenderer::~MasterRenderer()
    {
        LOG_THIS_MEMBER(DOM);
    }

    void MasterRenderer::onEvent(const OnSDLScanCode&)
    {
        // Todo
        // switch (event.key)
        // {
        // case SDL_Scancode::SDL_SCANCODE_I:
        //     camera.processCameraMovement(constant::Camera_Movement::FORWARD, 0.1);
        //     break;

        // case SDL_Scancode::SDL_SCANCODE_J:
        //     camera.processCameraMovement(constant::Camera_Movement::LEFT, 0.1);
        //     break;

        // case SDL_Scancode::SDL_SCANCODE_L:
        //     camera.processCameraMovement(constant::Camera_Movement::RIGHT, 0.1);
        //     break;

        // case SDL_Scancode::SDL_SCANCODE_K:
        //     camera.processCameraMovement(constant::Camera_Movement::BACKWARD, 0.1);
        //     break;


        // default:
        //     break;
        // }
    }

    void MasterRenderer::execute()
    { 
        // Todo Fix in group and ecs ! ( whereaver we are holding pointer of a comp actually ! )
        // Todo hold a ref to the component list and the component index inside of this list instead of the raw pointer to not get invalidated on resize !

        // inSwap = true;

        // {
        //     std::unique_lock lk(renderMutex);
        //     execCv.notify_all();
            
        //     renderCv.wait(lk, [this]{ return inBetweenRender.load();});
        // }

        // for (auto renderer : renderers)
        // {
        //     renderer->updateMeshes();
        // }

        if (inSwap)
        {
            return;
        }

        if (newMaterialRegistered)
        {
            return;
        }

        if (materialRegisterQueue.size() > 0)
        {
            materialListTemp.clear();
            materialDictTemp.clear();

            materialListTemp = materialList; 
            materialDictTemp = materialDict; 

            for (const auto& holder : materialRegisterQueue)
            {            
                LOG_INFO(DOM, "Registering material");
                materialListTemp.push_back(holder.material);

                if (holder.materialName != "")
                    materialDictTemp[holder.materialName] = nbMaterials;
                
                nbMaterials++;
            }

            newMaterialRegistered = true;

            materialRegisterQueue.clear();

            return;
        }
        
        // If the skip flag is set we unset it and we pass the current render update
        if (skipRenderPass)
        {
            skipRenderPass = false;
            return;
        }

        auto tempRenderList = currentRenderList == 0 ? 1 : 0;

        // auto start = std::chrono::steady_clock::now();

        renderCallList[tempRenderList].clear();

        for (auto renderer : renderers)
        {
            const auto& calls = renderer->getRenderCalls();

            renderCallList[tempRenderList].insert(renderCallList[tempRenderList].end(), calls.begin(), calls.end());
        }

        std::sort(renderCallList[tempRenderList].begin(), renderCallList[tempRenderList].end());

        // This loop batch all the same render call together
        if (renderCallList[tempRenderList].size() > 2)
        {
            std::vector<RenderCall> tempRenderCallList;
        
            RenderCall currentRenderCall = renderCallList[tempRenderList][0];

            for (size_t i = 1; i < renderCallList[tempRenderList].size(); ++i)
            {
                const auto& call = renderCallList[tempRenderList][i];

                if (currentRenderCall.batchable and call.key == currentRenderCall.key and call.state == currentRenderCall.state)
                {
                    currentRenderCall.data.insert(currentRenderCall.data.end(), call.data.begin(), call.data.end());
                }
                else
                {
                    tempRenderCallList.push_back(currentRenderCall);

                    currentRenderCall = call;
                }
            }

            tempRenderCallList.push_back(currentRenderCall);

            // Todo fix this auto batching make things worse, this takes a lot of cycles !
            renderCallList[tempRenderList].swap(tempRenderCallList);
        }

        nbGeneratedFrames++;

        // for ()

        // LOG_INFO(DOM, "Render call list size: " << renderCallList[tempRenderList].size());

        inSwap = true;

        // auto end = std::chrono::steady_clock::now();

        // {
        //     std::unique_lock lk(renderMutex);

        //     currentRenderList = tempRenderList;
        // }

        // std::cout << "System Renderer execute took: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns" << std::endl;

        // inSwap = true;

        // {
        //     std::unique_lock lk(renderMutex);
            
        //     renderCv.wait(lk, [this]{ return not inSwap.load();});
        // }
        
        // inSwap = false;

        // execCv.notify_all();
    }

    void MasterRenderer::processTextureRegister()
    {
        TextureRegisteringQueueItem item;

        bool found = textureRegisteringQueue.try_dequeue(item);

        while (found)
        {
            if (textureList.find(item.name) == textureList.end())
            {
                LOG_INFO(DOM, "Registering texture: " << item.name);

                auto texture = item.callback();

                if (texture.id != 0)
                    registerTexture(item.name, texture);
            }

            found = textureRegisteringQueue.try_dequeue(item);
        }
    }

    void MasterRenderer::renderAll()
    {
        // Todo Clear screen here

        processTextureRegister();

        for (const auto& call : renderCallList[currentRenderList])
        {
            processRenderCall(call);
        }

        nbRenderedFrames++;

        if (inSwap)
        {
            currentRenderList = currentRenderList == 0 ? 1 : 0;

            inSwap = false;
        }

        if (newMaterialRegistered)
        {
            std::swap(materialListTemp, materialList);
            std::swap(materialDictTemp, materialDict);

            newMaterialRegistered = false;
        }
    }

    void MasterRenderer::registerShader(const std::string& name, OpenGLShaderProgram *shaderProgram)
    {
        LOG_THIS_MEMBER(DOM);

        shaderList[name] = shaderProgram;
    }

    void MasterRenderer::registerShader(const std::string& name, const std::string& vsPath, const std::string& fsPath)
    {
        LOG_THIS_MEMBER(DOM);

        auto shaderProgram = new OpenGLShaderProgram(vsPath, fsPath);

        registerShader(name, shaderProgram);
    }

    // TODO mirror or not the texture
    // Todo add an argument to specify the type of texture loaded, e.g.: RGBA, RGB, ...
    void MasterRenderer::registerTexture(const std::string& name, const char* texturePath)
    { 
        LOG_THIS_MEMBER(DOM);

        int width, height, nrChannels;
        // Todo change this with our custom file opener (stbi_load_from_memory)

        unsigned char *data = stbi_load(texturePath, &width, &height, &nrChannels, STBI_rgb_alpha);

        if (not data)
        {
            if (stbi_failure_reason())
            {
                LOG_ERROR(DOM, "Failed to load texture: " << texturePath << ", error: " << stbi_failure_reason());
            }
            else
            {
                LOG_ERROR(DOM, "Failed to load texture: Unknown");
            }

            return;
        }

        LOG_INFO(DOM, "Loaded texture " << name << " from " << texturePath << " with width = " << width << " height = " << height << " nbchannels = " << nrChannels);

        unsigned int texture;

        glGenTextures(1, &texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, texture);
        // set the texture wrapping parameters
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
// #ifdef __EMSCRIPTEN__
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
// #else
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        // load image, create texture and generate mipmaps
// #endif   
        OpenGLTexture tex;

        tex.id = texture;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        tex.transparent = true;

        // Todo this doesn't work properly with fully solid textures
        // if (nrChannels == 4)
        // {
        //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        //     tex.transparent = true;
        // }
        // else
        // {
        //     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        //     tex.transparent = false;
        // }

        glGenerateMipmap(GL_TEXTURE_2D);

        registerTexture(name, tex);

        stbi_image_free(data);
    }

    void MasterRenderer::registerAtlasTexture(const std::string& name, const char* texturePath, const char* atlasFilePath)
    {
        registerTexture(name, texturePath);

        atlasMap.emplace(name, atlasFilePath);
    }

    void MasterRenderer::setState(const OpenGLState& state)
    {
        if (currentState.scissorEnabled != state.scissorEnabled)
        {
            if (state.scissorEnabled)
            {
                glEnable(GL_SCISSOR_TEST);
            }
            else
            {
                glDisable(GL_SCISSOR_TEST);
            }
        }

        if (currentState.scissorBound != state.scissorBound)
        {
            auto& rTable = getParameter();
            const int screenHeight = rTable["ScreenHeight"].get<int>();

            //glScissor defined the box from the bottom left corner (x, y, w, h);
            glScissor(state.scissorBound.x, (screenHeight - state.scissorBound.w) - state.scissorBound.y, state.scissorBound.z, state.scissorBound.w);
        }

        currentState = state;
    }

    void MasterRenderer::processRenderCall(const RenderCall& call)
    {
        if (not call.getVisibility())
            return;

        auto materialId = call.getMaterialId();

        if (materialId >= materialList.size())
        {
            LOG_ERROR(DOM, "Unknown material id: " << materialId);
            return;
        }

        const auto& material = getMaterial(materialId);

        // Todo initialize material in another call !
        if (not material.mesh->initialized)
        {
            LOG_INFO(DOM, "Generating mesh");
            material.mesh->generateMesh();
        }

        auto shaderProgram = material.shader;

        shaderProgram->bind();

        if (call.state != currentState)
        {
            setState(call.state);
        }

        for (size_t i = 0; i < material.nbTextures; ++i)
        {
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, material.textureId[i]);
            shaderProgram->setUniformValue("texture" + std::to_string(i), static_cast<int>(i));
        }

        auto& rTable = getParameter();
        const int screenWidth = rTable["ScreenWidth"].get<int>();
        const int screenHeight = rTable["ScreenHeight"].get<int>();

        glm::mat4 projection = glm::mat4(1.0f);
        // glm::mat4 view = glm::mat4(1.0f);
        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 scale = glm::mat4(1.0f);

        scale = glm::scale(scale, glm::vec3(2.0f / screenWidth, 2.0f / screenHeight, 1.0f));

        // Todo create a uniform feeder

        for (const auto& uniform : material.uniformMap)
        {
            switch (uniform.second.type)
            {
            case UniformType::INT:
                shaderProgram->setUniformValue(uniform.first, std::get<int>(uniform.second.value));
                break;
            case UniformType::FLOAT:
                shaderProgram->setUniformValue(uniform.first, std::get<float>(uniform.second.value));
                break;
            case UniformType::VEC2D:
                shaderProgram->setUniformValue(uniform.first, std::get<glm::vec2>(uniform.second.value));
                break;
            case UniformType::VEC3D:
                shaderProgram->setUniformValue(uniform.first, std::get<glm::vec2>(uniform.second.value));
                break;
            case UniformType::VEC4D:
                shaderProgram->setUniformValue(uniform.first, std::get<glm::vec4>(uniform.second.value));
                break;
            case UniformType::MAT4D:
                shaderProgram->setUniformValue(uniform.first, std::get<glm::mat4>(uniform.second.value));
                break;
            case UniformType::ID:
            {
                std::string id = std::get<std::string>(uniform.second.value);

                const auto& value = rTable[id];

                switch(value.type)
                {
                case ElementType::UnionType::FLOAT:
                    shaderProgram->setUniformValue(uniform.first, value.get<float>());
                    break;
                case ElementType::UnionType::INT:
                case ElementType::UnionType::SIZE_T:
                    shaderProgram->setUniformValue(uniform.first, value.get<int>());
                    break;
                case ElementType::UnionType::STRING:
                case ElementType::UnionType::BOOL:
                default:
                {
                    LOG_ERROR(DOM, "Cannot set uniform for id:" << id << ", Unsupported type :" << value.getTypeString());    
                }
                }
            }
            }
        }

        shaderProgram->setUniformValue("projection", projection);
        shaderProgram->setUniformValue("model", model);
        shaderProgram->setUniformValue("scale", scale);
        shaderProgram->setUniformValue("view", view);

        material.mesh->bind();

        if (material.nbAttributes == 0)
        {
            LOG_ERROR(DOM, "No attributes for this render call");
            return;
        }

        unsigned int nbElements = call.data.size() / material.nbAttributes;

        if (nbElements == 0)
        {
            LOG_ERROR(DOM, "No elements in the render call should be impossible !");
            return;
        }

        // LOG_INFO(DOM, call.nbElements * call.nbAttributes);

        if (call.batchable)
        {
            // Todo remove nbElement as it can be derived from call.data.size() / call.nbAttributes !
            material.mesh->openGLMesh.instanceVBO->allocate(call.data.data(), call.data.size() * sizeof(float));
            glDrawElementsInstanced(GL_TRIANGLES, material.mesh->modelInfo.nbIndices, GL_UNSIGNED_INT, 0, nbElements);
        }
        else
        {
            glDrawElements(GL_TRIANGLES, material.mesh->modelInfo.nbIndices, GL_UNSIGNED_INT, 0);
        }

        // Todo only release if the shader need to change
        shaderProgram->release();
    }

    void MasterRenderer::initializeParameters()
    {
        LOG_THIS_MEMBER(DOM);

        systemParameters["ScreenWidth"] = 1.0f;
        systemParameters["ScreenHeight"] = 1.0f;
        systemParameters["CurrentTime"] = 1;
    }
}