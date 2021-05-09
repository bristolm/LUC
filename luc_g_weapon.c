/* LUC weapon routines */

#include "g_local.h"
#include "m_player.h"
#include "luc_powerups.h"		// RSP 100299 - to pick up Matrix Shield powerup

// JBM 012399 - Added this so it can compile after moving the speargun in here
void check_dodge (edict_t *, vec3_t, vec3_t, int);

extern void Weapon_Generic (edict_t *ent, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST,
							int *pause_frames, int *fire_frames, 
							void (*fire)(edict_t *ent));
extern void P_ProjectSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
extern void NoAmmoWeaponChange (edict_t *ent);
extern void fire_lead (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod);

extern qboolean is_quad;
extern qboolean is_tri;

// RSP 070899 - Determine if the z value given is 1/6 from the top of the entity
//              Used for headshot determination

#define CHIN_ABOVE_ORIGIN 18	// Chin distance above origin for player

qboolean IsHeadShot(edict_t *ent, vec3_t hitspot)
{
	qboolean result;

	// Headshots can't be scored against the player unless this is DM.

	if (ent->client && !(deathmatch->value))
		result = false;

	// No heads on littleboys or turrets (or beetles - RSP 081600)

	else if ((ent->identity == ID_TURRET)		||
			 (ent->identity == ID_LITTLEBOY)	||
			 (ent->identity == ID_BEETLE))
	{
		result = false;
	}
	else if (ent->identity == ID_GREAT_WHITE)	// shark? his head is in front, defined by his YAW
	{
		float yaw = ent->s.angles[YAW];

		if ((yaw < 22.5) || (yaw >= 337.5))
			result = (hitspot[0] >= ent->absmax[0] - (ent->absmax[0] - ent->absmin[0])/6);
		else if ((22.5 <= yaw) && (yaw < 67.5))
			result = ((hitspot[0] >= ent->absmax[0] - (ent->absmax[0] - ent->absmin[0])/4.25) &&
					  (hitspot[1] >= ent->absmax[1] - (ent->absmax[1] - ent->absmin[1])/4.25));
		else if ((67.5 <= yaw) && (yaw < 112.5))
			result = (hitspot[1] >= ent->absmax[1] - (ent->absmax[1] - ent->absmin[1])/6);
		else if ((112.5 <= yaw) && (yaw < 157.5))
			result = ((hitspot[1] >= ent->absmax[1] - (ent->absmax[1] - ent->absmin[1])/4.25) &&
					  (hitspot[0] <= ent->absmin[0] + (ent->absmax[0] - ent->absmin[0])/4.25));
		else if ((157.5 <= yaw) && (yaw < 202.5))
			result = (hitspot[0] <= ent->absmin[0] + (ent->absmax[0] - ent->absmin[0])/6);
		else if ((202.5 <= yaw) && (yaw < 247.5))
			result = ((hitspot[0] <= ent->absmin[0] + (ent->absmax[0] - ent->absmin[0])/4.25) &&
					  (hitspot[1] <= ent->absmin[1] + (ent->absmax[1] - ent->absmin[1])/4.25));
		else if ((247.5 <= yaw) && (yaw < 292.5))
			result = (hitspot[1] <= ent->absmin[1] + (ent->absmax[1] - ent->absmin[1])/6);
		else
			result = ((hitspot[1] <= ent->absmin[1] + (ent->absmax[1] - ent->absmin[1])/4.25) &&
					  (hitspot[0] >= ent->absmax[0] - (ent->absmax[0] - ent->absmin[0])/4.25));
	}
	else if (ent->client)	// player head is on top
		result = (hitspot[2] >= ent->s.origin[2] + CHIN_ABOVE_ORIGIN);
	else					// monster head is on top
		result = (hitspot[2] >= ent->absmax[2] - (ent->absmax[2] - ent->absmin[2])/6);

	return (result);
}



//=================
//
//  Knife
//
//  MRB 021599
//
//=================

#define KNIFE_RANGE			40

// RSP 081600 - simplify sound code
#define KNIFE_SOUND_MISS	0
#define KNIFE_SOUND_MEAT	1
#define KNIFE_SOUND_SOLID	2

void Swing_Knife (edict_t *ent)
{
	int		damage, start_in_water,end_in_water;
	vec3_t  start,forward,right,end,fire_min,fire_max;
	trace_t	tr;
	int		knife_sound;	// RSP 081600

	if (deathmatch->value)
		damage = (1.5 * wd_table[WD_KNIFE]) + (random() * 10);	// RSP 032399
	else
		damage = wd_table[WD_KNIFE] + (random() * 10);			// RSP 032399
	damage += (damage * (random() * 0.5));

	//  WPO 231199 LUC has limited quad now
	if (is_quad)
		damage *= 4;
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;

	VectorSet(fire_min,-24,-24,-24);
	VectorSet(fire_max, 24, 24, 24);
	
	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorCopy(ent->s.origin,start);
	start[2] += ent->viewheight;

	// NOTE: Maybe shouldfigure the attack is in a sweeping motion???
	// 45 degrees left/right in front of us, 20 up/down
	
	// Start a little bit away from us
	VectorMA(start,10,forward,start);

	// End a ways further away
	VectorMA(start,KNIFE_RANGE,forward,end);

	tr = gi.trace (start, NULL, NULL, end, ent, MASK_SHOT);
	
	// See if we passed through a water barrier.
	start_in_water = gi.pointcontents (start) & MASK_WATER;
	end_in_water = gi.pointcontents (tr.endpos) & MASK_WATER;

	if (start_in_water || end_in_water)	// RSP 062400
	{
		// RSP 062400
		// Removed bubble trail code, as it spawned too few bubbles to sometimes even see

		gi.sound (ent,CHAN_VOICE,gi.soundindex("weapons/knife/hitwater.wav"),1,ATTN_NORM,0);	// RSP 081600 - switch from CHAN_BODY to CHAN_VOICE
	}

	knife_sound = KNIFE_SOUND_MISS;	// RSP 081600 - knife sound unless we hit something

	if (tr.fraction < 1.0)
	{	// we hit something
		// Now check if we damaged anything with our shot
		if (tr.ent->takedamage)
		{	// Armor does not protect
			T_Damage (tr.ent, ent, ent, forward, tr.endpos, tr.plane.normal, damage, 1, DAMAGE_NO_ARMOR, MOD_KNIFE);
			knife_sound = KNIFE_SOUND_MEAT;	// RSP 081600
		}
		else
			if (strncmp (tr.surface->name, "sky", 3) != 0)
			{
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_SPARKS);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.multicast (tr.endpos, MULTICAST_PVS);

				knife_sound = KNIFE_SOUND_SOLID;	// RSP 081600
			}
	}

	// RSP 081600 - consolidate knife sounds into one place
	switch(knife_sound)
	{
		default:
		case KNIFE_SOUND_MISS:
			gi.sound(ent,CHAN_BODY,gi.soundindex("weapons/knife/swing.wav"),1,ATTN_NORM,0);
			break;
		case KNIFE_SOUND_MEAT:
			gi.sound(ent,CHAN_BODY,gi.soundindex("weapons/knife/hitmeat.wav"),1,ATTN_NORM,0);
			break;
		case KNIFE_SOUND_SOLID:
			gi.sound(ent,CHAN_BODY,gi.soundindex("weapons/knife/hitsolid.wav"),1,ATTN_NORM,0);
			break;
	}

	ent->client->ps.gunframe++;
}

void Weapon_Knife (edict_t *ent)
{
	static int	pause_frames[]	= {14, 23, 0};
	static int	fire_frames[]	= {5, 9, 0};

	Weapon_Generic (ent, 2, 10, 27, 32, pause_frames, fire_frames, Swing_Knife);
}

//=================
//
//  Speargun
//
//  JBM
//
//=================

// RSP 062900 - copied from bomb_touch and made specific to rocketspear
void rocketspear_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t  origin;
	int     damage;

	if (other == ent->owner)
		return;

	if (other && (Q_stricmp(other->classname,"matrix blast") == 0))
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(ent);
		return;
	}

	// calculate position for the explosion entity

	VectorMA (ent->s.origin, -0.02, ent->velocity, origin);

	if (other && other->takedamage)
	{
		if (IsHeadShot(other,ent->s.origin))
			damage = (ent->dmg)<<1; // double
		else
			damage = ent->dmg;

		T_Damage (other, ent, ent->owner, ent->velocity, ent->s.origin, plane->normal, damage, 10, 0, MOD_ROCKET_SPEAR);        // RSP 070999
	}

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_R_SPLASH);

	// RSP 062900 - Use generic explosion routine

	if (ent->waterlevel)
		SpawnExplosion(ent,TE_ROCKET_EXPLOSION_WATER,MULTICAST_PHS,EXPLODE_ROCKETSPEAR_WATER);
	else
		SpawnExplosion(ent,TE_ROCKET_EXPLOSION,MULTICAST_PHS,EXPLODE_ROCKETSPEAR);

	G_FreeEdict(ent);

	// If the other entity is a projectile, remove it in the explosion

	if (other && Is_Projectile(other))
		G_FreeEdict(other);
}

// MRB 031899
edict_t *spear_impale(edict_t *flying_spear, qboolean is_normalspear)	// RSP 081500
{
	gitem_t *item;
	edict_t *spear;

	// Generate the 'item' copy
	// RSP 081500 - allow for generating rocket spears
	if (is_normalspear)
		item = FindItemByClassname("item_normal_spear");
	else
		item = FindItemByClassname("item_rocket_spear");

	spear = Drop_Item(flying_spear,item);

	// Now fix up some stuff Drop does that we don't like
	spear->s.renderfx = 0;
	spear->gravity = 0;
	VectorClear(spear->velocity);
	VectorCopy(flying_spear->s.angles,spear->s.angles);
	spear->owner = NULL;	// RSP 061899 - owner's about to go away

	// Free the original
	G_FreeEdict(flying_spear);

	return spear;
}

// MRB 032799
void spear_throw_backwards(edict_t *spear)
{
	vec3_t v;
	
	// throw it a bit
	spear->movetype = MOVETYPE_TOSS;

	// RSP 071800 - take on water gravity if underwater

	if (gi.pointcontents(spear->s.origin) & MASK_WATER)
		spear->gravity = 0.05;
	else
		spear->gravity = 1;

	// Push it a bit ('backwards' and up)
	AngleVectors(spear->s.angles,v,NULL,NULL);
	VectorInverse(v);
	v[2] = 0;	// RSP 071800 - not up
	
	VectorScale(v,60,spear->velocity);
	// Change the lower bound so they don't 'float' as badly
	VectorSet(spear->mins,-16,-16,-2);

	// Make them lie flat
	spear->s.angles[PITCH] = 0;

	// And stop the thinking.
	spear->locked_to = NULL;
	spear->prethink = NULL;
	spear->think = NULL;
	spear->nextthink = 0;
	spear->touch = Touch_Item;

	gi.linkentity(spear);
}

// MRB 031899
void spear_stuck_in_something(edict_t *spear)
{
	edict_t *anchor = spear->locked_to;	// RSP 072400

	if (!anchor ||
		(anchor->solid == SOLID_NOT) ||
		(anchor->health <= 0) ||	// RSP 082700 - leave when host is dead
		!Q_stricmp(anchor->classname, "freed"))
	{
		spear_throw_backwards(spear);
	}
	else   // WPO 100599  Remove spears from dead players in deathmatch
	if (deathmatch->value && !anchor->inuse) 
		spear_throw_backwards(spear);
	else
		spear->nextthink = level.time + FRAMETIME;
}


