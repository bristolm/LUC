#include "g_local.h"
#include "luc_blastdamage.h"

/*
=================
check_dodge

This is a support routine used when a client is firing
a non-instant attack weapon.  It checks to see if a
monster's dodge function should be called.
=================
*/

// JBM 012399 Changed this from static to void to get it to work with luc_g_weapon.c used to be :
//static void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed)
void check_dodge (edict_t *self, vec3_t start, vec3_t dir, int speed)
{
	vec3_t	end;
	vec3_t	v;
	trace_t	tr;
	float	eta;

	// easy mode only ducks one quarter the time
	if (skill->value == 0)
	{
		if (random() > 0.25)
			return;
	}
	VectorMA (start, 8192, dir, end);
	tr = gi.trace (start, NULL, NULL, end, self, MASK_SHOT);
	if ((tr.ent) &&
		(tr.ent->svflags & SVF_MONSTER) &&
		(tr.ent->health > 0) &&
		(tr.ent->monsterinfo.dodge) &&
		infront(tr.ent, self))
	{
		VectorSubtract (tr.endpos, start, v);
		eta = (VectorLength(v) - tr.ent->maxs[0]) / speed;
		tr.ent->monsterinfo.dodge (tr.ent, self, eta);
	}
}


/*
=================
fire_hit

Used for all impact (hit/punch/slash) attacks
=================
*/
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick)
{
	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	//see if enemy is in range
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
	range = VectorLength(dir);

	if (range > aim[0])
		return false;

	if ((aim[1] > self->mins[0]) && (aim[1] < self->maxs[0]))
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->mins[0];
		else
			aim[1] = self->enemy->maxs[0];
	}

	VectorMA (self->s.origin, range, dir, point);

	tr = gi.trace (self->s.origin, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
			tr.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA (self->s.origin, range, forward, point);
	VectorMA (point, aim[1], right, point);
	VectorMA (point, aim[2], up, point);
	VectorSubtract (point, self->enemy->s.origin, dir);

	// do the damage

	if (IsHeadShot(self->enemy,tr.endpos))	// RSP 070899 - added headshot check
		damage <<= 1;	// double

	T_Damage (tr.ent, self, self, dir, point, vec3_origin, damage, kick/2, DAMAGE_NO_KNOCKBACK, MOD_HIT);

	if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		return false;

	// RSP 082700 - add "punch" sound
	if ((self->identity != ID_CLAWBOT) && (self->identity != ID_GREAT_WHITE))	// RSP 090100 - no sound if clawbot hitting or shark biting
		gi.sound(self,CHAN_WEAPON,gi.soundindex("weapons/knife/hitmeat.wav"),1,ATTN_NORM,0);

	// do our special form of knockback here
	VectorMA (self->enemy->absmin, 0.5, self->enemy->size, v);
	VectorSubtract (v, point, v);
	VectorNormalize (v);
	VectorMA (self->enemy->velocity, kick, v, self->enemy->velocity);
	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;
	return true;
}


