#include "g_local.h"

#define FRAME_STEAM_BEGIN_START		0

#define FRAME_STEAM_THICK_START		1
#define FRAME_STEAM_THICK_END		5

#define FRAME_STEAM_THIN_START		6
#define FRAME_STEAM_THIN_END		10

#define FRAME_STEAM_PUFF_START 		11
#define FRAME_STEAM_PUFF_END 		16

void func_steam_think (edict_t *self)
{
	switch (self->s.frame)
	{
	case FRAME_STEAM_BEGIN_START:
		if (random()<0.5)
			self->s.frame = FRAME_STEAM_THICK_START;
		else
			self->s.frame = FRAME_STEAM_THIN_START;

		self->nextthink  = level.time  + FRAMETIME;
		break;

	case FRAME_STEAM_THICK_END:	// repeat or die?
	case FRAME_STEAM_THIN_END:
		if ((self->wait == 0) || (level.time < self->wait))
			self->s.frame -= 4;
		else
			self->s.frame = FRAME_STEAM_PUFF_START;
	
		self->nextthink  = level.time  + FRAMETIME;
		break;

	case FRAME_STEAM_PUFF_END:  // done ... wait before repeating.
		self->s.frame = FRAME_STEAM_BEGIN_START;
		//self->nextthink = level.time  + (FRAMETIME * (10 + random()*20)) ;
		self->wait = 0;			// reset values
		self->think = NULL;
		break;

	default:
		self->s.frame++;
		self->nextthink  = level.time  + FRAMETIME;
		break;
	}
}

// function that fires if the steam is 'used' by something else
void func_steam_use (edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t v;

	if (!(self->target_ent) && (self->target) )  //Check & set when first used - when I did this during 
	{											 // spawn it could fail if the target was not loaded previously!
		if ((self->target_ent = G_PickTarget(self->target)) != NULL)
		{
			VectorSubtract(self->s.origin, self->target_ent->s.origin,v);
			vectoangles(v,self->s.angles);
		}
	}

	if (!(other->client))						// If a player didn't use me
	{
		if (self->think)						// And I AM busy right now	
		{							
			if (self->spawnflags)				// Make sure I'm supposed to stop now
				self->wait = level.time;		// And lock me in to stop at the next chance.
		}
		else									// otherwise if I'm not busy
		{
			self->think = func_steam_think;				
			self->nextthink = level.time + 1 + (self->random * random() * FRAMETIME);	// start me going
			if (self->spawnflags != SPAWNFLAG_FS_OFF_BY_TIMER)						// And lock in my longevity if need be
				self->wait = level.time + (self->delay * FRAMETIME);
		}
	}
}

/*QUAKED func_steam (0 .5 .8) OFF_BY_TIMER

delay:  repetitions for each 'puff'
random: max random delay for start (in seconds)
*/
void SP_func_steam (edict_t *self)
{
	/*if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (self);
		return;
	}	*/

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_TRIGGER;
	self->target_ent = NULL;
	self->wait = 0;				// I'm using this to represent the next stoppage.

	self->s.modelindex = gi.modelindex ("models/objects/steam/tris.md2");
	self->s.renderfx |= RF_TRANSLUCENT;

//	VectorSet (self->mins, -8, -8, -8);
//	VectorSet (self->maxs, 8, 8, 8);

	self->target_ent = NULL;

	self->s.frame = FRAME_STEAM_BEGIN_START;
	self->use = func_steam_use;
//	self->think = func_steam_use;
//	self->nextthink = level.time + FRAMETIME;

	gi.linkentity (self);
}

// RSP 042999 - Added waypoints
// WPO 092699 - Add dreadlock wayoints

/*QUAKED waypoint (.5 .3 0) (-8 -8 -8) (8 8 8) RETREAT DREADLOCK
Waypoints provide attack and retreat paths for monsters
and dreadlocks.

target: next waypoint

RETREAT spawnflag marks this as a retreat waypoint. If
not set, this is an attack waypoint.

DREADLOCK spawnflag indicates this waypoint is for dreadlocks,
not monsters.  Currently there is no distinction between attack
and retreat waypoints for dreadlocks, but there may be someday.
*/

// RSP 062199 - waypoint_check() is called by waypointed monsters to see if they
// need to be on their retreat waypoint paths

