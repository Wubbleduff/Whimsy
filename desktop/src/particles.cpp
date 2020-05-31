
#include "particles.h"
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












void init_particle_renderer();
void shutdown_renderer();
void begin_frame();
void end_frame();
v2 mouse_world_position();
v2 get_camera_half_extents();

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
static ParticleData *particle_data;

static const int NUM_PARTICLES = 5000;



void reset_particle_positions()
{
    v2 half_camera = get_camera_half_extents();
    float camera_width  = half_camera.x * 2.0f;
    float camera_height = half_camera.y * 2.0f;

    for(int i = 0; i < particle_data->num_particles; i++)
    {
        RenderParticle *p = &(particle_data->render_particles[i]);

        float percent = (float)i / particle_data->num_particles;
        float theta = 2.0f * PI * percent;

        float random_number = (float)rand() / (float)RAND_MAX;
        float rx = (float)rand() / (float)RAND_MAX;
        float ry = (float)rand() / (float)RAND_MAX;

        //p->position = v2(cos(theta), sin(theta)) * 5.0f * random_number;
        p->position = v2(-half_camera.x + camera_width * rx,
                         -half_camera.y + camera_height * ry);
        p->scale = v2(1.0f, 1.0f) * random_number * 0.08f;
        //p->color = v4(0.5f * random_number, 0.0f, 1.0f * random_number, 1.0f);
        p->color = v4(1.0f, 1.0f, 1.0f, 0.5f);
    }

    for(int i = 0; i < particle_data->num_particles; i++)
    {
        PhysicsParticle *p = &(particle_data->physics_particles[i]);

        p->velocity = v2();

        float random_number = (float)rand() / (float)RAND_MAX;
        p->angular_velocity = (random_number - 0.5f) * 0.25f;
    }
}

void init_particles()
{
    particle_data = (ParticleData *)calloc(1, sizeof(ParticleData));

    particle_data->num_particles = NUM_PARTICLES;
    particle_data->render_particles = (RenderParticle *)calloc(particle_data->num_particles, sizeof(RenderParticle));
    particle_data->physics_particles = (PhysicsParticle *)calloc(particle_data->num_particles, sizeof(PhysicsParticle));

    particle_data->wrapping = true;

    init_particle_renderer();

    srand(1);
    reset_particle_positions();
}


