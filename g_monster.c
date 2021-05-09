#include "g_local.h"


//
// monster weapons
//

//FIXME monsters should call these with a totally accurate direction
// and we can mess it up based on skill.  Spread should be for normal
// and we can tighten or loosen based on skill.  We could muck with
// the damages too, but I'm not sure that's such a good idea.
void monster_fire_bullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype)
{
	// WPO 231199 monsters can have quad
	if (self->monsterinfo.quad_framenum > level.framenum)
		damage *= 4;

	fire_bullet (self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}


void monster_fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype)
{
	// WPO 231199 monsters can have quad
	if (self->monsterinfo.quad_framenum > level.framenum)
		damage *= 4;

	fire_shotgun (self, start, aimdir, damage, kick, hspread, vspread, count, MOD_UNKNOWN);

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}


edict_t *monster_fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect)
{
	// RSP 061700 - If this is a supervisor or fatman firing,
	// make sure target is within a 30 degree spread centered on direction shooter is facing.
	// Problem was that sometimes the target was off to the side of a shooter, and the shooter was
	// firing a blaster bolt at them anyway.

	edict_t *bolt;

	if ((self->identity == ID_FATMAN) || (self->identity == ID_SUPERVISOR))
	{
		float	adiff;
		vec3_t	a;

		vectoangles(dir,a);
		adiff = anglemod(anglemod(self->s.angles[YAW]) - anglemod(a[YAW]));
		if ((adiff > 15) && (adiff < 345))
			return NULL;	// leave w/o firing
	}

	// WPO 231199 monsters can have quad
	if (self->monsterinfo.quad_framenum > level.framenum)
		damage *= 4;

	bolt = fire_blaster (self, start, dir, damage, speed, effect, false);

	gi.WriteByte (svc_muzzleflash);	// RSP 062400
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
	return(bolt);	// RSP 090700
}	


// MRB 071299 - MONSTER_FIRE_* functions for Suspension Software
  
void monster_fire_diskgun (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype)
{
	// WPO 231199 monsters can have quad
	if (self->monsterinfo.quad_framenum > level.framenum)
		damage *= 4;

	launch_disk (self,  start, dir, damage, 1000, 0);	// RSP 071399 - change MOD_DISKGUN to 0

	gi.WriteByte (svc_muzzleflash2);
	gi.WriteShort (self - g_edicts);
	gi.WriteByte (flashtype);
	gi.multicast (start, MULTICAST_PVS);
}


//
// Monster utility functions
//

void AttackFinished (edict_t *self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}