void waypoint_check(edict_t *self)
{
	edict_t	*target;	// RSP 042999 - Waypoints
	vec3_t	v;

	if ((self->health < (self->max_health / 2)) && (rand()&1))	// 50% chance of retreating
	{
		// Move to the closest retreat waypoint that belongs to you
		target = G_Find_Waypoint(self,true);	// 'true' means find a retreat waypoint
		if (target)
		{
			if (self->enemy && self->enemy->client)	// player?
				Forget_Player(self);
			self->goalentity = self->movetarget = target;
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = vectoyaw(v);
			self->monsterinfo.run(self);
			self->target = NULL;
			self->flags &= ~FL_ATTACKING;	// No longer attacking
			self->flags |= FL_RETREATING;	// Now we're retreating
		}
	}
}


void waypoint_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t		v;
	edict_t		*next;

	if (other->movetarget != self)
		return;
	
	if (other->enemy)
		return;

	if (self->spawnflags & SPAWNFLAG_WAYPOINT_DLOCK) // WPO 092699 - Dreadlock waypoints
	{
		if (other->identity != ID_DLOCK)	// ignore if not a dreadlock
			return;

		if (self->target)
		{
			next = G_PickTarget(self->target);

			// If the next waypoint is a dreadlock waypoint allow the move. Else stop here.
			// TODO What the heck does the dreadlock do at the last waypoint?
			if (next->spawnflags & SPAWNFLAG_WAYPOINT_DLOCK)
				next = NULL;
		}
		else
			next = NULL;

		other->goalentity = other->movetarget = next;

		if (next != NULL)
		{
			// proceed to the next waypoint
			VectorSubtract (other->goalentity->s.origin, other->s.origin, v);
			other->ideal_yaw = vectoyaw (v);
		}

	}
	else
	{
		if (self->target)
		{
			next = G_PickTarget(self->target);
			// If both waypoints are ATTACK, or both are RETREAT, allow the move. Else stop here.
			if ((self->spawnflags & SPAWNFLAG_WAYPOINT_RETREAT) != (next->spawnflags & SPAWNFLAG_WAYPOINT_RETREAT))
				next = NULL;
		}
		else
			next = NULL;

		other->goalentity = other->movetarget = next;

		// Waiting at a waypoint
		if (self->wait)
		{
			other->monsterinfo.pausetime = level.time + self->wait;
			other->monsterinfo.stand (other);
			return;
		}

		if (!other->movetarget)	// If nowhere to proceed to, stand around
		{
			other->monsterinfo.pausetime = level.time + 100000000;
			other->monsterinfo.stand (other);
			other->flags &= ~(FL_ATTACKING|FL_RETREATING);	// Doing neither
		}
		else	// Proceed to next waypoint
		{
			VectorSubtract (other->goalentity->s.origin, other->s.origin, v);
			other->ideal_yaw = vectoyaw (v);
		}
	}
}

