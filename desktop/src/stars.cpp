
#include "stars.h"
#include "platform.h"
#include "input.h"
#include "my_math.h"
#include "profiling.h"
#include "common_graphics.h"

#include "imgui.h"
#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_opengl3.h"

#include <assert.h>

#include <stdio.h>
#include <stdlib.h> // rand








#define USE_COMPUTE_SHADER 0




void init_particle_renderer();
void shutdown_renderer();
void begin_frame();
void end_frame();
v2 mouse_world_position();
v2 get_camera_half_extents();
void run_compute_shader();

struct RenderParticle
{
    v2 position;
    v2 scale;
    v4 color;
    float rotation;
};

struct PhysicsParticle
{
    v2 velocity;
    float angular_velocity;
};


struct ParticleData
{
    int num_particles;
    RenderParticle *render_particles;
    PhysicsParticle *physics_particles;

    bool wrapping;
};
static ParticleData *particles_data;

static const int NUM_PARTICLES = 5000;



void reset_particle_positions()
{
    v2 half_camera = get_camera_half_extents();
    float camera_width  = half_camera.x * 2.0f;
    float camera_height = half_camera.y * 2.0f;

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        RenderParticle *p = &(particles_data->render_particles[i]);

        float percent = (float)i / particles_data->num_particles;
        float theta = 2.0f * PI * percent;

        float random_number = (float)rand() / (float)RAND_MAX;
        float rx = (float)rand() / (float)RAND_MAX;
        float ry = (float)rand() / (float)RAND_MAX;

        //p->position = v2(cos(theta), sin(theta)) * 5.0f * random_number;
        p->position = v2(-half_camera.x + camera_width * rx,
                         -half_camera.y + camera_height * ry);
        p->scale = v2(1.0f, 1.0f) * random_number * 0.08f;
        //p->color = v4(0.5f * random_number, 0.0f, 1.0f * random_number, 1.0f);
        p->color = v4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        PhysicsParticle *p = &(particles_data->physics_particles[i]);

        p->velocity = v2();

        float random_number = (float)rand() / (float)RAND_MAX;
        p->angular_velocity = (random_number - 0.5f) * 0.25f;
    }



    glBufferSubData(GL_ARRAY_BUFFER, 0,
      sizeof(RenderParticle) * particles_data->num_particles, particles_data->render_particles);
    check_gl_errors("send particle data");
}

void init_particles()
{
    particles_data = (ParticleData *)calloc(1, sizeof(ParticleData));

    particles_data->num_particles = NUM_PARTICLES;
    particles_data->render_particles = (RenderParticle *)calloc(particles_data->num_particles, sizeof(RenderParticle));
    particles_data->physics_particles = (PhysicsParticle *)calloc(particles_data->num_particles, sizeof(PhysicsParticle));

    particles_data->wrapping = true;

    init_particle_renderer();

    srand(1);
    reset_particle_positions();
}


void update_particles()
{
#if USE_COMPUTE_SHADER
    run_compute_shader();
#else
    v2 mouse_pos = mouse_world_position();

    v2 camera_extents = get_camera_half_extents();

    ImGui::Checkbox("wrap", &particles_data->wrapping);

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        RenderParticle *render = &(particles_data->render_particles[i]);
        PhysicsParticle *physics = &(particles_data->physics_particles[i]);

        v2 to_mouse = mouse_pos - render->position;

        if(key_state('Q'))
        {
            v2 direction = unit(to_mouse);
            float mag = length(to_mouse);
            physics->velocity += direction * mag * 0.001f;
        }

        if(key_state('W'))
        {
            //p->velocity += unit(to_mouse) * 0.01f;
            if(length(to_mouse) != 0.0f)
            {
                v2 direction = unit(to_mouse);
                float mag = 1.0f / (length(to_mouse)*length(to_mouse));
                physics->velocity += direction * mag * 0.005f;
            }
        }

        if(key_state('E'))
        {
            v2 direction = unit(to_mouse);
            float mag = length(to_mouse) * length(to_mouse);
            physics->velocity += direction * mag * 0.005f;
        }

        if(key_state('A'))
        {
            v2 normal = v2(-to_mouse.y, to_mouse.x);

            if(length(normal) == 0.0f) normal = v2(0.0f, 1.0f);
            else normal = unit(normal);

            v2 target = mouse_pos + normal;
            v2 to_target = target - render->position;

            float mag = length(to_target) * length(to_target);

            physics->velocity += unit(to_target) * mag * 0.05f;
        }

        if(key_state(' '))
        {
            physics->velocity -= physics->velocity * 0.1f;
        }

        physics->velocity -= physics->velocity * 0.015f;
        render->position += physics->velocity * 0.01;
        render->position += unit( v2(1.0f, -0.2f) ) * 0.002f;
        render->rotation += physics->angular_velocity;

        if(particles_data->wrapping)
        {
            while(render->position.x >=  camera_extents.x) render->position.x -= camera_extents.x * 2.0f;
            while(render->position.x <= -camera_extents.x) render->position.x += camera_extents.x * 2.0f;
            while(render->position.y >=  camera_extents.y) render->position.y -= camera_extents.y * 2.0f;
            while(render->position.y <= -camera_extents.y) render->position.y += camera_extents.y * 2.0f;
        }
    }

    //ImGui::Begin("E");
    if(key_toggled_down('R'))
    //if(ImGui::Button("Reset"))
    {
        reset_particle_positions();
    }
    //ImGui::End();
