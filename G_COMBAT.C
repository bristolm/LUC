// g_combat.c

#include "g_local.h"
#include "luc_powerups.h"	// WPO 231099 - Vampire
#include "luc_hbot.h"		// RSP 070700

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}
	
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	// RSP 061700 - need to turn off recharging if this is the supervisor getting killed
	// and he was recharging a buddy

	if  (targ->enemy &&
		(targ->enemy->monsterinfo.aiflags & AI_RECHARGING) &&
		(targ->identity == ID_SUPERVISOR))	// RSP 062199 - use identity flag
	{
		targ->enemy->monsterinfo.aiflags &= ~AI_RECHARGING;
	}

	targ->enemy = attacker;

	// WPO 081099 dreadlock
	// Was this the dreadlock which just got killed?

	if (targ->identity == ID_DLOCK)		// WPO 090299  Use identity
	{
		// Flag Think() to detonate on next frame

		targ->dlockOwner->client->pers.dreadlock = DLOCK_UNAVAILABLE;
		return; 
	}
	
	// ignore for a decoy

	if (targ->identity == ID_DECOY)
		return;

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
//		targ->svflags |= SVF_DEADMONSTER;	// now treat as a different content type
		if (!(targ->monsterinfo.aiflags & AI_GOOD_GUY))
		{
			level.killed_monsters++;
			if (coop->value && attacker->client)
				attacker->client->resp.score++;
		}
	}

	if ((targ->movetype == MOVETYPE_PUSH) || (targ->movetype == MOVETYPE_STOP) || (targ->movetype == MOVETYPE_NONE))
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
		targ->touch = NULL;
		monster_death_use (targ);
	}

	targ->s.effects &= ~EF_QUAD;			// WPO 251099 - freeze gun
	targ->freeze_framenum = 0;				// WPO 251099 - freeze gun
	targ->monsterinfo.quad_framenum = 0;	// WPO 231199 - quad
	targ->die (targ, inflictor, attacker, damage, point);
}


/*
================
SpawnDamage
================
*/
void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
//	gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

static int CheckPowerArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			power_armor_type;
	int			index;
	int			damagePerCell;
	int			pa_te_type;
	int			power;
	int			power_used;

	if (!damage)
		return 0;

	client = ent->client;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	if (client)
	{
		power_armor_type = PowerArmorType (ent);
		if (power_armor_type != POWER_ARMOR_NONE)
		{
			index = ITEM_INDEX(FindItem("Cells"));
			power = client->pers.inventory[index];
		}
	}
	else if (ent->svflags & SVF_MONSTER)
	{
		power_armor_type = ent->monsterinfo.power_armor_type;
		power = ent->monsterinfo.power_armor_power;
	}
	else
		return 0;

	if (power_armor_type == POWER_ARMOR_NONE)
		return 0;

	if (!power)
		return 0;

	if (power_armor_type == POWER_ARMOR_SCREEN)
	{
		vec3_t		vec;
		float		dot;
		vec3_t		forward;

		// only works if damage point is in front
		AngleVectors (ent->s.angles, forward, NULL, NULL);
		VectorSubtract (point, ent->s.origin, vec);
		VectorNormalize (vec);
		dot = DotProduct (vec, forward);
		if (dot <= 0.3)
			return 0;

		damagePerCell = 1;
		pa_te_type = TE_SCREEN_SPARKS;
		damage = damage / 3;
	}
	else
	{
		damagePerCell = 2;
		pa_te_type = TE_SHIELD_SPARKS;
		damage = (2 * damage) / 3;
	}

	save = power * damagePerCell;
	if (!save)
		return 0;

	if (save > damage)
		save = damage;

	SpawnDamage (pa_te_type, point, normal, save);
	ent->powerarmor_time = level.time + 0.2;

	power_used = save / damagePerCell;

	if (client)
		client->pers.inventory[index] -= power_used;
	else if (ent->identity != ID_GUARDBOT)	// RSP 100700 - guardbot screen uses no power; always up
		ent->monsterinfo.power_armor_power -= power_used;
	return save;
}