/*
=================
fire_lead

This is an internal support routine used for bullet/pellet based weapons.
=================
*/
void fire_lead (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int te_impact, int hspread, int vspread, int mod)
{
	trace_t		tr;
	vec3_t		dir;
	vec3_t		forward, right, up;
	vec3_t		end;
	float		r;
	float		u;
	vec3_t		water_start;
	qboolean	water = false;
	int			content_mask = MASK_SHOT | MASK_WATER;

	tr = gi.trace (self->s.origin, NULL, NULL, start, self, MASK_SHOT);

	if (!(tr.fraction < 1.0))
	{
		if (mod == MOD_RIFLE)
			r_vectoangles(aimdir,dir);	// RSP 070400 - more accurate shot
		else
			vectoangles(aimdir,dir);	// regular accuracy

		AngleVectors (dir, forward, right, up);

		VectorMA (start, 8192, forward, end);
		if (mod != MOD_RIFLE)
		{
			r = crandom()*hspread;
			u = crandom()*vspread;
			VectorMA (end, r, right, end);
			VectorMA (end, u, up, end);
		}

		if (gi.pointcontents (start) & MASK_WATER)
		{
			water = true;
			VectorCopy (start, water_start);
			content_mask &= ~MASK_WATER;
		}

		tr = gi.trace (start, NULL, NULL, end, self, content_mask);

		// see if we hit water
		if (tr.contents & MASK_WATER)
		{
			int color;

			water = true;
			VectorCopy (tr.endpos, water_start);

			if (!VectorCompare (start, tr.endpos))
			{
				if (tr.contents & CONTENTS_WATER)
				{
					if (strcmp(tr.surface->name, "*brwater") == 0)
						color = SPLASH_BROWN_WATER;
					else
						color = SPLASH_BLUE_WATER;
				}
				else if (tr.contents & CONTENTS_SLIME)
					color = SPLASH_SLIME;
				else if (tr.contents & CONTENTS_LAVA)
					color = SPLASH_LAVA;
				else
					color = SPLASH_UNKNOWN;

				if (color != SPLASH_UNKNOWN)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (TE_SPLASH);
					gi.WriteByte (8);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.WriteByte (color);
					gi.multicast (tr.endpos, MULTICAST_PVS);
				}

				// change bullet's course when it enters water
				VectorSubtract (end, start, dir);
				vectoangles (dir, dir);
				AngleVectors (dir, forward, right, up);
				r = crandom()*hspread*2;
				u = crandom()*vspread*2;
				VectorMA (water_start, 8192, forward, end);
				VectorMA (end, r, right, end);
				VectorMA (end, u, up, end);
			}

			// re-trace ignoring water this time
			tr = gi.trace (water_start, NULL, NULL, end, self, MASK_SHOT);
		}
	}

	// send gun puff / flash
    if (!(tr.surface && (tr.surface->flags & SURF_SKY)))
	{
		if (tr.fraction < 1.0)
		{
			if (tr.ent->takedamage)
			{
				if (IsHeadShot(tr.ent,tr.endpos))	// RSP 070899 - added headshot check
					if (mod == MOD_RIFLE)			// WPO 010700 Rifle
						damage = 1000;
					else
						damage <<= 1;	// double

				T_Damage (tr.ent, self, self, aimdir, tr.endpos, tr.plane.normal, damage, kick, DAMAGE_BULLET, mod);

				// WPO 010700 Sound when target is hit with rifle
				// RSP 071800 - removed, since you can't hear it over the sound of the rifle firing
//              if (mod == MOD_RIFLE)
//					gi.positioned_sound (tr.ent->s.origin, tr.ent, CHAN_WEAPON, gi.soundindex ("weapons/rifle/hit.wav"), 1, ATTN_NORM, 0);
			}
			else
			{
				if (strncmp (tr.surface->name, "sky", 3) != 0)
				{
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte (te_impact);
					gi.WritePosition (tr.endpos);
					gi.WriteDir (tr.plane.normal);
					gi.multicast (tr.endpos, MULTICAST_PVS);

                    // WPO 090699 - Blast damage // RSP 062800 - moved here
                    SpawnBlastDamage(BLAST_MODEL,&tr,BULLET_RADIUS);  

                    PlayerNoise(self, tr.endpos, PNOISE_IMPACT);

                    // WPO 010700 special noise if rifle misses
					// RSP 071800 - removed, since you can't hear it over the sound of the rifle firing
/*
					if (self->client && (mod == MOD_RIFLE))			// RSP 070400
					{
						// WPO 010700 Sound when target is missed with rifle
						// WPO 042800 changed sound
						// RSP 071800 splash sound if underwater
						if (gi.pointcontents(tr.endpos) & MASK_WATER)
							gi.positioned_sound(tr.endpos,tr.ent,CHAN_WEAPON,gi.soundindex("misc/h2ohit1.wav"),1,ATTN_NORM,0);
						else
							gi.positioned_sound(tr.endpos,tr.ent,CHAN_WEAPON,gi.soundindex ("world/ric2.wav"),1,ATTN_NORM,0);
					}
 */
				}
			}
		}
	}

    // WPO 010700 Rifle shots act like tracer bullets
    if (mod == MOD_RIFLE)
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (start);

		// if it hit water, just put the trail up to the water start
        if (water)
			gi.WritePosition(water_start);
		else
			gi.WritePosition(tr.endpos);

		gi.multicast(start, MULTICAST_PVS);
	}

	// if went through water, determine where the end and make a bubble trail
	if (water)
	{
		vec3_t	pos;

		VectorSubtract (tr.endpos, water_start, dir);
		VectorNormalize (dir);
		VectorMA (tr.endpos, -2, dir, pos);
		if (gi.pointcontents (pos) & MASK_WATER)
			VectorCopy (pos, tr.endpos);
		else
			tr = gi.trace (pos, NULL, NULL, water_start, tr.ent, MASK_WATER);

		VectorAdd (water_start, tr.endpos, pos);
		VectorScale (pos, 0.5, pos);

		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BUBBLETRAIL);
		gi.WritePosition (water_start);
		gi.WritePosition (tr.endpos);
		gi.multicast (pos, MULTICAST_PVS);
	}
}


