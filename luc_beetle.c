// LUC Beetle Bot

#include "g_local.h"

#define BEETLE_WALK				0
#define BEETLE_REST				1
#define BEETLE_TELEPORT			2
#define BEETLE_LEAP_UP			3
#define	BEETLE_LEAP_DOWN		4
#define BEETLE_DIRCHANGE		5
#define BEETLE_LEAP_SHOOT_UP	6	// RSP 081300
#define BEETLE_LEAP_SHOOT_DOWN	7	// RSP 081300

#define BEETLE_WALK_SPEED		40

#define BEETLE_LEAP_SPEED		800
#define BEETLE_FLEE_SPEED		80
#define BEETLE_LEAPSHOOT_SPEED	300	// RSP 081300

#define BUFFER				2	// extra distance beetle is kept off the 'floor'


// RSP 072100 - Introduced Beetle
//              This is an entity that wanders around the landscape, on floors, ceilings,
//              and walls. Occasionally it will jump from the ceiling to the floor, and
//              from the floor to the ceiling, if it can reach it. It will stay away from
//              lava and slime, but will travel through water.

//				When health is less than 100%, the beetle becomes tranclucent. When health
//				is less than 50%, the beetle becomes invisible. The beetle will regen its
//				health at the rate of 5 points per second, becoming visible, then opaque
//				as it recovers.


// AlignBeetle() aligns the beetle to its current "normal", which is straight up through
// its back. When a beetle is walking on a surface, this normal is that of the surface.
// It the beetle is leaping, the normal will adjust as it flies to give the best appearance.

void AlignBeetle(edict_t *self)
{
	vec3_t prev_normal,this_normal;

	// self->pos1 contains the normal of the beetle

	// _SetBaseAnglesFromPlaneNormal() changes both pos1 and pos2, so we have to save them
	// and restore them later.

	VectorCopy(self->pos2,prev_normal);
	VectorCopy(self->pos1,this_normal);
	_SetBaseAnglesFromPlaneNormal(this_normal,self);
	VectorCopy(self->pos1,self->s.angles);
	VectorCopy(prev_normal,self->pos2);
	VectorCopy(this_normal,self->pos1);
}


// Beetle kicked the bucket

void Beetle_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point) 
{
	// We're going to blow up the Beetle
	// Do damage to all ents within blast radius

	T_RadiusDamage(self, self, self->dmg, NULL, self->dmg_radius, MOD_BEETLE);

	// Spew some guts

	G_Make_Debris(self, DEBRIS1_MODEL, DEBRIS2_MODEL, DEBRIS3_MODEL);

	// Blow him up good

	SpawnExplosion(self,TE_EXPLOSION1,MULTICAST_PVS,EXPLODE_SMALL);

	G_FreeEdict(self); 
}


// Beetle_Stop() stops the beetle cold, in preparation for resting or changing direction

void Beetle_Stop(edict_t *self)
{
	VectorClear(self->velocity);	// clear velocity
	VectorClear(self->pos3);		// clear acceleration/deceleration
	self->speed = 0;
	self->pain_debounce_time = level.time;
	self->state = BEETLE_REST;
}


// Beetle touched something.

// When the beetle touches another entity, it will reverse direction unless it's
// resting (standing still). When the beetle touches the world, it will decide
// whether to bounce back and stay on its present surface, or move to the new
// surface. This routine can be called either by the engine or by beetle code
// farther down.

void Beetle_Touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t dir;

	// touch_debounce_time keeps you from getting multiple "touches" as the
	// bounding boxes overlap. All you need to recognize is one.

	if (level.time < self->touch_debounce_time)
		return;

	if (other && (Q_stricmp(other->classname,"freed") == 0))
		return;

	if (other && (Q_stricmp(other->classname,"worldspawn") != 0))
	{
		// Touched something other than the world

		if (VectorLength(self->velocity))
		{
			VectorNegate(self->velocity,self->velocity);	// head the other way
			VectorNegate(self->pos3,self->pos3);	// negate acceleration/deceleration also
		}

		self->touch_debounce_time = level.time + 1;
		return;
	}

	// Touched the world.

	// Beetle has to decide whether to stay on the surface it's currently on,
	// and ricochet, or move to the new surface.

	// If gravity > 0, then we've been leaping and we just hit a surface, so we definitely
	// want to switch to the new surface normal.

	if (self->gravity > 0)						// Done leaping, we're landing now
	{	// Move to new plane

		VectorCopy(self->pos1,self->pos2);		// save previous normal
		VectorCopy(plane->normal,self->pos1);	// save new normal
		AlignBeetle(self);						// align to the new surface
		self->gravity = 0;						// we take control of movement
	}

	// If other and surf are NULL, this is a call from Beetle_Think and not
	// from the engine. We've hit the ground and have to change planes.
	// If this is a call from the engine, then there's a 50% chance of changing
	// planes and a 50% chance of rebounding on the current plane.

	else if (((other == NULL) && (surf == NULL)) || (random() < 0.5))
	{	// Move to new plane

		VectorCopy(self->pos1,self->pos2);		// save previous normal
		VectorCopy(plane->normal,self->pos1);	// save new normal
		AlignBeetle(self);						// align to new surface

		// On the new plane, keep going in the direction of your old normal

		ProjectPointOnPlane(dir,self->pos2,self->pos1);
		VectorNormalize(dir);
		VectorScale(dir,BEETLE_WALK_SPEED + (self->style * 20),self->velocity);
		VectorScale(self->velocity,0.1,self->pos3);	// set acceleration/deceleration
		self->speed = BEETLE_WALK_SPEED + (self->style * 20);
		VectorAdd(self->s.origin,self->pos1,self->s.origin);	// lift off surface so it doesn't get stuck
		self->groundentity = NULL;
		return;
	}

	// Stay on your old plane, but pick a new direction

	Beetle_Stop(self);
	self->state = BEETLE_DIRCHANGE;
}


