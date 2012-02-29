/* COMP 426 Assignment 1
 * 
 * The following is an N-Body simulation that runs on the PPE of the Cell B.E.
 * 100 particles are placed randomly in four quadrants of 100 unit cube. 
 * Forces (Due to gravity) between them are computed and their position velocity
 * and acceleration are updated at each iteration.
 * 
 * Siddhartha Kattoju
 * 9209905
 * 
 *     This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <altivec.h>
#include <time.h>
#include <ppu_intrinsics.h>

#define NO_OF_PARTICLES 100 	/* number of particles (multiple of 4) */
#define INIT_BOUNDING_BOX 100	/* initial positions are bounded */
#define MASS_OF_PARTICLES 1.0	/* mass */
#define EPS2 200		/* softening factor */
#define COMPUTE_ITERATIONS 100	/* no of iterations to compute*/
#define TIME_STEP 0.1		/* time step */

#define EPS2_VECTOR (__vector float){EPS2,EPS2,EPS2,EPS2}
#define VECZERO (__vector float){0,0,0,0}
#define VEC3ZERO (__vector float){0,0,0,1}
#define VECHALF (__vector float){0.5, 0.5, 0.5, 0.5}
#define MASS_VECTOR (__vector float){MASS_OF_PARTICLES,MASS_OF_PARTICLES,MASS_OF_PARTICLES,MASS_OF_PARTICLES}
#define TIME_STEP_VECTOR (__vector float){TIME_STEP,TIME_STEP,TIME_STEP,TIME_STEP}
#define TIME_SQUARED vec_madd(TIME_STEP_VECTOR,TIME_STEP_VECTOR,VECZERO)

/* particle struct */
typedef struct
{
  __vector float position;
  __vector float velocity;
  __vector float acceleration;
  __vector float zero; //padding in order to byte align 64
  
}particle;

/* util functions */
void debug_print_float_vector(__vector float var, char* name){
  
  float *vec = (float*)&var;
  printf("%s : %f %f %f %f \n", name, vec[0], vec[1], vec[2], vec[3]);
  
}

void init_particles(particle* system){
  
/* 
 * particles will be placed randomly 4 quadrants of 3d space
 * q1: x>0, y>0
 * q2: x<0, y>0
 * q3: x<0, y<0
 * q4: x>0, y<0
 * 
 */

  int i = 0;
  srand(time(NULL));

  // q1 x>0, y>0
  for(; i < NO_OF_PARTICLES/4; i++){ 
    system[i].position = (__vector float){rand()%INIT_BOUNDING_BOX, rand()%INIT_BOUNDING_BOX, (rand()%INIT_BOUNDING_BOX*2)-INIT_BOUNDING_BOX, 0};
    system[i].velocity = (__vector float){0,0,0,0};
    system[i].acceleration = (__vector float){0,0,0,0};
  }

  // q2 x<0, y>0
  for(; i < NO_OF_PARTICLES/2; i++){
    system[i].position = (__vector float){-rand()%INIT_BOUNDING_BOX, rand()%INIT_BOUNDING_BOX, (rand()%INIT_BOUNDING_BOX*2)-INIT_BOUNDING_BOX, 0};
    system[i].velocity = (__vector float){0,0,0,0};
    system[i].acceleration = (__vector float){0,0,0,0};
    
    //debug_print_float_vector(particle_system[i].position,"particle_system[i].position");
    
  }
  
  // q3 x<0, y<0
  for(; i < 3*NO_OF_PARTICLES/4; i++){
    
    system[i].position = (__vector float){-rand()%INIT_BOUNDING_BOX, -rand()%INIT_BOUNDING_BOX, (rand()%INIT_BOUNDING_BOX*2)-INIT_BOUNDING_BOX, 0};
    system[i].velocity = (__vector float){0,0,0,0};
    system[i].acceleration = (__vector float){0,0,0,0};
    
  }

  // q4 x>0, y<0
  for(; i < NO_OF_PARTICLES; i++){
    
    system[i].position = (__vector float){rand()%INIT_BOUNDING_BOX, -rand()%INIT_BOUNDING_BOX, (rand()%INIT_BOUNDING_BOX*2)-INIT_BOUNDING_BOX, 0};
    system[i].velocity = (__vector float){0,0,0,0};
    system[i].acceleration = (__vector float){0,0,0,0};
    
  }
  
  
}