void SP_waypoint (edict_t *self)
{
//	trace_t	t;	// For waypointed flyers
//	vec3_t	v;	// For waypointed flyers

	if (!self->targetname)
	{
		gi.dprintf ("waypoint with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = waypoint_touch;

/*
	// The following section may be relevant if we can't get waypointed flyers to hit their
	// waypoints properly because of z-movement problems. Leave this code commented until
	// you can show the problem exists.

	// So that flyers that are using waypoints can touch their waypoints, the z value of 
	// the waypoint will be extended up and down to the ceiling and floor. Then, if a flyer
	// is heading for this waypoint, but hasn't had enough time to adjust his z value, then
	// at least he's able to touch the waypoint somewhere above or below the origin.

	// One problem that remains with this approach is if you're trying to get the flyer to be
	// at a certain height when he hits the waypoint.

	// Find floor
	VectorCopy(self->s.origin,v);
	v[2] -= 5000;
	t = gi.trace (self->s.origin, NULL, NULL, v, self, MASK_MONSTERSOLID);
	VectorSet (self->mins,-8,-8,t.endpos[2] - self->s.origin[2]);

	// Find ceiling
	VectorCopy(self->s.origin,v);
	v[2] += 5000;
	t = gi.trace (self->s.origin, NULL, NULL, v, self, MASK_MONSTERSOLID);
	VectorSet (self->maxs,8,8,t.endpos[2] - self->s.origin[2]);

printf("Waypoint floor is %5.2f, ceiling is %5.2f\n",self->s.origin[2]+self->mins[2],self->s.origin[2]+self->maxs[2]);	// RSP DEBUG

	// The following two lines to set the mins and maxs values can be deleted if the previous
	// code gets activated.
 */
	VectorSet (self->mins,-8,-8,-8);
	VectorSet (self->maxs,8,8,8);

	self->svflags |= SVF_NOCLIENT;
	self->identity = ID_WAYPOINT;
	gi.linkentity (self);

	// BTW, whether this is an attack or retreat waypoint is determined by the
	// SPAWNFLAG_WAYPOINT_RETREAT flag in spawnflags. On = retreat. Off = attack.
}


// RSP 052699 - Pick spot on target to shoot at.
// Returned in enemy_spot.

void Pick_Client_Target(edict_t *self,vec3_t enemy_spot)
{
	edict_t *enemy = self->enemy;

	VectorCopy(enemy->s.origin,enemy_spot);
	switch (rand()&7)
	{
	case 0:
	case 7:
		enemy_spot[2] += enemy->viewheight;
		break;
	case 1:
		break;
	case 2:
		enemy_spot[0] += enemy->mins[0]/2;
		enemy_spot[1] += enemy->mins[1]/2;
		enemy_spot[2] += (enemy->viewheight)/2;
		break;
	case 3:
		enemy_spot[0] += enemy->maxs[0]/2;
		enemy_spot[1] += enemy->maxs[1]/2;
		enemy_spot[2] += (enemy->viewheight)/2;
		break;
	case 4:
		enemy_spot[0] += enemy->mins[0];
		enemy_spot[1] += enemy->mins[1];
		enemy_spot[2] += (enemy->viewheight)/2;
		break;
	case 5:
		enemy_spot[0] += enemy->maxs[0];
		enemy_spot[1] += enemy->maxs[1];
		enemy_spot[2] += (enemy->viewheight)/2;
		break;
	case 6:
		enemy_spot[2] += enemy->maxs[2];
		break;
	}
}


// MRB 060999 - light stuff added in

static void light_throw_sparks(edict_t *ent)
{
	if (random() < 0.1)
	{	//stop
		ent->nextthink = 0;
		ent->think = NULL;
	}

	ent->nextthink = level.time + ((20 + rand() % 20) * FRAMETIME);
	ent->think = light_throw_sparks;
	gi.linkentity(ent);
}


/* The spread of the shards depends on:
 *	self->pos1 = the initial ANGULAR direction	
 *	speed = the initial speed of the shards
 *	random = the amount of 'width' to the spread.  0 - 90 (degrees)
 */
static void light_luc_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int		i;
	vec3_t	fwd,right;

	self->s.skinnum = 1;	// broken skin

	self->takedamage = DAMAGE_NO;
//	self->solid = SOLID_NOT;	// RSP 091500 - shouldn't walk through a floor light
	self->nextthink = 0;
	self->think = NULL;
	self->activator = attacker;

	gi.configstring (CS_LIGHTS + self->style, "a");	// Turn off the lights

	light_throw_sparks(self);

	gi.linkentity(self);

	// Need we worry about lights being turned back on accidently?
	G_UseTargets (self, attacker);

	gi.sound(self,CHAN_ITEM,gi.soundindex("misc/lightdie.wav"),1,ATTN_NORM,0);

	AngleVectors(self->s.angles,fwd,right,NULL);

	for (i = 0 ; i < self->count ; i++)
	{
		vec3_t v;
		edict_t *shard = G_Spawn();

		shard->movetype = MOVETYPE_BOUNCE;

		gi.setmodel (shard, "models/objects/lights/gib.md2");
		shard->solid = SOLID_NOT;
		shard->s.effects |= EF_GREENGIB;

		// set gib start
		v[YAW]   = self->pos1[YAW]   +((random() * self->random * 2) - self->random);
		v[PITCH] = self->pos1[PITCH] +((random() * self->random * 2) - self->random);
		v[ROLL]  = 0;
		AngleVectors(v,v,NULL,NULL);
		VectorMA(self->s.origin,5,v,shard->s.origin);

		//set gib velocity
		VectorSubtract(shard->s.origin,self->s.origin,v);
		VectorNormalize(v);
		VectorMA(vec3_origin,100 + (random() *self->speed),v,shard->velocity);

		//set gib avelocity
		shard->avelocity[0] = random()*600;
		shard->avelocity[1] = random()*600;
		shard->avelocity[2] = random()*600;

		shard->think = G_FreeEdict;
		shard->nextthink = level.time + 10 + random()*10;

		gi.linkentity (shard);
	}
}

/* allow use to cascade to my targets */
static void light_luc_use (edict_t *self, edict_t *other, edict_t *activator)
{
	G_UseTargets (self, activator);
}

static void luc_light_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (random() < 0.5)
		return;

	gi.sound(self,CHAN_ITEM,gi.soundindex("misc/lighthit.wav"),1,ATTN_NORM,0);
}

