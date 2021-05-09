#include "g_local.h"


/*
=================
fire_bomb
=================
*/

void littleboy_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	origin;
	int     n;

	if (other == ent->owner)
		return;

	if (other && (Q_stricmp(other->classname,"matrix blast") == 0))
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		// remove beacon if there
		if (ent->target_ent)
			G_FreeEdict(ent->target_ent);
		G_FreeEdict(ent);
		return;
	}

	// calculate position for the explosion entity

	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	if (other && other->takedamage)
		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, ent->dmg, 10, 0, MOD_UNKNOWN);
	else    // don't throw any debris in net games
		if (!deathmatch->value && !coop->value)
			if ((surf) && !(surf->flags & (SURF_WARP|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING)))
			{
				n = rand() % 5;
				while (n--)
					ThrowDebris (ent, "models/objects/debris2/tris.md2", 2, ent->s.origin);
			}

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_R_SPLASH);

//	RSP 062900 - move explosion further on
//	gi.WriteByte (svc_temp_entity);
//	if (ent->waterlevel)
//		gi.WriteByte (TE_ROCKET_EXPLOSION_WATER);
//	else
//		gi.WriteByte (TE_ROCKET_EXPLOSION);
//	gi.WritePosition (origin);
//	gi.multicast (ent->s.origin, MULTICAST_PHS);

	// remove beacon if there

	if (ent->target_ent)
		G_FreeEdict(ent->target_ent);

	// RSP 062900 - Use generic explosion routine
	if (ent->waterlevel)
		SpawnExplosion(ent,TE_ROCKET_EXPLOSION_WATER,MULTICAST_PHS,EXPLODE_LITTLEBOY_WATER);
	else
		SpawnExplosion(ent,TE_ROCKET_EXPLOSION,MULTICAST_PHS,EXPLODE_LITTLEBOY);

	G_FreeEdict(ent);

	// If the other entity is a projectile, remove it in the explosion

	if (other && Is_Projectile(other))
		G_FreeEdict(other);
}

#define BOMB_SPEED 1000

void spawn_beacon(edict_t *owner)
{
	edict_t	*beacon;

	beacon = G_Spawn();
	beacon->svflags = SVF_DEADMONSTER;	// 3.20
	beacon->s.effects |= EF_PLASMA;
	beacon->s.renderfx = RF_TRANSLUCENT;
	beacon->solid = SOLID_NOT;
	VectorClear (beacon->mins);
	VectorClear (beacon->maxs);
	beacon->s.modelindex = gi.modelindex ("sprites/beacon_0.sp2");
	beacon->owner = owner;
	beacon->classname = "beacon";
	owner->target_ent = beacon;
}

// RSP 050999 - look around for player
void bomb_seektarget(edict_t *self)
{
	edict_t *target = level.sight_client;
	vec3_t v,a,forward,end;
	float adiff;
	trace_t tr;
	qboolean berocket = false;

	// Littleboy now has a beacon. Spawn a temporary light entity

	if (!self->target_ent)
		spawn_beacon(self);
	AngleVectors (self->s.angles, forward, NULL, NULL);	// forward direction
	VectorNormalize(forward);
	VectorMA(self->s.origin,8000,forward,end);			// look for something to shine on
	tr = gi.trace (self->s.origin,NULL,NULL,end,self,MASK_SOLID);
	VectorMA (tr.endpos, -8, forward, end); // Back up
	VectorCopy(end,self->target_ent->s.origin);
	gi.linkentity(self->target_ent);

	// See if the player's nearby

	if (target && target->client && !(target->svflags & SVF_NOCLIENT))	// RSP 110799 - SVF_NOCLIENT for cloaking
	{
		if (target->decoy)	// RSP 110399 - see if there's a decoy
			target = target->decoy;
		VectorSubtract(target->s.origin,self->s.origin,v);
		vectoangles(v,a);
		adiff = self->s.angles[YAW] - a[YAW];
		if (fabs(adiff) < 10 || fabs(adiff) > 350)
		{
			// If there's a clear path, become a rocket

			VectorCopy (target->s.origin, end);
			end[2] += target->viewheight;
			tr = gi.trace (self->s.origin,self->mins,self->maxs,end,target,MASK_SOLID);
			if (tr.fraction == 1)
				berocket = true;
			else
			{	// Some of the time, if you can see the target, become a rocket even
				// if you don't have a clear path. Maybe you can catch him with some
				// blast damage.
				if (rand()&1)	// Check 50% of the time
				{
					tr = gi.trace (self->s.origin,NULL,NULL,end,target,MASK_SOLID);
					if (tr.fraction == 1)
						berocket = true;
				}
			}
			if (berocket)
			{	// All clear, bombs away!
				VectorSubtract (end, self->s.origin, forward);
				VectorNormalize(forward);
				VectorCopy (forward, self->movedir);
				VectorScale (forward, BOMB_SPEED, self->velocity);
				self->movetype = MOVETYPE_FLYMISSILE;
				self->s.effects = EF_ROCKET;
				_AlignDirVelocity(self);
				self->nextthink = level.time + 8000/BOMB_SPEED;
				self->think = G_FreeEdict;
				self->s.sound = gi.soundindex("weapons/rockfly.wav");	// RSP 063000

				// remove beacon if there
				if (self->target_ent)
				{
					G_FreeEdict(self->target_ent);
					self->target_ent = NULL;
				}
				return;
			}
		}
	}

	// keep rotating and looking

	self->s.angles[YAW] = anglemod(self->s.angles[YAW]+self->yaw_speed);
	self->nextthink = level.time + FRAMETIME;
}

