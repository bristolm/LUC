/*
luc_bubbles.c - Definition of bubble edicts and their behaviours.

Initial module revision - AJA 19980319
*/

#include "g_local.h"

//
// Bubble "style"s.
//
#define FLOAT_BUBBLE_1	1
#define FLOAT_BUBBLE_2	2

//
// Bubble speeds - 19980324 AJA
//
#define FLOAT_BUBBLE_1_SPEED	5
#define FLOAT_BUBBLE_2_SPEED	7
#define X_DRIFT					3
#define Y_DRIFT					3
#define Z_DRIFT					2

//
// Bubble frame counts
//
#define FLOAT_BUBBLE_1_FRAMES	10
#define FLOAT_BUBBLE_2_FRAMES	5

//--------------------------------------------------------------------
// Generic bubble "think" function. Takes care of animating the
// sprite, moving the bubble upwards, and canning the bubble
// when it gets out of liquid.
//--------------------------------------------------------------------
void bubble_think (edict_t *self)
{
	int		frames, contents;
	vec3_t	velocity, temp_vec;

	// AJA 19980413 - Check if this bubble has lived out its lifetime.
	// AJA 19980414 - Use the 'delay' member instead of the 'duration' member.
	if (level.time >= self->delay)
	{
		// it has lived long enough
		G_FreeEdict(self);
		return;
	}

	// Initialize velocity depending on bubble type.
	// AJA 19980414 - Apply the speed multiplier to the bubble's upward
	// velocity.
	// AJA 19980606 - Upward velocity already scaled by 'speed'. 'speed' now
	// contains that properly calculated number.
	if (self->style == FLOAT_BUBBLE_1 || self->style == FLOAT_BUBBLE_2)
	{
		VectorSet(velocity, 0, 0, self->speed);
	}
	else
	{
		// Bogus bubble.
		G_FreeEdict(self);
		return;
	}

	// Check for water current, and adjust velocity.
	contents = gi.pointcontents(self->s.origin);
	if (contents & MASK_CURRENT)
	{
		if (contents & CONTENTS_CURRENT_0)
		{
			VectorSet(temp_vec, X_DRIFT, 0, 0);
			VectorAdd(velocity, temp_vec, velocity);
		}
		else if (contents & CONTENTS_CURRENT_180)
		{
			VectorSet(temp_vec, -X_DRIFT, 0, 0);
			VectorAdd(velocity, temp_vec, velocity);
		}
		if (contents & CONTENTS_CURRENT_90)
		{
			VectorSet(temp_vec, 0, Y_DRIFT, 0);
			VectorAdd(velocity, temp_vec, velocity);
		}
		else if (contents & CONTENTS_CURRENT_270)
		{
			VectorSet(temp_vec, 0, -Y_DRIFT, 0);
			VectorAdd(velocity, temp_vec, velocity);
		}
		if (contents & CONTENTS_CURRENT_UP)
		{
			VectorSet(temp_vec, 0, 0, Z_DRIFT);
			VectorAdd(velocity, temp_vec, velocity);
		}
		else if (contents & CONTENTS_CURRENT_DOWN)
		{
			VectorSet(temp_vec, 0, 0, -Z_DRIFT);
			VectorAdd(velocity, temp_vec, velocity);
		}
	}

	// Move the bubble along its path upwards
	VectorAdd(self->s.origin, velocity, self->s.origin);

	// Remove the bubble if it is no longer in liquid.
	contents = gi.pointcontents(self->s.origin);
	// AJA 19981212 - Also remove if bubble is in a solid (cause if
	// a solid is in water, it will still pass the MASK_WATER check)
	if (!(contents & MASK_WATER) || (contents & MASK_SOLID))
	{
		G_FreeEdict(self);
		return;
	}

	// AJA 19980413 - Possibly remove the bubble if it is getting
	// close to the water's surface.
	VectorSet(temp_vec, 0, 0, 16);
	VectorAdd(self->s.origin, temp_vec, temp_vec);
	contents = gi.pointcontents(temp_vec);
	if (!(contents & MASK_WATER))
	{
		// The bubble is within 16 units of the surface, 33% chance
		// of deleting it now instead of later.
		// AJA 19980606 - Use Q2's random() instead of C's rand()
		if (random() < 0.33)
		{
			G_FreeEdict(self);
			return;
		}
	}

	// Animate the bubble.
	if (self->style == FLOAT_BUBBLE_1)
		frames = FLOAT_BUBBLE_1_FRAMES;
	else if (self->style == FLOAT_BUBBLE_2)
		frames = FLOAT_BUBBLE_2_FRAMES;
		
	if (++self->s.frame >= frames)
		self->s.frame = 0;

	self->nextthink = level.time + FRAMETIME;
}