//Common setup routine
static void light_luc_setup(edict_t *ent)
{
	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_AIM;

	if (ent->health > 0)
		ent->die = light_luc_die;

	ent->pain = luc_light_pain;

	// Adjust pitch 
	ent->s.angles[PITCH] = ent->speed;
	ent->use = light_luc_use;

	// RSP 091500 - Check style. Should be between 70-120 by definition
	if ((ent->style < 70) || (ent->style > 120))
		gi.dprintf("Invalid %s style setting %d at %s\n",ent->classname,ent->style,vtos(ent->s.origin));
}

/*QUAKED light_ceil1 (0 1 0) (-18 -18 -8) (18 18 8)

speed: pitch (positive = nose down)
*/
void SP_light_ceil1 (edict_t *ent)
{
	if (ent->health == 0)
		ent->health = 50;
	ent->count = 5 + (rand() % 5);

	VectorSet(ent->mins,-18,-18,-8);
	VectorSet(ent->maxs, 18, 18, 8);

	light_luc_setup(ent);

	ent->s.modelindex = gi.modelindex ("models/objects/lights/ceil1/tris.md2");

	// Setup death shard values
	VectorCopy(ent->s.angles, ent->pos1);
	ent->pos1[PITCH] += 90;
	ent->speed = 50;
	ent->random = 60;

	gi.linkentity (ent);
}

/*QUAKED light_ceil2 (0 1 0) (-8 -8 -6) (8 8 14)

speed: pitch (positive = nose down)
*/
void SP_light_ceil2 (edict_t *ent)
{
	if (ent->health == 0)
		ent->health = 20;
	ent->count = 2 + (rand() % 3);

	VectorSet(ent->mins,-8,-8,-6);
	VectorSet(ent->maxs, 8, 8, 14);

	light_luc_setup(ent);
	ent->s.modelindex = gi.modelindex ("models/objects/lights/ceil2/tris.md2");

	// Setup death shard values
	VectorCopy(ent->s.angles, ent->pos1);
	ent->pos1[PITCH] += 90;
	
	ent->speed = 0;
	ent->random = 90;

	gi.linkentity (ent);
}

/*QUAKED light_wall1 (0 1 0) (-5 -5 -10) (5 5 10)

speed: pitch (positive = nose down)
*/
void SP_light_wall1 (edict_t *ent)
{
	if (ent->health == 0)
		ent->health = 20;
	ent->count = 3 + (rand() % 3);

	VectorSet(ent->mins,-5,-5,-10);
	VectorSet(ent->maxs, 5, 5, 10);

	light_luc_setup(ent);
	ent->s.modelindex = gi.modelindex ("models/objects/lights/wall1/tris.md2");

	// Setup death shard values
	VectorCopy(ent->s.angles, ent->pos1);
	ent->speed = 20;
	ent->random = 80;

	gi.linkentity (ent);
}


/*QUAKED light_wall2 (0 1 0) (-10 -8 -4) (4 8 12)

speed: pitch (positive = nose down)
*/
void SP_light_wall2 (edict_t *ent)
{
	if (ent->health == 0)
		ent->health = 20;
	ent->count = 2 + (rand() % 3);

	VectorSet(ent->mins,-10,-8,-4);
	VectorSet(ent->maxs, 4, 8, 12);

	light_luc_setup(ent);
	ent->s.modelindex = gi.modelindex ("models/objects/lights/wall2/tris.md2");

	VectorCopy(ent->s.angles, ent->pos1);
	ent->speed = 50;
	ent->random = 90;

	gi.linkentity (ent);
}

