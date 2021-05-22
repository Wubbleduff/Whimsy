
#include "stars.h"
#include "platform.h"
#include "input.h"
#include "my_math.h"
#include "profiling.h"
#include "common_graphics.h"
#include "shader.h"
#include "scene.h"

#include "imgui.h"
#include "examples/imgui_impl_win32.h"
#include "examples/imgui_impl_opengl3.h"

//#include "easy/profiler.h"








#define USE_COMPUTE_SHADER 0




void init_particle_renderer();
void shutdown_renderer();
v2 mouse_world_position();
void run_compute_shader();

struct ParticleRenderingData
{
    v2 position;
    v2 scale;
    v4 color;
    float rotation;
};

struct ParticlePhysicsData
{
    v2 velocity;
    float angular_velocity;

    float modifier;
};

struct Particle
{
    ParticleRenderingData *rendering_data;
    ParticlePhysicsData *physics_data;
};

struct VectorField
{
    int width;
    int height;
    v2 *v;

    v2 bottom_left;
    v2 top_right;

    void init(int in_width, int in_height)
    {
        width = in_width;
        height = in_height;
        v = (v2 *)calloc(width * height, sizeof(v2));

        bottom_left = v2();
        top_right = v2();
    }

    void uninit()
    {
        free(v);
    }

    v2 world_to_grid(v2 world_pos)
    {
        v2 result = remap(world_pos, bottom_left, top_right, v2(), v2(width, height));
        result.y = (float)height - result.y;
        return result;
    }

    v2 grid_to_world(v2 grid_pos)
    {
        v2 result = remap(grid_pos, v2(), v2(width, height), bottom_left, top_right);
        result.y = (top_right.y - bottom_left.y) - result.y;
        return result;
    }

    v2 at(v2 world_pos)
    {
        v2 grid_pos = world_to_grid(world_pos);

        int row = (int)grid_pos.y;
        int column = (int)grid_pos.x;
        row = clamp(row, 0, height - 1);
        column = clamp(column, 0, width - 1);

        int index = row * width + column;
        v2 result = v[index];
        return result;
    }
};
static const int VECTOR_FIELD_SIZE = 32;


struct ParticleData
{
    int num_particles;
    ParticleRenderingData *rendering_data;
    ParticlePhysicsData *physics_data;

    Particle *particles;
    VectorField *velocity_field;

    bool wrapping;


    bool debug_draw_grid;
};
static ParticleData *particles_data;

static const int NUM_PARTICLES = 15000;



static void set_particle_colors()
{
    float power = 6.0f;

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        ParticleRenderingData *p = &(particles_data->rendering_data[i]);

        //float threshold = 0.25f;
        //if(random_01() <= threshold) p->color = v4(1.0f, 1.0f, 1.0f, 1.0f);
        float power_result = to_power(random_01(), power);
        float alpha = remap(power_result, 0.0f, 1.0f, 0.65f, 1.0f);

        if(random_01() < 0.3f)
        {
            p->color = v4(1.0f, 0.9f, 0.95f, alpha);
        }
        else
        {
            p->color = v4(0.9f, 0.9f, 1.0f, alpha);
        }
    }
}

static void reset_particle_positions()
{
    v2 sky_bl = v2(get_scene_bottom_left().x, 0.0f);
    v2 sky_tr = get_scene_top_right();


    for(int i = 0; i < particles_data->num_particles; i++)
    {
        ParticleRenderingData *p = &(particles_data->rendering_data[i]);

        float random_number = random_01();
        float rx = random_01();
        float ry = random_01();

        p->position = v2(lerp(sky_bl.x, sky_tr.x, rx), lerp(sky_bl.y, sky_tr.y, ry));
        float scale = lerp(0.005f, 0.010f, to_power(random_01(), 10.0f));
        p->scale = v2(1.0f, 1.0f) * scale;
    }

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        ParticlePhysicsData *p = &(particles_data->physics_data[i]);
        p->modifier = random_01();
        p->velocity = v2();
        p->angular_velocity = random_range(-0.05f, 0.05f);
    }



}