// The Beetle begins its jump down from the ceiling

qboolean Beetle_JumpDown(edict_t *self)
{
	vec3_t	p;
	trace_t	tr;

	VectorCopy(self->s.origin,p);
	p[2] -= 2048;

	// First, check for lava or slime, which it avoids leaping into

	tr = gi.trace(self->s.origin,self->mins,self->maxs,p,self,CONTENTS_LAVA|CONTENTS_SLIME);

	if (tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME))
		return false;

	// Now find the landing point, and check for entities in the way

	tr = gi.trace(self->s.origin,self->mins,self->maxs,p,self,MASK_MONSTERSOLID);

	if (Q_stricmp(tr.ent->classname,"worldspawn") != 0)
		return false;

	self->z_offset = (int) tr.endpos[2];	// z value of where you'll land
	self->gravity = 1.0;					// let engine's gravity code apply
	VectorSet(self->pos1,1,0,-4);			// fudge for flipping over
	VectorCopy(tr.plane.normal,self->pos2);	// save landing plane normal
	self->state = BEETLE_LEAP_DOWN;
	return true;
}


// The Beetle begins its jump up from the floor

qboolean Beetle_JumpUp(edict_t *self)
{
	vec3_t	p;
	trace_t	tr;

	VectorCopy(self->s.origin,p);
	p[2] += 2048;
	tr = gi.trace(self->s.origin,self->mins,self->maxs,p,self,MASK_MONSTERSOLID);

	if (Q_stricmp(tr.ent->classname,"worldspawn") != 0)
		return false;

	self->z_offset = (int) tr.endpos[2];	// z value of where you'll land
	self->gravity = 1.0;					// let gravity code apply
	VectorScale(self->pos1,BEETLE_LEAP_SPEED,self->velocity);	// initial velocity up
	VectorSet(self->pos1,1,0,4);			// fudge for flipping over
	VectorCopy(tr.plane.normal,self->pos2);	// save landing plane normal
	self->state = BEETLE_LEAP_UP;
	return true;
}


// Can the beetle see the player?
// If so, return a pointer to the player's edict.
// If not, return NULL.
// If the player has an active decoy, and you can see it, return a pointer to it.

edict_t *Client_Visible(edict_t *self)
{
	edict_t *enemy = level.sight_client;

	if (enemy == NULL)
		return NULL;		// can't see him

	if (enemy->decoy)
		if (visible(self,enemy->decoy))
			return (enemy->decoy);

	if (!visible(self,enemy))
		return NULL;		// can't see him

	if (enemy->svflags & SVF_NOCLIENT)
		return NULL;		// can't see him

	return enemy;
}


// RSP 081300
// The Beetle begins its jump up from the floor to shoot the player

#define JUMPSHOOTDISTANCE 100

qboolean Beetle_JumpUp_And_Shoot(edict_t *self)
{
	vec3_t	p;
	trace_t	tr;
	edict_t *enemy = Client_Visible(self);
	
	if (enemy == NULL)
		return false;		// can't see him

	VectorMA(self->s.origin,JUMPSHOOTDISTANCE,self->pos1,p);
	tr = gi.trace(self->s.origin,self->mins,self->maxs,p,self,MASK_MONSTERSOLID);

	if (tr.fraction < 1)
		return false;		// no room to jump up

	self->gravity = 1.0;					// let gravity code apply
	VectorScale(self->pos1,BEETLE_LEAPSHOOT_SPEED,self->velocity);	// initial velocity up
	self->state = BEETLE_LEAP_SHOOT_UP;
	return true;
}


// RSP 090700 - added killer beetle_bolt, which accelerates as it moves

void Beetle_bolt_think(edict_t *self)
{
	if (--self->count == 0)
	{
		G_FreeEdict(self);
		return;
	}

	// Double velocity each frame
	// There's a roundoff problem over 1000, so limit velocity to <2000
	if (VectorLength(self->velocity) < 1000)
		VectorScale(self->velocity,2,self->velocity);	
	self->nextthink = level.time + FRAMETIME;
}