// RSP 050999 - rotate into locked horizontal position
void bomb_intoposition(edict_t *self)
{
	// Open bomb, one frame every two counts
	if (self->count)
	{
		self->count++;
		if ((self->count & 1) == 1)
			self->s.frame++;
		if (self->s.frame == 6)
		{
			self->s.frame = 5;
			self->count = 0;	// Fully opened, so stop opening
		}
	}

	// Bring bomb slowly up to horizontal

	if (self->s.angles[PITCH] > -360)
		self->s.angles[PITCH] -= 5;

	if (self->s.angles[PITCH] <= -360)
	{
		// Stabilize to the horizontal, decide to rotate left or right

		self->s.angles[PITCH] = 0;
		self->yaw_speed = (rand()&1) ? 5 : -5;
		self->think = bomb_seektarget;
		self->s.sound = gi.soundindex("littleboy/idle.wav");	// RSP 063000
	}

	self->nextthink = level.time + FRAMETIME;
}


void bomb_drop (edict_t *self)
{
	_AlignDirVelocity(self);	// RSP 050999 - moved here from prethink()
	if (((int)self->s.origin[2]) < self->radius)
	{
		// RSP 050999 - stop and look for player
		VectorCopy (vec3_origin, self->velocity);
		VectorCopy (vec3_origin, self->avelocity);
		self->movetype = MOVETYPE_STEP;
		self->flags |= FL_FLY;
		self->s.effects = EF_IONRIPPER;
		self->count = 1;
		self->think = bomb_intoposition;
	}

	self->nextthink = level.time + FRAMETIME;
}


// Littleboy is killed by a weapon

void littleboy_die (edict_t *ent,edict_t *inflictor,edict_t *attacker,int damage,vec3_t point)
{
	littleboy_touch(ent,NULL,NULL,NULL);
}

void littleboy_start(edict_t *self)
{
	self->clipmask = MASK_SHOT;
	self->solid = SOLID_BBOX;
	self->flags |= FL_PASSENGER;		// RSP 061899 - possible BoxCar passenger
	self->identity = ID_LITTLEBOY;
	VectorSet(self->mins,-30,-30,-20);	// RSP 050999 - match model
	VectorSet(self->maxs,30,30,20);		// RSP 050999 - match model

	self->model = "models/objects/littleboy/tris.md2";	// RSP 061899
	self->s.modelindex = gi.modelindex (self->model);	// RSP 061899

	self->touch = littleboy_touch;
	self->takedamage = DAMAGE_YES;		// RSP 051799 - can kill littleboy
	self->health = hp_table[HP_LITTLEBOY];	// RSP 051799
	self->die = littleboy_die;			// RSP 051799

	// RSP 050999 - damage values are now user variable

	self->dmg = wd_table[WD_LITTLEBOY];
	self->radius_dmg = wd_table[WD_LITTLEBOYBLAST];
	self->dmg_radius = 2*self->radius_dmg;
	self->classname = "littleboy";		// RSP 052199 - "bomb" -> "littleboy"
}

void fire_bomb (edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	edict_t	*bomb;
	vec3_t   end;	// RSP 051199
	trace_t  tr;	// RSP 051199

	bomb = G_Spawn();

	littleboy_start(bomb);

	VectorCopy (start, bomb->s.origin);
	vectoangles (dir, bomb->s.angles);
	VectorScale (dir, speed, bomb->velocity);
	bomb->movetype = MOVETYPE_TOSS;
	bomb->s.effects = EF_ROCKET;

	bomb->owner = self;
	bomb->mass = 50;					// RSP 053099

	bomb->think = bomb_drop;
	bomb->nextthink = level.time + FRAMETIME;

	// RSP 050999 - how far to drop the bomb
	VectorCopy (self->s.origin, end);
	end[2] -= 8000;	// look straight down
	tr = gi.trace (self->s.origin,NULL,NULL,end,self,MASK_SOLID);
	bomb->radius = (self->s.origin[2] + tr.endpos[2])/2;

	gi.linkentity (bomb);
}

// RSP 052199 - Added ability to put a littleboy into a map at edit time

/*QUAKED monster_littleboy (1 .5 0) (-30 -30 -20) (30 30 20) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_littleboy (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	littleboy_start(self);

	self->movetype = MOVETYPE_STEP;
	self->flags |= FL_FLY;
	self->s.effects = EF_IONRIPPER;
	self->owner = self;
	self->mass = 50;					// RSP 053099

	self->s.frame = 5;					// fully opened position

	// decide to rotate left or right

	self->yaw_speed = (rand()&1) ? 5 : -5;
	self->think = bomb_seektarget;
	self->nextthink = level.time + FRAMETIME;
	self->s.sound = gi.soundindex("littleboy/idle.wav");	// RSP 063000

	gi.linkentity (self);
}