void spear_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *spear;	// MRB 031899

	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT); // monsters can hear

	// RSP 070899 - Is this a headshot?

	if (other->takedamage)
	{
		int damage;

		if (IsHeadShot(other,self->s.origin))
			damage = (self->dmg)<<1;	// double
		else
			damage = self->dmg;

		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, damage, 1, DAMAGE_ENERGY, MOD_SPEARGUN);	// RSP 070899
	}

	/* MRB 031398 - some spear stuff
	 *   The spear is Freed and becomes simply an item when it impacts something.
	 *   If that something is solid, it sits,  If that something is damagable,
	 *   it locks to it and awaits it's death - at which point it will be freed.
	 */

	if (other == world)
	{	// Prevent the engine from calling touch because the world is touching it ...
		spear = spear_impale(self,true);	// RSP 081500
		spear->groundentity = world;
	}

	// RSP 080100 - disallow spears stuck in beetles
	// RSP 100700 - disallow spears stuck in monsters with active armor
	else if (!other->takedamage					||
			(other->movetype == MOVETYPE_PUSH)	||
			(other->identity == ID_BEETLE)		||
			(other->monsterinfo.power_armor_power))
	{	// just drop to the ground (move backwards)- I'm worried what we might get stuck in.
		/* MRB 032799 - if this is a plat or something, drop to the floor.
		 * FIXME - I would like to do the following, but it would get 
		 * confused by drake's trick with the bathysphere ...
		 *  1) upon hitting a plat, thread the spear into the plat's teamchain string
		 *  2) Give it spear->blocked function that calls spear_throw_backwards()
		 */
		spear = spear_impale(self,true);	// RSP 081500
		spear_throw_backwards(spear);
	}
	else
	{	// We hit something other than the world

		// ... let's try and get it to look better when impaled.
		vec3_t mid_other, to_other;
		float dist;

		// find perpendicular distance to the 'center' 
		VectorMA(other->absmin,0.5,other->size,mid_other);	
		VectorSubtract(mid_other,self->s.origin,to_other);	// to enemy 'center'
		dist = VectorNormalize(to_other);

		VectorNormalize(to_other);
		vectoangles(to_other,self->s.angles);

		// MRB 032799 - just move origin onto the target's center.
		VectorCopy(mid_other, self->s.origin);

		spear = spear_impale(self,true);	// RSP 081500

		/* MRB 032799 - don't bother with this just now since they are not
		 * retrievable out of the impalee 
		if (dist < 4)
			spear->s.frame = 0;
		else
			if ((spear->s.frame = (int)(dist / 5)) > 5)
				spear->s.frame = 5;
		*/

		// NOTE that this sets a prethink function that aligns the spear with the target
		_LockToEnt(spear,other);

		spear->solid = SOLID_TRIGGER;
		spear->think = spear_stuck_in_something;
		spear->nextthink = level.time + FRAMETIME;
		spear->touch = NULL;		// MRB 031699 - DON'T allow them to retrieve spears from a living baddie

		/* FIXME:  Allow player to retrieve the spears, so don't make them disappear.
		 * Player has to USE spears that are embedded in something other than a 
		 * wall in order to retrieve them.  Issue now is that USE means 'use an item'
		 * and not 'push a button' like it used to.
		 */
	}
	gi.linkentity(spear);
}


// MRB 031399 - simple check to see if I should 'fall' because I'm out of the water
void spear_checkenv(edict_t *self)
{
	if (self->pain_debounce_time != 0)
	{	// still flying 'flat'
		self->gravity = 0;

		if (level.time > self->pain_debounce_time)
		{	// remove the initial 'push' so it doesn't continue to climb underwater
			self->pain_debounce_time = 0;
//			VectorSubtract(self->velocity,self->pos1,self->velocity);	// RSP 121899
//			VectorClear(self->pos1);									// RSP 121899
		}
	}
	else
	{
		// RSP 051399 - If a normal spear, gravity takes over in air.
		// Otherwise it's a rocket spear, and just continues to fly straight.

		if (Q_stricmp(self->classname,"spear") == 0)
		{	// Gravity takes over
			if (self->waterlevel == WL_DRY)
			{
				self->gravity = 0.1;	// out of water	// RSP 121899
				_AlignDirVelocity(self);// align to velocity
			}
			else
				self->gravity = 0;		// underwater
		}
	}
}


void launch_spear (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t	*spear;
	trace_t	tr;

	VectorNormalize (dir);

	spear = G_Spawn();
	VectorCopy (start, spear->s.origin);
	VectorCopy (start, spear->s.old_origin);
	vectoangles (dir, spear->s.angles);
	VectorScale (dir, speed, spear->velocity);

	// RSP 121899 - slight arc if a normal spear
	//            - better accuracy hitting sight
	if (self->client->pers.ammo_type == AMMO_TYPE_NORMAL_SPEAR)
	{	// MRB 032799 - slight upwards adjustment for spear trajectory. 
		vec3_t up;
		vec3_t right;	// RSP 121899

		AngleVectors(spear->s.angles,NULL,right,up);	// RSP 121899
		VectorMA(vec3_origin,40,up,spear->pos1);		// RSP 121899
		VectorMA(vec3_origin,-10,right,spear->pos2);	// RSP 121899
		VectorAdd(spear->velocity,spear->pos1,spear->velocity);	// save it for later ..
		VectorAdd(spear->velocity,spear->pos2,spear->velocity);	// save it for later ..

		//  set '~straight until' timer.
		spear->pain_debounce_time = level.time + (200 / speed);
	}

	spear->gravity = 0;					// RSP 121899
	spear->movetype = MOVETYPE_TOSS;	// MRB 031399 - use TOSS so gravity will affect me

	spear->clipmask = MASK_SHOT;
	spear->solid = SOLID_BBOX;
//	spear->s.renderfx |= RF_BEAM;		// MRB 031399
	VectorClear (spear->mins);
	VectorClear (spear->maxs);

	spear->owner = self; // set temporarily so engine will recognize if the spear
						 // is being spawned through a surface
	spear->prethink = spear_checkenv;	// MRB 031399 - check if in water each frame

	// MRB 032799 - check water level at this time - prevents it from angling down at the start
	if (gi.pointcontents (start) & MASK_WATER)
		spear->waterlevel = WL_FEETWET;

	// MRB 031399 - removed self-destructive tendancies as a test
	//spear->nextthink = level.time + 2;
	//spear->think = G_FreeEdict;

	// RSP 050999 - damage values are now user variable

	if (self->client->pers.ammo_type == AMMO_TYPE_ROCKET_SPEAR)
	{	// rocket spear
		spear->s.sound = gi.soundindex ("weapons/speargun/rockspear.wav");	// MRB 022799
		spear->model = "models/objects/spears/rocket.md2";	// MRB 091699
		spear->s.modelindex = gi.modelindex (spear->model);	// MRB 091699

		spear->s.effects |= EF_ROCKET;

		spear->touch = rocketspear_touch;
		spear->dmg = wd_table[WD_ROCKET_SPEAR];
		spear->radius_dmg = wd_table[WD_ROCKET_SPEAR_BLAST];

		spear->dmg_radius = 2*spear->radius_dmg;
		spear->classname = "rocket_spear";
	}
	else	// normal spear
	{
		spear->s.sound = gi.soundindex ("weapons/speargun/spear.wav");	// MRB 022799
		spear->model = "models/objects/spears/normal.md2";	// RSP 061899
		spear->s.modelindex = gi.modelindex (spear->model);	// RSP 061899

		spear->s.effects |= EF_IONRIPPER;	// RSP 031499 3.20 removed EF_BOOMER, but EF_IONRIPPER
											// is new and has the same value
		spear->touch = spear_touch;
		spear->dmg = damage;
		spear->classname = "spear";
	}

	// Add correct sound for firing and reloading spear gun
	// RSP 022499: Added check for under water or not, to change sound
	// WPO 042800: Same sound in air or water
	gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/speargun/firespear.wav"), 1, ATTN_NORM, 0);	// MRB 022799
	
	if (self->client)
		check_dodge (self, spear->s.origin, dir, speed);

	spear->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger

	gi.linkentity (spear);

	// See if there's something between the player and the spear spawn spot

	tr = gi.trace (self->s.origin, NULL, NULL, spear->s.origin, spear, MASK_SHOT);

	if (tr.fraction < 1.0) // if something between the player and the spear spawn spot 
	{
		VectorMA (spear->s.origin, -10, dir, spear->s.origin); // Back up 10 units

		// RSP 051399 - add alternate code for spawning a rocket spear real close
		// to something.

		if (self->client->pers.ammo_type == AMMO_TYPE_NORMAL_SPEAR)
		{
			if (tr.ent->takedamage) // If it's a monster, hurt it
				T_Damage (tr.ent, spear, spear->owner, spear->velocity, spear->s.origin,
					tr.plane.normal, spear->dmg, 1, DAMAGE_ENERGY, MOD_SPEARGUN);
			else	// MRB 031699
			{	// stick the spear to the ground
				gitem_t *item = FindItemByClassname ("item_normal_spear");
				edict_t *e = Drop_Item(spear,item);

				// Now fix up some stuff Drop does that we don't like
				e->s.renderfx = 0;
				e->gravity = 0;
				VectorClear(e->velocity);
				VectorCopy(spear->s.angles,e->s.angles);
			}
		}
		else	// Rocket spear
		{
			if (tr.ent->takedamage) // If it's a monster, hurt it
			{
				T_Damage (tr.ent, spear, spear->owner, spear->velocity, spear->s.origin,
					tr.plane.normal, spear->dmg, 1, DAMAGE_ENERGY, MOD_ROCKET_SPEAR);
			}
			else // It's a wall, so hurt the player
			{
				// Reduce damage to player
				T_Damage (self, spear, spear->owner, spear->velocity, spear->s.origin,
					tr.plane.normal, 5, 1, DAMAGE_ENERGY, MOD_ROCKET_SPEAR);
			}
		}

		G_FreeEdict (spear); // remove spear from world
	}	
}	

void speargun_fire (edict_t *ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	//  WPO 231199 LUC has limited quad now
	if (is_quad)
		damage *= 4;
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-8);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	launch_spear (ent, start, forward, damage, 1000, effect);

	// RSP 062400 - flash not needed
	// send muzzle flash
//	gi.WriteByte (svc_muzzleflash);
//	gi.WriteShort (ent-g_edicts);
//	gi.WriteByte (MZ_BLASTER | MZ_SILENCED);
//	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index] -= 1;
}


void weapon_speargun_fire (edict_t *ent)
{
	int damage;

	if (deathmatch->value)
		damage = wd_table[WD_SPEARGUN] + 5;	// RSP 032399
	else
		damage = wd_table[WD_SPEARGUN];		// RSP 032399
	// RSP 041699 - change the randomness to 0.8->1.2 per Drake
	damage *= 0.8;
	damage += (damage * (random() * 0.5));

	speargun_fire (ent, vec3_origin, damage,0);
	ent->client->ps.gunframe++;
}

void Weapon_Speargun (edict_t *ent)
{
	// MRB 011198 - new frames to reflect new v_speargun model
	static int pause_frames[] = {22, 24, 0};
	static int fire_frames[]  = {8, 0};

	Weapon_Generic (ent, 7, 20, 25, 31, pause_frames, fire_frames, weapon_speargun_fire);
}

//=================
//
//  Diskgun
//
//  RSP 121298
//
//=================