void M_CheckGround (edict_t *ent)
{
	vec3_t	point;
	trace_t	trace;

	if (ent->flags & (FL_SWIM|FL_FLY))
	{
		ent->groundentity = NULL;		// RSP 090200 - some insurance
		return;
	}

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 0.25;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

//	ent->groundentity = trace.ent;
//	ent->groundentity_linkcount = trace.ent->linkcount;
//	if (!trace.startsolid && !trace.allsolid)
//		VectorCopy (trace.endpos, ent->s.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}


void M_CatagorizePosition (edict_t *ent)
{
	vec3_t	point;
	int		cont;
	float	height = ent->maxs[2] - ent->mins[2];	// RSP 092000

//
// get waterlevel
//

	// First, check the feet

	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + ent->mins[2] + 1;	
	cont = gi.pointcontents (point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = WL_DRY;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;	// We now know the water type

	// Second, check the waist

//	point[2] += 26;
	point[2] += 0.46*height;	// RSP 092000 - generalize for all entities

	cont = gi.pointcontents (point);
	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = WL_FEETWET;
		return;
	}

	// Third, check the chin

//	point[2] += 22;
	point[2] += 0.39*height;	// RSP 092000 - generalize for all entities

	cont = gi.pointcontents (point);
	if (cont & MASK_WATER)
		ent->waterlevel = WL_HEADWET;
	else
		ent->waterlevel = WL_WAISTWET;
}


void M_WorldEffects (edict_t *ent)
{
	int dmg;

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < WL_HEADWET)
			{
				ent->air_finished = level.time + 12;
			}
			else if (ent->air_finished < level.time)
			{	// drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
		else
		{
			if (ent->waterlevel != WL_DRY)	// RSP 092000 != is easier to understand than >
			{
				ent->air_finished = level.time + 9;
			}
			else if (ent->air_finished < level.time)
			{	// suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
	}
	
	if (ent->waterlevel == WL_DRY)
	{
		if (ent->flags & FL_INWATER)
		{	
			gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) /* && !(ent->flags & FL_IMMUNE_LAVA) */)	// RSP 092100
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10*ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}
	if ((ent->watertype & CONTENTS_SLIME) /* && !(ent->flags & FL_IMMUNE_SLIME) */)	// RSP 092100
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4*ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}
	
	if (!(ent->flags & FL_INWATER))
	{	
		if (!(ent->svflags & SVF_DEADMONSTER))
		{
			if (ent->watertype & CONTENTS_LAVA)
				if (random() <= 0.5)
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}


void M_droptofloor (edict_t *ent)
{
	vec3_t	end;
	trace_t	trace;

	ent->s.origin[2] += 1;
	VectorCopy (ent->s.origin, end);
	end[2] -= 256;
	
	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy (trace.endpos, ent->s.origin);

	gi.linkentity (ent);
	M_CheckGround (ent);
	M_CatagorizePosition (ent);
}


void M_SetEffects (edict_t *ent)
{
	int remaining;

	ent->s.effects &= ~(EF_COLOR_SHELL|EF_POWERSCREEN);
	ent->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->health <= 0)
		return;

	// AJA 19980908 - Make the monster glow yellow if it is being
	// recharged by the supervisor bot (AI_RECHARGING is set).
	if (ent->monsterinfo.aiflags & AI_RECHARGING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED | RF_SHELL_GREEN);
	}

	// RSP 071099 - even if there's no armor in LUC, this section has to stay
	// so the supervisor can keep his armor.

	if (ent->powerarmor_time > level.time)
	{
		if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	// WPO 251099 freeze gun
	if (ent->freeze_framenum > level.framenum)
	{
		remaining = ent->freeze_framenum - level.framenum;
		// WPO 231199 monsters can have quad too
		if (remaining == 1)
		{
			ent->monsterinfo.quad_framenum = level.framenum + 300;
			ent->s.effects |= EF_QUAD;
			gi.sound(ent, CHAN_RELIABLE+CHAN_ITEM, gi.soundindex("items/quaddamage.wav"), 1, ATTN_NONE, 0);
		}
		if (remaining > 30 || (remaining & 4))
			ent->s.effects |= EF_QUAD;
		else
			ent->s.effects &= ~EF_QUAD;
	}
	// WPO 231199 monsters can have quad too
	else
	if (ent->monsterinfo.quad_framenum > level.framenum)
	{
		remaining = ent->monsterinfo.quad_framenum - level.framenum;
		if (remaining == 1)
			ent->monsterinfo.quad_framenum = 0;
		if (remaining > 30 || (remaining & 4))
			ent->s.effects |= EF_QUAD;
		else
			ent->s.effects &= ~EF_QUAD;
	}
}


void M_MoveFrame (edict_t *self)
{
	mmove_t	*move;
	int		index;

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + FRAMETIME;

	// RSP 051199 - this section moved from back of routine to front
	// otherwise, the first ai_func and thinkfunc of the first frame don't
	// get executed if there's only one item in the move table.

	if ((self->s.frame < move->firstframe) || (self->s.frame > move->lastframe))
	{
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->s.frame = move->firstframe;
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc (self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc (self, 0);

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc (self);
	// end of moved section

	if ((self->monsterinfo.nextframe >= 0) && (self->monsterinfo.nextframe >= move->firstframe) && (self->monsterinfo.nextframe <= move->lastframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = -1;	// RSP 051199
	}
	else
	{
		if (self->s.frame == move->lastframe)
		{
			if (move->endfunc)
			{
				move->endfunc (self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}
	}

	if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
	{
		self->s.frame++;
		if (self->s.frame > move->lastframe)
			self->s.frame = move->firstframe;
	}
}


void monster_think (edict_t *self)
{
	if (self->flags & FL_SWIM)
		if (!(gi.pointcontents(self->s.origin) & MASK_WATER))
			self->gravity = 1;
		else
			self->gravity = 0.05;

	M_MoveFrame (self);

	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
}


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;

// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget (self);
}


void monster_start_go (edict_t *self);


void monster_triggered_spawn (edict_t *self)
{
	self->s.origin[2] += 1;
	KillBox (self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity (self);

	// RSP 043099 - Commented out. Seemed redundant, since monster_start_go() has already
	// been called during the regular spawn of the routine of the monster.
//	monster_start_go (self);

	// These 2 lines were brought out of the now unused monster_start_go. Still needed.
	self->think = monster_think;
	self->nextthink = level.time + FRAMETIME;

	if (self->enemy &&
		!(self->spawnflags & SPAWNFLAG_AMBUSH) &&
		!(self->enemy->flags & FL_NOTARGET))
	{
		FoundTarget (self);
	}
	else
		self->enemy = NULL;
}

void monster_triggered_spawn_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;

	// RSP 043099
	// At this point, if the spawned monster is targeted at an attack waypoint, he has
	// already been set up to move toward it, so you don't have to do anything.
	// If this is true, then you don't want to set the enemy to the activator (the entity
	// that touched the trigger that spawned you).

	if (!(self->movetarget && (self->movetarget->identity == ID_WAYPOINT)))
		// RSP 032599 - don't set enemy if this is a PACIFIST
		if (activator->client && !(self->monsterinfo.aiflags & AI_PACIFIST))
			self->enemy = activator;

	self->use = monster_use;
}

void monster_triggered_start (edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = monster_triggered_spawn_use;
}


/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use (edict_t *self)
{
	self->flags &= ~(FL_FLY|FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item)
	{
		Drop_Item (self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	G_UseTargets (self, self->enemy);
}


//============================================================================

qboolean monster_start (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return false;
	}

	if ((self->spawnflags & SPAWNFLAG_SIGHT) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~SPAWNFLAG_SIGHT;
		self->spawnflags |= SPAWNFLAG_AMBUSH;
	}

	// MRB 120698 - add Pacifist into monster code for now
	if (self->spawnflags & SPAWNFLAG_PACIFIST)
		self->monsterinfo.aiflags |= AI_PACIFIST;

	if (!(self->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->air_finished = level.time + 12;
	self->use = monster_use;
	self->max_health = self->health;
	self->clipmask = MASK_MONSTERSOLID;

	self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = M_CheckAttack;
	VectorCopy (self->s.origin, self->s.old_origin);

	if (st.item)
	{
		self->item = FindItemByClassname (st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
	}

	// randomize what frame they start on
	if (self->monsterinfo.currentmove)
		self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));

	return true;
}

void monster_start_go (edict_t *self)
{
	vec3_t v;

	if (self->health <= 0)
		return;

	// check for target to combat_point and change to combattarget
	if (self->target)
	{
		qboolean	notcombat;
		qboolean	fixup;
		edict_t		*target;

		target = NULL;
		notcombat = false;
		fixup = false;
		while ((target = G_Find (target, FOFS(targetname), self->target)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.dprintf("%s at %s has target with mixed types\n", self->classname, vtos(self->s.origin));
		if (fixup)
			self->target = NULL;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t *target;

		target = NULL;
		while ((target = G_Find (target, FOFS(targetname), self->combattarget)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.dprintf("%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
					self->classname, (int)self->s.origin[0], (int)self->s.origin[1], (int)self->s.origin[2],
					self->combattarget, target->classname, (int)target->s.origin[0], (int)target->s.origin[1],
					(int)target->s.origin[2]);
			}
		}
	}

	if (self->attack)	// RSP 050199
	{
		self->goalentity = self->movetarget = G_PickTarget(self->attack);
		if (!self->movetarget)
		{
			gi.dprintf ("%s can't find attack target %s at %s\n", self->classname, self->attack, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
		// RSP 043099 - Added waypoints. Is self->attack an ATTACK waypoint?
		else if (self->movetarget->identity == ID_WAYPOINT)
		{
			if (self->movetarget->spawnflags & SPAWNFLAG_WAYPOINT_RETREAT)
			{
				gi.dprintf("%s %s has RETREAT as first waypoint\n",self->classname,vtos(self->s.origin));
				self->goalentity = self->movetarget = NULL;
				self->monsterinfo.pausetime = 100000000;
				self->monsterinfo.stand (self);
			}
			else	// Attack waypoint. Start moving toward it.
			{
				VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
				self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
				self->monsterinfo.run(self);
				self->target = NULL;
				self->flags |= FL_ATTACKING;
			}
		}
	}
	else if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);
		if (!self->movetarget)
		{
			gi.dprintf ("%s can't find target %s at %s\n", self->classname, self->target, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
		else if (strcmp (self->movetarget->classname, "path_corner") == 0)
		{
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			self->monsterinfo.walk (self);
			self->target = NULL;
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.stand (self);
	}

	self->think = monster_think;
	self->nextthink = level.time + FRAMETIME;
}


void walkmonster_start_go (edict_t *self)
{
	if (!(self->spawnflags & SPAWNFLAG_TRIGGER_SPAWN) && (level.time < 1))
	{
		M_droptofloor (self);

		if (self->groundentity)
			if (!M_walkmove (self, 0, 0))
				gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));
	}
	
	if (!self->yaw_speed)
		self->yaw_speed = 20;
//	self->viewheight = 25;
	self->viewheight = 0.75*(self->maxs[2] - self->mins[2]) + self->mins[2];	// RSP 080600

	monster_start_go (self);

	if (self->spawnflags & SPAWNFLAG_TRIGGER_SPAWN)
		monster_triggered_start (self);
}

void walkmonster_start (edict_t *self)
{
	self->think = walkmonster_start_go;
	monster_start (self);
}


void flymonster_start_go (edict_t *self)
{
	if (!M_walkmove (self, 0, 0))
		gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));

	if (!self->yaw_speed)
		self->yaw_speed = 10;
//	self->viewheight = 25;
	self->viewheight = (int) 0.75*(self->maxs[2] - self->mins[2]) + self->mins[2];	// RSP 080600

	monster_start_go (self);

	if (self->spawnflags & SPAWNFLAG_TRIGGER_SPAWN)
		monster_triggered_start (self);
}


void flymonster_start (edict_t *self)
{
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start (self);
}


void swimmonster_start_go (edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 10;

	monster_start_go (self);

	if (self->spawnflags & SPAWNFLAG_TRIGGER_SPAWN)
		monster_triggered_start (self);
}

void swimmonster_start (edict_t *self)
{
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start (self);
}
