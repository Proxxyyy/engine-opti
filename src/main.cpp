
#include <glad/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <Framebuffer.h>
#include <ImGuiRenderer.h>
#include <Scene.h>
#include <Terrain.h>
#include <Texture.h>
#include <TimestampQuery.h>
#include <graphics.h>

#include <imgui/imgui.h>

#include <filesystem>
#include <iostream>
#include <vector>

using namespace OM3D;


static float delta_time = 0.0f;
static float sun_altitude = 45.0f;
static float sun_azimuth = 45.0f;
static float sun_intensity = 7.0f;
static float ibl_intensity = 1.0f;
static float exposure = 0.33f;
static int gbuffer_debug_mode = 2; // 0=depth, 1=normal, 2=albedo, 3=metallic, 4=roughness

static std::unique_ptr<Scene> scene;
static std::shared_ptr<Texture> envmap;
static std::unique_ptr<Terrain> terrain;

namespace OM3D
{
    extern bool audit_bindings_before_draw;
}

void parse_args(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg = argv[i];

        if (arg == "--validate")
        {
            OM3D::audit_bindings_before_draw = true;
        }
        else
        {
            std::cerr << "Unknown argument \"" << arg << "\"" << std::endl;
        }
    }
}

void glfw_check(bool cond)
{
    if (!cond)
    {
        const char* err = nullptr;
        glfwGetError(&err);
        std::cerr << "GLFW error: " << err << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void update_delta_time()
{
    static double time = 0.0;
    const double new_time = program_time();
    delta_time = float(new_time - time);
    time = new_time;
}

void process_inputs(GLFWwindow* window, Camera& camera)
{
    static glm::dvec2 mouse_pos;

    glm::dvec2 new_mouse_pos;
    glfwGetCursorPos(window, &new_mouse_pos.x, &new_mouse_pos.y);

    {
        glm::vec3 movement = {};
        if (glfwGetKey(window, 'W') == GLFW_PRESS)
        {
            movement += camera.forward();
        }
        if (glfwGetKey(window, 'S') == GLFW_PRESS)
        {
            movement -= camera.forward();
        }
        if (glfwGetKey(window, 'D') == GLFW_PRESS)
        {
            movement += camera.right();
        }
        if (glfwGetKey(window, 'A') == GLFW_PRESS)
        {
            movement -= camera.right();
        }
        if (glfwGetKey(window, 'E') == GLFW_PRESS)
        {
            movement += camera.up();
        }
        if (glfwGetKey(window, 'Q') == GLFW_PRESS)
        {
            movement -= camera.up();
        }

        float speed = 10.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        {
            speed *= 10.0f;
        }

        if (movement.length() > 0.0f)
        {
            const glm::vec3 new_pos = camera.position() + movement * delta_time * speed;
            camera.set_view(glm::lookAt(new_pos, new_pos + camera.forward(), camera.up()));
        }
    }

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
    {
        const glm::vec2 delta = glm::vec2(mouse_pos - new_mouse_pos) * 0.01f;
        if (delta.length() > 0.0f)
        {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), delta.x, glm::vec3(0.0f, 1.0f, 0.0f));
            rot = glm::rotate(rot, delta.y, camera.right());
            camera.set_view(glm::lookAt(camera.position(), camera.position() + (glm::mat3(rot) * camera.forward()),
                                        (glm::mat3(rot) * camera.up())));
        }
    }

    {
        int width = 0;
        int height = 0;
        glfwGetWindowSize(window, &width, &height);
        camera.set_ratio(float(width) / float(height));
    }

    mouse_pos = new_mouse_pos;
}

void load_envmap(const std::string& filename)
{
    if (auto res = TextureData::from_file(filename); res.is_ok)
    {
        envmap = std::make_shared<Texture>(Texture::cubemap_from_equirec(res.value));
        scene->set_envmap(envmap);
    }
    else
    {
        std::cerr << "Unable to load envmap (" << filename << ")" << std::endl;
    }
}