void disk_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)	// RSP 070900 - bots can fire disks also
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT); // monsters can hear

	// RSP 032399: Added blast force when hitting something
	// If this is a direct hit, no blast damage is done to the receiver (other)
	// RSP 102399 - changed self->owner to self->dlockOwner for frag purposes

	T_RadiusDamage(self, self->dlockOwner, self->radius_dmg, other, self->dmg_radius, MOD_DISKGUN);

	if (other->takedamage)
	{
		int damage;

		if (IsHeadShot(other,self->s.origin))	// RSP 070899 - added headshot check
			damage = (self->dmg)<<1;			// double
		else
			damage = self->dmg;

		// RSP 102399 - changed self->owner to self->dlockOwner for frag purposes

		T_Damage (other, self, self->dlockOwner, self->velocity, self->s.origin, plane->normal, damage, 1, DAMAGE_ENERGY, MOD_DISKGUN);	// RSP 070899

		G_FreeEdict (self);
	}
	else
	{
		// issue ricochet sound
		// RSP 070300 - removed, since TE_SHIELD_SPARKS uses the lashit sound
//		gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/diskgun/diskhit.wav"), 1, ATTN_NORM, 0);	// MRB 022799

		// create sparks

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_SHIELD_SPARKS);
		gi.WritePosition (self->s.origin);

		// Need to check for existence of plane

		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);

		gi.multicast (self->s.origin, MULTICAST_PVS);

		self->owner = self;			// RSP 011199: Set so disk can hurt player

		self->radius_dmg *= 0.8;	// RSP 032399 - reduce radial damage w/each ricochet
		self->dmg_radius *= 0.8;	// These 2 go hand-in-hand
	}
}

void launch_disk (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t	*disk;
	trace_t	tr;

	VectorNormalize (dir);

	disk = G_Spawn();
	VectorCopy (start, disk->s.origin);
	VectorCopy (start, disk->s.old_origin);
	vectoangles (dir, disk->s.angles);
	VectorScale (dir, speed, disk->velocity);
	disk->movetype = MOVETYPE_RICOCHET;
	disk->clipmask = MASK_SHOT;
	disk->solid = SOLID_BBOX;
	disk->s.effects |= effect;
	VectorClear (disk->mins);
	VectorClear (disk->maxs);

	disk->model = "models/objects/disk/tris.md2";		// RSP 061899
	disk->s.modelindex = gi.modelindex (disk->model);	// RSP 061899

	disk->s.sound = gi.soundindex ("weapons/diskgun/diskfly.wav");	// MRB 022799
	disk->owner = self; // set temporarily so engine will recognize if the disk
						// is being spawned through a surface
	disk->dlockOwner = self;	// RSP 102399 - allow player to get a frag from a
								// ricocheting disk; this has nothing to do with the
								// dreadlock. we're just using the field for a
								// different purpose.
	disk->touch = disk_touch;
	disk->nextthink = level.time + 2;
	disk->think = G_FreeEdict;
	disk->dmg = damage;
	disk->classname = "disk";

	// RSP 032399 - add radius damage
	disk->radius_dmg = wd_table[WD_DISKGUNBLAST];	// RSP 032699 - user variable
	disk->dmg_radius = 2*disk->radius_dmg;

	// MRB 022299 - add energy trail to disk
	disk->s.frame = 0;
	disk->s.effects |= (EF_FLAG1|EF_SPHERETRANS); // MRB 032799 - thought SPHERETRANS looked cool

	// Add correct sound for firing and reloading disk gun

	gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/diskgun/diskfire.wav"), 1, ATTN_NORM, 0);	// MRB 022799

	if (self->client)
		check_dodge (self, disk->s.origin, dir, speed);

	disk->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger

	gi.linkentity (disk);

	// See if there's something between the player and the disk spawn spot

	tr = gi.trace (self->s.origin, NULL, NULL, disk->s.origin, disk, MASK_SHOT);

//	disk->owner = disk; // RSP 011199: Moved to disk_touch so player isn't hurt when firing
	                    // up at a 45 degree angle

	if (tr.fraction < 1.0) // if something between the player and the disk spawn spot 
	{
		VectorMA (disk->s.origin, -10, dir, disk->s.origin); // Back up 10 units

		if (tr.ent->takedamage) // If it's a monster, hurt it
			T_Damage (tr.ent, disk, disk->owner, disk->velocity, disk->s.origin,
				tr.plane.normal, disk->dmg, 1, DAMAGE_ENERGY, MOD_DISKGUN);
		else // It's a wall, so hurt the player
			// RSP 010399 Reduce damage to player
			T_Damage (self, disk, disk->owner, disk->velocity, disk->s.origin,
				tr.plane.normal, 5, 1, DAMAGE_ENERGY, MOD_DISKGUN);
		
		G_FreeEdict (disk); // RSP 010399 remove disk from world
	}
}	

void diskgun_fire (edict_t *ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	//  WPO 231199 LUC has limited quad now
	if (is_quad)
		damage *= 4;
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 6, ent->viewheight-10); // RSP 010399: reduced model, so spawn point changed
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	launch_disk (ent, start, forward, damage, 1000, effect);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash2);	// RSP 062400 - kill normal blaster sound
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BLASTER);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index] -= 1;
}


void weapon_diskgun_fire (edict_t *ent)
{
	int damage;

	if (deathmatch->value)
		damage = 1.5 * wd_table[WD_DISKGUN];	// RSP 032399: greater damage
	else
		damage = wd_table[WD_DISKGUN];			// RSP 032399: greater damage
	diskgun_fire (ent, vec3_origin, damage, 0);
	ent->client->ps.gunframe++;
}

// RSP 122498: Added vweap, so the frame numbers have changed

void Weapon_DiskGun (edict_t *ent)
{
	static int pause_frames[] = {16, 21, 26, 0};
	static int fire_frames[]  = {8, 0};

	Weapon_Generic (ent, 7, 15, 31, 34, pause_frames, fire_frames, weapon_diskgun_fire);
}

//=================
//
//  Plasmawad
//
//=================

#define PLASMA_DIM			2		// MRB 022899
#define PLASMA_MAX_SPEED	800

#define PLASMA_FRAME_ROUND	0
#define PLASMA_FRAME_FLY	1
#define PLASMA_FRAME_STICK	2
#define PLASMA_FRAME_LAUNCH	3
#define PLASMA_ATTACK_20	4
#define PLASMA_ATTACK_60	8

extern void SV_AddGravity (edict_t *ent);

static int plasma_fly;		// MRB 022799
static int plasma_burning;	// MRB 022799

void plasma_burn(edict_t *ent, edict_t *targ)
{
	edict_t *target = (targ ? targ : ent->enemy);
	vec3_t  dir, dirNeg;
	
	VectorSubtract(target->s.origin, ent->s.origin,dir);
	VectorNegate(dir, dirNeg);

//	gi.sound(ent, CHAN_VOICE, plasma_burning, 1, ATTN_NORM, 0);	// MRB 022799

	T_Damage (target, ent, (ent->owner ? ent->owner : ent) , dir, ent->s.origin, 
			   dirNeg, ent->dmg, 0, 0, MOD_PLASMAWAD);
}

void plasma_die(edict_t *ent)
{
}

// MRB 032799
// call this when you hit a door or something
void plasma_droptoground(edict_t *plasma)
{
	gitem_t *item = FindItemByClassname ("item_plasma_ball");
	edict_t *new_plasma = Drop_Item(plasma,item);

	// Reverse direction
	new_plasma->velocity[0] *= (-1);
	new_plasma->velocity[1] *= (-1);

	// Kill glow
	new_plasma->s.renderfx = 0;
	new_plasma->gravity = 0;

	// Set the amount of ammo I contain to be based on the amount of time
	// I had left to burn  [1 ent->count = 0.1 seconds] (0-15 for player, 0-12 for monster)
	new_plasma->count = (int)(plasma->count / 10);
	if (new_plasma->count == 0)
		new_plasma->count = 1;
	else if (new_plasma->count > 10)
		new_plasma->count = 10;

	// Free the original
	G_FreeEdict(plasma);
}

void plasma_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean fired_by_client = false;	// RSP 030599

	// MRB 032799 - I think this was a major problem - touch is called not only
	//  when 2 things touch, but when they are TOUCHING as well.  So - w/o this 
	//  check - plasma was burning twice per turn.
	if (ent->locked_to)
		return;

	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	// WPO 042800 Stop sound when plasma stops flying
	ent->s.sound = 0;

	if (ent->owner && ent->owner->client)
	{
		fired_by_client = true;	// RSP 030599
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);
	}

	if (other == world)
	{	// World - lock in place and wait
		ent->locked_to = other;
		ent->prethink = NULL;
		ent->s.frame = PLASMA_FRAME_STICK;

		VectorCopy(vec3_origin,ent->velocity);
		VectorCopy(vec3_origin,ent->avelocity);

//		vectoangles(plane->normal,ent->s.angles);	// RSP 030599 kept ball from seeking monsters
		VectorMA(ent->s.origin,2,plane->normal,ent->s.origin);	// RSP 030599 Change 10 to 2
	}
	else if (other->movetype == MOVETYPE_PUSH)
	{	// MRB 032799 - this is a bit of a nightmare - doors are bad things to
		// stick to - try reversing direction
		ClipVelocity(ent->velocity,plane->normal,ent->velocity,1);
	}
	else if (other->takedamage != DAMAGE_NO)
	{
		// RSP 030599 If fired by the player, lock and burn
		// If fired by a monster, evaporate
		if (fired_by_client || !(other->svflags & SVF_MONSTER))	// RSP 030599
		{
			// something I can hurt!  Lock in place and BURN!
			vec3_t v, other_center;

			// RSP 071800 - Change sound when plasma starts burning
			ent->s.sound = plasma_burning;

			ent->enemy = other;
			VectorMA (other->absmin, 0.5, other->size, other_center);

			VectorSubtract(other_center,ent->s.origin,v);

			// frames 4,5,6,7,8 correspond to attack distances of 20,30,40,50 and 60 units
			// advance until we have a proper length
			ent->random = (int)(VectorLength(v) / 10);
			if (ent->random < 2)
				ent->random = 2;
			else if (ent->random > 6)
				ent->random = 6;

			(ent->random) += 2;

			ent->s.frame = PLASMA_ATTACK_20;

			// aim toward center of target
			vectoangles(v,ent->s.angles);

			_LockToEnt(ent,other);
		}
		else
		{	//MRB 031599 - drop to the ground as a 'gettable' item
			plasma_droptoground(ent);
		}
	}

	gi.linkentity(ent);
}