/*
=================
fire_bullet

Fires a single round.  Used for machinegun and chaingun.  Would be fine for
pistols, rifles, etc....
=================
*/
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod)
{
	fire_lead (self, start, aimdir, damage, kick, TE_GUNSHOT, hspread, vspread, mod);
}


/*
=================
fire_shotgun

Shoots shotgun pellets.  Used by shotgun and super shotgun.
=================
 */

void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod)
{
	int i;

	for (i = 0 ; i < count ; i++)
		fire_lead (self, start, aimdir, damage, kick, TE_SHOTGUN, hspread, vspread, mod);
}


/*
=================
fire_blaster

Fires a single blaster bolt.  Used by the supervisor, dreadlock, evil guardbot, turret, and fatman.
=================
*/

// RSP 081700 - account for blaster bolts locked to a control_shot from a turret.
// One of them is going away, so the links need to be reset.

void blaster_lock_check(edict_t *self)
{
	if (self->locked_to)
	{
		edict_t *control,*bolt1,*bolt2;

		bolt1 = self;
		if (Q_stricmp(bolt1->locked_to->classname,"shot_control") == 0)
		{
			control = bolt1->locked_to;
			if (control->locked_to != bolt1)
				bolt2 = control->locked_to;
			else
				bolt2 = NULL;
		}
		else
		{
			bolt2 = bolt1->locked_to;
			control = bolt2->locked_to;
		}

		// Bolt1 is going away, so reset the links

		if (bolt2)
		{	// link the control entity and the remaining bolt to each other

			bolt2->locked_to = control;
			control->locked_to = bolt2;
		}
		else	// no bolts left, so let control entity die
			G_FreeEdict(control);
	}

	G_FreeEdict(self);
}


void blaster_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int mod;

	if (other == self->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		// RSP 081700 - account for bolts locked to a control_shot from a turret
		blaster_lock_check(self);	// Frees the blaster bolt
		return;
	}

	if (self->owner->client)
		PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage)
	{
		mod = MOD_BLASTER;
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, DAMAGE_ENERGY, mod);
	}
	else
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BLASTER);
		gi.WritePosition (self->s.origin);
		if (!plane)
			gi.WriteDir (vec3_origin);
		else
			gi.WriteDir (plane->normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}

	// RSP 081700 - account for bolts locked to a control_shot from a turret
	blaster_lock_check(self);	// Frees the blaster bolt
}


// RSP 081700 - now returns pointer to bolt edict
edict_t *fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper)
{
	edict_t	*bolt;
	trace_t	tr;

	VectorNormalize (dir);

	bolt = G_Spawn();
	bolt->svflags = SVF_DEADMONSTER;	// 3.20
	// yes, I know it looks weird that projectiles are deadmonsters
	// what this means is that when prediction is used against the object
	// (blaster shots), the player won't be solid clipped against
	// the object.
	VectorCopy (start, bolt->s.origin);
	VectorCopy (start, bolt->s.old_origin);
	vectoangles (dir, bolt->s.angles);
	VectorScale (dir, speed, bolt->velocity);
	bolt->movetype = MOVETYPE_FLYMISSILE;
	bolt->clipmask = MASK_SHOT;
	bolt->solid = SOLID_BBOX;
	bolt->s.effects |= effect;
	VectorClear (bolt->mins);
	VectorClear (bolt->maxs);
	bolt->model = "models/objects/laser/tris.md2";		// RSP 092000
	bolt->s.modelindex = gi.modelindex (bolt->model);	// RSP 092000
	bolt->s.sound = gi.soundindex ("misc/lasfly.wav");
	bolt->owner = self;
	bolt->touch = blaster_touch;
	bolt->nextthink = level.time + 4;	// RSP 052299 - doubled lifetime
	bolt->think = blaster_lock_check;	// RSP 081700 - will also free the bolt
//	bolt->think = G_FreeEdict;
	bolt->dmg = damage;
	bolt->classname = "bolt";

	gi.linkentity (bolt);

	if (self->client)
		check_dodge (self, bolt->s.origin, dir, speed);
	tr = gi.trace (self->s.origin, NULL, NULL, bolt->s.origin, bolt, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (bolt->s.origin, -10, dir, bolt->s.origin);
		bolt->touch (bolt, tr.ent, NULL, NULL);
		return NULL;
	}
	return(bolt);	// RSP 081700
}	
