
#include "particles.h"

struct Particle
{
    v2 position;
    v2 velocity;
    v4 color;
};


struct ParticleData
{
    int num_particles;
    Particle *particles;
};
static ParticleData *particle_data;

void init_particles()
{
    particle_data = (ParticleData *)calloc(1, sizeof(ParticleData));

    particle_data->num_particles = 1000;
    particle_data->particles = (Particle *)calloc(particle_data->num_particles, sizeof(ParticleData));

    for(int i = 0; i < particle_data->num_particles; i++)
    {
        Particle *p = &(particle_data->particles[i]);

        float percent = i / 20.0f;
        float theta = 2.0f * PI * percent + offset;

        p->pos = v2(cos(theta), sin(theta)) * 2.0f;
    }
}

void update_particles()
{



    render_particles();
}