/*QUAKED light_wall3 (0 1 0) (-8 -8 -12) (8 8 12)

speed: pitch (positive = nose down)
*/
void SP_light_wall3 (edict_t *ent)
{
	if (ent->health == 0)
		ent->health = 20;
	ent->count = 3 + (rand() % 3);

	VectorSet(ent->mins,-8,-8,-12);
	VectorSet(ent->maxs, 8, 8, 12);

	light_luc_setup(ent);
	ent->s.modelindex = gi.modelindex ("models/objects/lights/wall3/tris.md2");

	VectorCopy(ent->s.angles, ent->pos1);
	ent->speed = 100;
	ent->random = 80;

	gi.linkentity (ent);
}

/*QUAKED light_floor1 (0 1 0) (-8 -8 -80) (8 8 8)

speed: pitch (positive = nose down)
*/
void SP_light_floor1 (edict_t *ent)
{
	if (ent->health == 0)
		ent->health = 30;
	ent->count = 3 + (rand() % 3);

	VectorSet(ent->mins,-8,-8,-80);
	VectorSet(ent->maxs, 8, 8, 8);

	light_luc_setup(ent);
	ent->s.modelindex = gi.modelindex ("models/objects/lights/floor1/tris.md2");

	VectorCopy(ent->s.angles, ent->pos1);
	ent->pos1[PITCH] -= 90;
	ent->speed = 200;
	ent->random = 90;

	gi.linkentity (ent);
}

// RSP 052699 - Added generic decoration item

extern vec3_t VEC_UP;
extern vec3_t VEC_DOWN;
vec3_t NEW_UP	= {270, 0, 0};
vec3_t NEW_DOWN	= {90, 0, 0};

/*QUAKED item_deco (.3 .3 1) (-8 -8 0) (8 8 16) TRANSLUCENT SOLID START_OFF PUSH EXPLODE DROP

message: the directory where the model is found.
         For example, if "message" is "grass" then place the
		 model for your grass in models/objects/deco/grass.

sizes: "x,y,z" means x units by y units by z units centered
       about the item's origin. Default is 16,16,16.

frames: if non-zero, the model is animated and this is the number
        of animation frames. Animation can be started/stopped if
        you target the item with a trigger.

skins: if non-zero, the model has this number of skins to
       cycle through (animate)

delay: how long to pause between model frames. Default is 1.

If START_OFF is set, model animation won't start until the item is triggered.

Skin animation is unaffected by START_OFF. It is always ON.

If DROP is set, the item drops to the floor, otherwise
it stays at the given origin.

If PUSH is set, the item is pushable. This sets SOLID and DROP.
This key is important:

     mass: (default 400)

If EXPLODE is set, the item will blow up if shot. These keys are important:

     health: (default 10)

     dmg: (default 150)
*/


void deco_think (edict_t *self)
{
	// Do model animation if delay has expired

	if (level.time >= self->pain_debounce_time)
	{
		if (++self->s.frame == self->count2)	// RSP 080300 - switch from count to count2
			self->s.frame = 0;
		self->pain_debounce_time = level.time + (self->delay) * FRAMETIME;
	}

	// Do skin animation if needed

	if (++self->s.skinnum == self->count)
		self->s.skinnum = 0;

	self->nextthink = level.time + FRAMETIME;
}


void deco_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->count2 <= 1)		// RSP 080300 - switch from count to count2
		return;					// can't turn on an unanimated item_deco

	self->flags ^= FL_ON;		// toggle the on/off flag
	if (self->flags & FL_ON)
		deco_think(self);		// immediately switch to next frame
//	else
//		self->nextthink = 0;	// turn off animation
}

void deco_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;

	VectorSubtract (self->s.origin, other->s.origin, v);
	M_walkmove (self, vectoyaw(v), 20 * ((float)other->mass / (float)self->mass) * FRAMETIME);
}

void GenDebris(edict_t *self, float spd, int debris_type)
{
	vec3_t org;

	org[0] = self->s.origin[0] + crandom() * self->size[0];
	org[1] = self->s.origin[1] + crandom() * self->size[1];
	org[2] = self->s.origin[2] + crandom() * self->size[2];
	switch (debris_type)
		{
	case 1:
		ThrowDebris (self, "models/objects/debris1/tris.md2", spd, org);
		break;
	case 2:
		ThrowDebris (self, "models/objects/debris2/tris.md2", spd, org);
		break;
	case 3:
		ThrowDebris (self, "models/objects/debris3/tris.md2", spd, org);
		break;
		}
}