void update_particles()
{
    v2 mouse_pos = mouse_world_position();

    v2 camera_extents = get_camera_half_extents();

    ImGui::Checkbox("wrap", &particle_data->wrapping);

    for(int i = 0; i < particle_data->num_particles; i++)
    {
        RenderParticle *render = &(particle_data->render_particles[i]);
        PhysicsParticle *physics = &(particle_data->physics_particles[i]);

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

        if(particle_data->wrapping)
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
}




























struct RendererData
{
    FILE *log;

    float camera_width;

    GLuint quad_vao;
    GLuint quad_ebo;
    GLuint quad_shader_program;

    GLuint particles_shader_program;

    GLuint particles_vao;
    GLuint particles_vbo;

    GLuint default_texture;
};
static RendererData *particles_renderer_data;








static void check_gl_errors(const char *desc)
{
    GLint error = glGetError();
    if(error)
    {
        fprintf(particles_renderer_data->log, "Error: %s\n", desc);
        assert(false);
    }
}



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

    fopen_s(&particles_renderer_data->log, "output/renderer_log.txt", "wt");

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
        glBufferData(GL_ARRAY_BUFFER, sizeof(RenderParticle) * particle_data->num_particles,
                0, GL_DYNAMIC_DRAW);
        check_gl_errors("send vbo data");

        set_particle_attrib_pointers();
        check_gl_errors("vertex attrib pointer");
    }








    {
        const char *vert_shader_source =
            "#version 440 core\n"
            "layout (location = 0) in vec2 a_pos;\n"
            "layout (location = 1) in vec2 a_uv;\n"
            "uniform mat4 mvp;\n"
            "out vec2 uvs;\n"
            "void main()\n"
            "{\n"
            "    uvs = a_uv;\n"
            "    gl_Position = mvp * vec4(a_pos.x, a_pos.y, 0.0f, 1.0);\n"
            "}\0";

        const char *frag_shader_source =
            "#version 440 core\n"
            "uniform sampler2D texture0;\n"
            "in vec2 uvs;"
            "out vec4 frag_color;\n"
            "void main()\n"
            "{\n"
            "    frag_color = texture(texture0, uvs);\n"
            //"    frag_color = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
            //"    frag_color = vec4(uvs.x, uvs.y, 0.0f, 0.0f);\n"
            "}\0";

        particles_renderer_data->quad_shader_program = make_shader(vert_shader_source, frag_shader_source);
    }




    {
        const char *vert_shader_source =
            "#version 440 core\n"
            "layout (location = 0) in vec2 a_pos;\n"
            "layout (location = 1) in vec2 a_scale;\n"
            "layout (location = 2) in vec4 a_color;\n"
            "layout (location = 3) in float a_rotation;\n"

            "uniform mat4 vp;\n"

            "out VS_OUT\n"
            "{\n"
            "    mat4 mvp;\n"
            "    vec4 color;\n"
            "} vs_out;\n"

            "void main()\n"
            "{\n"
            "    float c = cos(a_rotation);\n"
            "    float s = sin(a_rotation);\n"
            "    mat4 scaling = mat4(\n"
            "       a_scale.x, 0.0f, 0.0f, 0.0f,\n"
            "       0.0f, a_scale.y, 0.0f, 0.0f,\n"
            "       0.0f, 0.0f, 1.0f, 0.0f,\n"
            "       0.0f, 0.0f, 0.0f, 1.0f\n"
            "       );\n"
            "    mat4 rotating = mat4(\n"
            "       c, s, 0.0f, 0.0f,\n"
            "       -s, c, 0.0f, 0.0f,\n"
            "       0.0f, 0.0f, 1.0f, 0.0f,\n"
            "       0.0f, 0.0f, 0.0f, 1.0f\n"
            "       );\n"
            "    mat4 translating = mat4(\n"
            "       1.0f, 0.0f, 0.0f, 0.0f,\n"
            "       0.0f, 1.0f, 0.0f, 0.0f,\n"
            "       0.0f, 0.0f, 1.0f, 0.0f,\n"
            "       a_pos.x, a_pos.y, 0.0f, 1.0f\n"
            "       );\n"
            "    vs_out.mvp = vp * translating * rotating * scaling;\n"
            "    vs_out.color = a_color;\n"
            "    gl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0);\n"
            "}\0";

#if 1
        const char *geom_shader_source =
        {
            "#version 440 core\n"
            "layout (points) in;\n"
            "layout (triangle_strip, max_vertices = 4) out;\n"

            "in VS_OUT\n"
            "{\n"
            "    mat4 mvp;\n"
            "    vec4 color;\n"
            "} gs_in[];\n"

            "out vec2 uvs;\n"
            "out vec4 color;\n"
            
            "void main() {\n"
            "    mat4 mvp = gs_in[0].mvp;\n"
            "    vec4 v_color = gs_in[0].color;\n"

            "    uvs = vec2(0.0f, 0.0f);\n"
            "    color = v_color;\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4(-0.5f, -0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    uvs = vec2(1.0f, 0.0f);\n"
            "    color = v_color;\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4( 0.5f, -0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    uvs = vec2(0.0f, 1.0f);\n"
            "    color = v_color;\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4(-0.5f,  0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    uvs = vec2(1.0f, 1.0f);\n"
            "    color = v_color;\n"
            "    gl_Position = mvp * (gl_in[0].gl_Position + vec4( 0.5f,  0.5f, 0.0f, 0.0f));\n"
            "    EmitVertex();\n"

            "    EndPrimitive();\n"
            "}\n"
        };
#else
        const char *geom_shader_source =
        {
            "#version 440 core\n"
            "layout (points) in;\n"
            "layout (points, max_vertices = 1) out;\n"
            //"in mat4 mvp;\n"
            "void main() {\n"
            "    gl_Position = gl_in[0].gl_Position;\n"
            "    EmitVertex();\n"

            "    EndPrimitive();\n"
            "}\n"
        };
#endif

        const char *frag_shader_source =
            "#version 440 core\n"
            "uniform sampler2D texture0;\n"

            "in vec2 uvs;"
            "in vec4 color;"

            "out vec4 frag_color;\n"

            "void main()\n"
            "{\n"
            "    frag_color = texture(texture0, uvs) * color;\n"
            //"    frag_color = color;\n"
            //"    frag_color = vec4(uvs.x, uvs.y, 0.0f, 1.0f);\n"
            "}\0";

        particles_renderer_data->particles_shader_program = make_shader(vert_shader_source, frag_shader_source, geom_shader_source);
        check_gl_errors("particle shader");
    }


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);






    particles_renderer_data->default_texture = make_texture("textures/star.png");


    particles_renderer_data->camera_width = 10.0f;
}

static void shutdown_renderer()
{
    fclose(particles_renderer_data->log);
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

void render_particles()
{

#if 0
    for(int i = 0; i < particle_data->num_particles; i++)
    {
        RenderParticle *p = &(particle_data->render_particles[i]);
        render_quad(p->position, p->scale);
    }
#else
    glUseProgram(particles_renderer_data->particles_shader_program);


    mat4 ndc_m_world = get_ndc_m_world();

    GLint loc = glGetUniformLocation(particles_renderer_data->particles_shader_program, "vp");
    glUniformMatrix4fv(loc, 1, true, &(ndc_m_world[0][0]));
    check_gl_errors("set uniform");


    glBindVertexArray(particles_renderer_data->particles_vao);
    check_gl_errors("use vao");




    // Rotate VBOs
    GLuint vbo = particles_renderer_data->particles_vbo;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    set_particle_attrib_pointers();
    check_gl_errors("set vbo");

    
    glBufferSubData(GL_ARRAY_BUFFER, 0,
      sizeof(RenderParticle) * particle_data->num_particles, particle_data->render_particles);
    check_gl_errors("send particle data");


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, particles_renderer_data->default_texture);

    glDrawArrays(GL_POINTS, 0, particle_data->num_particles);

    check_gl_errors("draw");
 
#endif


    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}




/*
void add_particles(RenderParticle *particles, int num_particles)
{
}
*/
