void load_scene(const std::string& filename)
{
    if (auto res = Scene::from_gltf(filename); res.is_ok)
    {
        scene = std::move(res.value);
        scene->set_envmap(envmap);
        scene->set_ibl_intensity(ibl_intensity);
        scene->set_sun(sun_altitude, sun_azimuth, glm::vec3(sun_intensity));
    }
    else
    {
        std::cerr << "Unable to load scene (" << filename << ")" << std::endl;
    }
}

std::vector<std::string> list_data_files(Span<const std::string> extensions = {})
{
    std::vector<std::string> files;
    for (auto&& entry: std::filesystem::directory_iterator(data_path))
    {
        if (entry.status().type() == std::filesystem::file_type::regular)
        {
            const auto ext = entry.path().extension();

            bool ext_match = extensions.is_empty();
            for (const std::string& e: extensions)
            {
                ext_match |= (ext == e);
            }

            if (ext_match)
            {
                files.emplace_back(entry.path().string());
            }
        }
    }
    return files;
}

template<typename F>
bool load_file_window(Span<std::string> files, F&& load_func)
{
    char buffer[1024] = {};
    if (ImGui::InputText("Load file", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue))
    {
        load_func(buffer);
        return true;
    }

    if (!files.is_empty())
    {
        for (const std::string& p: files)
        {
            const auto abs = std::filesystem::absolute(p).string();
            if (ImGui::MenuItem(abs.c_str()))
            {
                load_func(p);
                return true;
            }
        }
    }

    return false;
}