// RSP 090700 - added new beetle types
void Beetle_Shoot(edict_t *self)
{
	vec3_t	aimdir,ofs,start,end;
	edict_t	*bolt;
	edict_t	*enemy = Client_Visible(self);

	if (enemy == NULL)
		return;		// can't see him

	// Shoot at player (or decoy)

	VectorCopy(self->s.origin,start);
	start[2] += self->viewheight;
	self->enemy = enemy;	// Pick_Client_Target() needs this
	Pick_Client_Target(self,end);
	self->enemy = NULL;		// then clear it
	VectorSubtract(end,start,aimdir);

	VectorNormalize(aimdir);
	VectorScale(aimdir,self->maxs[0] + 2,ofs);
	VectorAdd(start,ofs,start);

	switch (self->style)
	{
	case BEETLE_STYLE_SHOOT_MILD:
		monster_fire_blaster(self,start,aimdir,2,400,MZ_BLASTER,EF_BLASTER);
		break;
	case BEETLE_STYLE_SHOOT_MED:
		monster_fire_blaster(self,start,aimdir,3,600,MZ_BLASTER,EF_BFG);
		break;
	case BEETLE_STYLE_SHOOT_HOT:
		monster_fire_blaster(self,start,aimdir,4,800,MZ_BLASTER,EF_FLAG1);
		break;
	case BEETLE_STYLE_SHOOT_KILLER:
		bolt = monster_fire_blaster(self,start,aimdir,100000,10,MZ_BLASTER,EF_FLAG2);
		if (bolt)
		{
			bolt->think = Beetle_bolt_think;
			bolt->nextthink = level.time + FRAMETIME;
			bolt->count = 40;	// life of bolt in frames
			bolt->s.modelindex = gi.modelindex("models/objects/laser2/tris.md2");
		}
		break;
	default:
		break;
	}
	self->damage_debounce_time = level.time + 3;	// RSP 090800 - wait 3 sec for next shot
}


// CheckBeetleGround() returns a bit mask based on which feet are touching a surface.

#define FRONT_LEFT_TOUCHING		0x01
#define FRONT_RIGHT_TOUCHING	0x02
#define BACK_LEFT_TOUCHING		0x04
#define BACK_RIGHT_TOUCHING		0x08
#define CENTER_TOUCHING			0x10

int CheckBeetleGround(edict_t *self)
{
	vec3_t	forward,right,offset;
	vec3_t	p;
	int		results = 0;	// assume none touching
	float	radius = self->maxs[0];

	M_CheckGround(self);	// sets self->groundentity if the engine thinks we're touching ground

	VectorNormalize2(self->velocity,forward);
	CrossProduct(forward,self->pos1,right);

	// Check right rear corner

	VectorSet(offset,-radius,radius,-(radius+4));
	R_ProjectSource(self->s.origin,offset,forward,right,self->pos1,p);

	if (gi.pointcontents(p) & MASK_SOLID)
		results |= BACK_RIGHT_TOUCHING;

	// Check left rear corner

	VectorSet(offset,-radius,-radius,-(radius+4));
	R_ProjectSource(self->s.origin,offset,forward,right,self->pos1,p);

	if (gi.pointcontents(p) & MASK_SOLID)
		results |= BACK_LEFT_TOUCHING;

	// Check right front corner

	VectorSet(offset,radius,radius,-(radius+4));
	R_ProjectSource(self->s.origin,offset,forward,right,self->pos1,p);

	if (gi.pointcontents(p) & MASK_SOLID)
		results |= FRONT_RIGHT_TOUCHING;

	// Check left front corner

	VectorSet(offset,radius,-radius,-(radius+4));
	R_ProjectSource(self->s.origin,offset,forward,right,self->pos1,p);

	if (gi.pointcontents(p) & MASK_SOLID)
		results |= FRONT_LEFT_TOUCHING;

	// Check center

	VectorSet(offset,0,0,-(radius+4));
	R_ProjectSource(self->s.origin,offset,forward,right,self->pos1,p);

	if (gi.pointcontents(p) & MASK_SOLID)
		results |= CENTER_TOUCHING;

	return results;
}


// Start Beetle walking in some direction

void Beetle_Start_Walk(edict_t *self,vec3_t targ_dir,int speed)
{
	vec3_t plane_dir;

	ProjectPointOnPlane(plane_dir,targ_dir,self->pos1);
	VectorNormalize(plane_dir);
	VectorScale(plane_dir,speed,self->velocity);
	self->speed = speed;
	VectorScale(self->velocity,0.1,self->pos3);	// set acceleration/deceleration
	VectorCopy(self->pos3,self->velocity);		// set initial velocity

	// Stay on this course for a small amount of time

	self->pain_debounce_time = level.time + 5 + crandom();

	AlignBeetle(self);   				// align with new surface
	VectorCopy(self->pos1,self->pos2);	// Both the same now
	self->state = BEETLE_WALK;
}