#endif
}




























struct RendererData
{
    float camera_width;

    GLuint quad_vao;
    GLuint quad_ebo;
    GLuint quad_shader_program;

    GLuint particles_shader_program;

    GLuint particles_vao;
    GLuint particles_vbo;

    RenderTexture render_texture;



#if USE_COMPUTE_SHADER
    GLuint compute_shader;
#endif
};
static RendererData *particles_renderer_data;







static void init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    
    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL3_Init("#version 440 core");
    ImGui_ImplWin32_Init(get_window_handle());
    check_gl_errors("imgui");
}



static void set_particle_attrib_pointers()
{
    float stride = sizeof(RenderParticle);

    // Position
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glEnableVertexAttribArray(0);

    // Scale
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(1 * sizeof(v2)));
    glEnableVertexAttribArray(1);

    // Color
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void *)(2 * sizeof(v2)));
    glEnableVertexAttribArray(2);

    // Rotation
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void *)(2 * sizeof(v2) + sizeof(v4)));
    glEnableVertexAttribArray(3);
}

static void init_particle_renderer()
{
    particles_renderer_data = (RendererData *)calloc(1, sizeof(RendererData));

    int width  = get_screen_width();
    int height = get_screen_height();

    glViewport(0, 0, width, height);


    float vertices[] =
    {
        // pos         uv
        -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.0f, 1.0f,
    };
    int indices[] =
    {
        0, 1, 2,
        2, 3, 0
    };


    {
        glGenVertexArrays(1, &particles_renderer_data->quad_vao);
        glBindVertexArray(particles_renderer_data->quad_vao);
        check_gl_errors("making vao");

        GLuint vbo;
        glGenBuffers(1, &vbo);
        check_gl_errors("making vbo");

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        check_gl_errors("send vbo data");

        float stride = 4 * sizeof(float);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void *)(sizeof(v2)));
        glEnableVertexAttribArray(1);
        check_gl_errors("vertex attrib pointer");

        glGenBuffers(1, &particles_renderer_data->quad_ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particles_renderer_data->quad_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    }





    {
        glGenVertexArrays(1, &particles_renderer_data->particles_vao);
        glBindVertexArray(particles_renderer_data->particles_vao);
        check_gl_errors("making vao");

        glGenBuffers(1, &particles_renderer_data->particles_vbo);
        check_gl_errors("making vbo");

        glBindBuffer(GL_ARRAY_BUFFER, particles_renderer_data->particles_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(RenderParticle) * particles_data->num_particles,
                0, GL_DYNAMIC_DRAW);
        check_gl_errors("send vbo data");

        set_particle_attrib_pointers();
        check_gl_errors("vertex attrib pointer");
    }



    particles_renderer_data->particles_shader_program = make_shader("shaders/stars.shader");
    check_gl_errors("particle shader");


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);



    particles_renderer_data->camera_width = 10.0f;











#if USE_COMPUTE_SHADER
    // Create the compute program, to which the compute shader will be assigned
    particles_renderer_data->compute_shader = glCreateProgram();

    // Create and compile the compute shader
    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);

    char *compute_source = read_file_as_string("shaders/move_particles.computeshader");

    glShaderSource(compute_shader, 1, &compute_source, NULL);
    glCompileShader(compute_shader);

    free(compute_source);

    // Check if there were any issues when compiling the shader
    int result;

    int log_length;
    char info_log[512];

    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &result);
    if(!result)
    {
        glGetShaderInfoLog(compute_shader, 512, &log_length, info_log);
        log_error("Error: Compiler log:\n%s\n", info_log);
        fclose(particles_renderer_data->log);
        return;
    }

    // Attach and link the shader against to the compute program
    glAttachShader(particles_renderer_data->compute_shader, compute_shader);

    glLinkProgram(particles_renderer_data->compute_shader);

    // Check if there were some issues when linking the shader.
    glGetProgramiv(particles_renderer_data->compute_shader, GL_LINK_STATUS, &result);

    if(!result)
    {
        glGetProgramInfoLog(particles_renderer_data->compute_shader, 512, &log_length, info_log);
        log_error("Error: Compiler log:\n%s\n", info_log);
        return;
    }
#endif


    particles_renderer_data->render_texture = make_render_texture(get_screen_width(), get_screen_height());
}