void gui(ImGuiRenderer& imgui)
{
    const ImVec4 error_text_color = ImVec4(1.0f, 0.3f, 0.3f, 1.0f);
    const ImVec4 warning_text_color = ImVec4(1.0f, 0.8f, 0.4f, 1.0f);

    static bool open_gpu_profiler = false;

    PROFILE_GPU("GUI");

    imgui.start();
    DEFER(imgui.finish());


    static std::vector<std::string> load_files;

    // ImGui::ShowDemoWindow();

    bool open_scene_popup = false;
    bool load_envmap_popup = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open Scene"))
            {
                open_scene_popup = true;
            }
            if (ImGui::MenuItem("Open Envmap"))
            {
                load_envmap_popup = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Lighting"))
        {
            ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.01f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);

            ImGui::Separator();

            ImGui::DragFloat("IBL intensity", &ibl_intensity, 0.01f, 0.0f, 1.0f);
            scene->set_ibl_intensity(ibl_intensity);

            ImGui::Separator();

            ImGui::DragFloat("Sun Altitude", &sun_altitude, 0.1f, 0.0f, 90.0f, "%.0f");
            ImGui::DragFloat("Sun Azimuth", &sun_azimuth, 0.1f, 0.0f, 360.0f, "%.0f");
            ImGui::DragFloat("Sun Intensity", &sun_intensity, 0.05f, 0.0f, 100.0f, "%.1f");
            scene->set_sun(sun_altitude, sun_azimuth, glm::vec3(sun_intensity));

            ImGui::EndMenu();
        }

        if (scene && ImGui::BeginMenu("Scene Info"))
        {
            ImGui::Text("%u objects", u32(scene->objects().size()));
            ImGui::Text("%u point lights", u32(scene->point_lights().size()));
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("G-Buffer Debug"))
        {
            ImGui::RadioButton("Depth", &gbuffer_debug_mode, 0);
            ImGui::RadioButton("Normal", &gbuffer_debug_mode, 1);
            ImGui::RadioButton("Albedo", &gbuffer_debug_mode, 2);
            ImGui::RadioButton("Metallic", &gbuffer_debug_mode, 3);
            ImGui::RadioButton("Roughness", &gbuffer_debug_mode, 4);
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("GPU Profiler"))
        {
            open_gpu_profiler = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

        ImGui::Separator();
        ImGui::Text("%.2f ms", delta_time * 1000.0f);

#ifdef OM3D_DEBUG
        ImGui::Separator();
        ImGui::TextColored(warning_text_color, ICON_FA_BUG " (DEBUG)");
#endif

        if (!bindless_enabled())
        {
            ImGui::Separator();
            ImGui::TextColored(error_text_color, ICON_FA_EXCLAMATION_TRIANGLE " Bindless textures not supported");
        }
        ImGui::EndMainMenuBar();
    }

    if (open_scene_popup)
    {
        ImGui::OpenPopup("###openscenepopup");

        const std::array<std::string, 2> extensions = {".gltf", ".glb"};
        load_files = list_data_files(extensions);
    }

    if (ImGui::BeginPopup("###openscenepopup", ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (load_file_window(load_files, load_scene))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (load_envmap_popup)
    {
        ImGui::OpenPopup("###openenvmappopup");

        const std::array<std::string, 3> extensions = {".png", ".jpg", ".tga"};
        load_files = list_data_files(extensions);
    }

    if (ImGui::BeginPopup("###openenvmappopup", ImGuiWindowFlags_AlwaysAutoResize))
    {
        if (load_file_window(load_files, load_envmap))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    if (open_gpu_profiler)
    {
        if (ImGui::Begin(ICON_FA_CLOCK " GPU Profiler"))
        {
            const ImGuiTableFlags table_flags = ImGuiTableFlags_SortTristate | ImGuiTableFlags_NoSavedSettings |
                                                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV |
                                                ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg;

            ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImVec4(1, 1, 1, 0.01f));
            DEFER(ImGui::PopStyleColor());

            if (ImGui::BeginTable("##timetable", 3, table_flags))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("CPU (ms)", ImGuiTableColumnFlags_NoResize, 70.0f);
                ImGui::TableSetupColumn("GPU (ms)", ImGuiTableColumnFlags_NoResize, 70.0f);
                ImGui::TableHeadersRow();

                std::vector<u32> indents;
                for (const auto& zone: retrieve_profile())
                {
                    auto color_from_time = [](float time)
                    {
                        const float t = std::min(time / 0.008f, 1.0f); // 8ms = red
                        return ImVec4(t, 1.0f - t, 0.0f, 1.0f);
                    };

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted(zone.name.data());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushStyleColor(ImGuiCol_Text, color_from_time(zone.cpu_time));
                    ImGui::Text("%.2f", zone.cpu_time * 1000.0f);

                    ImGui::TableSetColumnIndex(2);
                    ImGui::PushStyleColor(ImGuiCol_Text, color_from_time(zone.gpu_time));
                    ImGui::Text("%.2f", zone.gpu_time * 1000.0f);

                    ImGui::PopStyleColor(2);

                    if (!indents.empty() && --indents.back() == 0)
                    {
                        indents.pop_back();
                        ImGui::Unindent();
                    }

                    if (zone.contained_zones)
                    {
                        indents.push_back(zone.contained_zones);
                        ImGui::Indent();
                    }
                }

                ImGui::EndTable();
            }
        }
        ImGui::End();
    }
}


void load_default_scene()
{
    load_scene(std::string(data_path) + "DamagedHelmet.glb");
    load_envmap(std::string(data_path) + "skybox_clouds.jpg");

    // Add lights
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, 4.0f));
        light.set_color(glm::vec3(0.0f, 50.0f, 0.0f));
        light.set_radius(100.0f);
        scene->add_light(std::move(light));
    }
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, -4.0f));
        light.set_color(glm::vec3(50.0f, 0.0f, 0.0f));
        light.set_radius(50.0f);
        scene->add_light(std::move(light));
    }
}