// Teleport the Beetle back to its spawn point if it got stuck somewhere

void Beetle_Teleport(edict_t *self)
{
	gi.positioned_sound(self->s.origin,self,CHAN_VOICE,gi.soundindex("misc/tele.wav"),1,ATTN_NORM,0);
	gi.unlinkentity(self);				// remove from game, put it back after teleport
	VectorCopy(self->move_origin,self->s.origin);	// go back to spawn point
	VectorCopy(self->move_origin,self->s.old_origin);
	KillBox(self);						// Kill anything there
	Beetle_Stop(self);
	self->state = BEETLE_TELEPORT;		// teleport next frame, then continue to wander around
	self->svflags |= SVF_NOCLIENT;		// make invisible during teleport
	self->teleport_time = level.time + 10;
}


// Drop beetle to the 'floor' and align it

void Beetle_Start(edict_t *self,vec3_t newdir)
{
	vec3_t	p;
	trace_t	tr;

	// Find surface 'below'

	self->gravity = 0;						// Gives me complete movement control. No friction.
	VectorCopy(self->s.origin,p);
	VectorMA(self->s.origin,2048,newdir,p);
	tr = gi.trace(self->s.origin,self->mins,self->maxs,p,self,MASK_MONSTERSOLID);
	VectorMA(tr.endpos,BUFFER,tr.plane.normal,self->s.origin);	// raise up from floor, add BUFFER to get off the ground
	VectorCopy(self->pos1,self->pos2);		// set previous orientation
	VectorCopy(tr.plane.normal,self->pos1);	// set orientation
	AlignBeetle(self);						// properly align it
}


// Is the beetle on the floor? This is defined as having a z-component of the beetle's
// normal being positive, and also the largest component.

qboolean Beetle_OnFloor(edict_t *self)
{
	return ((self->pos1[2] > self->pos1[0]) && (self->pos1[2] > self->pos1[1]));
}


// Is the beetle on the ceiling? This is defined as having a z-component of the beetle's
// normal being negative, and also the largest component.

qboolean Beetle_OnCeiling(edict_t *self)
{
	return ((self->pos1[2] < 0) && (self->pos1[2] < self->pos1[0]) && (self->pos1[2] < self->pos1[1]));
}


// This is called each FRAME to do all the beetle thinking, state changes, model
// and skin animation, etc.

