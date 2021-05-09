/*
==============================================================================

Ambush bot - 1 weapon. moves fast.  Maybe physical knockback attack?

==============================================================================
*/

#include "g_local.h"
#include "luc_m_ambush.h"
extern qboolean enemy_vis;	// RSP 042999
extern int	enemy_range;	// RSP 042999

#define AMBUSH_WALK_SPEED	10
#define AMBUSH_HIT_POWER	200
#define AMBUSH_HIT_RANGE	90

static vec3_t fire_offsets = {20, 0, 40};	// RSP 082200
static float DiskSpeed = 1000;

//MRB 071899 - add in sounds
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die1;
static int	sound_die2;
//static int	sound_firedisk;	// RSP 102499 - not needed
static int	sound_attack;	// RSP 082700 - not needed
static int	sound_sight;
static int	sound_idle;	
static int  sound_step;

void ambush_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}


mframe_t ambush_frames_stand [] =
{
	ai_stand, 0, NULL,	// 1
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 10
	ai_stand, 0, NULL,	// 11
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 20
	ai_stand, 0, NULL,	// 21
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,	// 30
	ai_stand, 0, NULL,	// 31
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL,
	ai_stand, 0, NULL	// 40
};
mmove_t ambush_move_stand = {FRAME_stnd001, FRAME_stnd040, ambush_frames_stand, NULL};

void ambush_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &ambush_move_stand;
}

void AmbushStepSound (edict_t *self)
{
	gi.sound (self, CHAN_BODY, sound_step, 1, ATTN_IDLE, 0);
}

// WALK
mframe_t ambush_frames_walk [] =
{
	ai_walk, 10, NULL,	// 1
	ai_walk,  5, NULL,
	ai_walk,  5, AmbushStepSound,
	ai_walk,  8, NULL,
	ai_walk, 10, NULL,

	ai_walk, 13, NULL,
	ai_walk,  7, NULL,
	ai_walk,  7, AmbushStepSound,
	ai_walk,  4, NULL,
	ai_walk,  9, NULL	// 10
};
mmove_t ambush_move_walk = {FRAME_walk001, FRAME_walk010, ambush_frames_walk, NULL};

void ambush_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &ambush_move_walk;
}

// RUN

void ambush_run (edict_t *self);

mframe_t ambush_frames_run [] =
{
	ai_run, 15, NULL,	// 1
	ai_run, 35, NULL,
	ai_run, 15, AmbushStepSound,
	ai_run, 15, NULL,
	ai_run, 35, NULL,
	ai_run, 15, AmbushStepSound	// 6
};
mmove_t ambush_move_run = {FRAME_run001, FRAME_run006, ambush_frames_run, NULL};

void ambush_run (edict_t *self)
{
	self->monsterinfo.currentmove = &ambush_move_run;
}

void ambush_start_run (edict_t *self)
{
	// If we were told to run, but we are standing ground, stay put.
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &ambush_move_stand;
	else
		self->monsterinfo.currentmove = &ambush_move_run;
}

// SIGHT
void ambush_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

// MELEE
void ambush_hit_both(edict_t *self)
{
	vec3_t aim;

	VectorSet (aim, AMBUSH_HIT_RANGE * 1.3 , 0, 0);
	(void)fire_hit (self, aim, wd_table[WD_AMBUSH_HIT] * 3, AMBUSH_HIT_POWER);
}

void ambush_hit(edict_t *self);
mframe_t ambush_frames_melee_r [] =
{
	ai_charge, 0, NULL,	// 1
	ai_charge, 0, NULL,
	ai_charge, 0, ambush_hit,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,	
	ai_charge, 0, NULL	// 6
};
mmove_t ambush_move_melee_r = {FRAME_mel001, FRAME_mel006, ambush_frames_melee_r, ambush_run};

mframe_t ambush_frames_melee_l [] =
{
	ai_charge, 0, NULL,	// 7
	ai_charge, 0, NULL,
	ai_charge, 0, ambush_hit,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL	// 12
};
mmove_t ambush_move_melee_l = {FRAME_mel007, FRAME_mel012, ambush_frames_melee_l, ambush_run};

mframe_t ambush_frames_melee_b [] =
{
	ai_charge, 0, NULL,	// 13
	ai_charge, 0, NULL,
	ai_charge, 0, ambush_hit_both,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,	
	ai_charge, 0, NULL	// 20
};
mmove_t ambush_move_melee_b = {FRAME_mel013, FRAME_mel020, ambush_frames_melee_b, ambush_run};

void ambush_hit(edict_t *self)
{
	vec3_t		aim;
	qboolean	made_contact = false;

	VectorSet (aim, AMBUSH_HIT_RANGE, 0, 0);
	made_contact = fire_hit (self, aim, wd_table[WD_AMBUSH_HIT], AMBUSH_HIT_POWER/10);

	if (made_contact == true)
	{	// decide if we should keep pummeling
		int i = rand() & (5 + (int)skill->value);

		if (i > 4)
			self->monsterinfo.nextframe = FRAME_mel014;	// money shot
		else
			if (self->s.frame < FRAME_mel006)
			{
				self->monsterinfo.currentmove = &ambush_move_melee_l;
				self->monsterinfo.nextframe = FRAME_mel008;
			}
			else
			{
				self->monsterinfo.currentmove = &ambush_move_melee_r;
				self->monsterinfo.nextframe = FRAME_mel002;
			}
	}
}

