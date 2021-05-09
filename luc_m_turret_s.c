/*
==============================================================================

STATIC (wall, etc) TURRET

==============================================================================
*/

#include "g_local.h"
//#include "luc_m_turret_s.h"

#define START_OFF			1
#define TURRET_DAMAGE		2	// how much damage a turret's blaster does
#define TURRET_BLAST_SPEED	600	// how fast the blaster bolt travels
#define TURRET_DEATH_SKIN	1	// displayed when turret dies

void turret_s_target(edict_t *);

// RSP 052299 - added ability to toggle turret on/off

void turret_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->spawnflags ^= SPAWNFLAG_PACIFIST;	// RSP 052299 - turret toggles on/off
}


// RSP 051399 - Add general enemy-sighting routine

edict_t	*TurretFindEnemy(edict_t *self)
{
	vec3_t	end;
	trace_t	tr;
	edict_t	*enemy;

	if (self->spawnflags & SPAWNFLAG_PACIFIST)	// RSP 052299 - turret toggles on/off
	{
		self->s.renderfx &= ~RF_GLOW;			// RSP 081700
		return NULL;
	}

	self->s.renderfx |= RF_GLOW;				// RSP 081700
	enemy = level.sight_client;
	if (enemy && (enemy->health > 0) && !(enemy->svflags & SVF_NOCLIENT))	// RSP 110799 - SVF_NOCLIENT for cloaking
	{
		if (enemy->decoy)						// RSP 110399 - is there a decoy?
			enemy = enemy->decoy;
		VectorCopy (enemy->s.origin, end);
		end[2] += enemy->viewheight;
		tr = gi.trace (self->s.origin,NULL,NULL,end,enemy,MASK_SOLID);
		if (tr.fraction == 1)
			return enemy;
	}

	return NULL;
}


// RSP 081700
// Changed the way the turret shoots at you. Previously, it generated a blaster bolt 
// from one part of the turret ring, then generated the next 45 degrees along the
// ring, etc., all the way around the ring.

// The new way is to have a pair of blaster bolts spawned at the same time at the
// 90 degree and 270 degree positions (0 degrees is UP), on the ring, and have them
// travel in parallel toward the player. However, at each FRAMETIME, have them
// rotate 45 degrees around a center point that's halfway between them. This should
// give a spiraling look to the entire shot. The center of the shot pair is maintained
// by using an invisible sprite edict that travels with the blaster bolts. It's
// referred to below as "shot_control" and "control entity".

// Since the bolts and the control entity move with each other, they need to know
// about each other. The control entity will point to the "left" bolt using the
// locked_to field. The "left" bolt will point to the "right" bolt, and the "right"
// bolt will point to the control entity. If a bolt dies, then the control entity and
// the remaining bolt point to each other. When the second bolt dies, the control
// entity dies with it. The control entity should pass through anything as it
// travels.

#define SHOT_FORWARD  6	// shot axis starts 6 forward
#define SHOT_RADIUS  13	// radial distance from shot axis for bolts

// This table represents the 8 positions around the control entity.
// It assumes a major radius of the turret torus of 13. Each [][] pair
// represents the forward/right/up vector offset from the control entity's
// origin, with [][0] marking the left bolt and [][1] marking the right bolt. 

static vec3_t shot_radius_offset[8][2] = {
//	forward right	up		from self->s.origin for each frame
	0,		0,		13  ,	0,		0,		-13 ,	//   0 degrees, top of ring when ring is vertical
	0,		9.2,	9.2 ,	0,		-9.2,	-9.2,	//  45 degrees, moving clockwise looking out from the ring
	0,		13,		0   ,	0,		-13,	0   ,	//  90
	0,		9.2,	-9.2,	0,		-9.2,	9.2 ,	// 135
	0,		0,		-13 ,	0,		0,		13  ,	// 180
	0,		-9.2,	-9.2,	0,		9.2,	9.2 ,	// 225
	0,		-13,	0   ,	0,		13,		0   ,	// 270
	0,		-9.2,	9.2 ,	0,		9.2,	-9.2	// 315
};