void Beetle_Think(edict_t *self)
{
	vec3_t	p,targ_dir,dir,v;
	trace_t	tr;
	float	time_remaining;	// remaining time for walking
	float	current_speed;
	vec3_t	forward,right;
	vec3_t	offset;
	float	distance;
	int		footing;
	int		i;
	vec3_t	save_origin,save_normal,save_prev_normal,save_angles;

    self->nextthink = level.time + FRAMETIME;

	if (self->freeze_framenum > level.framenum)
	{
		M_SetEffects(self);							// RSP 081600 - take care of effects
		Beetle_Stop(self);
		return;
	}

    // Control model animations

	if (++self->s.frame == self->count2)
		self->s.frame = 0;

    // Control skin animations

    if (++self->s.skinnum == self->count)
        self->s.skinnum = 0;						// RSP 080100 - reset skins

	// Check health level

	if (self->health < self->max_health)
	{
		if (random() < 0.5)							// use random() because health is an int
			self->health++;							// regenerate

		if (self->health <= self->max_health/2)
			self->svflags |= SVF_NOCLIENT;			// become invisible
		else
			self->svflags &= ~SVF_NOCLIENT;			// visibility returns

		if (self->health < self->max_health)
			self->s.renderfx |= RF_TRANSLUCENT;		// become translucent
		else
			self->s.renderfx &= ~RF_TRANSLUCENT;	// become opaque
	}

	// See if beetle is stuck in the same place for 10 seconds. 'same place' means w/in 8 units
	// of s.old_origin, just in case the beetle is toggling back and forth somewhere.
	// If it's been 10 seconds, teleport the beetle back to original spawn point.

	VectorSubtract(self->s.origin,self->s.old_origin,v);	// RSP 092000
	if (VectorLength(v) < 8)
	{
		if (level.time >= self->teleport_time)
		{
			Beetle_Teleport(self);
			return;
		}
	}
	else
	{
		self->teleport_time = level.time + 10;			// movement occurred, so put off teleporting
		VectorCopy(self->s.origin,self->s.old_origin);	// for next comparison
	}

	footing = CheckBeetleGround(self);

	// Check for sudden touching of ground. If so, and we aren't already
	// scheduled to change direction, then call Beetle_Touch() and make
	// sure we change direction, because we just changed planes.

	// CheckBeetleGround() made a call to M_CheckGround() to set groundentity.
	// even though the beetle walks on walls, floors, and ceilings, groundentity
	// is only set when the beetle is walking on a floor (defined as having the z
	// value of its normal pointing up. The engine will call the touch functions when
	// an entity touches something other than the floor.

	if (self->groundentity && (self->state != BEETLE_DIRCHANGE))
	{
		// Find floor it's standing on

		self->gravity = 0;	// In case we fell from the ceiling
		VectorCopy(self->s.origin,p);
		p[2] -= 2048;
		tr = gi.trace (self->s.origin,self->mins,self->maxs,p,self,MASK_MONSTERSOLID);
		Beetle_Touch(self,NULL,&tr.plane,NULL);
		footing = CheckBeetleGround(self);	// reset your footing
	}

	// Do something based on current state

	switch(self->state)
		{
		default:

		case BEETLE_WALK:

			// See if we've walked off a step

			if (!footing)
			{
				VectorNegate(self->pos1,dir);		// look down for new surface
				Beetle_Start(self,dir);				// drop to the surface below, realign
				footing = CheckBeetleGround(self);	// reset your footing
			}

			// Check to see if there's still a surface under the beetle's origin
			// This is to see if we need to go over an edge.

			if (!(footing & CENTER_TOUCHING))
			{
				// Center is over space.
				// See if this is a step. Find a surface directly below you.

				VectorNormalize2(self->velocity,forward);
				VectorMA(self->s.origin,-2048,self->pos1,p);
				tr = gi.trace (self->s.origin,NULL,NULL,p,self,MASK_MONSTERSOLID);
				distance = tr.fraction*2048;

				// If 'distance' is > 35, this is a ledge and the beetle has to go over the
				// ledge and realign with the far side of the ledge.

				// If 'distance' is <= 35, then this is a step, and the beetle should just
				// drop down to the next step. The beetle will continue moving forward and
				// will drop down when all feet are out in space. (see above)

				if (distance > 35)	// < 35 is considered a step
				{
					// This is a ledge. Rotate over the edge.
					// First, find your new origin, which will be (self->maxs[0] + BUFFER) forward and
					// (self->maxs[0] + 6) down from where you are now.

					VectorCopy(self->s.origin,save_origin);		// save origin before going over edge
					CrossProduct(forward,self->pos1,right);
					VectorSet(offset,(self->maxs[0] + BUFFER),0,-(self->maxs[0] + 6));
					R_ProjectSource(self->s.origin,offset,forward,right,self->pos1,self->s.origin);

					// Either your front feet are in space, or your left/right feet are in space.
					// If your front feet are in space, you'll be pitching forward.
					// If your left/right feet are in space, you'll be rolling right or left.

					if ((footing & BACK_LEFT_TOUCHING) && (footing & BACK_RIGHT_TOUCHING))
					{
						// Your front legs are in space, back legs on ground

						VectorNegate(forward,dir);	// look behind you
					}
					else if (footing & BACK_LEFT_TOUCHING)
					{
						// Your back left is the only leg on the ground

						VectorNegate(right,dir);	// look to the left
					}
					else
					{
						// Your back right is the only leg on the ground

						VectorCopy(right,dir);	// look to the right
					}

					// See if the beetle will get stuck when it moves. If it will, then
					// disallow the move, and send it back the way it came.

					VectorCopy(self->pos1,save_normal);			// save normal
					VectorCopy(self->pos2,save_prev_normal);	// save previous normal
					VectorCopy(self->s.angles,save_angles);		// save model angles

					Beetle_Start(self,dir);				// drop to the surface below, realign

					if (!CheckSpawnPoint(self))			// if there's no room, disallow the move
					{
						VectorCopy(save_origin,self->s.origin);		// restore origin
						VectorCopy(save_normal,self->pos1);			// restore normal
						VectorCopy(save_prev_normal,self->pos2);	// restore previous normal
						VectorCopy(save_angles,self->s.angles);		// restore model angles
						AlignBeetle(self);							// restore orientation
						VectorNegate(self->velocity,self->velocity);	// velocity
						VectorNegate(self->pos3,self->pos3);			// acceleration/deceleration
						return;
					}
					else
					{
						// Keep going in the direction opposite to that of your old normal.
						// pos1 has your new normal and pos2 has your old normal.

						ProjectPointOnPlane(dir,self->pos2,self->pos1);
						VectorNormalize(dir);
						VectorScale(dir,-(BEETLE_WALK_SPEED + (self->style * 20)),self->velocity);
						VectorScale(self->velocity,0.1,self->pos3);	// set acceleration/deceleration
						self->speed = BEETLE_WALK_SPEED + (self->style * 20);
					}
				}
			}

			// Check ahead for lava/slime or entities

			VectorNormalize2(self->velocity,forward);
			VectorMA(self->s.origin,BEETLE_WALK_SPEED + (self->style * 20),forward,p);
			tr = gi.trace(self->s.origin,NULL,NULL,p,self,CONTENTS_LAVA|CONTENTS_SLIME);

			if ((tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME)) ||
				(tr.ent && (Q_stricmp(tr.ent->classname,"worldspawn") != 0)))
			{
				Beetle_Stop(self);
				self->state = BEETLE_DIRCHANGE;
				return;
			}

			// RSP 090700 - if this is a killer beetle on the floor, look for the player.
			// If this isn't a killer beetle, or it is and it can't see the
			// player, then do the velocity adjustments. Start the windup sound at this point.

			if ((self->style == BEETLE_STYLE_SHOOT_KILLER) &&
				Beetle_OnFloor(self) &&
				(self->damage_debounce_time <= level.time) &&
				Client_Visible(self))
			{	// stop moving, and the shot will be fired in the Rest code
				Beetle_Stop(self);
				self->pain_debounce_time = level.time;
				gi.sound(self, CHAN_VOICE, gi.soundindex("beetle/windup.wav"), 1, ATTN_NORM, 0);
				break;
			}

			// Adjust velocity. This takes care of acceleration when the beetle
			// starts moving, and deceleration at the end of the move.

			time_remaining = self->pain_debounce_time - level.time;
			current_speed = VectorLength(self->velocity);

			if (time_remaining > 1.0)
			{
				if (current_speed < self->speed)	// attained full speed yet?
					VectorAdd(self->velocity,self->pos3,self->velocity);	// No; speed up
				return;
			}
			else if (time_remaining > 0)
			{
				VectorSubtract(self->velocity,self->pos3,self->velocity);	// slow down
				return;
			}

			// sit still for a while

			Beetle_Stop(self);
			self->pain_debounce_time = level.time + 2 + crandom();
			break;

		case BEETLE_REST:

			if (level.time < self->pain_debounce_time)
				return;

			// RSP 081300
			// If an aggressive beetle, decide whether to shoot
			// RSP 090700 - add 2 new beetle styles
			// Only shoot if damage_debounce_time has expired

			if (Beetle_OnFloor(self) && (self->damage_debounce_time <= level.time))	// Can only jump up and shoot from the floor
			{
				if (self->style == BEETLE_STYLE_SHOOT_MILD)
				{
					if ((random() < 0.60) && Beetle_JumpUp_And_Shoot(self))
						return;
				}
				else if (self->style == BEETLE_STYLE_SHOOT_MED)
				{
					if ((random() < 0.75) && Beetle_JumpUp_And_Shoot(self))
						return;
				}
				else if (self->style == BEETLE_STYLE_SHOOT_HOT)
				{
					if ((random() < 0.90) && Beetle_JumpUp_And_Shoot(self))
						return;
				}
				else if (self->style == BEETLE_STYLE_SHOOT_KILLER)
				{
					if (Beetle_JumpUp_And_Shoot(self))
						return;
				}
			}

			// After resting, decide to jump up or down

			if (random() < 0.33)
				if (Beetle_OnCeiling(self))		// Jumping down from ceiling
				{
					if (Beetle_JumpDown(self))	// returns true if making the leap
						return;
				}
				else if (Beetle_OnFloor(self))	// Jumping up from floor
				{
					if (Beetle_JumpUp(self))	// returns true if making the leap
						return;
				}

			// Not jumping, so start walking.

			VectorSet(targ_dir,crandom(),crandom(),crandom());
			Beetle_Start_Walk(self,targ_dir,BEETLE_WALK_SPEED + (self->style * 20));
			break;

		case BEETLE_DIRCHANGE:

			if (self->groundentity)
			{
				self->s.origin[2]++;	// lift up so it doesn't get stuck
				self->groundentity = NULL;
			}

			// randomly change direction, but try to move away from your previous plane

			for (i = 0 ; i <= 2 ; i++)
			{
				targ_dir[i] = crandom();

				if (targ_dir[i]*self->pos2[i] < 0)
					targ_dir[i] *= -1;
			}

			Beetle_Start_Walk(self,targ_dir,BEETLE_WALK_SPEED + (self->style * 20));
			break;

		case BEETLE_LEAP_DOWN:

			if (gi.pointcontents(self->s.origin) & MASK_WATER)
				if (self->gravity > 0.10)	// this is the frame where it lands in the water
				{
					VectorScale(self->velocity,0.04,self->velocity);	// slow down
					self->gravity = 0.05;								// less gravity effect
					gi.positioned_sound(self->s.origin,g_edicts,CHAN_AUTO,gi.soundindex("misc/h2ohit1.wav"),1,ATTN_NORM,0);
				}

			self->pos1[2]++;

			// Are we close to the ground?

			if (self->s.origin[2] - (float)self->z_offset < 64)
				VectorCopy(self->pos2,self->pos1);

			AlignBeetle(self);   // change alignment
			break;

		case BEETLE_LEAP_UP:

			self->pos1[2]--;
			
			// Have we reached the apex of the leap w/o touching anything?
			// If so, we'll be falling back down.

			if (self->velocity[2] <= 0)
			{
				Beetle_JumpDown(self);
				return;
			}

			// Are we close to the ceiling?

			if ((float)self->z_offset - self->s.origin[2] < 64)
				VectorCopy(self->pos2,self->pos1);

			AlignBeetle(self);   // change alignment
			break;

		case BEETLE_TELEPORT:

			VectorSet(self->pos1,0,0,1);	// initialize normal to straight up
			Beetle_Start(self,tv(0,0,-1));	// drop to the surface below, realign
			if (self->health > self->max_health/2)
				self->svflags &= ~SVF_NOCLIENT;		// visible again

			self->pain_debounce_time = 0;
			self->state = BEETLE_REST;
			self->teleport_time = level.time + 10;
			gi.linkentity(self);
			break;

		case BEETLE_LEAP_SHOOT_UP:	// RSP 081300

			if (self->velocity[2] <= 0)
			{
				Beetle_Shoot(self);
				self->state = BEETLE_LEAP_SHOOT_DOWN;
			}
			break;

		case BEETLE_LEAP_SHOOT_DOWN:	// RSP 081300

			break;
		}
}


