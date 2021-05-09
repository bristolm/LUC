/*
==============================================================================

Battle bot - Slow moving, Wields 2 weapons max.

NOTE:  I'm simulating the bolt gun by just firing lead and allowing a random
       chance that the gun might jam.  It's just easier that way.  We can fix
	   if we want, but I think this is better

==============================================================================
*/

#include "g_local.h"
#include "luc_m_battle.h"
extern qboolean enemy_vis;	// RSP 042999
extern int	enemy_range;	// RSP 042999

// MRB 071999 - put sounds back in
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die1;
static int	sound_die2;
static int	sound_attack;
static int	sound_sight;
static int	sound_idle;	
static int  sound_step;

// MRB 091699 - added in the _MODEL stuff
#define BATTLE_RIGHT_WEAPON		self->s.modelindex2
#define BATTLE_RIGHT_MODEL		self->model2

#define BATTLE_LEFT_WEAPON		self->s.modelindex3
#define BATTLE_LEFT_MODEL		self->model3

//#define BOLT_LEFT_HEAT(ent)	(int)(ent->count & 0xFF00) >> 8			// RSP 081400
//#define BOLT_RIGHT_HEAT(ent)	(int)(ent->count & 0x00FF)				// RSP 081400

#define BATTLE_KICK_RANGE		90
#define BATTLE_KICK_POWER		350

void battle_attack(edict_t *self);

void battle_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void chooseFromNeutral (edict_t *self)
{
	int n = rand() % 4;

	if (n == 0)
		self->monsterinfo.nextframe = 0;
	else	// The frames just work out this way ...
		self->monsterinfo.nextframe = 10 + ((n -1) * 30);

	self->monsterinfo.nextframe += FRAME_stnd001;
}

void chooseStayOrGo (edict_t *self)
{
	int n = rand() % 2;

	if (n > 0)
		self->monsterinfo.nextframe = self->s.frame - 9;

	n = rand() % 10;

	// make an idle sound every now and then
	if (n == 1)
		gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_NORM, 0);
}

// This function is solely to try to keep 2 bots 
// standing side by side from totally mimicing each other
void jitterSync (edict_t *self)
{
	int n = rand() % 2;

	if (n == 0)
		self->monsterinfo.nextframe = self->s.frame + 2;
}


mframe_t battle_frames_stand [] =
{	// At neutral pose
	ai_stand, 0, NULL,	// 1
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseStayOrGo,	// 10
	// To left pose
	ai_stand, 0, NULL,	// 11
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 20
	// At left pose
	ai_stand, 0, NULL,	// 21
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseStayOrGo,	// 30
	// To Neutral pose
	ai_stand, 0, NULL,	// 31
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseFromNeutral,	// 40
	// To right pose
	ai_stand, 0, NULL,	// 41
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 50
	// At right pose
	ai_stand, 0, NULL,	// 51
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseStayOrGo,	// 60
	// to neutral pose
	ai_stand, 0, NULL,	// 61
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseFromNeutral,	// 70
	// to Up pose
	ai_stand, 0, NULL,	// 71
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 80
	// At up pose
	ai_stand, 0, NULL,	// 81
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseStayOrGo,	// 90
	//To Neutral pose
	ai_stand, 0, NULL,	// 91
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, jitterSync,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, chooseFromNeutral	// 100
};
mmove_t battle_move_stand = {FRAME_stnd001, FRAME_stnd100, battle_frames_stand, NULL};

void battle_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &battle_move_stand;
}

void battleStepSound (edict_t *self)
{
	gi.sound (self, CHAN_BODY, sound_step, 1, ATTN_IDLE, 0);
}

// WALK
mframe_t battle_frames_walk [] =
{
	ai_walk, 10, NULL,	// 1
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, battleStepSound,
	ai_walk, 10, NULL,

	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, NULL,
	ai_walk, 10, battleStepSound,
	ai_walk, 10, NULL	// 10
};
mmove_t battle_move_walk = {FRAME_walk001, FRAME_walk010, battle_frames_walk, NULL};

void battle_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &battle_move_walk;
}

// RUN

