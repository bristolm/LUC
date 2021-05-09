/*
==============================================================================

fatman bot

==============================================================================
*/

#include "g_local.h"
#include "luc_m_fatman.h"

qboolean visible (edict_t *self, edict_t *other);


static int	sound_pain1;
static int	sound_pain2;	// RSP 083000 - added 2nd pain sound
static int	sound_death1;
static int	sound_death2;
static int	sound_sight;
static int	sound_idle;
static int	sound_attack;	// MRB 022799


void fatman_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void fatman_idle (edict_t *self)
{	// RSP 083000 - if fatman's not rotating while standing, give idle sound
	if (self->spawnflags & SPAWNFLAG_AMBUSH)	// No rotation if AMBUSH set
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

void fatman_run (edict_t *self);
void fatman_stand (edict_t *self);
void fatman_dead (edict_t *self);
void fatman_attack (edict_t *self);
void fatman_end_attack (edict_t *self);
void fatman_reattack (edict_t *self);
void fatman_fire_blaster (edict_t *self);
void fatman_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

// RSP 083000 - Allow fatman to rotate randomly now and then
//              when standing
void fatman_rotate(edict_t *self)
{
	if (self->spawnflags & SPAWNFLAG_AMBUSH)	// No rotation if AMBUSH set
		return;
	
	// if debounce is not -1, and level.time hasn't reached it yet, fatman
	// is facing the way we want, and it's not time to change yet.

	if (self->pain_debounce_time > -1)
	{
		if (level.time < self->pain_debounce_time)
			return;

		// if level.time has reached debounce, then it's time to change the angle

		self->ideal_yaw = random()*360;
	}

	M_ChangeYaw (self);
	if (self->ideal_yaw == self->s.angles[YAW])
		self->pain_debounce_time = level.time + 2;	// stop turning for awhile
	else
		self->pain_debounce_time = -1;				// keep turning

	gi.linkentity (self);	// register the changes
}

mframe_t fatman_frames_stand [] =
{
	ai_stand, 0, fatman_rotate	// RSP 083000 - allow fatman to rotate while standing
};
mmove_t	fatman_move_stand = {FRAME_start01, FRAME_start01, fatman_frames_stand, NULL};

mframe_t fatman_frames_pain1 [] =
{
	ai_move, 4,	NULL
};
mmove_t fatman_move_pain1 = {FRAME_start01, FRAME_start01, fatman_frames_pain1, fatman_run};

mframe_t fatman_frames_walk [] =
{
	ai_walk, 4,	NULL
};
mmove_t fatman_move_walk = {FRAME_start01, FRAME_start01, fatman_frames_walk, NULL};

mframe_t fatman_frames_run [] =
{
	ai_run,	8,	NULL
};
mmove_t fatman_move_run = {FRAME_start01, FRAME_start01, fatman_frames_run, NULL};

mframe_t fatman_frames_run_back [] =
{
	ai_run,	-8,	NULL
};
mmove_t fatman_move_run_back = {FRAME_start01, FRAME_start01, fatman_frames_run_back, NULL};

mframe_t fatman_frames_death1 [] =
{
	ai_move, 7,	NULL
};
mmove_t fatman_move_death1 = {FRAME_start01, FRAME_start01, fatman_frames_death1, fatman_dead};

mframe_t fatman_frames_fire_blaster [] =
{
	ai_charge, 1,	fatman_fire_blaster
};
mmove_t fatman_move_fire_blaster = {FRAME_start01, FRAME_start01, fatman_frames_fire_blaster, fatman_run};

mframe_t fatman_frames_start_attack [] =
{
	ai_charge,	1,	NULL,	// 1
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL,
	ai_charge,	1,	NULL	// 6
};
mmove_t fatman_move_start_attack = {FRAME_attack001, FRAME_attack006, fatman_frames_start_attack, fatman_attack};


void fatman_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &fatman_move_fire_blaster;
}

#define BOMBING_RADIUS 200	// RSP 081700 - to bomb, this area must be clear of littleboys