struct RendererState
{
    static RendererState create(glm::uvec2 size)
    {
        RendererState state;

        state.size = size;

        if (state.size.x > 0 && state.size.y > 0)
        {

            state.depth_texture = Texture(size, ImageFormat::Depth32_FLOAT, WrapMode::Clamp);
            state.lit_hdr_texture = Texture(size, ImageFormat::RGBA16_FLOAT, WrapMode::Clamp);
            state.tone_mapped_texture = Texture(size, ImageFormat::RGBA8_UNORM, WrapMode::Clamp);
            state.main_framebuffer = Framebuffer(&state.depth_texture, std::array{&state.lit_hdr_texture});
            state.tone_map_framebuffer = Framebuffer(nullptr, std::array{&state.tone_mapped_texture});

            state.depth_program = Program::from_files("depth.frag", "basic.vert");

            state.gbuffer_albedo_roughness = Texture(size, ImageFormat::RGBA8_sRGB, WrapMode::Clamp);
            state.gbuffer_normal_metal = Texture(size, ImageFormat::RGBA8_UNORM, WrapMode::Clamp);
            state.gbuffer_framebuffer = Framebuffer(
                    &state.depth_texture, std::array{&state.gbuffer_albedo_roughness, &state.gbuffer_normal_metal});
            state.gbuffer_debug_output = Texture(size, ImageFormat::RGBA8_UNORM, WrapMode::Clamp);
            state.gbuffer_debug_framebuffer = Framebuffer(nullptr, std::array{&state.gbuffer_debug_output});
            state.gbuffer_debug_program = Program::from_files("gbuffer_debug.frag", "screen.vert");

            // Shading framebuffer outputs to lit_hdr_texture
            state.shading_framebuffer = Framebuffer(nullptr, std::array{&state.lit_hdr_texture});

            // shadow map resources
            const glm::uvec2 shadow_size = glm::uvec2(2048u, 2048u);
            state.shadow_depth_texture = Texture(shadow_size, ImageFormat::Depth32_FLOAT, WrapMode::Clamp);
            state.shadow_depth_texture.set_shadow_parameters();
            state.shadow_framebuffer = Framebuffer(&state.shadow_depth_texture);
            state.shadow_program = Program::from_files("shadow.frag", "shadow.vert");

            state.scene_shading_program = Program::from_files("scene.frag", "screen.vert"); // Without IBL
            state.pl_shading_program = Program::from_files("pl.frag", "screen.vert");
            state.point_light_material = Material::point_light_material();

            // Heightmap and normalmap resources
            const glm::uvec2 heightmap_size = glm::uvec2(8192u, 8192u);
            state.heightmap_texture =
                    std::make_shared<Texture>(heightmap_size, ImageFormat::R32_FLOAT, WrapMode::Clamp);
            state.normalmap_texture =
                    std::make_shared<Texture>(heightmap_size, ImageFormat::RGBA16_FLOAT, WrapMode::Clamp);
            state.heightmap_program = Program::from_file("terrain_gen.comp");

            // Terrain rendering programs (tessellation shaders)
            state.terrain_gbuffer_program =
                    Program::from_files("terrain_gbuffer.frag", "terrain.vert", "terrain.tesc", "terrain.tese");
            state.terrain_depth_program =
                    Program::from_files("depth.frag", "terrain.vert", "terrain.tesc", "terrain.tese");
            state.heightmap_program->bind();

            // Bind textures to image units
            state.heightmap_texture->bind_as_image(0, AccessType::ReadWrite);
            state.normalmap_texture->bind_as_image(1, AccessType::ReadWrite);
        }

        return state;
    }

    glm::uvec2 size = {};

    Texture depth_texture;
    Texture lit_hdr_texture;
    Texture tone_mapped_texture;

    // Shadow map resources
    Texture shadow_depth_texture;
    Framebuffer shadow_framebuffer;
    std::shared_ptr<Program> shadow_program;

    Texture gbuffer_albedo_roughness;
    Texture gbuffer_normal_metal;
    Framebuffer gbuffer_framebuffer;
    Texture gbuffer_debug_output;
    Framebuffer gbuffer_debug_framebuffer;
    std::shared_ptr<Program> gbuffer_debug_program;

    Framebuffer main_framebuffer;
    Framebuffer tone_map_framebuffer;

    std::shared_ptr<Program> depth_program;

    Framebuffer shading_framebuffer;
    std::shared_ptr<Program> scene_shading_program;
    std::shared_ptr<Program> pl_shading_program;
    Material point_light_material;