void turret_shot_think(edict_t *self)
{
	edict_t *left_bolt,*right_bolt;

	// One or both of the blaster bolts still lives.
	// When one bolt dies, the links are redone.
	// When the last bolt dies, the control entity dies with that bolt, in blaster_touch().

	// "count" is the primary index into the shot_radius_offset[][] table above.

	if (++self->count == 8)
		self->count = 0;

	// Find out which bolts still live

	if (self->locked_to->state == 0)	// left bolt?
	{
		left_bolt = self->locked_to;

		if (left_bolt->locked_to == self)
			right_bolt = NULL;
		else
			right_bolt = left_bolt->locked_to;
	}
	else
	{
		right_bolt = self->locked_to;
		left_bolt = NULL;
	}

	// pos1, pos2 and pos3 contain the forward, right and up components of the flight direction.
	// move bolts to their new positions.

	if (left_bolt)
	{
		R_ProjectSource(self->s.origin,shot_radius_offset[self->count][0],self->pos1,self->pos2,self->pos3,left_bolt->s.origin);
		gi.linkentity(left_bolt);
	}

	if (right_bolt)
	{
		R_ProjectSource(self->s.origin,shot_radius_offset[self->count][1],self->pos1,self->pos2,self->pos3,right_bolt->s.origin);
		gi.linkentity(right_bolt);
	}

	self->nextthink = level.time + FRAMETIME;
}


void fire_turret_shots(edict_t *self,vec3_t center,vec3_t left,vec3_t right,vec3_t dir)
{
	edict_t *shot_control;
	edict_t *left_bolt,*right_bolt;
	int		damage;

	damage = TURRET_DAMAGE;

	if (self->monsterinfo.quad_framenum > level.framenum)
		damage *= 4;

	// Spawn the control entity
	shot_control = G_Spawn();
	VectorCopy(center,shot_control->s.origin);
	VectorCopy(center,shot_control->s.old_origin);
	vectoangles(dir,shot_control->s.angles);

	// Set the forward, right and up components of the flight direction
	AngleVectors(shot_control->s.angles,shot_control->pos1,shot_control->pos2,shot_control->pos3);
	VectorNormalize(dir);
	VectorScale(dir,TURRET_BLAST_SPEED,shot_control->velocity);
	shot_control->movetype = MOVETYPE_FLYMISSILE;
	shot_control->svflags = SVF_DEADMONSTER;
	shot_control->solid = SOLID_NOT;
	VectorClear(shot_control->mins);
	VectorClear(shot_control->maxs);
	shot_control->s.modelindex = gi.modelindex ("sprites/beacon_0.sp2");
	shot_control->classname = "shot_control";

	// Spawn the two blaster bolts
	left_bolt  = fire_blaster(self,left,dir,damage,TURRET_BLAST_SPEED,EF_BFG,false);
	left_bolt->state = 0;	// mark as left bolt
	right_bolt = fire_blaster(self,right,dir,damage,TURRET_BLAST_SPEED,EF_BFG,false);
	right_bolt->state = 1;	// mark as right bolt

	// Link everyone together. Given one, you can find the others.
	left_bolt->locked_to = right_bolt;
	right_bolt->locked_to = shot_control;
	shot_control->locked_to = left_bolt;

	// Initialize "count" which controls the position of the bolts around the control
	shot_control->count = 6;	// left bolt spawned on left side of ring (facing out)
								// and right bolt spawned on right side of ring

	// Set up thinking
	shot_control->think = turret_shot_think;
	shot_control->nextthink = level.time + FRAMETIME;
}


