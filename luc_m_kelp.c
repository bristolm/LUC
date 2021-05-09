//  luc_g_kelp.c - Definition of the various kelp I'm going to end out with in the game
//  Initial module revision - MRB 042798

//  Several object's behaviors are covered in here.  
//  No real reason - trying to consolidate, I guess.
//
//  Contains - misc_kelp_small and misc_kelp_large

#include "g_local.h"

#define FRAME_KELP_STAND_START		0
#define FRAME_KELP_STAND_END		20

#define FRAME_KELP_WIGGLEA_START	21
#define FRAME_KELP_WIGGLEA_END		25
#define FRAME_KELP_WIGGLEB_START	26
#define FRAME_KELP_WIGGLEB_END		30

#define FRAME_KELP_TWISTLEFT_START	31
#define FRAME_KELP_TWISTLEFT_END	35
#define FRAME_KELP_TWISTRIGHT_START	36
#define FRAME_KELP_TWISTRIGHT_END	40


// Wiggle if touched
void kelp_wiggle (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	diff, my_up, other_fwd, vec_result;
	float	effect;

	// if the same thing is still touching me in the same place
	// then I'll ignore it until it moves enough to make a 
	// big enough difference.
	VectorSubtract(other->s.origin, self->pos1, diff);
	if ( (self->target_ent) && 
		 (self->target_ent == other) && (VectorLength(diff) < 10))
		return;

	// Store info on toucher so it can be compared the next time
	self->target_ent = other;
	VectorCopy(other->s.origin, self->pos1);

	// Store other's direction, my relative 'up' and a vector 
	// bewteen the origins.
	AngleVectors(other->s.angles, other_fwd, NULL, NULL);
	AngleVectors(self->s.angles, NULL, NULL, my_up);
	VectorSubtract(other->s.origin, self->s.origin, diff);

	// Cross to find the resulting force, dot to find out 
	// the impact relative to myself.
	CrossProduct(other_fwd, diff, vec_result);
	effect = DotProduct(vec_result, my_up);

	if (fabs(effect) > 16)
	{
		if (effect < 0)
			self->s.frame = FRAME_KELP_TWISTRIGHT_START;
		else
			self->s.frame = FRAME_KELP_TWISTLEFT_START;
	}
	else
	{
		if ((rand() + 1) % 2 == 1)
			self->s.frame = FRAME_KELP_WIGGLEA_START;
		else
			self->s.frame = FRAME_KELP_WIGGLEB_START;
	}
	self->touch = NULL;
}

// Hey - kelp thinks!
void kelp_think (edict_t *self)
{
	switch (++self->s.frame)
	{
	case FRAME_KELP_WIGGLEA_END:
	case FRAME_KELP_WIGGLEB_END:
	case FRAME_KELP_TWISTRIGHT_END:
	case FRAME_KELP_TWISTLEFT_END:
		self->touch = kelp_wiggle;

	case FRAME_KELP_STAND_END:
		self->s.frame = FRAME_KELP_STAND_START;
		self->nextthink = level.time + FRAMETIME;

		if (random() < .5)
			self->s.frame++;
		break;

	default:
		self->nextthink = level.time + FRAMETIME;
		break;
	}
}

// Spawn some kelp ...
void kelp_start( edict_t *self)
{
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_TRIGGER;

	self->pos1[0] = 0;
	self->pos1[1] = 0;
	self->pos1[2] = 0;
	self->target_ent = NULL;

	self->s.frame = 0;
	self->think = kelp_think;
	self->touch = kelp_wiggle;
	self->nextthink = level.time + 2 * FRAMETIME;
	gi.linkentity (self);
}

/*QUAKED monster_kelp_small (1 .5 0) (-32 -32 0) (32 32 60) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_kelp_small (edict_t *self)
{
	VectorSet (self->mins, -40, -40, 0);
	VectorSet (self->maxs, 40, 40, 60);
	self->s.modelindex = gi.modelindex ("models/monsters/kelp/small/tris.md2");

	kelp_start(self);
}

/*QUAKED monster_kelp_large (1 .5 0) (-32 -32 0) (32 32 128) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_kelp_large (edict_t *self)
{
	VectorSet (self->mins, -40, -40, 0);
	VectorSet (self->maxs, 40, 40, 130);
	self->s.modelindex = gi.modelindex ("models/monsters/kelp/large/tris.md2");

	kelp_start(self);
}