#define NUMDADDYCHUNKS 2
#define NUMMOMMYCHUNKS 4
#define NUMBABYCHUNKS  8

void deco_explode (edict_t *self)
{
	float	spd;
	vec3_t	save;
	int		i;

	T_RadiusDamage (self, self->activator, self->dmg, NULL, self->dmg+40, MOD_BARREL);

	VectorCopy (self->s.origin, save);
	VectorMA (self->absmin, 0.5, self->size, self->s.origin);

	spd = 1.50 * (float)self->dmg / 200.0;
	for (i = 0 ; i < NUMDADDYCHUNKS ; i++)
		GenDebris(self,spd,1);

	spd = 1.75 * (float)self->dmg / 200.0;
	for (i = 0 ; i < NUMMOMMYCHUNKS ; i++)
		GenDebris(self,spd,3);

	spd = 2.00 * (float)self->dmg / 200;
	for (i = 0 ; i < NUMBABYCHUNKS ; i++)
		GenDebris(self,spd,2);

	VectorCopy (save, self->s.origin);
	if (self->groundentity)
		SpawnExplosion2(self,EXPLODE_DECO);   // RSP 062900
	else
		SpawnExplosion1(self,EXPLODE_DECO);   // RSP 062900
	G_FreeEdict(self);
}

void deco_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + 2 * FRAMETIME;
	self->think = deco_explode;
	self->activator = attacker;
}

void deco_droptofloor(edict_t *self)
{
	vec3_t	mins,maxs;
	int		solid;
	float	origin_delta;

	// Since item_deco's origins are at their center, we have to move the
	// origin to the base of the item_deco so the M_droptofloor() routine
	// works properly. We'll reset the origin after that to where it should be.

	// The item_deco has to be solid for this to work.

	origin_delta = self->s.origin[2] - self->absmin[2];
	VectorCopy(self->mins,mins);
	VectorCopy(self->maxs,maxs);
	self->s.origin[2] = self->absmin[2];	// RSP DEBUG - is absmin set right at this point?
	self->mins[2] = 0;
	self->maxs[2] = self->absmax[2] - self->s.origin[2];
	solid = self->solid;
	self->solid = SOLID_BBOX;		// make sure it stops at the floor

	M_droptofloor(self);

	self->s.origin[2] = self->absmin[2] + origin_delta;	// restore
	VectorCopy(mins,self->mins);		// restore
	VectorCopy(maxs,self->maxs);		// restore
	self->solid = solid;				// restore

	if (self->count2)	// RSP 062099 - animated? 080300 - switch from count to count2
	{
		self->think = deco_think;
		if (self->flags & FL_ON)	// animation starts on
			self->nextthink = level.time + (self->delay) * FRAMETIME;
	}
}


