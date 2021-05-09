// g_misc.c

#include "g_local.h"


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void Use_Areaportal (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->count ^= 1;		// toggle state
	gi.SetAreaPortalState (ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal (edict_t *ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0;		// always start closed;
}

//=====================================================


/*
=================
Misc functions
=================
*/
void VelocityForDamage (int damage, vec3_t v)
{
	v[0] = 100.0 * crandom();
	v[1] = 100.0 * crandom();
//	v[2] = 200.0 + 100.0 * random();
	v[2] = 1000.0 * random();	// RSP 061900 - more variability in z velocity

	if (damage < 50)
		VectorScale (v, 0.7, v);
	else 
		VectorScale (v, 1.2, v);
}

void ClipGibVelocity (edict_t *ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200;	// always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}


/*
=================
gibs
=================
*/

/* RSP 073000 - not used

void gib_think (edict_t *self)
{
	self->s.frame++;
	self->nextthink = level.time + FRAMETIME;

	if (self->s.frame == 10)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 8 + random()*10;
	}
}
 */

// RSP 061900 - This didn't work in Q2. Fixed it in LUC
void gib_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
//	vec3_t	normal_angles, right;	// RSP 061900

//	if (!self->groundentity)		// RSP 061900
//		return;						// RSP 061900

	self->touch = NULL;

	if (plane)
	{
		// RSP 071800 - no sound underwater
		if (!(gi.pointcontents(self->s.origin) & MASK_WATER))
			// RSP 061900 - changed ATTN_NORM to ATTN_IDLE so the sound isn't loud from far away
			gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/fhit3.wav"), 1, ATTN_IDLE, 0);

//		vectoangles (plane->normal, normal_angles);			// RSP 061900
//		AngleVectors (normal_angles, NULL, right, NULL);	// RSP 061900
//		vectoangles (right, self->s.angles);				// RSP 061900

/* RSP 073000 - sm_meat only has 1 frame, so none of this is needed

		if (self->s.modelindex == sm_meat_index)
		{
			self->s.frame++;	// RSP 073000 - model only has 1 frame
			self->think = gib_think;
			self->nextthink = level.time + FRAMETIME;
		}
 */
	}
}

void gib_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}

void ThrowGib (edict_t *self, char *gibname, int damage, int type)
{
	edict_t *gib;
	vec3_t	vd;
	vec3_t	origin;
	vec3_t	size;
	float	vscale;

	gib = G_Spawn();

	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel (gib, gibname);
	gib->solid = SOLID_BBOX;	// RSP 061900 - this was SOLID_NOT, and as such would never
								// get to gib_touch
	gib->s.effects |= EF_GIB;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;
	gib->watertype = gi.pointcontents(gib->s.origin);	// RSP 071800

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_TOSS;
		gib->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

//	VelocityForDamage(damage,vd);
	VelocityForDamage(2*damage,vd);	// RSP 061900 - more variability in gib throwing
	VectorMA(self->velocity,vscale,vd,gib->velocity);
	ClipGibVelocity(gib);
	gib->avelocity[0] = random()*600;
	gib->avelocity[1] = random()*600;
	gib->avelocity[2] = random()*600;

	gib->think = G_FreeEdict;
	gib->nextthink = level.time + 10 + random()*10;

	// RSP 052599 - Add color shell if it was present on the parent who died

	if (self->s.renderfx & RF_GLOW)
	{
		gib->s.renderfx |= RF_GLOW;
		gib->s.renderfx |= RF_TRANSLUCENT;
	}

	gi.linkentity (gib);
}

void ThrowHead (edict_t *self, char *gibname, int damage, int type)
{
	vec3_t	vd;
	float	vscale;

	self->s.skinnum = 0;
	self->s.frame = 0;
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;	// RSP 091700 - stop incrementing frames
	VectorClear (self->mins);
	VectorClear (self->maxs);

	self->s.modelindex2 = 0;
	gi.setmodel (self, gibname);
	self->model = gibname;	// RSP 062099
	self->solid = SOLID_NOT;
	self->s.effects |= EF_GIB;
	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;
	self->flags &= ~FL_FROZEN;	// RSP 062099 - thaw out if killed by matrix
	self->svflags &= ~SVF_MONSTER;
	self->takedamage = DAMAGE_YES;
	self->die = gib_die;
	self->s.effects &= ~EF_QUAD;

	if (type == GIB_ORGANIC)
	{
		self->movetype = MOVETYPE_TOSS;
		self->touch = gib_touch;
		vscale = 0.5;
	}
	else
	{
		self->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage (damage, vd);
	VectorMA (self->velocity, vscale, vd, self->velocity);
	ClipGibVelocity (self);

	self->avelocity[YAW] = crandom()*600;

	self->think = G_FreeEdict;
	self->nextthink = level.time + 10 + random()*10;

	gi.linkentity (self);
}


void ThrowClientHead (edict_t *self, int damage)
{
	vec3_t	vd;
	char	*gibname;

	if (rand()&1)
	{
		gibname = "models/objects/gibs/head2/tris.md2";
		self->s.skinnum = 1;		// second skin is player
	}
	else
	{
		gibname = "models/objects/gibs/skull/tris.md2";
		self->s.skinnum = 0;
	}

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel (self, gibname);
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 16);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	self->movetype = MOVETYPE_BOUNCE;
	VelocityForDamage (damage, vd);
	VectorAdd (self->velocity, vd, self->velocity);

	if (self->client)	// bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else // 3.20
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity (self);
}


/*
=================
debris
=================
*/
void debris_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}