void M_ReactToDamage (edict_t *targ, edict_t *attacker)
{
	// WPO 090199 If the dreadlock attacks a monster, have the
	// monster see the dreadlock as an enemy.
	// RSP 070800 If the target already has this dreadlock as its enemy, do nothing

	if (attacker->identity == ID_DLOCK)
	{
		if (attacker != targ->enemy)
		{
			edict_t *e = targ->enemy;

			// disengage the current enemy if it's a player

			if (e->client)
				Forget_Player(targ);

			targ->oldenemy = e;		// RSP 070800 - so player will be re-targetted if the dreadlock dies

			// set the dreadlock as the current enemy

			targ->enemy = targ->goalentity = attacker;
		}
		return;
	}

	if (!(attacker->client) && !(attacker->svflags & SVF_MONSTER))
		return;

	// RSP 110299 - recognize decoy
	// At this point, we used to return if the attacker was already our
	// enemy. Now we want to check to see if the attacker has a decoy that
	// we can see. If so, we want to switch our enemy to the decoy, away
	// from the attacker.

	if (attacker == targ)
		return;
	
	if (attacker == targ->enemy)
	{
		if (attacker->decoy)
		{
			if (visible(targ,attacker->decoy))
			{
				Forget_Player(targ);	// you can see the decoy, so target it
				targ->enemy = targ->goalentity = attacker->decoy;
			}
		}

		// RSP 110799 - if this is a client who's cloaked, the monster can't see it
		if (attacker->svflags & SVF_NOCLIENT)
			Forget_Player(targ);	// player is invisible

		return;
	}

	// AJA 19980906 - If we are a passive creature and another passive
	// creature harms us, don't get mad.
	if ( (targ->monsterinfo.aiflags & AI_PACIFIST) &&
		 (attacker->monsterinfo.aiflags & AI_PACIFIST) )
		 return;

	// if we are a good guy monster and our attacker is a player
	// or another good guy, do not get mad at them
	if (targ->monsterinfo.aiflags & AI_GOOD_GUY)
	{
		if (attacker->client || (attacker->monsterinfo.aiflags & AI_GOOD_GUY))
			return;
	}

	// we now know that we are not both good guys

	//MRB 031999 - Need to turn off the glowing target if this is a supervisor being hurt
	if  (targ->enemy &&
		(targ->enemy->monsterinfo.aiflags & AI_RECHARGING) &&
		(targ->identity == ID_SUPERVISOR))	// RSP 062199 - use identity flag
	{
		targ->enemy->monsterinfo.aiflags &= ~AI_RECHARGING;
	}

	// if attacker is a client, get mad at them because he's good and we're not
	if (attacker->client)
	{
		// RSP 061599 - can't see this client if he has NOTARGET set
		if (attacker->flags & FL_NOTARGET)
			return;

		// RSP 110799 - if this is a client who's cloaked, the monster can't see it
		if (attacker->svflags & SVF_NOCLIENT)
			return;	// player is invisible

		targ->monsterinfo.aiflags &= ~AI_SOUND_TARGET;	// 3.20

		// RSP 110299 - recognize decoy
		// if the attacker has dropped a decoy, go after the decoy if you can see it
		if (attacker->decoy)
		{
			if (visible(targ,attacker->decoy))
			{
				Forget_Player(targ);	// you can see the decoy, so target it
				targ->enemy = targ->goalentity = attacker->decoy;
				return;
			}
		}

		// this can only happen in coop (both new and old enemies are clients)
		// only switch if can't see the current enemy
		if (targ->enemy && targ->enemy->client)
		{
			if (visible(targ, targ->enemy))
			{
				targ->oldenemy = attacker;
				return;
			}
			targ->oldenemy = targ->enemy;
		}

		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
		return;
	}

	// it's the same base (walk/swim/fly) type and a different classname, 
	// get mad at them
	if (((targ->flags & (FL_FLY|FL_SWIM)) == (attacker->flags & (FL_FLY|FL_SWIM))) &&
		 (strcmp (targ->classname, attacker->classname) != 0))
	{
		if (targ->enemy && targ->enemy->client)	// 3.20
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}

	// 3.20
	// if they *meant* to shoot us, then shoot back
	else if (attacker->enemy == targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// otherwise get mad at whoever they are mad at (help our buddy) unless it is us!
	else if (attacker->enemy && attacker->enemy != targ)
	{
		if (targ->enemy && targ->enemy->client)
			targ->oldenemy = targ->enemy;
		targ->enemy = attacker->enemy;
		if (!(targ->monsterinfo.aiflags & AI_DUCKED))
			FoundTarget (targ);
	}
	// 3.20 end
}

qboolean CheckTeamDamage (edict_t *targ, edict_t *attacker)
{
		//FIXME make the next line real and uncomment this block
		// if ((ability to damage a teammate == OFF) && (targ's team == attacker's team))
	return false;
}

// WPO 090499 - Knock prethink function for flyers
#define KB_DECELLERATION	0.500
#define KB_DECEL_LIMIT		0.100

void Knockback_Prethink(edict_t *ent) 
{
	int i;

	// if below the threshold, clear velocity
	if (ent->velocity[0] < KB_DECEL_LIMIT &&
		ent->velocity[1] < KB_DECEL_LIMIT && 
		ent->velocity[2] < KB_DECEL_LIMIT)
	{
		VectorClear(ent->velocity);
		ent->prethink = NULL;
		return;
	}

	// slowly decelerate velocity
	for (i = 0; i < 3; i++)
		ent->velocity[i] *= KB_DECELLERATION;
}

// RSP 061600
// If a monster gets hurt, wake up any pacifist that can see the monster.
// If your buddy's hurt, you need to defend him by attacking his attacker.

void WakeUpPacifists(edict_t *self)
{
	edict_t	*item;
	float	dist;

	item = NULL;
	while (item = _findlifeinradius(self,item,self->s.origin,2000,false,true,false,&dist))
		if ((item != self) &&
			(item->svflags & SVF_MONSTER) &&
			(item->identity != ID_TURRET) &&
			(item->identity != ID_JELLY) &&
			visible(self,item))
		{
			// turn off the pacifist flags
			item->spawnflags &= ~SPAWNFLAG_PACIFIST;
			item->monsterinfo.aiflags &= ~AI_PACIFIST;
		}
}


void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t	*client;
	int			take;
	int			save  = 0;
	int			asave = 0;
	int			psave = 0;
	int			te_sparks;

	edict_t		*orig_targ = targ;

	if (!targ || !inflictor || !attacker)	// RSP 092500
		return;

	// RSP 022499: Kludge to keep LUC monsters from hurting each other
	if ((targ->svflags & SVF_MONSTER) && (attacker->svflags & SVF_MONSTER))
		return;

	// MRB 19980917 - shift 'pain' to master entity.
	if (targ->flags & FL_MASTER_EXT)
		targ = targ->teammaster;

	if (!targ->takedamage)
		return;

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker) && ((deathmatch->value && ((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS))) || coop->value))
	{
		if (OnSameTeam (targ, attacker))
		{
			if ((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE)
				damage = 0;
			else
				mod |= MOD_FRIENDLY_FIRE;
		}
	}
	meansOfDeath = mod;

	// easy mode takes half damage
	if ((skill->value == 0) && (deathmatch->value == 0) && targ->client)
	{
		damage *= 0.5;
		if (!damage)
			damage = 1;
	}

	client = targ->client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			if (targ->client  && (attacker == targ))
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);

			// WPO 090399- Signal knockback in progress
			if (targ->movetype == MOVETYPE_FLY)
				if (targ->prethink == NULL)
					targ->prethink = Knockback_Prethink;

			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}

	take = damage;
	save = 0;

	// check for godmode
	if (((targ->flags & FL_GODMODE) || (client && (client->invincible_framenum > level.framenum))) &&
		!(dflags & DAMAGE_NO_PROTECTION))
	{
		take = 0;
		save = damage;
		SpawnDamage (te_sparks, point, normal, save);
	}

	// RSP 032399 - There's no armor in LUC, but the supervisor can put up a powerarmor shield
	if (client)		// RSP 081600 - keep player from calling CheckPowerArmor
		psave = 0;
	else
		psave = CheckPowerArmor (targ, point, normal, take, dflags);
	take -= psave;

	//treat cheat/powerup savings the same as armor
	asave += save;

	// team damage avoidance
	if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage (targ, attacker))
		return;

	// do the damage (if this isn't a guardbot in the endgame)
	if (take && (targ->identity != ID_GUARDBOT))	// RSP 100700
	{	// RSP 051899 - FL_SPARKDAMAGE now says whether we spawn blood for organic monsters
		// or sparks for metallic ones
		if (((targ->svflags & SVF_MONSTER) && !(targ->flags & FL_SPARKDAMAGE)) || client)
			SpawnDamage (TE_BLOOD, point, normal, take);
		else
			SpawnDamage (te_sparks, point, normal, take);

		// WPO 141199 - Vampire change
		if (attacker->client)
			if ((targ != attacker) && (targ->health > 0) && (attacker->client->vampire_framenum > level.framenum))
			{
				if (attacker->health < VAMPIRE_MAX_HEALTH)
				{
					attacker->health += (take > targ->health) ? targ->health : take;
					if (attacker->health > VAMPIRE_MAX_HEALTH)
						attacker->health = VAMPIRE_MAX_HEALTH;
				}
			}
			
		targ->health = targ->health - take;

		if (targ->health <= 0)
		{
			if ((targ->svflags & SVF_MONSTER) || client)
				targ->flags |= FL_NO_KNOCKBACK;
			Killed (targ, inflictor, attacker, take, point);
			return;
		}
	}

	if ((targ != orig_targ) && orig_targ->pain)
		orig_targ->pain (orig_targ, attacker, knockback, take);

	if (targ->svflags & SVF_MONSTER)
	{

		// RSP 100299 - turrets and littleboys don't react the
		// way other monsters do. This is the fix
		// for the matrix shot killing a turret in the train area of lm2a.
		// RSP 100700 - guardbots don't react either

		if ((targ->identity != ID_TURRET)		&&
			(targ->identity != ID_LITTLEBOY)	&&
			(targ->identity != ID_BEETLE)		&&		// RSP 080700 - added beetle check
			(targ->identity != ID_GUARDBOT))
		{
			M_ReactToDamage (targ, attacker);
		}

		if (!(targ->monsterinfo.aiflags & AI_DUCKED) && take)
		{
			targ->pain (targ, attacker, knockback, take);
			// nightmare mode monsters don't go into pain frames often
			if (skill->value == 3)
				targ->pain_debounce_time = level.time + 5;
		}

		// RSP 061600 - de-pacify any nearby pacifists, to come to your aid, if you were hurt
		// by the player
		if (attacker && attacker->client)	// RSP 091400
			WakeUpPacifists(targ);
	}
	else if (client)
	{
		if (take && !((targ->flags & FL_GODMODE) || (targ->client->invincible_framenum > level.framenum)))
			targ->pain (targ, attacker, knockback, take);
	}
	else if (take)
	{	// MRB - allow extent to feel damage if need be
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends, view angle kicks, and pain sounds
	// at the end of the frame
	if (client)
	{
		client->damage_parmor += psave;
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
// RSP 010399: T_RadiusDamage now returns true if a victim was found, false if not
qboolean T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;
	qboolean found_one = false; // RSP 010399: true if something struck
	trace_t	trace;	// RSP 032399 - for proper placement of blood splat

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;

		if (!ent->takedamage)
			continue;

		if (ent == inflictor)	// RSP 051799
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);

				// RSP 032399 - On a radial hit, blood from the hit entity was spawning at the spot
				// where the projectile was striking. It should be spawning at a spot on the entity
				// closest to where the projectile is striking. Added a gi.trace() to find that
				// spot.
				trace = gi.trace (inflictor->s.origin,NULL,NULL,ent->s.origin,inflictor,MASK_MONSTERSOLID);
				T_Damage (ent, inflictor, attacker, dir, trace.endpos, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
				found_one = true; // RSP 011199: We'll be needing sparks
			}
		}
	}
	return found_one; // RSP 010399: return results
}