void turret_s_think(edict_t *self)
{
	vec3_t	fire_dir,start_control,start_left,start_right;
	vec3_t	enemy_spot;

	self->nextthink = level.time + 10*FRAMETIME;

	if (random() < 0.25)	// Sometimes we don't fire
		return;

	// Don't shoot if you can't see the enemy

	self->enemy = TurretFindEnemy(self);

	if (!self->enemy)
		return;

	// Set current firing position offset
	// Use more accurate R_ProjectSource()
	// pos1, pos2, and pos3 hold the forward, right, and up offsets determined at spawn time
	// Find the start points for the control sprite, and the left and right blaster bolts

	R_ProjectSource(self->s.origin,tv(SHOT_FORWARD,0,0),self->pos1,self->pos2,self->pos3,start_control);
	R_ProjectSource(self->s.origin,tv(SHOT_FORWARD,SHOT_RADIUS,0),self->pos1,self->pos2,self->pos3,start_right);
	R_ProjectSource(self->s.origin,tv(SHOT_FORWARD,-SHOT_RADIUS,0),self->pos1,self->pos2,self->pos3,start_left);

	// Get vector to target
	// Vary the target spot slightly for each shot.

	Pick_Client_Target(self,enemy_spot);			// RSP 052699 - generalized routine
	VectorSubtract(enemy_spot,start_control,fire_dir);

	// Now the shots ...
	fire_turret_shots(self,start_control,start_left,start_right,fire_dir);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (MZ_BLASTER);
	gi.multicast (start_control, MULTICAST_PVS);
}	


void turret_pain(edict_t *self ,edict_t *other, float kick, int damage)
{
	// No animations or special handling
}


void turret_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->s.skinnum = TURRET_DEATH_SKIN;
	self->s.renderfx &= ~RF_GLOW;				// RSP 081700

	// RSP 062900 - use generic explosion routine
	SpawnExplosion(self,TE_EXPLOSION1,MULTICAST_PVS,EXPLODE_TURRET);
	self->nextthink = 0;			// Stop thinking
	self->takedamage = DAMAGE_NO;	// RSP 080500 - prevents further explosions when shot
}


/*QUAKED monster_turret_static (1 .5 0) (-16 -16 -16) (16 16 16) Ambush Trigger_Spawn Sight Pacifist

The turret will travel in the direction of "angle" until it hits a surface.
That's where it will plant itself.

Turn a turret on and off by targetting it. To start "off", set the Pacifist flag.
*/

void SP_monster_turret_static (edict_t *self)
{
	// When the turret is spawned, move in the direction of the angles
	// until you hit something.

	trace_t	t;
	vec3_t	start,move_fwd;

	G_SetMovedir (self->s.angles, move_fwd);	// RSP 051599 - handles UP and DOWN

	VectorMA(self->s.origin,2047.0f,move_fwd,move_fwd);

	VectorCopy(self->s.origin,start);
	
	t = gi.trace(start, NULL, NULL, move_fwd, self, MASK_SOLID);
	if ((t.fraction == 1) || (t.fraction == 0))
	{
		gi.dprintf("Turret at %s didn't find proper anchor\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	// t.endpos == center of my anchor spot.  Set up the ring, and ...

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->svflags = SVF_MONSTER;
	self->flags |= (FL_NO_KNOCKBACK|FL_SPARKDAMAGE);	// RSP 051899
	self->identity = ID_TURRET;
	self->health = hp_table[HP_TURRET];		// RSP 032399
	self->classname = "turret";				// RSP 051599
	self->takedamage = DAMAGE_YES;			// RSP 051599
	self->mass = 25;						// RSP 053099
	
	VectorClear(self->velocity);
	VectorClear(self->avelocity);

	// Store base angles in an appropriate manner
	_SetBaseAnglesFromPlaneNormal(t.plane.normal, self);

	VectorSet (self->mins, -16, -16, -16);
	VectorSet (self->maxs, 16, 16, 16);
	VectorCopy(t.endpos,self->s.origin);
	VectorCopy(self->pos1,self->s.angles);

	// Set firing vectors, offset from ring center and forward
	vectoangles(self->pos2,self->pos1);
	AngleVectors(self->pos1,self->pos1,self->pos2,self->pos3);

	self->s.modelindex = gi.modelindex ("models/monsters/turret_s/base.md2");

	self->think = turret_s_think;

	// Turrets should not all start thinking on the same frame, otherwise you
	// will get synchronized firing when more than one can see you simultaneously

	self->nextthink = level.time + FRAMETIME*(1 + ((int) (random()*10)));

	self->die = turret_die;
	self->pain = turret_pain;
	self->use = turret_use;	// RSP 052299 - to toggle turret on/off
}