void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin)
{
	edict_t	*chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy (origin, chunk->s.origin);
	gi.setmodel (chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA (self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random()*600;
	chunk->avelocity[1] = random()*600;
	chunk->avelocity[2] = random()*600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random()*5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;
	gi.linkentity (chunk);
}


// RSP 062900 - Generic explosion routine
//              You can vary the explosion type and sound
void SpawnExplosion(edict_t *self,int explosion_type,int multicast_type,int sound)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(explosion_type);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin,multicast_type);

	switch (sound)
	{
	case EXPLODE_SILENT:
	default:
		break;

	case EXPLODE_LITTLEBOY:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explode2.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_LITTLEBOY_WATER:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explode3.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_DLOCK:
	case EXPLODE_GRENADE:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/Small_Explosion.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_GRENADE_WATER:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explode4.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_GBOT:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/Medium_Explosion.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_FATMAN:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/Big_Explosion.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_DECO:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explode6.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_ROCKETSPEAR:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explode.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_ROCKETSPEAR_WATER:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explode5.wav"),1,ATTN_NORM,0);
		break;

	case EXPLODE_TURRET:
		gi.sound(self,CHAN_WEAPON,gi.soundindex("lucworld/explosion.wav"),1,ATTN_NORM,0);
		break;
	}
}

void SpawnExplosion1(edict_t *self,int sound)
{
	// MRB 032799 This should mean a door is trying to destroy a spear
	if ((Q_stricmp(self->classname,"item_normal_spear") == 0) ||
		(Q_stricmp(self->classname,"spear") == 0))
	{
		gi.sound(self,CHAN_WEAPON,gi.soundindex("weapons/speargun/spearbrk.wav"),1,ATTN_NORM,0);
		G_FreeEdict(self);
		return;
	}

	// MRB 032799 No explosion for plasma
	if (Q_stricmp(self->classname,"plasma") == 0)
	{
		G_FreeEdict(self);
		return;
	}

	// Everything else explodes

	// RSP 062900 - call generic routine
//	gi.WriteByte (svc_temp_entity);
//	gi.WriteByte (TE_EXPLOSION1);
//	gi.WritePosition (self->s.origin);
//	gi.multicast (self->s.origin, MULTICAST_PVS);

	SpawnExplosion(self,TE_EXPLOSION1,MULTICAST_PVS,sound);
}