void ambush_melee(edict_t *self)
{
	int i = rand() & 7;

	if (i == 0)
		self->monsterinfo.currentmove = &ambush_move_melee_b;
	else if (i < 4)
		self->monsterinfo.currentmove = &ambush_move_melee_l;
	else
		self->monsterinfo.currentmove = &ambush_move_melee_r;
}

// ATTACK
void ambush_fire(edict_t *self)
{	// Lead the target

	vec3_t	vstart, fire_dir;
	vec3_t	forward, right;

	if (self->enemy->health <= 0)
	{
		if (self->enemy->client)	// RSP 082300
			Forget_Player(self);
		ambush_run(self);
		return;
	}

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin,fire_offsets,forward,right,vstart);	// RSP 082200

	if (self->identity == ID_AMBUSHBOT_DISK)	// RSP 082200
	{
		if (self->s.frame == FRAME_atk004)	// RSP 082200 - only fire on this frame
		{	// Fire Disk gun
			extern qboolean LeadTarget(edict_t *self, edict_t *enemy, vec3_t vstart, vec3_t fire_dir);
			int damage = wd_table[WD_DISKGUN] / 2;	// damage from player's table

			if (!LeadTarget(self, self->enemy, vstart, fire_dir))
			{
				ambush_run(self);
				return;
			}
		
			monster_fire_diskgun (self, vstart, fire_dir, damage, DiskSpeed, 0);

//			gi.sound (self, CHAN_AUTO, sound_firedisk, 1, ATTN_NORM, 0);	// RSP 102499 - not needed, diskgun
																			// takes care of firing sound
		}
	}
	else if (rand()&1)
	{	// Fire 'bolt' gun
		vec3_t	vecEnemy,fire_dir;

		// RSP 082400 - project enemy back a bit and target there
		VectorCopy (self->enemy->s.origin, vecEnemy);
		VectorMA (vecEnemy, -0.2, self->enemy->velocity, vecEnemy);
		vecEnemy[2] += self->enemy->viewheight;

		VectorSubtract(vecEnemy,vstart,fire_dir);
		VectorNormalize(fire_dir);
		monster_fire_bullet (self, vstart, fire_dir, 1, 2, 
							 DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MZ2_TANK_BLASTER_3);
//		self->monsterinfo.nextframe = self->s.frame + 6;	// RSP 082200
		gi.sound (self, CHAN_AUTO, sound_attack, 1, ATTN_NORM, 0);
	}
}

// RSP 082200 - changed to allow for multiple bolt shots
mframe_t ambush_frames_attack [] =
{
	NULL, 0, ambush_fire,	// 1
	NULL, 0, ambush_fire,
	NULL, 0, ambush_fire,
	NULL, 0, ambush_fire,	// diskgun fires on this frame only
	NULL, 0, ambush_fire,
	NULL, 0, ambush_fire,
	NULL, 0, ambush_fire,
	NULL, 0, ambush_fire	// 8
};

mmove_t ambush_move_attack = {FRAME_atk001,FRAME_atk008,ambush_frames_attack,ambush_run};

void ambush_attack(edict_t *self)
{
	float	adiff;
	vec3_t	a,dir;

	// RSP 082700 - make sure target is w/in 30 degrees of the facing direction
	VectorSubtract(self->enemy->s.origin, self->s.origin,dir);
	vectoangles(dir,a);
	adiff = anglemod(anglemod(self->s.angles[YAW]) - anglemod(a[YAW]));
	if ((adiff > 15) && (adiff < 345))
		ambush_run(self);
	else
		self->monsterinfo.currentmove = &ambush_move_attack;
}


// PAIN
mframe_t ambush_frames_pain1 [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 5
};

mmove_t ambush_move_pain1 = {FRAME_pain001, FRAME_pain005, ambush_frames_pain1, ambush_run};

mframe_t ambush_frames_pain2 [] =
{
	ai_move, 0, NULL,	// 6
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 10
};

mmove_t ambush_move_pain2 = {FRAME_pain006, FRAME_pain010, ambush_frames_pain2, ambush_run};

void ambush_pain (edict_t *self, edict_t *other, float kick, int damage)
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
		self->monsterinfo.currentmove = &ambush_move_pain1;
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		self->monsterinfo.currentmove = &ambush_move_pain2;
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}
}

// DEAD

void ambush_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 32);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;	// WPO 102299 - stop thinking
	gi.linkentity (self);
}