void plasma_think (edict_t *self)
{
	edict_t	*target = NULL;
	float	dist;

	self->nextthink = level.time + FRAMETIME;
	self->s.skinnum = ++(self->s.skinnum) % 8;

	if ((level.time < self->powerarmor_time) && !self->locked_to && !self->enemy)
		SV_AddGravity(self); // using powerarmor_time for 'pauses' so the skin can always be changing

	if (--(self->count) < 0)
	{	// Out of time
		self->think = G_FreeEdict;
		SV_AddGravity(self);
		gi.linkentity(self);
		return;
	}

	if (self->locked_to)
	{	// BURN !!!
		if (self->locked_to->takedamage && !self->locked_to->deadflag)
		{
			if (self->s.frame < self->random)
				(self->s.frame)++;
			plasma_burn(self,self->locked_to);
			gi.linkentity(self);
			return;
		}
		else if (self->locked_to != world)
		{
			self->enemy = NULL;
			self->locked_to = NULL;
			self->s.sound = plasma_fly;	// RSP 071800 - enemy dead, plasma is flying again

		}
	}

	if (self->enemy && !visible(self,self->enemy))
		 self->enemy = NULL;

	if (!self->enemy &&
		((self->locked_to == NULL) || (self->locked_to == world)))
	{	// just flying along
		edict_t	*requester = NULL;
		edict_t	*best_cli = NULL;
		float	best_cli_dist = self->radius + 1;
		edict_t	*best_mon = NULL;
		float	best_mon_dist = self->radius + 1;
		qboolean target_clients, target_monsters;

		if (deathmatch->value || !self->owner->client)	// MRB 030599
		{	// deathmatch game or fired by a monster - want onlyclients
			target_clients = true;
			target_monsters = false;
		}
		else
		{
			target_clients = true;
			target_monsters = true;
		}
		while (target = _findlifeinradius(self->owner, target, self->s.origin, self->radius,
										 target_clients,target_monsters,false,
										 &dist) )
		{
			if (self == target 	// RSP 030599 Don't target yourself
				|| (Q_stricmp (target->classname, "plasma") == 0))	//MRB 032799 - or other plasmaballs
				continue;

			if (target != self->owner && visible(self,target))
			{	// potential target != whatever fired me 
				//   (change so it DOES hurt me, but only after a certaim amount of time - like rockets)
				if (target->client && dist < best_cli_dist)
				{
					best_cli = target;
					best_cli_dist = dist;
					dist = 0;
					continue;
				}

				if (((target->svflags & SVF_MONSTER) || (target->identity == ID_LITTLEBOY))	// RSP 050999 - allow plasma on littleboy
					&& (dist < best_mon_dist))
				{
					best_mon = target;
					best_mon_dist = dist;
					dist = 0;
					continue;
				}
			}
		}

		if (self->owner->client && !deathmatch->value)
		{
			if (best_mon)
				target = best_mon, dist = best_mon_dist;
			else
				target = best_cli, dist = best_cli_dist;
		}
		else
		{
			if (best_mon_dist < best_cli_dist)
				target = best_mon, dist = best_mon_dist;
			else
				target = best_cli, dist = best_cli_dist;
		}

		self->enemy = target;
	}

	if (self->enemy)
	{	// start moving toward the target by changing my velocity
		vec3_t dir,v;
		float d = PLASMA_MAX_SPEED * 0.2 / self->radius;
		int i = 0;

		if (self->locked_to == world)
		{	// push away from the world on the first frame
			vec3_t fwd;
			AngleVectors(self->s.angles,fwd,NULL,NULL);
			VectorMA(self->s.origin,10,fwd,self->s.origin);
			self->locked_to = NULL;
			self->solid = SOLID_BBOX;
			self->s.frame = PLASMA_FRAME_LAUNCH;
			self->s.sound = plasma_fly;	// RSP 071800 - flying sound again

			gi.linkentity(self);
		}
		else
			self->s.frame = PLASMA_FRAME_FLY;

		self->prethink = _AlignDirVelocity;

		VectorMA(self->enemy->absmin,0.5,self->enemy->size,v);
		VectorSubtract (v, self->s.origin, dir);
		//VectorNormalize(dir);

		for (i = 0 ; i < 3; i++)
		{
			self->velocity[i] += (dir[i] * d);
			if (self->velocity[i] > PLASMA_MAX_SPEED)
				self->velocity[i] = PLASMA_MAX_SPEED;
		}
	}
	else if (self->locked_to == NULL)
	{
		self->powerarmor_time = self->nextthink + FRAMETIME;
		SV_AddGravity(self);
	}
	
	gi.linkentity(self);
}

void launch_plasma (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float lifetime, int radius)
{
	edict_t	*plasma;

	plasma = G_Spawn();
	VectorCopy (start, plasma->s.origin);
	vectoangles (dir, plasma->s.angles);
	VectorScale (dir, speed, plasma->velocity);
	plasma->movetype = MOVETYPE_FLY;
	plasma->clipmask = MASK_SHOT;
	plasma->solid = SOLID_BBOX;
	plasma->s.effects |= EF_BLASTER;

	VectorSet(plasma->mins,-PLASMA_DIM,-PLASMA_DIM,-PLASMA_DIM);
	VectorSet(plasma->maxs,PLASMA_DIM,PLASMA_DIM,PLASMA_DIM);		// RSP 022799 (was mins)

	plasma->model = "models/objects/plasma/tris.md2";	// RSP 061899
	plasma->s.modelindex = gi.modelindex (plasma->model);	// RSP 061899

	self->s.frame = 1;
	plasma->owner = self;
	plasma->gravity = 0.5;
	
	if (!self->client && self->enemy)
		plasma->enemy = self->enemy;
	else
		plasma->enemy = NULL;

	plasma->dmg = damage;
	plasma->radius = radius;
	plasma->count = (int)(lifetime / FRAMETIME);

	plasma_fly = gi.soundindex ("weapons/plasma/plasfly.wav");		// MRB 022799
	plasma_burning = gi.soundindex ("weapons/plasma/plasburn.wav");	// MRB 022799
	plasma->s.sound = plasma_fly;

	plasma->classname = "plasma";

	plasma->s.frame = PLASMA_FRAME_FLY;
	plasma->s.skinnum = 0;

	plasma->touch = plasma_touch;
	plasma->think = plasma_think;
	plasma->prethink = _AlignDirVelocity;
	plasma->nextthink = level.time + FRAMETIME;
	plasma->powerarmor_time = plasma->nextthink + 0.5;// move for 1/2 second before starting to target
	plasma->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger

	gi.linkentity (plasma);
}


void Plasma_Fire (edict_t *ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 0, ent->viewheight-8);	// RSP 022799 adjusted gloop origin to fit muzzle
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -1;

	// MRB 022899 - noise for firing the plasmawad
	gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/plasma/plasfire.wav"), 1, ATTN_NORM, 0);	// RSP 063000
	ent->client->weapon_sound = 0;	// RSP 071800

	if (ent->client)
		launch_plasma(ent,start,forward,damage,(75 * ent->client->grenade_time),5+ent->client->grenade_time,128);
	else
		launch_plasma(ent,start,forward,damage,500,12,128);
	
	// send muzzle flash
	gi.WriteByte (svc_muzzleflash2);	// RSP 062400 - kill the normal blaster sound
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BLASTER);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}

#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)

void Weapon_PlasmaWad (edict_t *ent)
{
	int FRAME_ACTIVATE_LAST		= 7;
	int FRAME_FIRE_LAST			= 17;
	int FRAME_IDLE_LAST			= 22;
	int FRAME_DEACTIVATE_LAST	= 27;

	if (ent->client->weaponstate == WEAPON_DROPPING)
	{
		if (ent->client->ps.gunframe == FRAME_DEACTIVATE_LAST)
		{
			ChangeWeapon (ent);
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		if (ent->client->ps.gunframe == FRAME_ACTIVATE_LAST)
		{
			ent->client->weaponstate = WEAPON_READY;
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		ent->client->ps.gunframe++;
		return;
	}

	if ((ent->client->newweapon) && (ent->client->weaponstate != WEAPON_FIRING))
	{
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{	// Shift to firing frames and clear out 'grenade time'
				ent->client->ps.gunframe = FRAME_FIRE_FIRST;
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;

				// Subtract one unit of plasma for this first one // MRB 031899
				if (!((int)dmflags->value & DF_INFINITE_AMMO))
					ent->client->pers.inventory[ent->client->ammo_index]--;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

		// Deal with 'idle' sequence
		if (ent->client->ps.gunframe == FRAME_IDLE_LAST)
		{
			ent->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}
		ent->client->ps.gunframe++;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{	// Firing - should setup increasingly louder sound
		if (ent->client->ps.gunframe == FRAME_FIRE_FIRST)
		{
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/plasma/plaschrg.wav"), 1, ATTN_NORM, 0);	// MRB 022799
			ent->client->grenade_time++;
			ent->client->ps.gunframe++;

			// Subtract one unit of plasma (this sort of gives the player a free one,
			// but it looked odd subtracting 1 at firing time ...
			if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
				if (--ent->client->pers.inventory[ent->client->ammo_index] < 0)
					ent->client->pers.inventory[ent->client->ammo_index] = 0;
		}
		else if (ent->client->pers.inventory[ent->client->ammo_index] == 0 ||
			 ent->client->ps.gunframe == FRAME_FIRE_LAST -1)	// MRB 031899
		{	// Make some sort of noise (sustained hummmm) - need to get this to repeat well & do only once
			//gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/plasma/plasfull.wav"), 1, ATTN_NORM, 0);	// MRB 022799
			// RSP 071800 - Added
			ent->client->weapon_sound = gi.soundindex("weapons/plasma/plasma_hum.wav");
		}
		else // Increase charged time
		{
			// Subtract one unit of plasma // MRB 031899
			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->ammo_index]--;
			ent->client->grenade_time++;
			ent->client->ps.gunframe++;
		}

		// If fire button is still down, just return
		if (ent->client->buttons & BUTTON_ATTACK)
			return;

		// Ooo!  Fire away!
		{
			int damage;

			if (deathmatch->value)
				damage = 2 * wd_table[WD_PLASMAWAD];	// RSP 032399
			else
				damage = wd_table[WD_PLASMAWAD];		// RSP 032399
			Plasma_Fire (ent, vec3_origin, damage, EF_BLASTER);
		}

		ent->client->grenade_time = 0;
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = FRAME_IDLE_FIRST;
	}
}

//=================
//
//  Boltgun
//
// RSP 051799 - Put back machinegun as a boltgun and modify for LUC.
//				This gun will jam when it heats up.
//
//=================

// heat_level is the probability that the boltgun will fire at a given
// temperature level. There are 7 temperature levels. bolt_heat holds
// the temperature level.

float heat_level[] = {1, 1, 0.9, 0.8, 0.6, 0.4, 0};

void weapon_boltgun_fire (edict_t *ent)
{
	int     i;
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	angles;
	int     damage = wd_table[WD_BOLTGUN];
	int     kick = 2;
	vec3_t	offset;
	int     slide_position;
	int     shots;

	//  WPO 231199 LUC has limited quad now
	if (is_quad)
		damage *= 4;
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;
  
	if (!(ent->client->buttons & BUTTON_ATTACK))
	{
		if ((ent->client->ps.gunframe >= 4) && (ent->client->ps.gunframe <= 19))
		{	// changing from firing state to idle state
			ent->client->ps.gunframe = 20;	// RSP 102299
			ent->client->machinegun_shots = 4;	// reset to middle of "bow"
		}
		else
			ent->client->ps.gunframe++;
		return;
	}

	// bolt_heat is used as a measure of heat buildup

	ent->client->pers.bolt_heat += FRAMETIME;
	if (ent->client->pers.bolt_heat > 7.0)
		ent->client->pers.bolt_heat = 7.0;	// max out

	// Use machinegun_shots to determine where the bolt origin will be, offset from the
	// the leftmost firing position. There is a slider in the front of the gun that 
	// slides left to right and back. The bolt fires from the slider's position as it
	// moves.

	ent->client->machinegun_shots++;
	shots = ent->client->machinegun_shots & 15;

	// shots is now a number from 0 to 15. Add 4 to it and you get the
	// gunframe number.

	// RSP 102299 - change to correct firing frames
	// Frames 4->12 are traveling right. Frames 13->19 are
	// traveling left.

	ent->client->ps.gunframe = shots + 4;

	if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
	{	// out of ammo
		if (level.time >= ent->pain_debounce_time)
		{
			// make one out of ammo sound per second
			gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
			ent->pain_debounce_time = level.time + 1;
		}
		NoAmmoWeaponChange (ent);
		return;
	}

	// At this point, we decide if we're going to allow the shot, based on heat buildup
	// in the gun. bolt_heat represents the level of heat buildup.

	if (random() > heat_level[(int) ent->client->pers.bolt_heat])
		return;

	// Random kick to the gun
	for (i = 1 ; i < 3 ; i++)
	{
		ent->client->kick_origin[i] = crandom() * 0.35;
		ent->client->kick_angles[i] = crandom() * 0.7;
	}
	ent->client->kick_origin[0] = crandom() * 0.35;

	// slide_position is measured from the left-hand side. There are a total of
	// 9 positions: -8,-4,0,4,8,12,16,20,24 units from the left. As machinegun_shots
	// increments from 0 through 8, the slide is moving from positions -8 through 24.
	// As machinegun_shots increments from 9 through 15, the slide is moving from
	// positions 20 back to -4. Since the behavior is different (first traveling left,
	// then traveling right), we split the math into two calculations.

	if (shots <= 8)	// traveling right
		slide_position = (shots<<2) - 8;
	else			// travelling left
		slide_position = 56 - (shots<<2);

	// get start / end positions
	VectorAdd (ent->client->v_angle, ent->client->kick_angles, angles);
	AngleVectors (angles, forward, right, NULL);
	VectorSet(offset, 24, slide_position, ent->viewheight-8);	// RSP 090200 - changed forward 0->24
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	fire_bullet (ent, start, forward, damage, kick, 0, 200, MOD_BOLTGUN);
//	fire_bullet (ent, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_BOLTGUN);

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_MACHINEGUN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (! ( (int)dmflags->value & DF_INFINITE_AMMO ) )
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->anim_priority = ANIM_ATTACK;
	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->s.frame = FRAME_crattak1 - (int) (random()+0.25);
		ent->client->anim_end = FRAME_crattak9;
	}
	else
	{
		ent->s.frame = FRAME_attack1 - (int) (random()+0.25);
		ent->client->anim_end = FRAME_attack8;
	}
}

void Weapon_Boltgun (edict_t *ent)
{	// RSP 102299 - set to correct frames
	static int pause_frames[] = {22,26,0};
	static int fire_frames[]  = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,0};

	Weapon_Generic (ent, 3, 19, 29, 33, pause_frames, fire_frames, weapon_boltgun_fire);
}

//=================
//
//  LUC Shotgun
//
//  WPO 021199
//
//=================

void weapon_lucshotgun_fire (edict_t *ent)
{
	vec3_t start;
	vec3_t forward, right;
	vec3_t offset;
	vec3_t v;
	int    damage = 6;
	int    kick = 12;

	//  WPO 231199 LUC has limited quad now
	if (is_quad)
		damage *= 4;
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;
  
	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorScale (forward, -2, ent->client->kick_origin);
	ent->client->kick_angles[0] = -2;

	VectorSet(offset,24,8,ent->viewheight-8);	// RSP 090200 - changed forward 0->24
	P_ProjectSource(ent->client,ent->s.origin,offset,forward,right,start);

	v[PITCH] = ent->client->v_angle[PITCH];
	v[YAW]   = ent->client->v_angle[YAW] - 5;
	v[ROLL]  = ent->client->v_angle[ROLL];
	AngleVectors (v, forward, NULL, NULL);
	fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);
	v[YAW]   = ent->client->v_angle[YAW] + 5;
	AngleVectors (v, forward, NULL, NULL);
	fire_shotgun (ent, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT/2, MOD_SSHOTGUN);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_SSHOTGUN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index] -= 2;
}