static v2 rotate_around_cursor(v2 cell_pos, v2 mouse_pos)
{
    v2 to_mouse = mouse_pos - cell_pos;

    v2 normal = v2(-to_mouse.y, to_mouse.x);

    if(length(normal) == 0.0f) normal = v2(0.0f, 1.0f);
    else normal = unit(normal);

    float mag = 1.0f;

    v2 v = normal * mag;
    return v;
}

static v2 move_to_cursor_gravity(v2 cell_pos, v2 mouse_pos)
{
    v2 to_mouse = mouse_pos - cell_pos;

    float to_mouse_length2 = length(to_mouse)*length(to_mouse);
    float min_dist = 0.05f;
    to_mouse_length2 = max(min_dist, to_mouse_length2);

    v2 dir = unit(to_mouse);
    float mag = (1.0f / to_mouse_length2);
    v2 result = dir * mag;

    return result;
}


static void update_velocity_field(v2 mouse_pos)
{
    for(int y = 0; y < particles_data->velocity_field->height; y++)
    {
        for(int x = 0; x < particles_data->velocity_field->width; x++)
        {
            v2 cell_pos = v2(x, y);
            cell_pos = particles_data->velocity_field->grid_to_world(cell_pos);

            v2 v;
            if(key_state('A'))
            {
                v = rotate_around_cursor(cell_pos, mouse_pos);
            }

            if(key_state('W'))
            {
            v = move_to_cursor_gravity(cell_pos, mouse_pos);
            }

            v *= 0.001f;

            particles_data->velocity_field->v[y * particles_data->velocity_field->width + x] = v;
        }
    }
}

void init_particles()
{
    particles_data = (ParticleData *)calloc(1, sizeof(ParticleData));

    particles_data->num_particles = NUM_PARTICLES;
    particles_data->rendering_data = (ParticleRenderingData *)calloc(particles_data->num_particles, sizeof(ParticleRenderingData));
    particles_data->physics_data = (ParticlePhysicsData *)calloc(particles_data->num_particles, sizeof(ParticlePhysicsData));
    particles_data->particles = (Particle *)calloc(particles_data->num_particles, sizeof(Particle));
    particles_data->velocity_field = (VectorField *)malloc(sizeof(VectorField));

    int width = 100;
    int height = 100 * (get_sky_height() / get_scene_width());
    particles_data->velocity_field->init(width, height);
    particles_data->velocity_field->bottom_left = v2(get_scene_bottom_left().x, 0.0f);
    particles_data->velocity_field->top_right = get_scene_top_right();

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        particles_data->particles[i].rendering_data = &(particles_data->rendering_data[i]);
        particles_data->particles[i].physics_data = &(particles_data->physics_data[i]);
    }

    update_velocity_field(v2());


    particles_data->wrapping = true;

    init_particle_renderer();

    seed_random(1);
    set_particle_colors();
    reset_particle_positions();
    glBufferSubData(GL_ARRAY_BUFFER, 0,
            sizeof(ParticleRenderingData) * particles_data->num_particles, particles_data->rendering_data);
    check_gl_errors("send particle data");
}