mframe_t battle_frames_run [] =
{
	ai_run, 15, NULL,	// 1
	ai_run, 15, NULL,
	ai_run, 15, battleStepSound,
	ai_run, 15, NULL,
	ai_run, 15, NULL,

	ai_run, 15, NULL,
	ai_run, 15, battleStepSound,
	ai_run, 15, NULL	// 8
};
mmove_t battle_move_run = {FRAME_run001, FRAME_run008, battle_frames_run, NULL};

void battle_run (edict_t *self)
{
	self->monsterinfo.currentmove = &battle_move_run;
}

// SIGHT

void battle_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// MELEE (short range) attack

void battle_kick (edict_t *self)
{
	vec3_t aim;

	VectorSet(aim,BATTLE_KICK_RANGE,0,0);
	(void)fire_hit(self,aim,wd_table[WD_BATTLE_KICK],BATTLE_KICK_POWER);
}


mframe_t battle_frames_melee [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, battle_kick,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 7
};
mmove_t battle_move_melee = {FRAME_mel001, FRAME_mel007, battle_frames_melee, battle_run};

void battle_melee (edict_t *self)
{
	self->monsterinfo.currentmove = &battle_move_melee;
}

// ATTACK

static vec3_t vecDiskLeft  = {24,-25.5, 40};
static vec3_t vecDiskRight = {24, 25.5, 40};

static float DiskSpeed = 1000;

void battle_fire_diskgun (edict_t *self, vec3_t start, vec3_t fire_dir)
{
	int damage = wd_table[WD_DISKGUN] / 2;		// MRB 071099 - seed damage from player table
	
	monster_fire_diskgun (self, start, fire_dir, damage, DiskSpeed, 0);
}

void battle_fire_boltgun (edict_t *self, edict_t *enemy, vec3_t vstart, int flash)
{
	vec3_t vecEnemy, fire_dir;

	// RSP 082400 - project enemy back a bit and target there
	VectorCopy (enemy->s.origin, vecEnemy);
	VectorMA (vecEnemy, -0.2, enemy->velocity, vecEnemy);
	vecEnemy[2] += enemy->viewheight;

	VectorSubtract(vecEnemy,vstart,fire_dir);
	VectorNormalize(fire_dir);
	gi.sound (self, CHAN_AUTO, sound_attack, 1, ATTN_NORM, 0);
	monster_fire_bullet (self, vstart, fire_dir, 1, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash);
}

// Return false if the end target is too far out of sight ...

qboolean LeadTarget(edict_t *self, edict_t *enemy, vec3_t vstart, vec3_t fire_dir)
{	// Try to find out where the player is going - lead him a bit.
	// We'll hack this really roughly based on distance ...
	vec3_t	vDiff, vEnd, vecEnemyCenter;
	float	DiffLen, Timetohit;
	int		i;
	int		j = 1 + skill->value;

	if ((enemy == NULL) || !wellInfront(self,enemy, 0.6))
		return false;

	VectorMA(enemy->absmin,0.5,enemy->size,vecEnemyCenter);
	VectorCopy(vecEnemyCenter,vEnd);

	while (j-- > 0)
	{
		// First see how close we'd be if we fired at the enemy.
		VectorSubtract(vEnd, vstart,vDiff);
		DiffLen = VectorLength(vDiff);
		Timetohit = DiffLen / DiskSpeed;

		for (i = 0 ; i < 3 ; i++)
			vEnd[i] = vecEnemyCenter[i] + (enemy->velocity[i] * Timetohit);
	}

	VectorSubtract(vEnd, vstart,fire_dir);
	VectorNormalize(fire_dir);

	return true;
}


void battle_fire_left(edict_t *self)
{
	vec3_t	vstart, fire_dir;
	vec3_t	forward, right;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin,vecDiskLeft,forward,right,vstart);

	if (self->identity == ID_BATTLEBOT_DISK)	// RSP 082200
	{	// Fire Disk gun
		if (self->s.frame == FRAME_fireleft1)
		{
			if (!LeadTarget(self, self->enemy, vstart, fire_dir))
			{
				battle_run(self);
				return;
			}
			battle_fire_diskgun(self, vstart, fire_dir);
//			battleMakeGunSounds(self,0,1);	// RSP 102499 - not needed, diskgun takes care of sounds
		}
	}
	else
	{	// Fire 'bolt' gun
		if (rand()&1)
			battle_fire_boltgun(self, self->enemy, vstart, MZ2_TANK_BLASTER_3);
//		self->monsterinfo.nextframe = self->s.frame + 6;	// RSP 082400
	}
}