void Weapon_LUCShotgun (edict_t *ent)
{
	static int pause_frames[] = {29, 42, 57, 0};
	static int fire_frames[]  = {7, 0};

	Weapon_Generic (ent, 6, 17, 57, 61, pause_frames, fire_frames, weapon_lucshotgun_fire);
}

//=================
//
//  Teleportgun
//
//  WPO 231099
//
//  RSP 083100 - this gun picks up the target and stores it w/in the gun. The next time the gun
//  is fired, the target is teleported to the farthest point along the line-of-sight where it
//  fits. (Like the warp powerup.) 
//
//=================

qboolean fire_teleport(edict_t *self, vec3_t start, vec3_t aimdir)
{
	vec3_t		end;
	trace_t		tr;
	edict_t		*target;
	edict_t		*victim = self->teleportgun_victim;
	float		victim_size;
	vec3_t		v;

	// RSP 083100 - changed behavior of this gun
	// If teleportgun_victim is not zero, then we're holding a victim
	// in storage, and are now launching him.
	if (victim)
	{
		// Launch the stored entity

		VectorMA(start, 8192, aimdir, end);
		tr = gi.trace(start,NULL,NULL,end,self,MASK_SHOT);
		if ((tr.ent && ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->identity == ID_LITTLEBOY))))
		{	// smacked into another monster
			VectorCopy(tr.ent->s.origin,victim->s.origin);
			if (victim->identity == ID_GUARDBOT)		// Allow for guardbot vertical movement
				VectorCopy(victim->s.origin,victim->pos1);
			gi.linkentity(victim);

			// kill the other monster
			T_Damage(tr.ent,victim,self,vec3_origin,tr.ent->s.origin,vec3_origin,100000,0,DAMAGE_NO_PROTECTION,MOD_TELEFRAG);

			// Now kill the launched monster
			T_Damage(victim,victim,self,vec3_origin,victim->s.origin,vec3_origin,100000,0,DAMAGE_NO_PROTECTION,MOD_TELEFRAG);
		}
		else
		{	// smacked into something other than a monster

			// if sky, remove monster
			if (tr.surface && (tr.surface->flags & SURF_SKY))
			{
				if (!(victim->monsterinfo.aiflags & AI_GOOD_GUY))
				{
					level.killed_monsters++;
					if (coop->value)
						self->client->resp.score++;
				}
				G_FreeEdict(victim);
			}
			else
			{	// unlink to make sure it can't possibly interfere with KillBox

				// backup a bit from wherever we hit, allowing for the size of the victim
				VectorSubtract(victim->maxs,victim->mins,v);	// diameter across victim
				victim_size = VectorLength(v)/2 + 16;			// add cushion of 16 to radius
				VectorMA(start,(tr.fraction * 8192) - victim_size,aimdir,end);

				gi.unlinkentity(victim);

				// make sure we don't do something silly like
				// try and materialize inside something
				VectorCopy(end,victim->s.origin);
				VectorCopy(end,victim->s.old_origin);

				if (!CheckSpawnPoint(victim))
				{
					HelpMessage(self,"Cannot respawn\nteleportgun contents!");	// RSP 091600 - replaces gi.centerprintf
//					gi.centerprintf(self,"Cannot respawn teleportgun contents!");
					VectorCopy(self->s.origin,victim->s.origin);
					VectorCopy(self->s.origin,victim->s.old_origin);	// RSP 090500
					gi.linkentity(victim);
					return false;
				}
	
				// draw the teleport splash on the victim
				victim->s.event = EV_PLAYER_TELEPORT;

				// kill anything at the destination
				KillBox(victim);

				// If this is a beetle, it has to be aligned to the surface you shot
				// it at. Set it up in a resting state.
				if (victim->identity == ID_BEETLE)
				{
					VectorCopy(tr.plane.normal,victim->pos1);	// set beetle's normal
					AlignBeetle(victim);
					Beetle_Stop(victim);
					victim->nextthink = level.time + FRAMETIME;
				}
				else
				{
					// If you want to face the victim in a particular direction,
					// put the code in here.
				}

				if (victim->identity == ID_GUARDBOT)		// Allow for guardbot vertical movement
					VectorCopy(victim->s.origin,victim->pos1);

				// Restore victim's attributes
				victim->flags   &= ~FL_FROZEN;		// can move
				victim->svflags &= ~SVF_NOCLIENT;	// visible
				victim->solid = SOLID_BBOX;			// touches things
				victim->locked_to = NULL;			// no longer linked to player

				// If I was mean enough to teleport this victim, then that victim should
				// no longer stay pacifist. Unless it's a good guy.

				if (!(victim->monsterinfo.aiflags & AI_GOOD_GUY))
					victim->monsterinfo.aiflags &= ~AI_PACIFIST;

				if (victim->identity == ID_LITTLEBOY)
				{	// Just in case the littleboy was grabbed before it was fully
					// opened, set attributes of a fully-opened littleboy here

					victim->s.frame = 5;			// fully opened position
					spawn_beacon(victim);			// spawn beacon; it was turned off while in storage
					victim->s.angles[PITCH] = 0;	// be horizontal
					victim->yaw_speed = (rand()&1) ? 5 : -5;
					victim->think = bomb_seektarget;
					victim->nextthink = level.time + FRAMETIME;
					victim->s.sound = gi.soundindex("littleboy/idle.wav");
				}

				// If the victim had been frozen by a freezegun, thaw it out. This is
				// necessary because otherwise a walker wouldn't drop to the ground if spawned
				// in mid-air, because being frozen keeps physics from dropping it. Also turn
				// off quad damage.
				if ((victim->freeze_framenum >= level.framenum) ||
					(victim->monsterinfo.quad_framenum >= level.framenum))
				{
					victim->freeze_framenum = 0;
					victim->monsterinfo.quad_framenum = 0;
					victim->s.effects &= ~EF_QUAD;
				}

				M_CatagorizePosition(victim);	// Set waterlevel
				M_CheckGround(victim);			// Check ground presence

				gi.linkentity(victim);
			}
		}
		// Clear out the teleportgun storage

		self->teleportgun_victim = NULL;
		self->teleportgun_framenum = 0;
        self->tportding_framenum = 0; // RSP 090500
		return true;
	}

	// We're firing the gun at something, and will pick it up and store it

	VectorMA(start, 8192, aimdir, end);

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	target = tr.ent;

	if (((target->svflags & SVF_MONSTER) && (target->identity != ID_TURRET)) ||
		(target->identity == ID_LITTLEBOY) ||
		(target->identity == ID_DLOCK))
	{
		if (target->flags & FL_FROZEN)	// Can't grab a monster under the spell of a matrix ball
			return false;

		if (!target->inuse || (target->deadflag != DEAD_NO))			// Can't grab dead things
			return false;

		// sound and light show on a successful shot
		gi.positioned_sound (target->s.origin, target, CHAN_VOICE, gi.soundindex ("misc/tele.wav"), 1, ATTN_NORM, 0);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BOSSTPORT);
		gi.WritePosition (target->s.origin);
		gi.multicast(target->s.origin, MULTICAST_PVS);

		// Store pointer to target entity and set storage timeout
		self->teleportgun_victim = target;
		self->teleportgun_framenum = level.framenum + TGUN_STORE_TIME*10;
		target->locked_to = self;
		target->flags   |= FL_FROZEN;		// can't move
		target->svflags |= SVF_NOCLIENT;	// invisible
		target->solid = SOLID_NOT;			// doesn't touch anything
		VectorClear(target->velocity);
		VectorClear(target->avelocity);

		// Is target a supervisor that's recharging a friend? If so,
		// stop the recharging and turn off the yellow glow on the friend.
		if ((target->identity == ID_SUPERVISOR)	&&
			target->enemy						&&
			(target->enemy->monsterinfo.aiflags & AI_RECHARGING))
		{
			target->enemy->monsterinfo.aiflags &= ~AI_RECHARGING;
			target->enemy = NULL;
		}

		// Is target a workerbot_beam? We have to turn off his lasers if they're on.
		if ((target->identity == ID_BEAMBOT) && target->teamchain)
			workerbeam_laser_off(target->teamchain);

		// Is target a littleboy? If so, we have to turn off his beacon
		// and move him immediately to the shooter's location so fatman
		// can launch more littleboys
		if ((target->identity == ID_LITTLEBOY) && target->target_ent)
		{
			G_FreeEdict(target->target_ent);
			target->target_ent = NULL;
			VectorCopy(self->s.origin,target->s.origin);
		}

		// Free from monster's grasp
		if (target->enemy && target->enemy->locked_to)
			if (target->identity == ID_GREAT_WHITE)		// Shark?
				gwhite_ungrab(target);
			else										// Claw
				botUnlockFrom(target,target->enemy);

		gi.linkentity(target);

        // Set frame to play teleport storage sound (1 second later)
		self->tportding_framenum = level.framenum + 10;

		return true;
	}
	
	return false;
}

/* MRB 092600
 * Teleport gun firing runs frames 8, 9, 10 thru 17, then cycle between 10 and 17
 * When fire is hit again, jump to 18, 19 then idle
 */