void SpawnExplosion2 (edict_t *self, int sound)
{	// RSP 062900 - call generic routine
//	gi.WriteByte (svc_temp_entity);
//	gi.WriteByte (TE_EXPLOSION2);
//	gi.WritePosition (self->s.origin);
//	gi.multicast (self->s.origin, MULTICAST_PVS);

	SpawnExplosion(self,TE_EXPLOSION2,MULTICAST_PVS,sound);
}


// RSP 071699 - Added ability to rotate func_trains.

/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT X_AXIS Y_AXIS REVERSE

Keys:

"target"	next path corner
"pathtarget" gets used when an entity that has
	this path_corner targeted touches it

Spawnflags:

"teleport"		teleport to target

Path_corners carry more information when they're used with func_rotrains.

A func_rotrain can behave 5 different ways:
1) straight translation, like a regular func_train
2) rotate at a constant speed while translating
3) rotate through a specified arc while translating
4) through a specified arc at a speed that completes the arc at the next path_corner
5) rotate through a specified arc w/o translating, then translate

In all cases, rotation will stop when the next path_corner is reached, and new
rotation values--if any--will be read from that path_corner.

The func_rotrain has to include an origin brush about which rotation will occur.

Rotation won't be applied to monsters on path_corners. 

Keys:

"style" 1-5 as described above
"speed"	angular rotation (default 10)
"angle" arc to travel through 

Spawnflags:

"x_axis"		rotate about x axis (default is z)
"y_axis"		rotate about y axis (default is z)
"reverse"		rotate in the opposite direction.

*/

void path_corner_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	v;
	edict_t	*next;

	if (other->movetarget != self)
		return;
	
	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets (self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = G_PickTarget(self->target);
	else
		next = NULL;

	if (next && (next->spawnflags & SPAWNFLAG_PC_TELEPORT))
	{
		VectorCopy (next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy (v, other->s.origin);
		next = G_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;	// 3.20
	}

	other->goalentity = other->movetarget = next;

	if (self->wait)
	{
		other->monsterinfo.pausetime = level.time + self->wait;
		other->monsterinfo.stand (other);
		return;
	}

	if (!other->movetarget)
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.stand (other);
	}
	else
	{
		VectorSubtract (other->goalentity->s.origin, other->s.origin, v);
		other->ideal_yaw = vectoyaw (v);
	}
}

void SP_path_corner (edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);

	// RSP 071699 - set the default rotation speed for use with func_rotrains
	switch(self->style)
	{
	case PCSTYLE_TRANS:
	case PCSTYLE_TRANS_ROT_NEXTPATH:
		break;
	case PCSTYLE_TRANS_ROT:
	case PCSTYLE_TRANS_ROT_ARC:
	case PCSTYLE_ROT_ARC_TRANS:
		if (!self->speed)
			self->speed = 10;
		break;
	default:
		break;
	}

	self->svflags |= SVF_NOCLIENT;
	gi.linkentity (self);
}


/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
void point_combat_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t	*activator;

	if (other->movetarget != self)
		return;

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = G_PickTarget(other->target);
		if (!other->goalentity)
		{
			gi.dprintf("%s at %s target %s does not exist\n", self->classname, vtos(self->s.origin), self->target);
			other->movetarget = self;
		}
		self->target = NULL;
	}
	else if ((self->spawnflags & SPAWNFLAG_CP_HOld) && !(other->flags & (FL_SWIM|FL_FLY)))
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand (other);
	}

	if (other->movetarget == self)
	{
		other->target = NULL;
		other->movetarget = NULL;
		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		if (other->enemy && other->enemy->client)
			activator = other->enemy;
		else if (other->oldenemy && other->oldenemy->client)
			activator = other->oldenemy;
		else if (other->activator && other->activator->client)
			activator = other->activator;
		else
			activator = other;
		G_UseTargets (self, activator);
		self->target = savetarget;
	}
}

void SP_point_combat (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	VectorSet (self->mins, -8, -8, -16);
	VectorSet (self->maxs, 8, 8, 16);
	self->svflags = SVF_NOCLIENT;
	gi.linkentity (self);
};