void SP_item_deco (edict_t *self)
{
	char buffer[MAX_QPATH];
	int  x,y,z;

	if (deathmatch->value && (self->spawnflags & SPAWNFLAG_ID_EXPLODE))
	{	// auto-remove explodable item_decos for deathmatch
		G_FreeEdict (self);
		return;
	}

	// Get item's true name

	if (!self->message || (self->message[0] == 0))
	{
		gi.dprintf("item_deco w/no message key at %s\n",vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	// Orient up or down?

	if (VectorCompare (self->s.angles, VEC_UP))
		VectorCopy (NEW_UP, self->s.angles);
	else if (VectorCompare (self->s.angles, VEC_DOWN))
		VectorCopy (NEW_DOWN, self->s.angles);

	Com_sprintf(buffer,sizeof(buffer),"models/objects/deco/%s/tris.md2",self->message);
	self->s.modelindex = gi.modelindex(buffer);
	self->classname = self->message;

	// Translucent?

	if (self->spawnflags & SPAWNFLAG_ID_TRANSLUCENT)
		self->s.renderfx |= RF_TRANSLUCENT;
	
	// Solid?

	if (self->spawnflags & (SPAWNFLAG_ID_SOLID|SPAWNFLAG_ID_EXPLODE|SPAWNFLAG_ID_PUSH))
		self->solid = SOLID_BBOX;
	else
		self->solid = SOLID_NOT;

	// Get model size

	if (st.sizes && st.sizes[0] != 0)
	{
		sscanf(st.sizes,"%d,%d,%d",&x,&y,&z);
		VectorSet(self->maxs, ((float)x)/2, ((float)y)/2, ((float)z)/2);
		VectorSet(self->mins, -((float)x)/2, -((float)y)/2, -((float)z)/2);
	}
	else
	{
		VectorSet(self->maxs,8,8,16);	// RSP 072299 - bottom of model sits on floor
		VectorSet(self->mins,-8,-8,0);
	}

	if (!self->mass)
		self->mass = 400;

	if (self->spawnflags & SPAWNFLAG_ID_EXPLODE)
	{
		gi.modelindex ("models/objects/debris1/tris.md2");
		gi.modelindex ("models/objects/debris2/tris.md2");
		gi.modelindex ("models/objects/debris3/tris.md2");

		if (!self->health)
			self->health = 10;
		if (!self->dmg)
			self->dmg = 150;
		self->die = deco_die;
		self->takedamage = DAMAGE_YES;
	}
	else
	{
		self->health = 0;
		self->takedamage = DAMAGE_NO;
	}

	self->monsterinfo.aiflags |= AI_NOSTEP;

	if (self->spawnflags & SPAWNFLAG_ID_PUSH)
	{
		self->spawnflags |= SPAWNFLAG_ID_DROP;	// PUSH sets DROP. otherwise can't PUSH
		self->touch = deco_touch;
	}

	// Set up model frame animations and skin animations if designated

	self->count = self->count2 = 1;				// RSP 080300
	self->s.frame = self->s.skinnum = 0;		// RSP 080300

	if (st.frames)	// RSP 062099 - model animated?
	{
		self->count2 = st.frames;	// RSP 080300 - switch from count to count2
		self->use = deco_use;
		if (!self->delay)
			self->delay = 1;
		if (!(self->spawnflags & SPAWNFLAG_ID_START_OFF))	// if set, animation is triggered
		{	// start on
			self->flags |= FL_ON;	// animation starts on
			self->pain_debounce_time = level.time + (self->delay) * FRAMETIME;
		}
	}

	// RSP 080300 - add skin animation

	if (st.skins)
		self->count = st.skins;

	if ((self->count > 1) || (self->count2 > 1))
	{
		self->think = deco_think;
		self->nextthink = level.time + FRAMETIME;
	}

	// Do we leave the item where it is, or drop it to the floor?

	if (self->spawnflags & SPAWNFLAG_ID_DROP)
	{
		// pause to make sure world stabilizes. we'll have to restore the
		// think function later if animation is used.

		self->movetype = MOVETYPE_STEP;			// allows gravity to affect the item_deco
		self->think = deco_droptofloor;
		self->nextthink = level.time + 2 * FRAMETIME;
	}

	gi.linkentity(self);
}


//=================================================================
//
// RSP 081500
// spearlight and rocketlight
// These lights use effects, and give up spears when broken.
//
//=================================================================

void spearlight_die(edict_t *ent, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int			i;
	edict_t		*spear;
	vec3_t		v,origin;
	qboolean	is_normalspear = (ent->identity == ID_ROCKETLIGHT) ? false : true;

	VectorCopy(ent->s.origin,origin);	// save model origin
	origin[2] += ent->size[2]/2;		// start at middle of model

	gi.sound(ent,CHAN_ITEM,gi.soundindex("misc/lightdie.wav"),1,ATTN_NORM,0);

	G_FreeEdict(ent);				// kill model

	// throw some spears

	for (i = 0 ; i < 8 ; i++)
	{
		spear = G_Spawn();
		VectorCopy(origin,spear->s.origin);
		VectorCopy(origin,spear->s.old_origin);
		AngleVectors(tv(0,i*45,0),v,NULL,NULL);			// throw direction
		vectoangles(v,spear->s.angles);					// RSP 081600
		spear = spear_impale(spear,is_normalspear);
		spear_throw_backwards(spear);
		spear->flags &= ~FL_PASSENGER;					// not allowed in func_BoxCar
		VectorSet(spear->s.angles,0,random()*359,0);	// rotate spear about Z axis
		gi.linkentity(spear);
	}
}


void spearlight_think(edict_t *ent)
{
    // Control model animations

	if (++ent->s.frame == ent->count2)
		ent->s.frame = 0;						// reset model frame

    // Control skin animations

    if (++ent->s.skinnum == ent->count)
        ent->s.skinnum = 0;						// reset skin

    ent->nextthink = level.time + FRAMETIME;
}


void spearlight_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// Nothing happens if something touches the light
}

void spearlight_drop(edict_t *ent)
{
	ent->spawnflags |= ITEM_NO_TOUCH;
	droptofloor(ent);
	ent->touch = spearlight_touch;
	if (ent->identity == ID_ROCKETLIGHT)
	{
		ent->s.effects = EF_HYPERBLASTER;	// rocketlight gets yellow effect
		VectorSet(ent->maxs,17,17,54);
	}
	else
	{
		ent->s.effects = EF_FLAG2;			// RSP 081600 - spearlight gets blue effect
		VectorSet(ent->maxs,17,17,35);
	}
	VectorSet(ent->mins,-17,-17,0);

	ent->think = spearlight_think;
    ent->nextthink = level.time + FRAMETIME;

	gi.linkentity(ent);
}


void spearlight_common(edict_t *ent)
{
	ent->s.skinnum = 0;
	ent->s.frame = 0;
	ent->s.modelindex = gi.modelindex (ent->model);
	ent->takedamage = DAMAGE_YES;
	ent->health = 1;					// easily broken
	ent->die = spearlight_die;
	ent->nextthink = level.time + 2*FRAMETIME;    // start after other solids
	ent->think = spearlight_drop;
}

/*QUAKED spearlight (.3 .3 1) (-17 -17 0) (17 17 35)

This object gives you 8 normal spears when broken.
Do not place inside a BoxCar area.
 */

void SP_Spear_Light(edict_t *ent)
{
	ent->model = "models/objects/lights/spearlight/tris.md2";
	ent->count = 8;		// number of skins
	ent->count2 = 1;	// number of model frames
	spearlight_common(ent);
}


/*QUAKED rocketlight (.3 .3 1) (-17 -17 0) (17 17 54)

This object gives you 8 rocket spears when broken.
Do not place inside a BoxCar area.
 */

void SP_RocketSpear_Light(edict_t *ent)
{
	ent->model = "models/objects/lights/rocketlight/tris.md2";
	ent->count = 8;		// number of skins
	ent->count2 = 12;	// number of model frames
	ent->identity = ID_ROCKETLIGHT;
	spearlight_common(ent);
}


// RSP 102700 - Embracer for endgame

void embracer_think(edict_t *ent)
{
    // Control skin animations

	if (++ent->s.skinnum > MATRIX_END_SKIN)
		ent->s.skinnum = MATRIX_START_SKIN;

    ent->nextthink = level.time + FRAMETIME;
}


void embracer_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other->client)
	{		// player loses
		if (other->health <= 0)
			return;	// dead players cause no action
		BeginIntermission (self);
		G_FreeEdict(self);	// only one use
	}
}


void embracer_die(edict_t *ent, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict(ent);
}

/*QUAKED embracer (.3 .3 1) (-16 -16 -16) (16 16 16)

This object is for the endgame only. It uses the matrixball model.
If the player touches it, the player loses and the game is over.

map: *.cin to play
 */

void SP_embracer(edict_t *ent)
{
	ent->model = "models/items/ammo/matrix/tris.md2";
	ent->s.modelindex = gi.modelindex (ent->model);
	ent->clipmask = MASK_SHOT;
	ent->solid = SOLID_BBOX;
	ent->s.renderfx |= RF_TRANSLUCENT;
	ent->s.sound = gi.soundindex ("weapons/matrix/matrixfly.wav");
	VectorSet(ent->maxs,16,16,16);
	VectorSet(ent->mins,-16,-16,-16);
	ent->s.frame = 10;
	ent->s.skinnum = MATRIX_START_SKIN;
	ent->takedamage = DAMAGE_NO;
	ent->health = 50;
	ent->touch = embracer_touch;
	ent->die = embracer_die;
	ent->nextthink = level.time + FRAMETIME;
	ent->think = embracer_think;
	gi.linkentity (ent);
}