void weapon_teleportgun_fire (edict_t *ent)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	offset;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorSet(offset,24,7,ent->viewheight-8);	// RSP 090200 - changed forward 0->24
	P_ProjectSource(ent->client,ent->s.origin,offset,forward,right,start);

	if (fire_teleport(ent,start,forward))
	{
		if (ent->teleportgun_victim)
		{	// Victim is being loaded
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/teleportgun/tport_store.wav"), 1, ATTN_NORM, 0);
			ent->client->weaponstate = WEAPON_READY;
		}
		else
		{	// Victim is being emptied
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/teleportgun/tport_launch.wav"), 1, ATTN_NORM, 0);
		}
	
		// send muzzle flash
		gi.WriteByte (svc_muzzleflash);
		gi.WriteShort (ent-g_edicts);
		gi.WriteByte (MZ_BFG);
		gi.multicast (ent->s.origin, MULTICAST_PVS);

		// Only use 2 rods of ammo if you successfully picked up a victim. If you successfully launched
		// a victim that was in storage, or you were unable to pick up or launch, then use no ammo.
		// Not using ammo on a launch allows you to get rid of what's in storage, w/o getting stuck w/it
		// and then committing suicide when the gun blows up.
		if (ent->teleportgun_victim)
			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				ent->client->pers.inventory[ent->client->ammo_index] -= 2;	// RSP 090100 - use 2 rods

		PlayerNoise(ent,start,PNOISE_WEAPON);
	}
	else
	{	// MRB 092600 - go back to idle if it failed
		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/teleportgun/tport_fail.wav"), 1, ATTN_NORM, 0);
		ent->client->ps.gunframe = 19;	// RSP 092700 - 19, not 21
		ent->client->weaponstate = WEAPON_READY;
	}
	ent->client->ps.gunframe++;
}


void Weapon_Teleportgun (edict_t *ent)
{
	static int pause_frames[] = {20, 25, 0};
	static int fire_frames[]  = {8, 18, 0};

	Weapon_Generic (ent, 7, 19, 30, 35, pause_frames, fire_frames, weapon_teleportgun_fire);
}

//=================
//
//  Freezegun
//
//  WPO 111199 - new and improved freeze gun
//
//=================


/*
=================
Freeze Gun
=================
*/

#define FREEZE_TIME 10  // WPO 111199 - Amount of time frozen, in seconds

void freeze_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (self);
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if ((other->svflags & SVF_MONSTER) || other->client || (other->identity == ID_JELLY))	// RSP 0704 freeze jellies too
	{
		// WPO 151199 Changed multiplication factor to correct value
		other->freeze_framenum = level.framenum + (FREEZE_TIME * 10);

		if (other->enemy && other->enemy->locked_to)	// RSP 070400 Free from monster's grasp
			if (other->identity == ID_GREAT_WHITE)		// Shark?
				gwhite_ungrab(other);
			else										// Claw
				botUnlockFrom(other,other->enemy);

		// WPO 151199 Re-enabled hit sound, and changed filename
		gi.positioned_sound (other->s.origin, other, CHAN_VOICE, gi.soundindex ("weapons/freeze/freezehit.wav"), 1, ATTN_NORM, 0);
	}

	G_FreeEdict(self);
}

void launch_freeze(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t	*freezeball;

	freezeball = G_Spawn();
	VectorCopy (start, freezeball->s.origin);
	VectorCopy (dir, freezeball->movedir);
	vectoangles (dir, freezeball->s.angles);
	VectorScale (dir, speed, freezeball->velocity);
	freezeball->movetype = MOVETYPE_FLYMISSILE;
	freezeball->clipmask = MASK_SHOT;
	freezeball->solid = SOLID_BBOX;
	freezeball->s.effects |= EF_FLAG2;	// RSP 070400 - changed effect from green to blue
	VectorClear (freezeball->mins);
	VectorClear (freezeball->maxs);
	freezeball->s.modelindex = gi.modelindex ("sprites/s_freeze.sp2");
	freezeball->owner = self;
	freezeball->touch = freeze_touch;
	freezeball->nextthink = level.time + 8000/speed;
	freezeball->think = G_FreeEdict;
//	freezeball->radius_dmg = damage;			// RSP 070300 - not used
//	freezeball->dmg_radius = damage_radius;		// RSP 070300 - not used
	freezeball->classname = "freezeball";
	freezeball->s.sound = gi.soundindex ("weapons/freeze/freeze.wav");

	if (self->client)
		check_dodge(self, freezeball->s.origin, dir, speed);

	gi.linkentity(freezeball);
}

void weapon_freezegun_fire (edict_t *ent)
{
	vec3_t	offset, start;
	vec3_t	forward, right;

	if (ent->client->ps.gunframe == 9)
	{
		ent->client->ps.gunframe++;

		// RSP 063000 - add firing sound, which starts at a frame earlier than the launching
		// of the freezeball.
		gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/freeze/freezefire.wav"), 1, ATTN_NORM, 0);
		return;
	}

	// When we get to here, we're on frame 17, so it's time to launch the freezeball

	if (ent->client->pers.inventory[ent->client->ammo_index] < 1)
	{
		ent->client->ps.gunframe++;
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorScale (forward, -2, ent->client->kick_origin);

	// make a big pitch kick with an inverse fall
	ent->client->v_dmg_pitch = -40;
	ent->client->v_dmg_roll = crandom()*8;
	ent->client->v_dmg_time = level.time + DAMAGE_TIME;

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_BFG);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
	launch_freeze (ent, start, forward, 0, 400, 0);	// RSP 070300 - damage not used

	ent->client->ps.gunframe++;

	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]--;
}

void Weapon_Freezegun (edict_t *ent)
{
	static int pause_frames[] = {39, 45, 50, 55, 0};
	static int fire_frames[]  = {9, 17, 0};

	Weapon_Generic (ent, 8, 32, 55, 58, pause_frames, fire_frames, weapon_freezegun_fire);
}

//=================
//
//  WPO 070100
//  Rifle
//
//=================

void weapon_rifle_fire (edict_t *ent)
{
	vec3_t start;
	vec3_t forward, right;
	vec3_t offset;
	int    damage = wd_table[WD_RIFLE];		// RSP 091900

	int    kick = 0;	// RSP 070400 - disable kick because of scope magnification

	if (is_quad)
	{
		damage *= 4;
//		kick *= 4;		// RSP 082300 - no kick to the rifle
	}
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;

	AngleVectors (ent->client->v_angle, forward, right, NULL);

//  RSP 070400 - disable kick because of scope magnification
//	VectorScale (forward, -3, ent->client->kick_origin);
//	ent->client->kick_angles[0] = -3;

	VectorSet(offset,24,0,ent->viewheight);	// RSP 070400 - more accurate targetting	// RSP 090200 - changed forward 0->24
	P_ProjectSource(ent->client,ent->s.origin,offset,forward,right,start);

	fire_lead(ent,start,forward,damage,0,TE_SHOTGUN,0,0,MOD_RIFLE);	// RSP 070400 - no spreads, no kick

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_RAILGUN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->client->ps.gunframe++;
	PlayerNoise(ent, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]--;
}


void Weapon_Rifle (edict_t *ent)
{
	static int pause_frames[] = {56, 0};
	static int fire_frames[]  = {4, 0};

	Weapon_Generic (ent, 3, 18, 56, 61, pause_frames, fire_frames, weapon_rifle_fire);
}


//=================
//
//  Matrix
//
//  RSP 051999
//
//=================

void matrix_think_redirect (edict_t *);

void matrix_adjust(edict_t *self)
{
	vec3_t v;

	// RSP 080600 - enemy might be standing on a plat or train that moves, so you have
	// to keep up with him.
	VectorSubtract(self->enemy->s.origin,self->pos2,v);
	if (VectorLength(v) > 0)
	{
		VectorAdd(self->s.origin,v,self->s.origin);
		VectorCopy(self->s.origin,self->locked_to->s.origin);
		VectorCopy(self->enemy->s.origin,self->pos2);
		self->pos1[2] = (self->enemy->absmin[2] + self->enemy->absmax[2])/2;
		gi.linkentity(self);
		gi.linkentity(self->locked_to);
	}
}


void matrix_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict(self->locked_to);
		G_FreeEdict(self);
	}
}

#define BLASTRADIUS 1000
#define BLASTFORCE  1500

void matrix_think_blastwave(edict_t *self)
{
	edict_t		*item;
	float		dist,damage,force;
	vec3_t		v;
	qboolean	blastit;

	item = NULL;
	while (item = _findlifeinradius(self,item,self->s.origin,BLASTRADIUS,true,true,false,&dist))
	{
		if (item == self)
			continue;

		if (!(item->client) && !(item->svflags & SVF_MONSTER) && (item->identity != ID_LITTLEBOY))
			continue;

		// Ignore it if there's no line-of-sight from the matrix to the item.

		if (!visible(self,item))
			continue;

		blastit = true;

		if (item->identity == ID_TURRET)	// don't blow turret off its ring
			blastit = false;

		// add the proper amount of force to the entity's velocity

		if (blastit)
		{
			VectorSubtract(item->s.origin,self->s.origin,v);
			VectorNormalize(v);
			force = BLASTFORCE*(1-dist/BLASTRADIUS)*(50/item->mass);
			VectorMA(item->velocity,force,v,item->velocity);
		}

		// subtract health from this entity

		if (item->takedamage)
		{
			damage = BLASTFORCE*(1-dist/BLASTRADIUS)/10;
			T_Damage (item, self, self->owner, self->velocity, item->s.origin, vec3_origin, (int) damage, 0, DAMAGE_ENERGY, MOD_MATRIX_EFFECT);
		}
	}
	
	// Kill the matrix shot

	G_FreeEdict(self);	// The sequence is finally over (whew!)
}

#define NOALPHA_DISTANCE 2000

void matrix_think_finale(edict_t *self)
{
	vec3_t	d;
	int		i;
	edict_t	*e;

	// Kill the enemy if you're not in DM. If you're in DM, the amount of damage
	// will depend on the enemy's distance from the blast point, and will be done
	// in the matrix_think_blastwave() routine.

	if (!deathmatch->value)	// RSP 062399 - prepare for DM
		T_Damage (self->enemy, self, self->owner, self->velocity, self->enemy->s.origin, vec3_origin, 10000, 0, DAMAGE_ENERGY, MOD_MATRIX_EFFECT);

	// Kill matrix effect
	
	G_FreeEdict(self->locked_to);

	// Flash

	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_BFG);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	// Nuclear Particle effect

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_NUKEBLAST);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_ALL);

	// Clients get matrix blend. Amount depends on distance away. Don't bother checking
	// line-of-sight. Looks better if the client knows when the blast goes off, even if
	// there are walls/doors in the way or the client is looking in the other direction.

	e = g_edicts;
	for (i = 0 ; i < globals.max_edicts ; i++, e++)	// RSP 062399 - for DM
		if (e->client && e->inuse)
		{
			VectorSubtract(self->s.origin,e->s.origin,d);
			e->client->matrix_alpha = 0.6*(1 - VectorLength(d)/NOALPHA_DISTANCE);
		}

	self->think = matrix_think_blastwave;
	self->nextthink = level.time + FRAMETIME;

	// Make the model invisible
	self->s.modelindex = gi.modelindex ("sprites/beacon_0.sp2");
}


// bump up the skin animation

void matrix_skin_animate(edict_t *self)
{
	if (++self->s.skinnum > MATRIX_END_SKIN)
		self->s.skinnum = 0;
}


void matrix_think_drain (edict_t *self)
{
	matrix_skin_animate(self);	// bump up the skin animation

	matrix_adjust(self);	// RSP 080600 - enemy might be standing on a plat or train
							// that moves, so you have to keep up with him.
	if (--self->count == 0)
		self->think = matrix_think_finale;

	self->nextthink = level.time + FRAMETIME;
}