void battle_fire_right(edict_t *self)
{	// Lead the target
	vec3_t	vstart, fire_dir;
	vec3_t	forward, right;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin,vecDiskRight,forward,right,vstart);

	if (self->identity == ID_BATTLEBOT_DISK)	// RSP 082200
	{	// Fire Disk gun
		if (self->s.frame == FRAME_fireright1)
		{
			if (!LeadTarget(self, self->enemy, vstart, fire_dir))
			{
				battle_run(self);
				return;
			}		
			battle_fire_diskgun(self, vstart, fire_dir);
//			battleMakeGunSounds(self,0,1);	// RSP 102499 - not needed, diskgun takes care of sounds
		}
	}
	else
	{	// Fire 'bolt' gun
		if (rand()&1)
			battle_fire_boltgun(self, self->enemy, vstart, MZ2_TANK_BLASTER_3);
//		self->monsterinfo.nextframe = self->s.frame + 6;	// RSP 082400
	}
}

void battle_fire_both(edict_t *self)
{	// Fire these directly at the target
	vec3_t	vecStart, vecDiff, vecEnemyCenter;
	vec3_t	forward, right;

	VectorCopy(self->enemy->s.origin,vecEnemyCenter);
	vecEnemyCenter[2] += self->enemy->viewheight;

	// Find unit vector to enemy from right side
	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin,vecDiskRight,forward,right,vecStart);

	VectorSubtract(vecEnemyCenter, vecStart, vecDiff);
	VectorNormalize(vecDiff);

	if (self->identity == ID_BATTLEBOT_DISK)						// RSP 082300
	{	// Fire Disk gun
		if (self->s.frame == FRAME_fireboth1)					// RSP 082300
		{
			battle_fire_diskgun(self, vecStart, vecDiff);
		}
	}
	else
	{	// Fire 'bolt' gun
		if (rand()&1)
		{
			gi.sound (self, CHAN_AUTO, sound_attack, 1, ATTN_NORM, 0);
			monster_fire_bullet (self, vecStart, vecDiff, 1, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_TANK_BLASTER_3);
		}
	}

	// Find unit vector to enemy from left side
	G_ProjectSource(self->s.origin,vecDiskLeft,forward,right,vecStart);

	VectorSubtract(vecEnemyCenter, vecStart, vecDiff);
	VectorNormalize(vecDiff);

	if (self->identity == ID_BATTLEBOT_DISK)						// RSP 082300
	{	// Fire Disk gun
		if (self->s.frame == FRAME_fireboth1)					// RSP 082300
		{
			battle_fire_diskgun(self, vecStart, vecDiff);
		}
	}
	else
	{	// Fire 'bolt' gun
		if (rand()&1)
		{
			gi.sound (self, CHAN_AUTO, sound_attack, 1, ATTN_NORM, 0);
			monster_fire_bullet (self, vecStart, vecDiff, 1, 2, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_TANK_BLASTER_3);
		}
	}
//	self->monsterinfo.nextframe = self->s.frame + 9;			// RSP 082300
}

void battle_check_repeat(edict_t *self)
{	// Just repeat the sequence if the target is still within range.

	if (self->enemy->health <= 0)
	{
		if (self->enemy->client)	// RSP 082300
			Forget_Player(self);
		battle_run(self);
		return;
	}

	// But don't repeat with the disk gun
	if ((self->identity == ID_BATTLEBOT_DISK)						// RSP 082300
		&& ((rand()&2) != 0))
	{
		battle_run(self);
		return;
	}

	if (self->enemy && infront(self,self->enemy) && visible(self,self->enemy))
		battle_attack(self);
	else
		battle_run(self);
}

// RSP 082300 - changed to allow for multiple bolt shots

mframe_t battle_frames_attack_both [] =
{
	NULL, 0, battle_fire_both,	// 1	// diskgun fires on this frame only
	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,

	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both,

	NULL, 0, battle_fire_both,
	NULL, 0, battle_fire_both	// 12
};
mmove_t battle_move_attack_both = {FRAME_fireboth1, FRAME_fireboth12,
									battle_frames_attack_both, battle_check_repeat};