/* RSP 081400 - removed from LUC
/QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)
Just for the debugging level.  Don't use

void TH_viewthing(edict_t *ent)
{
	ent->s.frame = (ent->s.frame + 1) % 7;
	ent->nextthink = level.time + FRAMETIME;
}

void SP_viewthing(edict_t *ent)
{
	gi.dprintf ("viewthing spawned\n");

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->s.renderfx = RF_FRAMELERP;
	VectorSet (ent->mins, -16, -16, -24);
	VectorSet (ent->maxs, 16, 16, 32);
	ent->s.modelindex = gi.modelindex ("models/objects/banner/tris.md2");
	gi.linkentity (ent);
	ent->nextthink = level.time + 0.5;
	ent->think = TH_viewthing;
	return;
}
 */

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null (edict_t *self)
{
	G_FreeEdict (self);
};


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull (edict_t *self)
{
	VectorCopy (self->s.origin, self->absmin);
	VectorCopy (self->s.origin, self->absmax);
};


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

static void light_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & SPAWNFLAG_LT_START_OFF)		// RSP 082400
	{
		gi.configstring (CS_LIGHTS+self->style, "m");
		self->spawnflags &= ~SPAWNFLAG_LT_START_OFF;	// RSP 082400
	}
	else
	{
		gi.configstring (CS_LIGHTS+self->style, "a");
		self->spawnflags |= SPAWNFLAG_LT_START_OFF;	// RSP 082400
	}
}

void SP_light (edict_t *self)
{
	// no targeted lights in deathmatch, because they cause global messages
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & SPAWNFLAG_LT_START_OFF)	// RSP 082400
			gi.configstring (CS_LIGHTS+self->style, "a");
		else
			gi.configstring (CS_LIGHTS+self->style, "m");
	}
}


/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

void func_wall_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		KillBox (self);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity (self);

	if (!(self->spawnflags & SPAWNFLAG_FWALL_TOGGLE))
		self->use = NULL;
}

void SP_func_wall (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);

	if (self->spawnflags & SPAWNFLAG_FWALL_ANIMATED)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & SPAWNFLAG_FWALL_ANIMATED_FAST)
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self->spawnflags & (SPAWNFLAG_FWALL_TRIGGER_SPAWN|SPAWNFLAG_FWALL_TOGGLE|SPAWNFLAG_FWALL_START_ON)) == 0)
	{
		self->solid = SOLID_BSP;
		gi.linkentity (self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & SPAWNFLAG_FWALL_TRIGGER_SPAWN))
		self->spawnflags |= SPAWNFLAG_FWALL_TRIGGER_SPAWN;

	// yell if the spawnflags are odd
	if (self->spawnflags & SPAWNFLAG_FWALL_START_ON)
	{
		if (!(self->spawnflags & SPAWNFLAG_FWALL_TOGGLE))
		{
			gi.dprintf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= SPAWNFLAG_FWALL_TOGGLE;
		}
	}

	self->use = func_wall_use;
	if (self->spawnflags & SPAWNFLAG_FWALL_START_ON)
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity (self);
}


/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

void func_object_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// only squash thing we fall on top of
	if (!plane)
		return;
	if (plane->normal[2] < 1.0)
		return;
	if (other->takedamage == DAMAGE_NO)
		return;
	T_Damage (other, self, self, vec3_origin, self->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void func_object_release (edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void func_object_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox (self);
	func_object_release (self);
}

void SP_func_object (edict_t *self)
{
	gi.setmodel (self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0)
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 2 * FRAMETIME;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & SPAWNFLAG_FOBJ_ANIMATED)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & SPAWNFLAG_FOBJ_ANIMATED_FAST)
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;

	gi.linkentity (self);
}


/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST
Any brush that you want to explode or break apart.  If you want an
explosion, set dmg and it will do a radius explosion of that amount
at the center of the brush.