void matrix_think_drop (edict_t *self)
{
	if (self->s.origin[2] <= self->pos1[2])
	{
		self->think = matrix_think_drain;
		self->nextthink = level.time + FRAMETIME;
		self->prethink = NULL;	// turn off sphere expansion
		self->count = 30;		// pause for 30 frames
		return;
	}

	self->s.origin[2] -= 10;	// drop
	VectorCopy(self->s.origin,self->locked_to->s.origin);
	VectorSet(self->mins,self->mins[0]-MATRIX_MIN_RAD,self->mins[1]-MATRIX_MIN_RAD,self->mins[2]-MATRIX_MIN_RAD);
	VectorSet(self->maxs,self->maxs[0]+MATRIX_MIN_RAD,self->maxs[1]+MATRIX_MIN_RAD,self->maxs[2]+MATRIX_MIN_RAD);
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity (self);
	gi.linkentity (self->locked_to);
}


void matrix_prethink_expand(edict_t *self)
{
	if (++self->s.frame > MATRIX_EXPANSION_FRAMES)
		self->s.frame = MATRIX_EXPANSION_FRAMES;

	matrix_skin_animate(self);	// bump up the skin animation

	matrix_adjust(self);	// RSP 080600 - enemy might be standing on a plat or train
							// that moves, so you have to keep up with him.
}

// Done hovering, start to expand

void matrix_think_expand (edict_t *self)
{
	matrix_skin_animate(self);	// bump up the skin animation

	matrix_adjust(self);	// RSP 080600 - enemy might be standing on a plat or train
							// that moves, so you have to keep up with him.

	// Expand until your center has dropped to the same height as the enemy's center.

	self->pos1[2] = (self->enemy->absmin[2] + self->enemy->absmax[2])/2;
	self->clipmask = 0;

	// lower yourself in equal steps

	self->think = matrix_think_drop;
	self->nextthink = level.time + FRAMETIME;
	self->prethink = matrix_prethink_expand;
}


void matrix_think_pause (edict_t *self)
{
	matrix_skin_animate(self);	// bump up the skin animation

	matrix_adjust(self);	// RSP 080600 - enemy might be standing on a plat or train
							// that moves, so you have to keep up with him.

	if (--self->count == 0)
		self->think = matrix_think_expand;
	self->nextthink = level.time + FRAMETIME;
}


// While you fly, adjust your velocity toward the target spot. Keep the shot's
// velocity at zero, and calculate the x,y,z moves yourself, since the math
// is different for x,y than it is for z changes.

void matrix_think_fly(edict_t *self)
{
	float	len;
	float	dpf;	// distance per frame
	vec3_t	dir;
	vec3_t	targ_angles;
	float	yaw;
	float	adiff;
	vec3_t	pos_shot,pos_target;

	// Determine where the matrix shot gets moved to

	dpf = self->speed*FRAMETIME;
	yaw = self->s.angles[YAW]*M_PI*2 / 360;
	self->s.origin[0] += cos(yaw)*dpf;
	self->s.origin[1] += sin(yaw)*dpf;

	self->s.origin[2] += (self->enemy->absmax[2] + 5 - self->s.origin[2])/self->count;
	VectorCopy(self->s.origin,self->locked_to->s.origin);

	matrix_skin_animate(self);	// bump up the skin animation

	self->count--;

	// RSP 093099
	// If we're in DM, and the target player has a Matrix counter-effect powerup, then
	// turn the matrix shot around and head it back toward the shooter when the shot
	// is halfway to the intended target.

	if (deathmatch->value && (self->count == 5) && luc_ent_has_powerup(self->enemy,POWERUP_MATRIXSHIELD))
	{
		luc_ent_used_powerup(self->enemy,POWERUP_MATRIXSHIELD);	// deplete the powerup 
		self->enemy = self->owner;				// switch targets
		self->think = matrix_think_redirect;	// pause routine
	}

	// See if we're near the target. We have to freeze the enemy one frame before we quit
	// moving because otherwise the supervisor--who is quick--can dart out from under the
	// matrix.

	// Freeze the target at the next-to-last frame.
	// If we're in DM, don't freeze the other player.

	else if ((self->count == 1) && !deathmatch->value)	// RSP 062399 - for DM
	{
		self->enemy->monsterinfo.aiflags |= AI_PACIFIST;	// tame that beast
		VectorClear(self->enemy->velocity);					// freeze that beast
		VectorClear(self->enemy->avelocity);				// freeze that beast
		self->enemy->nextthink = 0;							// dumb him down
		self->enemy->flags |= FL_FROZEN;					// freeze that beast
		self->enemy->s.renderfx |= RF_GLOW;					// make him glow
		gi.linkentity(self->enemy);
	}

	// See if we've reached the target.

	else if (self->count == 0)
	{
		VectorClear(self->velocity);
		VectorCopy(self->enemy->s.origin,self->s.origin);
		self->s.origin[2] += self->enemy->maxs[2] + 5;
		VectorCopy(self->s.origin,self->locked_to->s.origin);	// RSP 093099 - effect also has to change
		self->think = matrix_think_pause;
		self->nextthink = level.time + FRAMETIME;
		self->count = 10;									// pause for 10 frames
		VectorCopy(self->enemy->s.origin,self->pos2);		// note his origin
		self->solid = SOLID_NOT;							// RSP 080700 - allows matrix to not block plats
		gi.linkentity(self);
		gi.linkentity(self->locked_to);	// RSP 093099 - effect also has to change
		return;
	}

	// Determine the math for the next incremental move.
	// The z change is a simple division of the entire height change over
	// the frame count, and is applied at the top of this routine, the next
	// time through. So take it out of the x,y length calculation by
	// mapping the path onto the x,y plane.

	VectorCopy(self->s.origin,pos_shot);
	VectorCopy(self->enemy->s.origin,pos_target);
	pos_shot[2] = pos_target[2];	// map to x,y plane

	VectorSubtract(pos_target,pos_shot,dir);
	len = VectorLength(dir);
	self->speed = (len/FRAMETIME)/self->count;
	vectoangles (dir, targ_angles);	// angles from current position to target

	// determine the delta change in YAW. We want to take the total angular change
	// and spread it evenly across all the frames necessary to get from current
	// position to target.

	if (self->enemy == self->owner) // RSP 100199 - returning to shooter
		self->s.angles[YAW] = targ_angles[YAW];
	else
	{
		adiff = targ_angles[YAW] - self->s.angles[YAW];
		if (adiff > 180)
			adiff -= 360;
		else if (adiff < -180)
			adiff += 360;
		self->s.angles[YAW] += adiff/self->count;
	}

	// self->s.angles[YAW] is now the angle that the shot will take at the next
	// think time.

	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
	gi.linkentity(self->locked_to);
}

// RSP 100199 - new routine for redirecting matrix back toward shooter
void matrix_think_redirect (edict_t *self)
{
	matrix_skin_animate(self);	// bump up the skin animation

	if (--self->count == -5)	// counting down from +5 to -5, so it's
								// a 10-frame pause
	{
		self->count = 5;		// reset to continue path back to shooter
		self->think = matrix_think_fly;
	}
	self->nextthink = level.time + FRAMETIME;
}


void launch_matrix  (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t	*matrix,*matrix2;
	vec3_t	d,pos_shot,pos_target;

	// Spawn matrix model

	matrix = G_Spawn();
	VectorCopy (start, matrix->s.origin);
	VectorCopy (dir, matrix->movedir);
	vectoangles (dir, matrix->s.angles);
	VectorScale (dir, speed, matrix->velocity);
	matrix->movetype = MOVETYPE_FLYMISSILE;
	matrix->clipmask = MASK_SHOT;
	matrix->solid = SOLID_BBOX;
	matrix->s.renderfx |= RF_TRANSLUCENT;
	VectorSet(matrix->mins,-MATRIX_MIN_RAD,-MATRIX_MIN_RAD,-MATRIX_MIN_RAD);
	VectorSet(matrix->maxs, MATRIX_MIN_RAD, MATRIX_MIN_RAD, MATRIX_MIN_RAD);

	matrix->model = "models/items/ammo/matrix/tris.md2";	// RSP 061899
	matrix->s.modelindex = gi.modelindex (matrix->model);	// RSP 061899

	matrix->owner = self;
	matrix->touch = matrix_touch;
	matrix->classname = "matrix blast";
	matrix->s.sound = gi.soundindex ("weapons/matrix/matrixfly.wav");
	matrix->count = 10;		// get to target in this many frames
	matrix->enemy = self->enemy;	// pass along the target enemy

	VectorCopy(matrix->s.origin,pos_shot);
	VectorCopy(matrix->enemy->s.origin,pos_target);
	pos_shot[2] = pos_target[2];	// map to x,y plane

	VectorSubtract(pos_target,pos_shot,d);
	matrix->speed = (VectorLength(d)/FRAMETIME)/matrix->count;

	// Initially, fly straight for one frame. Then let the first in a series of
	// matrix_think functions change the direction according to where the target is.

	matrix->think = matrix_think_fly;
	matrix->nextthink = level.time + FRAMETIME;
	matrix->s.frame = 1;
	matrix->s.skinnum = 0;
	self->enemy = NULL;
	matrix->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger

	if (self->client)
		check_dodge (self, matrix->s.origin, dir, speed);

	// Spawn matrix effect

	matrix2 = G_Spawn();
	matrix2->classname = "matrix effect";
	VectorCopy(matrix->s.origin,matrix2->s.origin);
	matrix2->s.effects |= (EF_BFG|EF_ANIM_ALLFAST);

	matrix2->model = "sprites/beacon_0.sp2";				// RSP 061899
	matrix2->s.modelindex = gi.modelindex (matrix2->model);	// RSP 061899

	matrix2->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger

	matrix->locked_to = matrix2;

	gi.linkentity (matrix);
	gi.linkentity (matrix2);
}

qboolean FindMatrixTarget (edict_t *self)
{
	// Find target with the most health.

	edict_t *e,*best_target;
	int     i;
	int     best_health = 0;
 
	// RSP 062399 - If DM, look for player with best health

	e = g_edicts;
	if (deathmatch->value)
	{	// look for players
		for (i = 0 ; i < globals.max_edicts ; i++, e++)
			if (e->client && e->inuse && (e != self))
			{
				if ((e->health > best_health)	&&
					visible(self,e)				&&	// Line-of-sight from me to this target?
					infront(self,e))				// Is this target in front of me?
				{
					best_health = e->health;
					best_target = e;
				}
			}
	}
	else
	{	// look for non-pacifist monsters
		for (i = 0 ; i < globals.max_edicts ; i++, e++)
			if (((e->svflags & SVF_MONSTER) || (e->identity == ID_LITTLEBOY)) &&
				!(e->monsterinfo.aiflags & AI_PACIFIST)	&&
				(e->identity != ID_GUARDBOT) &&	// RSP 100700 - matrix can't target guardbots
				e->inuse)
			{
				if ((e->health > best_health) &&
					visible(self,e)			  &&	// Line-of-sight from me to this monster?
					infront(self,e))				// Is this monster in front of me?
				{
					best_health = e->health;
					best_target = e;
				}
			}
	}

	if (best_health == 0)
		return false;	// No monster found

	// Place target directly above this target

	self->enemy = best_target;
	return true;
}

#define MATRIX_AMMO_USE 50