void Beetle_pain(edict_t *self, edict_t *attacker, float kick, int damage)
{
	vec3_t v;

	if (self->gravity > 0)			// If in the air or dropping through water, do nothing
		return;

	if (Beetle_OnFloor(self))		// If on the ground, jump
		if (Beetle_JumpUp(self))	// returns true if making the leap
			return;

	// Otherwise, flee from the attacker!

	if (attacker)
	{
		VectorSubtract(self->s.origin,attacker->s.origin,v);	// find vector
		Beetle_Start_Walk(self,v,BEETLE_FLEE_SPEED + (self->style * 20));
	}
}

// Beetle spawn code

/*QUAKED monster_beetle (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist

"message" the directory where the model is found.
          For example, if "message" is "smalltop" then place the
		  model for your small top in models/monsters/beetle/smalltop.

"frames" - number of animation frames
"skins"  - number of skins, for skin animation
"sizes"  - size of bounding box (i.e. '8,8,8' or '16,16,16'. Default is '8,8,8'.
"style"  - if none given, you get a style 1
           1 = pacifist
           2 = randomly leaps to player eye-height & shoots
           3 = moves faster, leaps & shoots more frequently, hurts more
           4 = fastest mover, leaps & shoots when seeing the player, kills
               with one shot. shot moves slow at first, then accelerates.
*/