If targeted it will not be shootable.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
void func_explosive_explode (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	vec3_t	origin;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	// bmodel origins are (0 0 0), we need to adjust that here
	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	VectorCopy (origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
		T_RadiusDamage (self, attacker, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE);

	VectorSubtract (self->s.origin, inflictor->s.origin, self->velocity);
	VectorNormalize (self->velocity);
	VectorScale (self->velocity, 150, self->velocity);

	// start chunks towards the center
	VectorScale (size, 0.5, size);

	mass = self->mass;
	if (!mass)
		mass = 75;

	// big chunks
	if (mass >= 100)
	{
		count = mass / 100;
		if (count > 8)
			count = 8;
		while (count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris (self, "models/objects/debris1/tris.md2", 1, chunkorigin);
		}
	}

	// small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;
	while (count--)
	{
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.md2", 2, chunkorigin);
	}

	G_UseTargets (self, attacker);

	if (self->dmg)
        {
			if (mass >= 100)
				SpawnExplosion1(self,EXPLODE_BIG);      // RSP 062900
			else
				SpawnExplosion1(self,EXPLODE_SMALL);    // RSP 062900
        }

	G_FreeEdict(self);
}

void func_explosive_use(edict_t *self, edict_t *other, edict_t *activator)
{
	func_explosive_explode (self, self, other, self->health, vec3_origin);
}

void func_explosive_spawn (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox (self);
	gi.linkentity (self);
}

void SP_func_explosive (edict_t *self)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (self);
		return;
	}

	self->movetype = MOVETYPE_PUSH;

	gi.modelindex ("models/objects/debris1/tris.md2");
	gi.modelindex ("models/objects/debris2/tris.md2");

	gi.setmodel (self, self->model);

	if (self->spawnflags & SPAWNFLAG_FEXPL_TRIGGER_SPAWN)
	{
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	else
	{
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags & SPAWNFLAG_FEXPL_ANIMATED)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & SPAWNFLAG_FEXPL_ANIMATED_FAST)
		self->s.effects |= EF_ANIM_ALLFAST;

	if (self->use != func_explosive_use)
	{
		if (!self->health)
			self->health = 100;
		self->die = func_explosive_explode;
		self->takedamage = DAMAGE_YES;
	}

	gi.linkentity (self);
}


//
// miscellaneous specialty items
//

// 3.20 (down to light_mine1)


/*QUAKED target_character (0 0 1) ?
used with target_string (must be on same "team")
"count" is position in the string (starts at 1)
*/

void SP_target_character (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);
	self->solid = SOLID_BSP;
	self->s.frame = 12;
	gi.linkentity (self);
	return;
}


/*QUAKED target_string (0 0 1) (-8 -8 -8) (8 8 8)
*/

void target_string_use (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t *e;
	int		n, l;
	char	c;

	l = strlen(self->message);
	for (e = self->teammaster; e; e = e->teamchain)
	{
		if (!e->count)
			continue;
		n = e->count - 1;
		if (n > l)
		{
			e->s.frame = 12;
			continue;
		}

		c = self->message[n];
		if ((c >= '0') && (c <= '9'))
			e->s.frame = c - '0';
		else if (c == '-')
			e->s.frame = 10;
		else if (c == ':')
			e->s.frame = 11;
		else
			e->s.frame = 12;
	}
}

void SP_target_string (edict_t *self)
{
	if (!self->message)
		self->message = "";
	self->use = target_string_use;
}


/*QUAKED func_clock (0 0 1) (-8 -8 -8) (8 8 8) TIMER_UP TIMER_DOWN START_OFF MULTI_USE
target a target_string with this

The default is to be a time of day clock

TIMER_UP and TIMER_DOWN run for "count" seconds and the fire "pathtarget"
If START_OFF, this entity must be used before it starts

"style"		0 "xx"
			1 "xx:xx"
			2 "xx:xx:xx"
*/

#define	CLOCK_MESSAGE_SIZE	16

// don't let field width of any clock messages change, or it
// could cause an overwrite after a game load