void fatman_drop_bomb (edict_t *self)
{
	vec3_t	offset, bomb_start, self_fwd, self_right;
	edict_t	*item;
	float	dist,fatman_x,fatman_y,delta_x,delta_y;	// RSP 081700

	// RSP 081700 - The BOMBING_RADIUS distance isn't long enough if fatman is way
	// above the floor, because littleboys could be below fatman, but farther away than BOMBING_RADIUS.
	// New way is to look inside a "drop zone" cylinder below the fatman by ignoring z values. If
	// sqrt((fatman[x] - littleboy[x])^2 + (fatman[y] - littleboy[y])^2) is less than or equal to the
	// BOMBING_RADIUS, then the drop must be aborted.

	dist = BOMBING_RADIUS*BOMBING_RADIUS;
	fatman_x = self->s.origin[0];
	fatman_y = self->s.origin[1];
	for (item = g_edicts ; item < &g_edicts[globals.num_edicts] ; item++)
		if ((item->identity == ID_LITTLEBOY) && item->inuse)
		{
			delta_x = fatman_x - item->s.origin[0];
			delta_y = fatman_y - item->s.origin[1];
			if ((delta_x * delta_x + delta_y * delta_y) <= dist)
			{	// littleboy is in the drop zone, so abort bomb, and fire blaster instead
				fatman_melee(self);
				return;
			}
		}

	// All clear, OK to bomb

	VectorSet (offset, 72, 0, -44);
	AngleVectors (self->s.angles, self_fwd, self_right, NULL);
	G_ProjectSource(self->s.origin,offset,self_fwd,self_right,bomb_start);

	// RSP 051199 - don't spawn bomb inside walls
	if (!(gi.pointcontents(bomb_start) & CONTENTS_SOLID))
	{
		gi.sound (self,CHAN_VOICE,sound_attack,1,ATTN_NORM,0);	// MRB 022799
		fire_bomb(self,bomb_start,self_fwd,100);	// RSP 050999 - move damage values into fire_bomb
	}
}

mframe_t fatman_frames_attack1 [] =
{
	ai_charge,	-8,	fatman_drop_bomb,	// 7
	ai_charge,	-2,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL				// 10
};
mmove_t fatman_move_attack1 = {FRAME_attack007, FRAME_attack010, fatman_frames_attack1, fatman_end_attack};

mframe_t fatman_frames_end_attack [] =
{
	ai_charge,	1,	NULL,	// 11
	ai_charge,	1,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL,
	ai_charge,	0,	NULL	// 15
};
mmove_t fatman_move_end_attack = {FRAME_attack011, FRAME_attack015, fatman_frames_end_attack, fatman_run};

void fatman_reattack (edict_t *self)
{
	if (self->enemy->health > 0)
		if (visible (self, self->enemy))
			if (random() <= 0.6)		
			{
				self->monsterinfo.currentmove = &fatman_move_attack1;
				return;
			}
	self->monsterinfo.currentmove = &fatman_move_end_attack;
}

void fatman_fire_blaster (edict_t *self)
{
	vec3_t	start,offset;	// RSP 083000
	vec3_t	forward, right, up;
	vec3_t	end;
	vec3_t	dir;

	// RSP 081800 - reduce firing rate
	if (random() < 0.75)
		return;

	VectorSet(offset,60,0,-30);	// RSP 083000
	AngleVectors(self->s.angles,forward,right,up);
	R_ProjectSource(self->s.origin,offset,forward,right,up,start);	// RSP 062400 & 083000
	Pick_Client_Target(self,end);			// RSP 052699 - generalized routine
	VectorSubtract(end,start,dir);
	monster_fire_blaster(self,start,dir,1,1000,MZ_BLASTER,EF_HYPERBLASTER);	// RSP 062400
}

void fatman_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &fatman_move_stand;
}

void fatman_run (edict_t *self)
{
	if (self->enemy)
	{
		edict_t	*enemy = self->enemy;
		vec3_t	enemy_spot;

		VectorCopy(enemy->s.origin, enemy_spot);
		enemy_spot[2] += enemy->viewheight;
		VectorSubtract(enemy_spot, self->s.origin, enemy_spot);
		if (enemy_spot[2] > 0)
		{
			// enemy is ABOVE us - try to get up over them
			self->monsterinfo.aiflags &= ~AI_NOSTEP;
		}
		else
		{
			// re-establish our 'hovering'
			self->monsterinfo.aiflags |= AI_NOSTEP;
		}
	}

	self->monsterinfo.currentmove = &fatman_move_run;
}

void fatman_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &fatman_move_walk;
}

void fatman_start_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &fatman_move_start_attack;
}

void fatman_attack(edict_t *self)
{	// fire bomb
	self->monsterinfo.currentmove = &fatman_move_attack1;
}


void fatman_end_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &fatman_move_end_attack;
}

void fatman_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	// RSP 051099 - removed pain skin to match what we did with other
	// monsters
//	if (self->health < (self->max_health / 2))
//		self->s.skinnum = 1;

	waypoint_check(self);	// RSP 071800

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;

	if (skill->value == 3)
		return;		// no pain anims in nightmare

	// RSP 083000 - added 2nd pain sound
	if (rand() & 1)
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	self->monsterinfo.currentmove = &fatman_move_pain1;
}