void compute_interaction(particle* i , particle* j){
  
  __vector float radius, radius_sqr, s_vector, displ, accel, distSqr, distSixth, invDistCube;

  //debug_print_float_vector(j->position,"init j.position");
  //debug_print_float_vector(i->position,"init i.position");

  /*compute acceleration of particle i*/
  radius = vec_sub(j->position,i->position);
  
  //debug_print_float_vector(radius,"radius");
  
  radius_sqr = vec_madd(radius,radius, VECZERO);
  distSqr = vec_add(radius_sqr,EPS2_VECTOR);
  distSixth = vec_madd(distSqr,distSqr,VECZERO);
  distSixth = vec_madd(distSixth,distSqr,VEC3ZERO);
  invDistCube = vec_rsqrte(distSixth);  
  s_vector = vec_madd(MASS_VECTOR,invDistCube,VECZERO);
  i->acceleration = vec_madd(radius,s_vector,i->acceleration);
  
  //debug_print_float_vector(i->acceleration,"new i.acceleration");
  
  /*compute new position & velocity of particle i*/
  displ = vec_madd(i->velocity,TIME_STEP_VECTOR,i->position);
  accel = vec_madd(VECHALF,i->acceleration, VECZERO);
  i->position = vec_madd(accel,TIME_SQUARED, displ);
  
  //debug_print_float_vector(i->position,"new i.position");
  
  i->velocity = vec_madd(i->acceleration,TIME_STEP_VECTOR, i->velocity);
  
  //debug_print_float_vector(i->velocity,"new i.velocity");
  
}


void update_particles(particle* system){
  
  int i, j;
  
  //Thread 1 ?
  for(i = 0;i<NO_OF_PARTICLES/4;i++){
    for(j = 0;j<NO_OF_PARTICLES;j++){
      compute_interaction(&system[i],&system[j]);
    }
  }
  
  //Thread 2 ?
  for(i = NO_OF_PARTICLES/4;i<NO_OF_PARTICLES/2;i++){
    for(j = 0;j<NO_OF_PARTICLES;j++){
      compute_interaction(&system[i],&system[j]);
    }
  }
  
  //Thread 3 ?
  for(i = NO_OF_PARTICLES/2;i<3*NO_OF_PARTICLES/4;i++){
    for(j = 0;j<NO_OF_PARTICLES;j++){
      compute_interaction(&system[i],&system[j]);
    }
  }
  
  //Thread 4 ? 
  for(i = 3*NO_OF_PARTICLES/4;i<NO_OF_PARTICLES;i++){
    for(j = 0;j<NO_OF_PARTICLES;j++){
      compute_interaction(&system[i],&system[j]);
    }
  }
    
}

void print_quadrant(particle* system){
  
  int i;
  __vector int qCount = (__vector int){0,0,0,0};
  int *quad = (int*)&qCount;
  
  for(i = 0; i<NO_OF_PARTICLES ; i++){
    
    float *pos = (float*)&system[i].position;
    
    if(pos[0] > 0 && pos[1] > 0){
      quad[0] = quad[0]+1;
    }
    
    if(pos[0] < 0 && pos[1] > 0){
       quad[1] = quad[1]+1;
    }
    
    if(pos[0] < 0 && pos[1] < 0){
       quad[2] = quad[2]+1;
    }
    
    if(pos[0] > 0 && pos[1] < 0){
       quad[3] = quad[3]+1;
    }
  }
  
  printf(" q1:%d q2:%d q3:%d q4:%d",quad[0], quad[1], quad[2], quad[3]);
    
}

void render(particle* system){
  
  int i = 0;
  for(; i < NO_OF_PARTICLES; i++){
    
    float *pos = (float*) &system[i].position;
    float *vel = (float*) &system[i].velocity;
    float *acc = (float*) &system[i].acceleration;
   
    printf("position : %f %f %f ", pos[0], pos[1], pos[2]);
    printf("velocity : %f %f %f ", vel[0], vel[1], vel[2]);
    printf("acceleration : %f %f %f \n", acc[0], acc[1], acc[2]);
    
  }
}

int main ()
{
  
  /* particle system  --> array of particles */
  particle particle_system[NO_OF_PARTICLES] __attribute__((aligned(64)));
  
  /* place particles in 4 quadrants */
  init_particles(particle_system);
  
  /* run simulation */
  float simulationTime = 0.0;
  int iterations = COMPUTE_ITERATIONS;
  printf("----------------------------------------------");
  printf("----------------------------------------------\n");  
  printf("Running Simulation with %d particles & %d iterations with %f seconds time steps\n", 
	 NO_OF_PARTICLES, COMPUTE_ITERATIONS, TIME_STEP);
  printf("----------------------------------------------");
  printf("----------------------------------------------\n");  
  
  while(iterations > 0){
  
    /* Compute */
    update_particles(particle_system);
       
    /* Display */
    render(particle_system);
    
    /* Update Time */
    simulationTime = simulationTime + TIME_STEP;
    printf("----------------------------------");  
    printf("Simulation Time: %f |",simulationTime);
    print_quadrant(particle_system);
    printf("----------------------------------\n"); 
    
    iterations --;
  }
  
  return 0;
}