static void func_clock_reset (edict_t *self)
{
	self->activator = NULL;
	if (self->spawnflags & SPAWNFLAG_FCLK_TIMER_UP)
	{
		self->health = 0;
		self->wait = self->count;
	}
	else if (self->spawnflags & SPAWNFLAG_FCLK_TIMER_DOWN)
	{
		self->health = self->count;
		self->wait = 0;
	}
}

static void func_clock_format_countdown (edict_t *self)
{
	if (self->style == 0)
	{
		Com_sprintf (self->message, CLOCK_MESSAGE_SIZE, "%2i", self->health);
		return;
	}

	if (self->style == 1)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i", self->health / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		return;
	}

	if (self->style == 2)
	{
		Com_sprintf(self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", self->health / 3600, (self->health - (self->health / 3600) * 3600) / 60, self->health % 60);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
		return;
	}
}

void func_clock_think (edict_t *self)
{
	if (!self->enemy)
	{
		self->enemy = G_Find (NULL, FOFS(targetname), self->target);
		if (!self->enemy)
			return;
	}

	if (self->spawnflags & SPAWNFLAG_FCLK_TIMER_UP)
	{
		func_clock_format_countdown (self);
		self->health++;
	}
	else if (self->spawnflags & SPAWNFLAG_FCLK_TIMER_DOWN)
	{
		func_clock_format_countdown (self);
		self->health--;
	}
	else
	{
		struct tm	*ltime;
		time_t		gmtime;

		time(&gmtime);
		ltime = localtime(&gmtime);
		Com_sprintf (self->message, CLOCK_MESSAGE_SIZE, "%2i:%2i:%2i", ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		if (self->message[3] == ' ')
			self->message[3] = '0';
		if (self->message[6] == ' ')
			self->message[6] = '0';
	}

	self->enemy->message = self->message;
	self->enemy->use (self->enemy, self, self);

	if (((self->spawnflags & SPAWNFLAG_FCLK_TIMER_UP) && (self->health > self->wait)) ||
		((self->spawnflags & SPAWNFLAG_FCLK_TIMER_DOWN) && (self->health < self->wait)))
	{
		if (self->pathtarget)
		{
			char *savetarget;
			char *savemessage;

			savetarget = self->target;
			savemessage = self->message;
			self->target = self->pathtarget;
			self->message = NULL;
			G_UseTargets (self, self->activator);
			self->target = savetarget;
			self->message = savemessage;
		}

		if (!(self->spawnflags & SPAWNFLAG_FCLK_MULTI_USE))
			return;

		func_clock_reset (self);

		if (self->spawnflags & SPAWNFLAG_FCLK_START_OFF)
			return;
	}

	self->nextthink = level.time + 1;
}

void func_clock_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!(self->spawnflags & SPAWNFLAG_FCLK_MULTI_USE))
		self->use = NULL;
	if (self->activator)
		return;
	self->activator = activator;
	self->think (self);
}