// RSP 082300 - changed to allow for multiple bolt shots

mframe_t battle_frames_attack_left [] =
{
	NULL, 0, battle_fire_left,	// 1	// diskgun fires on this frame only
	NULL, 0, battle_fire_left,
	NULL, 0, battle_fire_left,
	NULL, 0, battle_fire_left,
	NULL, 0, battle_fire_left,

	NULL, 0, battle_fire_left,
	NULL, 0, battle_fire_left,
	NULL, 0, battle_fire_left				// 8
};
mmove_t battle_move_attack_left = {FRAME_fireleft1, FRAME_fireleft8,
									battle_frames_attack_left, battle_check_repeat};

// RSP 082300 - changed to allow for multiple bolt shots

mframe_t battle_frames_attack_right [] =
{
	NULL, 0, battle_fire_right,	// 1	// diskgun fires on this frame only
	NULL, 0, battle_fire_right,
	NULL, 0, battle_fire_right,
	NULL, 0, battle_fire_right,
	NULL, 0, battle_fire_right,

	NULL, 0, battle_fire_right,
	NULL, 0, battle_fire_right,
	NULL, 0, battle_fire_right	// 8
};
mmove_t battle_move_attack_right = {FRAME_fireright1, FRAME_fireright8,
									battle_frames_attack_right, battle_check_repeat};

void battle_attack(edict_t *self)
{
	// For now just fire based on distance and movement - we may like to 
	// vary by skill as well

	vec3_t v;
	float	adiff;
	vec3_t	a,dir;

	// RSP 082700 - make sure target is w/in 30 degrees of the facing direction
	VectorSubtract(self->enemy->s.origin, self->s.origin,dir);
	vectoangles(dir,a);
	adiff = anglemod(anglemod(self->s.angles[YAW]) - anglemod(a[YAW]));
	if ((adiff > 15) && (adiff < 345))
	{
		battle_run(self);
		return;
	}

	VectorCopy(self->enemy->velocity,v);

	if (VectorNormalize(v) < 50)
		self->monsterinfo.currentmove = &battle_move_attack_both;
	else
	{	// if vaguely in our direction, then fire 2
		vec3_t	fwd;
		
		AngleVectors (self->s.angles, fwd, NULL, NULL);

		if (fabs(DotProduct(fwd,v)) > 0.8)
			self->monsterinfo.currentmove = &battle_move_attack_both;
		else
		{
			if (rand()&1)
				self->monsterinfo.currentmove = &battle_move_attack_right;
			else
				self->monsterinfo.currentmove = &battle_move_attack_left;
		}
	}
}

// PAIN
mframe_t battle_frames_pain1 [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 5
};
mmove_t battle_move_pain1 = {FRAME_paina1, FRAME_paina5, battle_frames_pain1, battle_run};

mframe_t battle_frames_pain2 [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 5
};
mmove_t battle_move_pain2 = {FRAME_painb1, FRAME_painb5, battle_frames_pain2, battle_run};

void battle_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	int	n;

	waypoint_check(self);

//	if (self->health < (self->max_health / 2))
//		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	
	if (skill->value == 3)
		return;		// no pain anims in nightmare

	n = rand() % 2;
	if (n == 0)
	{
		self->monsterinfo.currentmove = &battle_move_pain1;
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{	
		self->monsterinfo.currentmove = &battle_move_pain2;
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

// DEATH
void battle_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;	// WPO 102299 - stop thinking
	gi.linkentity (self);
}

// I kind of screwed up rendering the frames - the 3 death things are 1-9, 10-19, 20-40

mframe_t battle_frames_deatha [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 9
};
mmove_t battle_move_deatha = {FRAME_die001, FRAME_die009, battle_frames_deatha, battle_dead};


mframe_t battle_frames_deathb [] =
{
	ai_move, 0, NULL,	// 10
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 19
};
mmove_t battle_move_deathb = {FRAME_die010, FRAME_die019, battle_frames_deathb, battle_dead};

mframe_t battle_frames_deathc [] =
{
	ai_move, 0, NULL,	// 20
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,	// 30
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	//40
};
mmove_t battle_move_deathc = {FRAME_die020, FRAME_die040, battle_frames_deathc, battle_dead};


void battle_gib(edict_t *self, int damage)
{
	int n;

	gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);

	for (n = 0 ; n < 2 ; n++)
		ThrowGib (self, "models/objects/gibs/gear/tris.md2", damage, GIB_ORGANIC);
	for (n = 0 ; n < 4 ; n++)
		ThrowGib (self, "models/objects/gibs/sm_metal/tris.md2", damage, GIB_ORGANIC);
	ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
	self->deadflag = DEAD_DEAD;
}

