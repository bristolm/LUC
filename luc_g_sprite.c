/*
luc_sprite.c - Implementation of a generic stationary sprite entity.
*/

/*QUAKED misc_sprite (0 .5 .8) (-16 -16 0) (16 16 96) Translucent
"speed" - Controls the speed of the animation:
  < 0 = no animation, stay on the first frame indefinately
  0 = animate at 10 fps (default speed)
  1 = animate at 10 fps
  2 = animate at 5 fps
  10 = animate at 1 fps
  Other values permissable according to the following formula:
  fps = 10 / speed

"name" - Name of the .sp2 file to use as the sprite (without the
  .sp2 extension, ex: "s_bubble_1")

"z_offset" - Number of pixels to shift the sprite up by. This should
  be half the height of the sprite (ie a sprite 16 tall, this should
  be 8).
*/

#include "g_local.h"

//
// Sprite Frame Count.
//
#define SPR_FRAMES	8

//--------------------------------------------------------------------
// Sprite's "think" function. Animates the sprite at the appropriate
// speed.
//--------------------------------------------------------------------
void sprite_think (edict_t *self) {
	
	if (++self->s.frame >= SPR_FRAMES)
		self->s.frame = 0;

	self->nextthink = level.time + FRAMETIME * self->speed;
}


//--------------------------------------------------------------------
// misc_sprite spawn function.
//--------------------------------------------------------------------
void SP_misc_sprite (edict_t *self) {
	char sprite_name[64];
	
	// Initialize the entity
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -16, -16, 0);
	VectorSet(self->maxs, 16, 16, 96);
	self->think = sprite_think;
	gi.linkentity(self);

	// Load the sprite into the edict
	if (!self->name)
	{
		gi.dprintf("name field of %s at %s has not been set\n",
			self->classname, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}
	sprintf(sprite_name, "sprites/%s.sp2", self->name);
	self->s.modelindex = gi.modelindex(sprite_name);
	self->s.frame = 0;

	// Offset the sprite's position
	if (self->z_offset)
	{
		vec3_t offset;

		VectorSet(offset, 0, 0, self->z_offset);
		VectorAdd(self->s.origin, offset, self->s.origin);
	}

	// Default to animating at 10Hz, if speed < 0, don't animate at
	// all. If speed > 0, use that as the timebase for animation.
	if (self->speed == 0)
	{
		self->speed = 1;
		self->nextthink = level.time + FRAMETIME;
	}
	else if (self->speed > 0)
		self->nextthink = level.time + FRAMETIME * self->speed;
	else
	{
		self->speed = -1;
		self->think = NULL;
	}

	// If the level designer chose to make it translucent, make it translucent.
	if (self->spawnflags & SPAWNFLAG_SP_TRANSLUCENT)
		self->s.renderfx = RF_TRANSLUCENT;
}