void SP_func_clock (edict_t *self)
{
	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	if ((self->spawnflags & SPAWNFLAG_FCLK_TIMER_DOWN) && (!self->count))
	{
		gi.dprintf("%s with no count at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	if ((self->spawnflags & SPAWNFLAG_FCLK_TIMER_UP) && (!self->count))
		self->count = 60*60;

	func_clock_reset (self);

	self->message = gi.TagMalloc (CLOCK_MESSAGE_SIZE, TAG_LEVEL);

	self->think = func_clock_think;

	if (self->spawnflags & SPAWNFLAG_FCLK_START_OFF)
		self->use = func_clock_use;
	else
		self->nextthink = level.time + 1;
}

//=================================================================================

void teleporter_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t		*dest;
	int			i;

	if (!other->client)
		return;
	dest = G_Find (NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf ("Couldn't find destination\n");
		return;
	}

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity (other);

	VectorCopy (dest->s.origin, other->s.origin);
	VectorCopy (dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	//JBM 081298 if the Maintain Motion spawnflag is not set, clear the velocity
	if (!(self->owner->spawnflags & SPAWNFLAG_MTEL_MAINTAIN_MOTION))
	{
		// clear the velocity and hold them in place briefly
		VectorClear (other->velocity);
	}

	// JBM 081298 If the No Pause spawnflag is not set, use the pause
	if (!(self->owner->spawnflags & SPAWNFLAG_MTEL_NO_PAUSE))
		other->client->ps.pmove.pm_time = 160>>3;		// hold time
	else
		other->client->ps.pmove.pm_time = 160>>6;		// hold time

	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	//JBM 081298 if the No Flash spawnflag is not set, use the flash effect
	if (!(self->owner->spawnflags & SPAWNFLAG_MTEL_NO_FLASH))
	{
		// draw the teleport splash at source and on the player
		self->owner->s.event = EV_PLAYER_TELEPORT;
		other->s.event = EV_PLAYER_TELEPORT;
	}

	// JBM 0812 if the Maintain Motion spawnflag is not set, adjust the angles (?)
	if (!(self->owner->spawnflags & SPAWNFLAG_MTEL_MAINTAIN_MOTION))
	{
		// set angles
		for (i = 0 ; i < 3 ; i++)
			other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);
	}
		
	// JBM 0812 if the Maintain Motion spawnflag is not set, adjust the player's view
	if (!(self->owner->spawnflags & SPAWNFLAG_MTEL_MAINTAIN_MOTION))
	{
		VectorClear (other->s.angles);
		VectorClear (other->client->ps.viewangles);
		VectorClear (other->client->v_angle);
	}

	// kill anything at the destination
	KillBox (other);

	gi.linkentity (other);
}

// WPO 042800 removed misc_teleporter reference
/* misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16) NO_FOUNTAIN_FX NO_SOUND NO_PAUSE NO_FLASH MAINTAIN_MOTION
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter (edict_t *ent)
{
	edict_t *trig;

	/* JBM 081298 Added some spawnflags for the teleporter :
		1 = No Fountain Effect
		2 = No Sound
		4 = No Pause before teleport
		8 = No teleport Flash when teleporting
	   16 = Maintain Motion
	*/

	if (!ent->target)
	{
		gi.dprintf ("teleporter without a target.\n");
		G_FreeEdict (ent);
		return;
	}

	gi.setmodel (ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 1;

	
	// JBM 081298 If the No Fountain FX spawnflag is not set, use the effect
	if (!(ent->spawnflags & SPAWNFLAG_MTEL_NO_FOUNTAIN_FX))
	{
		ent->s.effects = EF_TELEPORTER;
	}

/* RSP 081800 - not used
	// JBM 081298 If the No Sound spawnflag is not set, use sound
	if (!(ent->spawnflags & SPAWNFLAG_MTEL_NO_SOUND))
	{
		// WPO 042800 removed this entity
//		ent->s.sound = gi.soundindex ("world/amb10.wav");
	}
 */

	ent->solid = SOLID_BBOX;

	VectorSet (ent->mins, -32, -32, -24);
	VectorSet (ent->maxs, 32, 32, -16);
	gi.linkentity (ent);

	trig = G_Spawn ();
	
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	VectorCopy (ent->s.origin, trig->s.origin);
	VectorSet (trig->mins, -8, -8, 8);
	VectorSet (trig->maxs, 8, 8, 24);
	gi.linkentity (trig);
}

// RSP 063099 - removed this from the editor's hands...
// WPO 042800 removed misc_teleporter reference
//misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
/* Point teleporters at these.
*/
void SP_misc_teleporter_dest (edict_t *ent)
{
	// RSP 063099 - Remove spawn spot model. This is used at deathmatch starts,
	// and we're not supporting it. I assume it's not needed as a general
	// teleporter destination, since it was using a model that we don't have.
/*
	gi.setmodel (ent, "models/objects/dmspot/tris.md2");
	ent->s.skinnum = 0;
	ent->solid = SOLID_BBOX;
	ent->s.effects |= EF_FLIES;
	VectorSet (ent->mins, -32, -32, -24);
	VectorSet (ent->maxs, 32, 32, -16);
 */

	gi.linkentity (ent);
}