void battle_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int i;

	self->s.modelindex2 = 0;
	self->model2 = "";

	self->s.modelindex3 = 0;
	self->model3 = "";

	// check for gib
	if (self->health <= self->gib_health)
	{
		battle_gib(self,damage);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	srand(self->health);	// seed randomizer
	i = rand() & 5;

	if (i == 0)		// C should be seen less ofen
		self->monsterinfo.currentmove = &battle_move_deathc;
	else if (i < 3)
		self->monsterinfo.currentmove = &battle_move_deathb;
	else
		self->monsterinfo.currentmove = &battle_move_deatha;

	if (i > 0)
		gi.sound (self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
}

// RSP 092500
void monster_battle_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	sound_pain1     = gi.soundindex ("battle/pain1.wav");
	sound_pain2     = gi.soundindex ("battle/pain2.wav");
	sound_die1      = gi.soundindex ("battle/death1.wav");
	sound_die2      = gi.soundindex ("battle/death2.wav");
	sound_attack	= gi.soundindex ("gbot/attack.wav");
	sound_sight     = gi.soundindex ("battle/sight.wav");
	sound_idle      = gi.soundindex ("battle/idle.wav");
	sound_step      = gi.soundindex ("battle/step.wav");

	self->s.modelindex = gi.modelindex("models/monsters/bots/battle/tris.md2");		// RSP 082200
}


void monster_battle (edict_t *self)
{
    // RSP 082100 - no longer needed, since ambushbot w/o guns isn't
    // a valid monster
//  if (deathmatch->value)
//  {
//      G_FreeEdict (self);
//      return;
//  }

	self->yaw_speed = 20;

	monster_battle_indexing(self);	// RSP 110900 - fixes invisible monster bug Chris found

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
//	self->model = "models/monsters/bots/battle/tris.md2";	// RSP 082200

	VectorSet (self->mins, -32, -32, 0);
	VectorSet (self->maxs, 32, 32, 70);

	self->health = hp_table[HP_BATTLE];	// RSP 071299
//	self->health = 200;
	self->gib_health = -40;
	self->mass = 200;

	self->count = 0;

	self->pain = battle_pain;
	self->die = battle_die;

	self->monsterinfo.stand = battle_stand;
	self->monsterinfo.walk = battle_walk;
	self->monsterinfo.run = battle_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = battle_attack;
	self->monsterinfo.melee = battle_melee;		// for testing
	self->monsterinfo.sight = battle_sight;
	self->monsterinfo.idle = battle_idle;	// RSP 102799
	self->flags |= FL_PASSENGER;	// RSP 071299 - possible BoxCar passenger

	gi.linkentity (self);

	self->monsterinfo.currentmove = &battle_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}

/*QUAKED monster_battle_disk (1 .5 0) (-32 -32 0) (32 32 70) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_battle_disk (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	// Right is index2, left is index3
//	self->model2 = DISKGUN_RIGHT;
//	self->model3 = DISKGUN_LEFT;
	self->s.modelindex2 = gi.modelindex(DISKGUN_RIGHT);
	self->s.modelindex3 = gi.modelindex(DISKGUN_LEFT);
	self->identity = ID_BATTLEBOT_DISK;	// RSP 082200

	monster_battle(self);
}


/*QUAKED monster_battle_bolt (1 .5 0) (-32 -32 0) (32 32 70) Ambush Trigger_Spawn Sight Pacifist
 */
void SP_monster_battle_bolt (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	// Right is index2, left is index3
//	self->model2 = BOLTGUN_RIGHT;
//	self->model3 = BOLTGUN_LEFT;
	self->s.modelindex2 = gi.modelindex(BOLTGUN_RIGHT);
	self->s.modelindex3 = gi.modelindex(BOLTGUN_LEFT);
	self->identity = ID_BATTLEBOT_BOLT;	// RSP 082200

	monster_battle(self);
}