void weapon_matrix_fire (edict_t *ent)
{
	vec3_t	offset, start;
    vec3_t	forward, right;
    
    // make sure there's enough plasma, and that you have something to shoot at
    
    if ((ent->client->pers.inventory[ent->client->ammo_index] < MATRIX_AMMO_USE) ||
    	(!FindMatrixTarget(ent)))	// sets 'ent->enemy' to the target enemy
    {
    	if (ent->client->ps.gunframe == 9)	// RSP 061800
    		gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/matrix/mat_whine.wav"), 1, ATTN_NORM, 0);
		ent->client->ps.gunframe++;	// RSP 061800
    	return;
    }
    
    if (ent->client->ps.gunframe == 9)
    {

		ent->client->ps.gunframe++;
    
		// RSP 061100 - add matrix firing sound, which starts earlier than the launching
		// of the matrix ball
    	gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/matrix/mat_fire.wav"), 1, ATTN_NORM, 0);
		return;	// RSP 061800
    }

	// When we get here, we're on frame 13, which is the firing frame to launch
	// the matrix ball.
    
    AngleVectors (ent->client->v_angle, forward, right, NULL);
    VectorScale (forward, -2, ent->client->kick_origin);
    
    // Fire matrix sphere

    // send muzzle flash
    gi.WriteByte (svc_muzzleflash);
    gi.WriteShort (ent-g_edicts);
    gi.WriteByte (MZ_BFG);
    gi.multicast (ent->s.origin, MULTICAST_PVS);
    
    VectorSet(offset, 8, 8, ent->viewheight-8);
    P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);
    launch_matrix (ent, start, forward, 0, 0, 0);
    
    ent->client->ps.gunframe++;
    
    PlayerNoise(ent, start, PNOISE_WEAPON);
    
    if (!((int)dmflags->value & DF_INFINITE_AMMO))
    	ent->client->pers.inventory[ent->client->ammo_index] -= MATRIX_AMMO_USE;
}

void Weapon_Matrix (edict_t *ent)
{
	// These frame numbers have to be adjusted for the matrix frames

	static int pause_frames[] = {29, 34, 39, 44, 0};	// RSP 061100
	static int fire_frames[]  = {9, 13, 0};				// RSP 061800

	Weapon_Generic (ent, 8, 28, 48, 55, pause_frames, fire_frames, weapon_matrix_fire);	// RSP 061100
}

//=================
//
//  Grenades
//
//  WPO 071699
//
//=================

#define GRENADE_TIMER		3.0
#define GRENADE_MINSPEED	400
#define GRENADE_MAXSPEED	800

static void Grenade_Explode (edict_t *ent)
{
	int mod;

	if (ent->owner->client)
		PlayerNoise(ent->owner, ent->s.origin, PNOISE_IMPACT);

    // WPO: 71899  changed to correct single player mode problem
	if (ent->enemy)
	{
		float	points;
		vec3_t	v;
		vec3_t	dir;

		VectorAdd (ent->enemy->mins, ent->enemy->maxs, v);
		VectorMA (ent->enemy->s.origin, 0.5, v, v);
		VectorSubtract (ent->s.origin, v, v);
		points = ent->dmg - 0.5 * VectorLength (v);
		VectorSubtract (ent->enemy->s.origin, ent->s.origin, dir);
		if (ent->spawnflags & SPAWNFLAG_GR_HANDGRENADE)
			mod = MOD_HANDGRENADE;
		T_Damage (ent->enemy, ent, ent->owner, dir, ent->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
	}

	if (ent->spawnflags & SPAWNFLAG_GR_HELD)
		mod = MOD_HELD_GRENADE;
	else if (ent->spawnflags & SPAWNFLAG_GR_HANDGRENADE)
		mod = MOD_HG_SPLASH;
	T_RadiusDamage(ent, ent->owner, ent->dmg, ent->enemy, ent->dmg_radius, mod);

	VectorMA (ent->s.origin, -0.02, ent->velocity, ent->s.origin);  // RSP 062900

	if (ent->waterlevel)
		SpawnExplosion(ent,TE_GRENADE_EXPLOSION_WATER,MULTICAST_PHS,EXPLODE_GRENADE_WATER);
	else
		SpawnExplosion(ent,TE_GRENADE_EXPLOSION,MULTICAST_PHS,EXPLODE_GRENADE);
	G_FreeEdict(ent);
}


static void Grenade_Touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}

	if (!other->takedamage)
	{
		if (ent->spawnflags & SPAWNFLAG_GR_HANDGRENADE)
		{	// Hand grenade is bouncing off a surface
			if (random() > 0.5)
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/grenade/hgrenb1a.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/grenade/hgrenb2a.wav"), 1, ATTN_NORM, 0);
		}

		return;
	}

    // WPO: 71899  changed to correct single player mode problem
	ent->enemy = other;
	Grenade_Explode (ent);
}

// When a grenade 'dies', it blows up next frame
static void Grenade_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{	
	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + FRAMETIME;
	self->think = Grenade_Explode;
}

void throw_grenade2 (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, float timer, float damage_radius, qboolean held)
{
	edict_t	*grenade;
	vec3_t	dir;
	vec3_t	forward, right, up;

	vectoangles (aimdir, dir);
	AngleVectors (dir, forward, right, up);

	grenade = G_Spawn();
	VectorCopy (start, grenade->s.origin);
	VectorScale (aimdir, speed, grenade->velocity);
	VectorMA (grenade->velocity, 200 + crandom() * 10.0, up, grenade->velocity);
	VectorMA (grenade->velocity, crandom() * 10.0, right, grenade->velocity);
	VectorSet (grenade->avelocity, 300, 300, 300);
	grenade->movetype = MOVETYPE_BOUNCE;
	grenade->clipmask = MASK_SHOT;
	grenade->solid = SOLID_BBOX;
	grenade->s.effects |= EF_GRENADE;
	VectorClear (grenade->mins);
	VectorClear (grenade->maxs);
	grenade->s.modelindex = gi.modelindex ("models/objects/grenade2/tris.md2");
	grenade->owner = self;
	grenade->touch = Grenade_Touch;
	grenade->nextthink = level.time + timer;
	grenade->think = Grenade_Explode;

	grenade->dmg = damage;
	grenade->dmg_radius = damage_radius;
	grenade->classname = "hgrenade";
	if (held)
		grenade->spawnflags = (SPAWNFLAG_GR_HANDGRENADE|SPAWNFLAG_GR_HELD);
	else
		grenade->spawnflags = SPAWNFLAG_GR_HANDGRENADE;

	grenade->s.sound = gi.soundindex("weapons/grenade/hgrenc1b.wav");

	// WPO: a few more attributes to let the grenade 'die'
	VectorSet(grenade->mins, -5, -5, -5);	// RSP 081600 - should match final model
	VectorSet(grenade->maxs, 5, 5, 5);		// RSP 081600 - should match final model
	grenade->mass = 2;
	grenade->health = 10;
	grenade->die = Grenade_Die;
	grenade->takedamage = DAMAGE_YES;
	grenade->monsterinfo.aiflags = AI_NOSTEP;

	if (timer <= 0.0)
		Grenade_Explode (grenade);
	else
	{
		gi.sound (self, CHAN_WEAPON, gi.soundindex ("weapons/grenade/hgrent1a.wav"), 1, ATTN_NORM, 0);
		gi.linkentity (grenade);
	}
}

void weapon_grenade_fire (edict_t *ent, qboolean held)
{
	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int     damage = 125;
	float	timer;
	int     speed;
	float	radius;

	radius = damage + 40;
	if (is_quad)
		damage *= 4;
	else if (is_tri)	// RSP 082300 - tri-damage
		damage *= 3;

	VectorSet(offset, 8, 8, ent->viewheight-8);
	AngleVectors (ent->client->v_angle, forward, right, NULL);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	timer = ent->client->grenade_time - level.time;
	speed = GRENADE_MINSPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAXSPEED - GRENADE_MINSPEED) / GRENADE_TIMER);
	throw_grenade2 (ent, start, forward, damage, speed, timer, radius, held);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		ent->client->pers.inventory[ent->client->ammo_index]--;

	ent->client->grenade_time = level.time + 1.0;

	if (ent->deadflag || (ent->s.modelindex != 255)) // VWep animations screw up corpses
		return;

	if (ent->health <= 0)
		return;

	if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
	{
		ent->client->anim_priority = ANIM_ATTACK;
		ent->s.frame = FRAME_crattak1-1;
		ent->client->anim_end = FRAME_crattak3;
	}
	else
	{
		ent->client->anim_priority = ANIM_REVERSE;
		ent->s.frame = FRAME_wave08;
		ent->client->anim_end = FRAME_wave01;
	}
}

void Weapon_Grenade (edict_t *ent)
{
	if ((ent->client->newweapon) && (ent->client->weaponstate == WEAPON_READY))
	{
		ChangeWeapon (ent);
		return;
	}

	if (ent->client->weaponstate == WEAPON_ACTIVATING)
	{
		ent->client->weaponstate = WEAPON_READY;
		ent->client->ps.gunframe = 10;	// RSP 080800 - reduced frames; return to idle
//		ent->client->ps.gunframe = 16;
		return;
	}

	if (ent->client->weaponstate == WEAPON_READY)
	{
		if (((ent->client->latched_buttons|ent->client->buttons) & BUTTON_ATTACK))
		{
			ent->client->latched_buttons &= ~BUTTON_ATTACK;
			if (ent->client->pers.inventory[ent->client->ammo_index])
			{
				ent->client->ps.gunframe = 0;	// RSP 080800
				ent->client->weaponstate = WEAPON_FIRING;
				ent->client->grenade_time = 0;
			}
			else
			{
				if (level.time >= ent->pain_debounce_time)
				{
					gi.sound(ent, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1, ATTN_NORM, 0);
					ent->pain_debounce_time = level.time + 1;
				}
				NoAmmoWeaponChange (ent);
			}
			return;
		}

//		if ((ent->client->ps.gunframe == 29) || (ent->client->ps.gunframe == 34) || (ent->client->ps.gunframe == 39) || (ent->client->ps.gunframe == 48))
		if ((ent->client->ps.gunframe == 12) || (ent->client->ps.gunframe == 16) || (ent->client->ps.gunframe == 19))	// RSP 080800 - reduced frames
		{
			if (rand()&15)
				return;
		}

//		if (++ent->client->ps.gunframe > 48)
//			ent->client->ps.gunframe = 16;
		if (++ent->client->ps.gunframe > 19)	// RSP 080800 - reduced frames
			ent->client->ps.gunframe = 10;
		return;
	}

	if (ent->client->weaponstate == WEAPON_FIRING)
	{
//		if (ent->client->ps.gunframe == 5)
		if (ent->client->ps.gunframe == 2)	// RSP 080800 - reduced frames
			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/grenade/hgrena1b.wav"), 1, ATTN_NORM, 0);

//		if (ent->client->ps.gunframe == 11)
		if (ent->client->ps.gunframe == 6)	// RSP 080800 - reduced frames
		{
			if (!ent->client->grenade_time)
			{
				ent->client->grenade_time = level.time + GRENADE_TIMER + 0.2;
				ent->client->weapon_sound = gi.soundindex("weapons/grenade/hgrenc1b.wav");
			}

			// they waited too long, detonate it in their hand
			if (!ent->client->grenade_blew_up && (level.time >= ent->client->grenade_time))
			{
				ent->client->weapon_sound = 0;
				weapon_grenade_fire (ent, true);
				ent->client->grenade_blew_up = true;
			}

			if (ent->client->buttons & BUTTON_ATTACK)
				return;

			if (ent->client->grenade_blew_up)
			{
				if (level.time >= ent->client->grenade_time)
				{
//					ent->client->ps.gunframe = 15;
					ent->client->ps.gunframe = 9;	// RSP 080800 - reduced frames
					ent->client->grenade_blew_up = false;
				}
				else
				{
					return;
				}
			}
		}

//		if (ent->client->ps.gunframe == 12)
		if (ent->client->ps.gunframe == 7)	// RSP 080800 - reduced frames
		{
			ent->client->weapon_sound = 0;
			weapon_grenade_fire (ent, false);
		}

//		if ((ent->client->ps.gunframe == 15) && (level.time < ent->client->grenade_time))
		if ((ent->client->ps.gunframe == 9) && (level.time < ent->client->grenade_time))	// RSP 080800 - reduced frames
			return;

		ent->client->ps.gunframe++;

//		if (ent->client->ps.gunframe == 16)
		if (ent->client->ps.gunframe == 10)	// RSP 080800 - reduced frames
		{
			ent->client->grenade_time = 0;
			ent->client->weaponstate = WEAPON_READY;
		}
	}
}