//--------------------------------------------------------------------
// Spawn a bubble. 66% prob. of floatbubble1, 33% prob. of floatbubble2.
//--------------------------------------------------------------------
edict_t *SpawnBubble (vec3_t bubble_origin) {
	edict_t	*bubble;

	bubble  = G_Spawn();
	VectorCopy(bubble_origin, bubble->s.origin);
	bubble->movetype = MOVETYPE_NONE;
	bubble->solid = SOLID_NOT;
	VectorSet(bubble->mins, -8, -8, 8);
	VectorSet(bubble->maxs, 8, 8, 8);
	bubble->s.renderfx = RF_TRANSLUCENT;
	bubble->think = bubble_think;
	bubble->nextthink = level.time + FRAMETIME;

	// AJA 19980606 - Use Q2's random() instead of C's rand()
	if (random() < 0.66) {
		// This will be a larger, slower bubble (floatbubble1)
		VectorSet(bubble->velocity, 0, 0, 5);	// float up
		bubble->s.modelindex = gi.modelindex ("sprites/s_bubble_1.sp2");
		bubble->s.frame = random() * FLOAT_BUBBLE_1_FRAMES;
		bubble->style = FLOAT_BUBBLE_1;
		bubble->speed = FLOAT_BUBBLE_1_SPEED;
	}
	else {
		// This will be a smaller, faster bubble (floatbubble2)
		VectorSet(bubble->velocity, 0, 0, 7);	// float up
		bubble->s.modelindex = gi.modelindex ("sprites/s_bubble_2.sp2");
		bubble->s.frame = random() * FLOAT_BUBBLE_2_FRAMES;
		bubble->style = FLOAT_BUBBLE_2;
		bubble->speed = FLOAT_BUBBLE_2_SPEED;
	}

	// AJA 19980413 - Set a default duration (ie. time that the bubble
	// will be deleted no matter what) for this bubble.
	// AJA 19980414 - Use the 'delay' member instead of the 'duration' member.
	bubble->delay = level.time + 45.0;

	// AJA 19980414 - Set the default speed multiplier to 1.0 (normal).
	// AJA 19980721 - Change that. We set the bubble's speed according
	// to its type (see above if statement). The bubble generator can
	// still override the bubble's speed if it wants to.
	//bubble->speed = 1.0;

	gi.linkentity(bubble);
	return bubble;
}


/*QUAKED misc_bubblegen1 (0 .5 .8) (-8 -8 -8) (8 8 8)
"speed" - A multiplier that controls the speed of the rising bubbles.
	0.5 = half speed, 2.0 = twice speed, 1.0 = normal speed.
"delay" - Maximum number of seconds a spawned bubble can live (use
	to limit the max length of the "plume"). Default is 15.0
"wait" - Number of frames between bubbles (10 == 1 bubble / sec)
	Default is 3 (1 bubble every 3 frames)
*/
//--------------------------------------------------------------------
// Bubble Generator 1's think function. Spawns a new bubble each time
// it is called.
//--------------------------------------------------------------------
void bubblegen1_think (edict_t *self) {
	edict_t	*bubble;
	vec3_t	variance, bubble_origin;
	int		rx, ry;

	// Randomize the x-y position of the new bubble
	// AJA 19980606 - Use Q2's random() instead of C's rand()
	rx = random() * 16 - 8;
	ry = random() * 16 - 8;
	VectorSet(variance, rx, ry, 0);

	// Randomize the origin and spawn a bubble at that location.
	VectorAdd(self->s.origin, variance, bubble_origin);
	bubble = SpawnBubble(bubble_origin);

	// AJA 19980413 - Randomize the bubble's time to live according
	// to the bubble generator's duration.
	// AJA 19980414 - Use the 'delay' member instead of the 'duration' member.
	// AJA 19980606 - Use Q2's random() instead of C's rand()
	bubble->delay = level.time + self->delay - random();

	// AJA 19980414 - Initialize the bubble's 'speed' value to the 'speed'
	// value given to the bubble generator by the level designer.
	// AJA 19980606 - Scale the 'speed' value by the bubble type's upward
	// velocity, so that doesn't have to be calculated every frame.
	bubble->speed = self->speed * (bubble->style == FLOAT_BUBBLE_1 ?
		FLOAT_BUBBLE_1_SPEED : FLOAT_BUBBLE_2_SPEED);

	// Set next bubble spawn time.
	self->nextthink = level.time + self->wait * FRAMETIME;
}

//--------------------------------------------------------------------
// Bubble Generator 1's spawn function. Initialize the generator.
//--------------------------------------------------------------------
void SP_misc_bubblegen1 (edict_t *self) {
	
	// Default to spawning bubbles every third frame if not
	// otherwise specified in the level editor.
	// AJA 19980414 - Now uses the 'wait' member to determine the speed
	// at which bubbles are spawned, instead of 'speed'. 'speed' now
	// determines how fast the bubbles rise.
	if (self->wait <= 0)
		self->wait = 3;

	// AJA 19980413 - Added the "duration" var that determines the
	// maximum length of the bubble plume by placing a maximum
	// lifetime on all spawned bubbles (in seconds).
	// AJA 19980414 - Use the 'delay' member instead of the 'duration' member.
	if (self->delay <= 0)
		self->delay = 15.0;

	// AJA 19980414 - Use the 'speed' member as a multiplier to determine
	// the speed at which spawned bubbles rise to the surface. (default
	// to 1 obviously)
	if (self->speed <= 0)
		self->speed = 1;

	// Initialize the bubble generator.
	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorSet(self->mins, -8, -8, 8);
	VectorSet(self->maxs, 8, 8, 8);
	self->think = bubblegen1_think;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}
