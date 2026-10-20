#include "Engine.h"
#include "Core/File.h"
#include "Renderer/VertexBuffer.h"
#include "Renderer/IndexBuffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/Shader.h"
#include "Math/Transform.h"
#include "Renderer/Model.h"

#include "Resources/ResourceManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

using namespace nu;
namespace nu {
    struct Color4 {
        float r, g, b, a;
    };
}

struct Vertex
{
    Vector3 position;
    Color4 color;
};

std::vector<Vertex> vertices =
{
    Vertex{ Vector3{ -0.5f, -0.5f, 0.0f}, Color4{1, 0, 0, 1} }, // Bottom-Left
    Vertex{ Vector3{ 0.5f, -0.5f, 0.0f}, Color4{0, 1, 0, 1} }, // Bottom-Right
    Vertex{ Vector3{ 0.5f, 0.5f, 0.0f}, Color4{0, 0, 1, 1} }, // Top-Right
    Vertex{ Vector3{ -0.5f, 0.5f, 0.0f}, Color4{1, 1, 0, 1} }, // Top-Left
};

std::vector<uint32_t>indices = { 0, 1, 2, 2, 3, 0 };


int main()
{
    SetWorkingDirectory("Assets");

    // INITIALIZATION
    Engine::Instance().Initialize();

    auto vb = std::make_shared<VertexBuffer>();
    vb->Create<Vertex>(vertices, Engine::Instance().GetRenderer());

    auto ib = std::make_shared<IndexBuffer>();
    ib->Create(indices, Engine::Instance().GetRenderer());

    auto model = Resources().Get<Model>("models/spot.obj", Engine::Instance().GetRenderer());
    auto texture = Resources().Get<Texture>("textures/spot_diffuse.png", Engine::Instance().GetRenderer());

    auto vshader = Resources().Get<nu::Shader>("shaders/unlit.vert", Engine::Instance().GetRenderer(), 1);
    auto fshader = Resources().Get<nu::Shader>("shaders/unlit.frag", Engine::Instance().GetRenderer(), 1, 1);

    //auto model = Resources().Get<Model>("models/suzanne.obj", Engine::Instance().GetRenderer());

    //auto vshader = Resources().Get<nu::Shader>("shaders/unlit.vert", Engine::Instance().GetRenderer(), 1);
    //auto fshader = Resources().Get<nu::Shader>("shaders/unlit.frag", Engine::Instance().GetRenderer(), 1);

    auto pipeline = std::make_shared<Pipeline>();

    pipeline->AddVertexBuffer(sizeof(Model::Vertex));
    pipeline->AddVertexAttribute(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Model::Vertex, position));
    pipeline->AddVertexAttribute(1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, offsetof(Model::Vertex, texcoord));
    pipeline->AddVertexAttribute(2, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Model::Vertex, normal));
    pipeline->AddVertexAttribute(3, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, offsetof(Model::Vertex, tangent));

    pipeline->Create(*vshader.get(), *fshader.get(),
        Engine::Instance().GetRenderer().GetGPUDevice(),
        Engine::Instance().GetRenderer().GetWindow());


    //auto pipeline = std::make_shared<Pipeline>();
    //pipeline->AddVertexBuffer(sizeof(Vertex));
    //// position
    //pipeline->AddVertexAttribute(
    //    0,
    //    SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
    //    offsetof(Vertex, position));

    //// color
    //pipeline->AddVertexAttribute(
    //    1,
    //    SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
    //    offsetof(Vertex, color));


    //pipeline->Create(*vshader.get(), *fshader.get(),
    //    Engine::Instance().GetRenderer().GetGPUDevice(),
    //    Engine::Instance().GetRenderer().GetWindow());


    float scale = 0.5f;
    Color4 color { 0, 0, 0, 1 };

    struct vertexUniform
    {
        glm::mat4 mvp;
    };

    vertexUniform uniform;

    glm::vec3 cameraposition{0, 0, 5};

    Transform transform;


    // MAIN LOOP
    bool quit = false;
    while (!quit) 
    {
        // UPDATE
        SDL_Event event;
        while (SDL_PollEvent(&event)) 
        {
            if (event.type == SDL_EVENT_QUIT) 
            {
                quit = true;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE)
            {
                quit = true;
            }
        }

        // ENGINE
        Engine::Instance().Update();
        float dt = Engine::Instance().GetTime().GetDeltaTime();
        float time = Engine::Instance().GetTime().GetTime();

        scale = sin(Engine::Instance().GetTime().GetTime());
        color.r = sin(Engine::Instance().GetTime().GetTime()) * 0.5f + 0.5f;
        color.b = sin(Engine::Instance().GetTime().GetTime()) * 0.5f + 0.5f;

        glm::vec3 direction{0, 0, 0};

        if (Engine::Instance().GetInput().GetKeyDown(SDL_SCANCODE_A)) direction.x = -1;
        if (Engine::Instance().GetInput().GetKeyDown(SDL_SCANCODE_D)) direction.x = +1;
        if (Engine::Instance().GetInput().GetKeyDown(SDL_SCANCODE_E)) direction.y = -1;
        if (Engine::Instance().GetInput().GetKeyDown(SDL_SCANCODE_Q)) direction.y = +1;
        if (Engine::Instance().GetInput().GetKeyDown(SDL_SCANCODE_W)) direction.z = -1;
        if (Engine::Instance().GetInput().GetKeyDown(SDL_SCANCODE_S)) direction.z = +1;


        cameraposition += direction * 4.0f * dt;

        // TRANSFORM

        transform.rotation.z = time * 10;

        // RENDER

        // View Matrix
        auto view = glm::lookAtRH(cameraposition, cameraposition + glm::vec3{ 0, 0, -1 }, glm::vec3{ 0, 1, 0 });

        // Projection Matrix
        float aspectRatio = Engine::Instance().GetRenderer().GetWidth() / (float)Engine::Instance().GetRenderer().GetHeight();
        auto projection = glm::perspective(glm::radians(70.0f), aspectRatio, 0.01f, 20.0f);

        //MVP
        uniform.mvp = projection * view * transform.GetMatrix();



        Engine::Instance().GetRenderer().BeginFrame();

        Engine::Instance().GetRenderer().SetPipeline(*pipeline);
        Engine::Instance().GetRenderer().SetVertexBuffer(*vb);
        Engine::Instance().GetRenderer().SetIndexBuffer(*ib);

        Engine::Instance().GetRenderer().SetVertexUniform(0, &uniform, sizeof(uniform));
        //Engine::Instance().GetRenderer().SetFragmentUniform(0, &tint, sizeof(tint));
        
        Engine::Instance().GetRenderer().SetFragmentUniform(0, &color, sizeof(color));
        Engine::Instance().GetRenderer().SetFragmentTexture(0, *texture);
        model->Draw(Engine::Instance().GetRenderer());

        Engine::Instance().GetRenderer().DrawIndex(*ib);

        Engine::Instance().GetRenderer().EndFrame();

        //Engine::Instance().GetRenderer().BeginFrame();

        
        //Engine::Instance().GetRenderer().SetVertexUniform(0, &uniform, sizeof(uniform));
        ////Engine::Instance().GetRenderer().SetVertexUniform(0, &scale, sizeof(scale));
        //Engine::Instance().GetRenderer().SetFragmentUniform(0, &color, sizeof(color));
        //Engine::Instance().GetRenderer().Draw(vb->GetVertexCount());
        //Engine::Instance().GetRenderer().EndFrame();
    }

    // SHUTDOWN
    Engine::Instance().Shutdown();    

    return 0;
}