mframe_t ambush_frames_deatha [] =
{
	ai_move, 0, NULL,	// 1
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 5
};
mmove_t ambush_move_deatha = {FRAME_die001, FRAME_die005, ambush_frames_deatha, ambush_dead};


mframe_t ambush_frames_deathb [] =
{
	ai_move, 0, NULL,	// 6
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,	// 10

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 20
};
mmove_t ambush_move_deathb = {FRAME_die006, FRAME_die020, ambush_frames_deathb, ambush_dead};

mframe_t ambush_frames_deathc [] =
{
	ai_move, 0, NULL,	// 21
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL	// 25
};
mmove_t ambush_move_deathc = {FRAME_die021, FRAME_die025, ambush_frames_deathc, ambush_dead};

void ambush_gib(edict_t *self, int damage)
{
	int n;

	gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);

	for (n = 0 ; n < 2 ; n++)
		ThrowGib (self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
	for (n = 0 ; n < 4 ; n++)
		ThrowGib (self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
	ThrowHead (self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
	self->deadflag = DEAD_DEAD;
}

void ambush_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int i;

	// check for gib
	if (self->health <= self->gib_health)
	{
		ambush_gib(self,damage);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	// regular death
	self->s.modelindex2 = 0;
	self->model2 = "";

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	srand(self->health);	// seed randomizer
	i = rand() & 5;

	if (i == 0)		// B should be seen less often
		self->monsterinfo.currentmove = &ambush_move_deathb;
	else if (i < 3)
		self->monsterinfo.currentmove = &ambush_move_deatha;
	else
		self->monsterinfo.currentmove = &ambush_move_deathc;

	// RSP 102799 - added death sounds
	if (random() < 0.5)
		gi.sound (self, CHAN_VOICE, sound_die1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);
}

// RSP 092500
void monster_ambush_indexing (edict_t *self)
{
	// MRB 022799 Change sound dirs
	// RSP 102799 - uniform sound filenames
	sound_pain1  = gi.soundindex ("ambush/pain1.wav");
	sound_pain2  = gi.soundindex ("ambush/pain2.wav");
	sound_die1   = gi.soundindex ("ambush/death1.wav");
	sound_die2   = gi.soundindex ("ambush/death2.wav");
	sound_attack = gi.soundindex ("gbot/attack.wav");
	sound_sight  = gi.soundindex ("ambush/sight.wav");
	sound_idle   = gi.soundindex ("ambush/idle.wav");
	sound_step   = gi.soundindex ("ambush/step.wav");

	self->s.modelindex = gi.modelindex("models/monsters/bots/ambush/tris.md2");
}


void monster_ambush (edict_t *self)
{
    // RSP 082100 - no longer needed, since ambushbot w/o guns isn't
    // a valid monster
//  if (deathmatch->value)
//  {
//      G_FreeEdict (self);
//      return;
//  }

	self->yaw_speed = 20;

	monster_ambush_indexing (self);	// RSP 092500

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
//	self->model = "models/monsters/bots/ambush/tris.md2";	// RSP 082200
	VectorSet (self->mins, -32, -32, 0);
	VectorSet (self->maxs, 32, 32, 70);

	self->health = hp_table[HP_AMBUSH];	// RSP 071299
//	self->health = 200;
	self->gib_health = -40;
	self->mass = 200;

	self->pain = ambush_pain;
	self->die = ambush_die;

	self->monsterinfo.stand = ambush_stand;
	self->monsterinfo.walk = ambush_walk;
	self->monsterinfo.run = ambush_start_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = ambush_attack;
	self->monsterinfo.melee = ambush_melee;
	self->monsterinfo.sight = ambush_sight;
	self->monsterinfo.idle = ambush_idle;	// RSP 102799
	self->flags |= FL_PASSENGER;	// RSP 071299 - possible BoxCar passenger

    gi.linkentity(self);

	self->monsterinfo.currentmove = &ambush_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

    walkmonster_start(self);
}

/*QUAKED monster_ambush_bolt (1 .5 0) (-32 -32 0) (32 32 70) Ambush Trigger_Spawn Sight Pacifist
 */
void SP_monster_ambush_bolt(edict_t *self)
{
	if (deathmatch->value)
	{
        G_FreeEdict(self);
		return;
	}

//	self->model2 = "models/monsters/bots/ambush/BoltGun.md2";
	self->s.modelindex2 = gi.modelindex("models/monsters/bots/ambush/BoltGun.md2");
	self->identity = ID_AMBUSHBOT_BOLT;	// RSP 082200

	monster_ambush(self);
}


/*QUAKED monster_ambush_disk (1 .5 0) (-32 -32 0) (32 32 70) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_ambush_disk(edict_t *self)
{
	if (deathmatch->value)
	{
        G_FreeEdict(self);
		return;
	}

//	self->model2 = "models/monsters/bots/ambush/DiskGun.md2";
	self->s.modelindex2 = gi.modelindex("models/monsters/bots/ambush/DiskGun.md2");
	self->identity = ID_AMBUSHBOT_DISK;	// RSP 082200

	monster_ambush(self);
}