void Beetle_Common(edict_t *beetle)
{
	if (beetle->spawnflags & SPAWNFLAG_PACIFIST)
		beetle->monsterinfo.aiflags |= AI_PACIFIST;

	if ((beetle->spawnflags & SPAWNFLAG_SIGHT) && !(beetle->monsterinfo.aiflags & AI_GOOD_GUY))
		beetle->spawnflags &= ~SPAWNFLAG_SIGHT;

	// RSP 081300 - all beetles are AMBUSH; they don't respond to player sounds

	beetle->spawnflags |= SPAWNFLAG_AMBUSH;

	beetle->movetype = MOVETYPE_STEP;
	beetle->solid    = SOLID_BBOX;			// Enable touch capability
	beetle->clipmask = MASK_MONSTERSOLID;	// RSP 080300
	beetle->deadflag = DEAD_NO;				// RSP 080300
	beetle->svflags |= SVF_MONSTER;			// RSP 080600

	beetle->s.skinnum = 0;

	// RSP 080200 - start on a random frame so models don't change in sync

	beetle->s.frame = rand() % beetle->count2;

	beetle->viewheight = beetle->maxs[2]/2;
	beetle->dmg = 20;
	beetle->dmg_radius = 100;			// 100 unit radius damage upon explosion.
	beetle->health = beetle->style * hp_table[HP_BEETLE];	// RSP 090700 - vary health
	beetle->max_health = beetle->health;
    beetle->touch = Beetle_Touch;
	beetle->takedamage = DAMAGE_YES;

	beetle->mass = 200;
	beetle->identity = ID_BEETLE;
    beetle->flags |= FL_PASSENGER;		// possible BoxCar passenger
	beetle->flags |= FL_NO_KNOCKBACK;	// no momentum impact when beetle is hit by something

	VectorCopy(beetle->s.origin,beetle->s.old_origin);	// previous location

	beetle->pain = Beetle_pain;
	beetle->die = Beetle_die;
	beetle->think = Beetle_Think;
	beetle->nextthink = level.time + FRAMETIME;
	beetle->pain_debounce_time = level.time;	// used to control length of time spent in a state
	beetle->teleport_time = level.time + 10;

	VectorSet(beetle->pos1,0,0,1);		// initialize orientation to straight up
	Beetle_Start(beetle,tv(0,0,-1));

	beetle->state = BEETLE_REST;
	VectorCopy(beetle->s.origin,beetle->move_origin);	// for teleporting when stuck

	gi.linkentity(beetle);
}