#if USE_COMPUTE_SHADER
static void run_compute_shader()
{
    // Bind the compute program
    glUseProgram(particles_renderer_data->compute_shader);

    // Set the radius uniform
    //glUniform1f(iLocRadius, (float)frameNum);

    // Bind the VBO onto SSBO, which is going to filled in witin the compute
    // shader.
    // gIndexBufferBinding is equal to 0 (same as the compute shader binding)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particles_renderer_data->particles_vbo);

    check_gl_errors("Bind buffer base");

    // Submit job for the compute shader execution.
    // GROUP_SIZE_HEIGHT = GROUP_SIZE_WIDTH = 8
    // NUM_VERTS_H = NUM_VERTS_V = 16
    // As the result the function is called with the following parameters:
    // glDispatchCompute(2, 2, 1)
    /*
    glDispatchCompute(
            (NUM_VERTS_H % GROUP_SIZE_WIDTH + NUM_VERTS_H) / GROUP_SIZE_WIDTH,
            (NUM_VERTS_V % GROUP_SIZE_HEIGHT + NUM_VERTS_V) / GROUP_SIZE_HEIGHT,
            1);
            */
    glDispatchCompute(particles_data->num_particles, 1, 1);
    check_gl_errors("Compute");

    // Unbind the SSBO buffer.
    // gIndexBufferBinding is equal to 0 (same as the compute shader binding)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

    check_gl_errors("After compute");
}
#endif

static void shutdown_renderer()
{
}


static mat4 get_ndc_m_world()
{
    float aspect_ratio = get_aspect_ratio();
    float camera_width = particles_renderer_data->camera_width;
    mat4 m =
    {
        2.0f / camera_width, 0.0f, 0.0f, 0.0f,
        0.0f, (2.0f / camera_width) * aspect_ratio, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };

    return m;
}

static v2 ndc_to_world(v2 p)
{
    float aspect_ratio = get_aspect_ratio();
    float camera_width = particles_renderer_data->camera_width;
    float x_term = (2.0f / camera_width);
    float y_term = (2.0f / camera_width) * aspect_ratio; 
    v2 world = 
    {
        p.x * (1.0f / x_term),
        p.y * (1.0f / y_term)
    };

    return world;
}

v2 mouse_world_position()
{
    v2 p = mouse_screen_position();
    float screen_width = get_screen_width();
    float screen_height = get_screen_height();

    p.y = screen_height - p.y;

    v2 ndc =
    {
        (p.x / screen_width)  * 2.0f - 1.0f,
        (p.y / screen_height) * 2.0f - 1.0f,
    };

    v2 world = ndc_to_world(ndc);
    return world;
}

static v2 get_camera_half_extents()
{
    float half_width = particles_renderer_data->camera_width / 2.0f;
    float half_height = (particles_renderer_data->camera_width / get_aspect_ratio()) / 2.0f;
    return v2(half_width, half_height);
}

/*
 * idk why this is here
static void render_quad(v2 pos, v2 scale)
{
    check_gl_errors("start draw");

    glUseProgram(particles_renderer_data->quad_shader_program);
    check_gl_errors("use program");

    mat4 world_m_model =
    {
        scale.x, 0.0f, 0.0f, pos.x,
        0.0f, scale.y, 0.0f, pos.y,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };

    float camera_width = 10.0f;
    mat4 ndc_m_world = get_ndc_m_world();

    mat4 mvp = ndc_m_world * world_m_model;

    GLint loc = glGetUniformLocation(particles_renderer_data->quad_shader_program, "mvp");
    glUniformMatrix4fv(loc, 1, true, &(mvp[0][0]));
    check_gl_errors("set uniform");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particles_renderer_data->default_texture);

    glBindVertexArray(particles_renderer_data->quad_vao);
    check_gl_errors("use vao");

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particles_renderer_data->quad_ebo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    check_gl_errors("draw");

}
*/

void draw_stars()
{
    set_render_target(particles_renderer_data->render_texture);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // Make sure the vbo is finished updating before drawing from it
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    glUseProgram(particles_renderer_data->particles_shader_program);

    mat4 ndc_m_world = get_ndc_m_world();
    GLint loc = glGetUniformLocation(particles_renderer_data->particles_shader_program, "vp");
    glUniformMatrix4fv(loc, 1, true, &(ndc_m_world[0][0]));
    check_gl_errors("set uniform");

    glBindVertexArray(particles_renderer_data->particles_vao);
    check_gl_errors("use vao");

    glBindBuffer(GL_ARRAY_BUFFER, particles_renderer_data->particles_vbo);
    set_particle_attrib_pointers();
    check_gl_errors("set vbo");

#if USE_COMPUTE_SHADER
#else
    glBufferSubData(GL_ARRAY_BUFFER, 0,
            sizeof(RenderParticle) * particles_data->num_particles, particles_data->render_particles);
    check_gl_errors("send particle data");
#endif

    GLuint texture = get_or_make_texture("textures/star.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glDrawArrays(GL_POINTS, 0, particles_data->num_particles);
    check_gl_errors("draw");



    reset_render_target();
    draw_fullscreen_quad(particles_renderer_data->render_texture.texture);
}

void draw_reflected_stars()
{
    v2 pos = v2(0.0f, -0.5f);
    v2 scale = v2(0.1f, -0.1f);
    draw_screen_quad(pos, scale, particles_renderer_data->render_texture.texture);
}




/*
void add_particles(RenderParticle *particles, int num_particles)
{
}
*/
