    // Heightmap and normalmap resources
    std::shared_ptr<Texture> heightmap_texture;
    std::shared_ptr<Texture> normalmap_texture;
    std::shared_ptr<Program> heightmap_program;

    std::shared_ptr<Program> terrain_gbuffer_program;
    std::shared_ptr<Program> terrain_depth_program;
};

int main(int argc, char** argv)
{
    DEBUG_ASSERT(
            []
            {
                std::cout << "Debug asserts enabled" << std::endl;
                return true;
            }());

    parse_args(argc, argv);

    glfw_check(glfwInit());
    DEFER(glfwTerminate());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    GLFWwindow* window = glfwCreateWindow(1600, 900, "OM3D", nullptr, nullptr);
    glfw_check(window);
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    init_graphics();

    std::unique_ptr<ImGuiRenderer> imgui = std::make_unique<ImGuiRenderer>(window);


    load_default_scene();


    terrain = std::make_unique<Terrain>();


    auto tonemap_program = Program::from_files("tonemap.frag", "screen.vert");
    RendererState renderer;

    for (;;)
    {
        glfwPollEvents();
        if (glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE))
        {
            break;
        }

        process_profile_markers();

        {
            int width = 0;
            int height = 0;
            glfwGetWindowSize(window, &width, &height);

            if (renderer.size != glm::uvec2(width, height))
            {
                renderer = RendererState::create(glm::uvec2(width, height));
                terrain->init(renderer.heightmap_program, renderer.heightmap_texture);
            }
        }

        update_delta_time();

        if (const auto& io = ImGui::GetIO(); !io.WantCaptureMouse && !io.WantCaptureKeyboard)
        {
            process_inputs(window, scene->camera());
        }

        // Draw everything
        {
            PROFILE_GPU("Frame");
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Frame");
            // Z-prepass (for G-Buffer)
            {
                PROFILE_GPU("Z-prepass");
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Z-prepass");

                renderer.gbuffer_framebuffer.bind(true, false);

                // Disable color channels (this improves the performance)
                glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                renderer.depth_program->bind();
                scene->render();
                


                renderer.terrain_depth_program->bind();
                terrain->render(*renderer.terrain_depth_program, scene->camera());
                glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

                glPopDebugGroup();
            }

            // {
            //     // --- Shadow depth pass ---
            //     PROFILE_GPU("Shadow pass");
            //     glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Shadow pass");

            //     const float alt = glm::radians(sun_altitude);
            //     const float azi = glm::radians(sun_azimuth);
            //     const glm::vec3 light_dir =
            //             glm::normalize(glm::vec3(sin(azi) * cos(alt), sin(alt), cos(azi) * cos(alt)));

            //     const glm::vec3 scene_center = glm::vec3(0.0f);
            //     const glm::vec3 light_pos = scene_center - light_dir * sun_altitude;

            //     const float ortho_size = 20.0f;
            //     const glm::mat4 light_view = glm::lookAt(light_pos, scene_center, glm::vec3(0.0f, 1.0f, 0.0f));
            //     const glm::mat4 light_proj = glm::ortho(-ortho_size, ortho_size, -ortho_size, ortho_size, 0.1f,
            //     100.0f); const glm::mat4 light_space = light_proj * light_view;

            //     scene->set_light_view_proj(light_space);

            //     const glm::uvec2 shadow_size = renderer.shadow_depth_texture.size();
            //     renderer.shadow_framebuffer.bind(true, false);
            //     glViewport(0, 0, shadow_size.x, shadow_size.y);

            //     GLint prev_depth_func = 0;
            //     glGetIntegerv(GL_DEPTH_FUNC, &prev_depth_func);
            //     GLdouble prev_clear_depth = 0.0;
            //     glGetDoublev(GL_DEPTH_CLEAR_VALUE, &prev_clear_depth);

            //     glEnable(GL_CULL_FACE);
            //     glCullFace(GL_BACK);
            //     glEnable(GL_DEPTH_TEST);
            //     glDepthFunc(GL_LEQUAL);
            //     glClearDepth(1.0);
            //     glClear(GL_DEPTH_BUFFER_BIT);

            //     renderer.shadow_program->bind();
            //     renderer.shadow_program->set_uniform(HASH("lightSpaceMatrix"), light_space);
            //     for (const SceneObject& obj: scene->objects())
            //     {
            //         obj.render_depth_only(*renderer.shadow_program);
            //     }

            //     glDepthFunc(prev_depth_func);
            //     glClearDepth(prev_clear_depth);

            //     int w = int(renderer.size.x);
            //     int h = int(renderer.size.y);
            //     glViewport(0, 0, w, h);

            //     glPopDebugGroup();
            // }


            // G-Buffer pass

            {

                PROFILE_GPU("G-Buffer pass");
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "G-Buffer pass");

                renderer.gbuffer_framebuffer.bind(false, true);
                renderer.shadow_depth_texture.bind(6);
                scene->render();

                renderer.terrain_gbuffer_program->bind();
                terrain->render(*renderer.terrain_gbuffer_program, scene->camera());

                glPopDebugGroup();
            }

            /*{
                PROFILE_GPU("G-Buffer Debug");
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "G-Buffer Debug");

                renderer.gbuffer_debug_framebuffer.bind(false, true);
                renderer.gbuffer_debug_program->bind();
                renderer.gbuffer_debug_program->set_uniform(HASH("debug_mode"), u32(gbuffer_debug_mode));

                renderer.gbuffer_albedo_roughness.bind(0);
                renderer.gbuffer_normal_metal.bind(1);
                renderer.depth_texture.bind(2);

                draw_full_screen_triangle();

                glPopDebugGroup();
            }

            */
            {
                PROFILE_GPU("Scene Shading Pass");
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Scene Shading Pass");

                glDisable(GL_BLEND);
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glDisable(GL_CULL_FACE);

                renderer.shading_framebuffer.bind(false, true);
                renderer.scene_shading_program->bind();

                renderer.gbuffer_albedo_roughness.bind(0);
                renderer.gbuffer_normal_metal.bind(1);
                renderer.depth_texture.bind(2);
                scene->bind_buffer();

                draw_full_screen_triangle();

                glPopDebugGroup();
            }

            // {
            //     PROFILE_GPU("Point Lights Shading Pass");
            //     glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Point Lights Shading Pass");

            //     renderer.shading_framebuffer.bind(false, false);

            //     renderer.gbuffer_albedo_roughness.bind(0);
            //     renderer.gbuffer_normal_metal.bind(1);
            //     renderer.depth_texture.bind(2);
            //     scene->bind_buffer_pl();

            //     renderer.point_light_material.bind();
            //     draw_full_screen_triangle();

            //     glDepthMask(GL_TRUE);
            //     glDisable(GL_BLEND);
            //     glEnable(GL_CULL_FACE);
            //     glCullFace(GL_BACK);

            //     glPopDebugGroup();
            // }

            // Apply a tonemap as a full screen pass
            {
                PROFILE_GPU("Tonemap");
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Tonemap");

                renderer.tone_map_framebuffer.bind(false, true);
                tonemap_program->bind();
                tonemap_program->set_uniform(HASH("exposure"), exposure);
                renderer.lit_hdr_texture.bind(0);
                draw_full_screen_triangle();

                glPopDebugGroup();
            }

            // Blit debug result to screen
            {
                PROFILE_GPU("Blit");
                glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Blit");

                blit_to_screen(renderer.tone_mapped_texture);

                glPopDebugGroup();
            }

            // Draw GUI on top
            glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "GUI");
            gui(*imgui);
            glPopDebugGroup();

            glPopDebugGroup();
        }

        glfwSwapBuffers(window);
    }


    // destroy scene and child OpenGL objects
    scene = nullptr;
    envmap = nullptr;
    imgui = nullptr;
    tonemap_program = nullptr;
    renderer = {};
    destroy_graphics();
}