static void move_particle(Particle *p, v2 mouse_pos)
{
    ParticleRenderingData *render = p->rendering_data;
    ParticlePhysicsData *physics = p->physics_data;

    v2 to_mouse = mouse_pos - render->position;

    if(key_state('Q'))
    {
        v2 direction = unit(to_mouse);
        float mag = length(to_mouse);
        physics->velocity += direction * mag * 0.1f;
    }

    if(key_state('W'))
    {
        //p->velocity += unit(to_mouse) * 0.01f;
        if(length(to_mouse) != 0.0f)
        {
            v2 direction = unit(to_mouse);

            float to_mouse_length2 = length(to_mouse)*length(to_mouse);
            float min_dist = 0.05f;
            if(to_mouse_length2 <= min_dist) to_mouse_length2 = min_dist;

            float mag = 1.0f / to_mouse_length2;
            physics->velocity += direction * mag * 0.0005f;
        }
    }

    if(key_state('E'))
    {
        v2 direction = unit(to_mouse);
        float mag = length(to_mouse) * length(to_mouse);
        physics->velocity += direction * mag * 0.05f;
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

    render->position += physics->velocity * physics->modifier * 0.01;

    render->position += unit( v2(1.0f, -0.2f) ) * 0.00002f;

    render->rotation += physics->angular_velocity;


    v2 sky_bl = v2(get_scene_bottom_left().x, 0.0f);
    v2 sky_tr = get_scene_top_right();
    float width = sky_tr.x - sky_bl.x;
    float height = get_sky_height();
    if(particles_data->wrapping)
    {
        while(render->position.x >= sky_tr.x) render->position.x -= width;
        while(render->position.x <= sky_bl.x) render->position.x += width;
        while(render->position.y >= sky_tr.y) render->position.y -= height;
        while(render->position.y <= sky_bl.y) render->position.y += height;
    }
}


/*
static void move_particle(Particle *p, v2 mouse_pos)
{
    ParticleRenderingData *render = p->rendering_data;
    ParticlePhysicsData *physics = p->physics_data;

    v2 velocity = particles_data->velocity_field->at(render->position);
    velocity *= physics->modifier;

    render->position += velocity;

    v2 sky_bl = v2(get_scene_bottom_left().x, 0.0f);
    v2 sky_tr = get_scene_top_right();
    float width = sky_tr.x - sky_bl.x;
    float height = get_sky_height();
    if(particles_data->wrapping)
    {
        while(render->position.x >= sky_tr.x) render->position.x -= width;
        while(render->position.x <= sky_bl.x) render->position.x += width;
        while(render->position.y >= sky_tr.y) render->position.y -= height;
        while(render->position.y <= sky_bl.y) render->position.y += height;
    }
}
*/

void update_particles()
{
    //EASY_FUNCTION();

    v2 mouse_pos = mouse_world_position();

    //update_velocity_field(mouse_pos);

#if USE_COMPUTE_SHADER
    run_compute_shader();
#else

    ImGui::Checkbox("wrap", &particles_data->wrapping);

    for(int i = 0; i < particles_data->num_particles; i++)
    {
        move_particle(&(particles_data->particles[i]), mouse_pos);
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

    Shader particles_shader;

    GLuint particles_vao;
    GLuint particles_vbo;

    RenderTexture render_texture;



#if USE_COMPUTE_SHADER
    GLuint compute_shader;
#endif
};
static RendererData *particles_renderer_data;










static void set_particle_attrib_pointers()
{
    float stride = sizeof(ParticleRenderingData);

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


    /*
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
    */





    {
        glGenVertexArrays(1, &particles_renderer_data->particles_vao);
        glBindVertexArray(particles_renderer_data->particles_vao);
        check_gl_errors("making vao");

        glGenBuffers(1, &particles_renderer_data->particles_vbo);
        check_gl_errors("making vbo");

        glBindBuffer(GL_ARRAY_BUFFER, particles_renderer_data->particles_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ParticleRenderingData) * particles_data->num_particles,
                0, GL_DYNAMIC_DRAW);
        check_gl_errors("send vbo data");

        set_particle_attrib_pointers();
        check_gl_errors("vertex attrib pointer");
    }



    particles_renderer_data->particles_shader= make_shader("shaders/stars.shader");
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


    float rt_resolution = 2560.0f;
    particles_renderer_data->render_texture = make_render_texture(get_scene_width() * rt_resolution, get_sky_height() * rt_resolution);
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
    // glDispatchCompute(2, 2, 1)
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

static mat4 get_stars_ndc_m_world()
{
    // map scene bl and scene tr
    // to -1, -1 and 1, 1
    // for sky quad
    float scene_width = get_scene_width();
    float sky_height = get_sky_height();
    return
    {
        (1.0f / scene_width) * 2.0f, 0.0f, 0.0f,  0.0f,
        0.0f, (1.0f / sky_height) * 2.0f,  0.0f, -1.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

static void draw_grid_cells()
{
    float pad = 0.001f;
    float width = get_scene_width() / particles_data->velocity_field->width - pad;
    float height = get_sky_height() / particles_data->velocity_field->height - pad;
    v2 scale = v2(width, height);

    v4 bg_color = v4(1.0f, 1.0f, 1.0f, 0.1f);

    for(int y = 0; y < particles_data->velocity_field->height; y++)
    {
        for(int x = 0; x < particles_data->velocity_field->width; x++)
        {
            v2 pos = v2(x, y);
            pos = particles_data->velocity_field->grid_to_world(pos);

            v2 vel = particles_data->velocity_field->at(pos);
            float mag = 0.015f;
            v2 arrow_end = pos + unit(vel) * mag;

            draw_quad(bg_color, pos, scale);
            draw_arrow(pos, arrow_end, v4(1.0f, 1.0f, 1.0f, 0.5f));
        }
    }
}

void draw_stars()
{
    //EASY_FUNCTION();

    float sky_height = get_sky_height();

#define RENDER_QUAD 1
#if RENDER_QUAD
    set_render_target(particles_renderer_data->render_texture);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif


    // Make sure the vbo is finished updating before drawing from it
    glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

    use_shader(particles_renderer_data->particles_shader);

    mat4 ndc_m_world = get_stars_ndc_m_world();
    //mat4 ndc_m_world = get_ndc_m_world();
    set_uniform(particles_renderer_data->particles_shader, "vp", ndc_m_world);

    glBindVertexArray(particles_renderer_data->particles_vao);
    check_gl_errors("use vao");

    glBindBuffer(GL_ARRAY_BUFFER, particles_renderer_data->particles_vbo);
    set_particle_attrib_pointers();
    check_gl_errors("set vbo");

#if USE_COMPUTE_SHADER
#else
    glBufferSubData(GL_ARRAY_BUFFER, 0,
            sizeof(ParticleRenderingData) * particles_data->num_particles, particles_data->rendering_data);
    check_gl_errors("send particle data");
#endif

    GLuint texture = get_or_make_texture("textures/star.png");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    glFinish();

    glDrawArrays(GL_POINTS, 0, particles_data->num_particles);
    check_gl_errors("draw");
    //draw_ndc_quad(v4(0.0f, 1.0f, 0.0f, 1.0f), v2(), v2(2.0f, 2.0f));
    
    glFlush();
    glFinish();

#if RENDER_QUAD 
    reset_render_target();
    draw_bottom_left_quad(particles_renderer_data->render_texture.texture, v2(get_scene_bottom_left().x, 0.0f),
            v2(get_scene_width(), get_sky_height()));
    //draw_bottom_left_quad(v4(1, 0, 0, 1), v2(-0.5f, 0.0f), v2(1.0f, sky_height));
#endif
    

    //v2 pos = mouse_world_position();
    //draw_quad(v4(1.0f, 0.0f, 1.0f, 1.0f), pos, v2(0.01f, 0.01f));
}

void draw_reflected_stars()
{
    //EASY_FUNCTION();

    float sky_height = get_sky_height();
    float scene_height = get_scene_top_right().y - get_scene_bottom_left().y;
    v2 position = v2(-0.5f, 0.0f);
    v2 scale = v2(1.0f, -(scene_height - sky_height));

    // Draw stars reflection
    draw_bottom_left_quad(particles_renderer_data->render_texture.texture, position, scale);

    // Tint the lake
    v4 color = v4(0.0f, 0.0025f, 0.04f, 0.65f);
    draw_bottom_left_quad(color, position, scale);
}