void SP_Beetle(edict_t *beetle) 
{
	char	buffer[MAX_QPATH];
	int		x,y,z;

	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	// Get item's model location from 'message' key

	if (!beetle->message || (beetle->message[0] == 0))
	{
		gi.dprintf("beetle w/no message key at %s\n",vtos(beetle->s.origin));
		G_FreeEdict(beetle);
		return;
	}

	Com_sprintf(buffer,sizeof(buffer),"models/monsters/beetle/%s/tris.md2",beetle->message);
	beetle->s.modelindex = gi.modelindex(buffer);

	// RSP 090700 - if no style give, default to pacifist

	if (!beetle->style)
		beetle->style = BEETLE_STYLE_PACIFIST;

	// RSP 081300 - BEETLE_STYLE_PACIFIST is a pacifist, and doesn't know how to attack

	if (beetle->style == BEETLE_STYLE_PACIFIST)
	{
		beetle->spawnflags |= SPAWNFLAG_PACIFIST;
		beetle->monsterinfo.aiflags |= AI_GOOD_GUY;
	}

	if (!(beetle->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	// Get model size if specified by map author

	if (st.sizes && (st.sizes[0] != 0))
	{
		sscanf(st.sizes,"%d,%d,%d",&x,&y,&z);
		VectorSet(beetle->maxs, ((float)x)/2, ((float)y)/2, ((float)z)/2);
		VectorSet(beetle->mins, -((float)x)/2, -((float)y)/2, -((float)z)/2);
	}
	else
	{
		VectorSet(beetle->maxs, 8, 8, 8);
		VectorSet(beetle->mins, -8, -8, -8);
	}

	if (st.skins)	// skin animated?
		beetle->count = st.skins;
	else
		beetle->count = 1;

	if (st.frames)	// model animated?
		beetle->count2 = st.frames;
	else
		beetle->count2 = 1;

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_top1 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_top1(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/redsmall/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_PACIFIST;

	beetle->spawnflags |= SPAWNFLAG_PACIFIST;
	beetle->monsterinfo.aiflags |= AI_GOOD_GUY;

	VectorSet(beetle->maxs, 8,8,8);
	VectorSet(beetle->mins, -8,-8,-8);

	beetle->count = 8;		// number of skins
	beetle->count2 = 40;	// number of model frames

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_top2 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_top2(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/bluemedium/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_PACIFIST;

	beetle->spawnflags |= SPAWNFLAG_PACIFIST;
	beetle->monsterinfo.aiflags |= AI_GOOD_GUY;

	VectorSet(beetle->maxs, 12,12,12);
	VectorSet(beetle->mins, -12,-12,-12);

	beetle->count = 8;		// number of skins
	beetle->count2 = 40;	// number of model frames

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_top3 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_top3(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/greenlarge/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_PACIFIST;

	beetle->spawnflags |= SPAWNFLAG_PACIFIST;
	beetle->monsterinfo.aiflags |= AI_GOOD_GUY;

	VectorSet(beetle->maxs, 16,16,16);
	VectorSet(beetle->mins, -16,-16,-16);

	beetle->count = 8;		// number of skins
	beetle->count2 = 40;	// number of model frames

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_spider1 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_spider1(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/4leggersmall/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_SHOOT_MILD;

	level.total_monsters++;

	VectorSet(beetle->maxs, 10,10,10);
	VectorSet(beetle->mins, -10,-10,-10);

	beetle->count = 8;		// number of skins
	beetle->count2 = 12;	// number of model frames

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_spider2 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_spider2(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/spidersmall/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_SHOOT_MED;

	level.total_monsters++;

	VectorSet(beetle->maxs, 12,12,12);
	VectorSet(beetle->mins, -12,-12,-12);

	beetle->count = 8;
	beetle->count2 = 12;

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_spider3 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_spider3(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/4leggerlarge/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_SHOOT_HOT;

	level.total_monsters++;

	VectorSet(beetle->maxs, 20,20,20);
	VectorSet(beetle->mins, -20,-20,-20);

	beetle->count = 8;
	beetle->count2 = 12;

	Beetle_Common(beetle);
}


/*QUAKED monster_beetle_spider4 (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_Beetle_spider4(edict_t *beetle) 
{
	if (deathmatch->value)
	{
		G_FreeEdict(beetle);
		return;
	}

	beetle->model = "models/monsters/beetle/spiderlarge/tris.md2";	// RSP 091600
	beetle->s.modelindex = gi.modelindex(beetle->model);	// RSP 091600
	beetle->message = NULL;				// RSP 091600 - for func_boxcar to work

	beetle->style = BEETLE_STYLE_SHOOT_KILLER;

	level.total_monsters++;

	VectorSet(beetle->maxs, 22,22,22);
	VectorSet(beetle->mins, -22,-22,-22);

	beetle->count = 8;
	beetle->count2 = 12;

	Beetle_Common(beetle);

	// RSP 091700 - reduce health so it's easy to kill
	beetle->health = 20;
	beetle->max_health = 20;
}