void fatman_deadthink (edict_t *self)
{
	if (!self->groundentity && (level.time < self->timestamp))
	{
		self->nextthink = level.time + FRAMETIME;
		return;
	}
        
	SpawnExplosion1(self,EXPLODE_FATMAN);  // RSP 062900
	G_FreeEdict(self);
}

void fatman_dead (edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->think = fatman_deadthink;
	self->nextthink = level.time + FRAMETIME;
	self->timestamp = level.time + 15;
	gi.linkentity (self);
}

void fatman_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		for (n = 0 ; n < 2 ; n++)
		{
			ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		}
		ThrowHead (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

// regular death
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &fatman_move_death1;
}

int fatman_checkattack(edict_t *self)
{
	extern int enemy_range;

	vec3_t	spot1, spot2, diff;
	float	dz;
	trace_t	tr;
	float	chance = 0.3; // RSP 011199 make chance a float instead of an int
						  // Allows fatman to fire littleboy

	// This code taken from M_CheckAttack and spruced up a bit
	if (self->enemy->health > 0)
	{
		// see if any entities are in the way of the shot
		VectorCopy (self->s.origin, spot1);
		spot1[2] += self->viewheight;
		VectorCopy (self->enemy->s.origin, spot2);
		spot2[2] += self->enemy->viewheight;

		tr = gi.trace (spot1, NULL, NULL, spot2, self, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_WINDOW);

		// do we have a clear shot?
		if (tr.ent != self->enemy)
			return false;
	}

	// don't always attack in easy mode
	if ((skill->value == 0) && (random() < 0.2))
		return false;

	VectorSubtract(spot2,spot1,diff);
	dz = fabs(diff[2]);

	// melee attack
	if ((enemy_range == RANGE_MELEE) || (dz < (64 + (int)enemy_range * 32)))
	{
		self->monsterinfo.attack_state = AS_MELEE;
		return true;
	}

	// RSP 061799 - It's now ok to be higher than fatman, since littleboys
	// will pause and seek you out; they no longer have to drop on you
	// from above.
//	if (diff[2] > 0)	// too high up
//		return false;

	// fire a bomb now
	if (skill->value == 0)
		chance *= 0.5;
	else if (skill->value >= 2)
		chance *= 2;

	if (random() < chance)
	{
		self->monsterinfo.attack_state = AS_MISSILE;
		self->monsterinfo.attack_finished = level.time + 2*random();
		return true;
	}
	return false;
}

void monster_fatman_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	// MRB 022799 New sound locations
	sound_pain1  = gi.soundindex ("fatman/pain1.wav");	
	sound_pain2  = gi.soundindex ("fatman/pain2.wav");	// RSP 083000 - added 2nd pain sound	
	sound_death1 = gi.soundindex ("fatman/death1.wav");	
	sound_death2 = gi.soundindex ("fatman/death2.wav");	
	sound_sight  = gi.soundindex ("fatman/sight.wav");	
	sound_idle   = gi.soundindex ("fatman/idle.wav");	
	sound_attack = gi.soundindex ("fatman/attack.wav");	

	self->s.sound = gi.soundindex ("fatman/flyby.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/bots/fatman/tris.md2");	// RSP 061899
}


// RSP 011199: Change QUAKED x,y,z values for Fatman
/*QUAKED monster_bot_fatman (1 .5 0) (-88 -88 -64) (88 88 64) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_bot_fatman (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	monster_fatman_indexing(self);

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

//	self->model = "models/monsters/bots/fatman/tris.md2";	// RSP 061899

	self->flags |= FL_PASSENGER;	// RSP 061799
	self->identity = ID_FATMAN;

	VectorSet (self->mins, -88, -88, -64);	// RSP 011199 adjust for reality
	VectorSet (self->maxs,  88,  88,  64);	// RSP 011199 adjust for reality

	self->health = hp_table[HP_FATMAN];	// RSP 032399
	self->mass = 600;					// RSP 053099 - fatman's FAT!
	self->gib_health = -100;
	self->yaw_speed = 4;

	self->pain = fatman_pain;
	self->die = fatman_die;

	self->monsterinfo.stand = fatman_stand;
	self->monsterinfo.walk = fatman_walk;
	self->monsterinfo.run = fatman_run;
	self->monsterinfo.attack = fatman_start_attack;
	self->monsterinfo.melee = fatman_melee;
	self->monsterinfo.sight = fatman_sight;
	self->monsterinfo.idle = fatman_idle;
	self->monsterinfo.aiflags |= AI_NOSTEP;
	self->monsterinfo.checkattack = fatman_checkattack;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &fatman_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}
