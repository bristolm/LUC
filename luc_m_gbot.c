/*
==============================================================================

gatlin' bot file

==============================================================================
*/

#include "g_local.h"
#include "luc_m_gbot.h"

// RSP 061599 - not used
//static int	nextmove;			// Used for start/stop frames

static int	sound_sight;
static int	sound_idle;
static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;
static int	sound_attack;	// MRB 022799

// RSP 062200 - Allow gbot to rotate randomly now and then
//              when standing
void gbot_rotate(edict_t *self)
{
	if (self->spawnflags & SPAWNFLAG_AMBUSH)	// No rotation if AMBUSH set
		return;
	
	// if debounce is not -1, and level.time hasn't reached it yet, gbot
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


void gbot_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void gbot_idle (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

mframe_t gbot_frames_stand [] =
{
	ai_stand, 0, gbot_rotate        // RSP 062200 - rotate now and then
};
mmove_t gbot_move_stand = {FRAME_start01, FRAME_start01, gbot_frames_stand, NULL};


mframe_t gbot_frames_walk [] =
{
	ai_walk, 10, NULL
};
mmove_t	gbot_move_walk = {FRAME_start01, FRAME_start01, gbot_frames_walk, NULL};

mframe_t gbot_frames_run [] =
{
	ai_run, 20, NULL
};
mmove_t	gbot_move_run = {FRAME_start01, FRAME_start01, gbot_frames_run, NULL};

void gbot_run (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &gbot_move_stand;
	else
		self->monsterinfo.currentmove = &gbot_move_run;
}

void gbot_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &gbot_move_walk;
}

void gbot_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &gbot_move_stand;
}

mframe_t gbot_frames_pain1 [] =
{
	ai_move, -10, NULL,	// RSP 042500 - these were 0,
	ai_move, -10, NULL,	// now try
	ai_move, -10, NULL,	// retreating slightly
	ai_move, -10, NULL,	// when
	ai_move, -10, NULL	// hurt
};
mmove_t gbot_move_pain1 = {FRAME_start01, FRAME_start05, gbot_frames_pain1, gbot_run};

void gbot_fire (edict_t *self)
{
	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	end;
	vec3_t	dir;
	int		flash_number = MZ2_TANK_BLASTER_3;

	AngleVectors (self->s.angles, forward, right, NULL);
	G_ProjectSource (self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	// RSP 061700 - project enemy back a bit and target there
	VectorCopy (self->enemy->s.origin, end);
	VectorMA (end, -0.2, self->enemy->velocity, end);
	end[2] += self->enemy->viewheight;

	VectorSubtract (end, start, dir);
	gi.sound (self, CHAN_VOICE, sound_attack, 1, ATTN_NORM, 0);	// MRB 022799

	// RSP 022499: Shooting direction was 'forward', so player wasn't getting hit (shooting over player's head).
	//             Changed direction to 'dir' and now it works.
	monster_fire_bullet (self, start, dir, 3, 4, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

mframe_t gbot_frames_attack [] =
{
	ai_charge, 0, NULL,		// 1
	ai_charge, 0, NULL,
	ai_charge, -2, gbot_fire,
	ai_charge, -2, gbot_fire,
	ai_charge, -2, gbot_fire,
	ai_charge, -2, gbot_fire,
	ai_charge, -2, gbot_fire,
	ai_charge, -2, gbot_fire,
	// RSP 042500 - try retreating after firing	
	ai_charge, -10, NULL,	
	ai_charge, -10, NULL	// 10	
};

// RSP 022499: Changed ending frame to 10 from 1 so the gbot would actually SHOOT at you.
mmove_t gbot_move_attack = {FRAME_start01, FRAME_start10, gbot_frames_attack, gbot_run};

/* RSP 061599 - gbot_loop_melee() isn't used
void gbot_loop_melee (edict_t *self)
{
//	if (random() <= 0.5)	
//		self->monsterinfo.currentmove = &gbot_move_attack1;
//	else
	self->monsterinfo.currentmove = &gbot_move_attack;
}
 */


void gbot_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &gbot_move_attack;
}

void gbot_melee (edict_t *self)
{
	self->monsterinfo.currentmove = &gbot_move_attack;
}

void gbot_pain (edict_t *self, edict_t *other, float kick, int damage)
{
//	MJB 042699 - this skin shouldn't be there
//	if (self->health < (self->max_health / 2))
//		self->s.skinnum = 1;

	waypoint_check(self);	// RSP 071800

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	if (skill->value == 3)
		return;		// no pain anims in nightmare

	// RSP 041799 - randomize pain sound
	if (rand()&1)
		gi.sound (self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound (self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	self->monsterinfo.currentmove = &gbot_move_pain1;
}

void gbot_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);

	// RSP 062900 - use generic explosion routine
//	BecomeExplosion1(self);
	SpawnExplosion(self,TE_EXPLOSION1,MULTICAST_PVS,EXPLODE_GBOT);
	G_FreeEdict(self);
}

void monster_gbot_indexing (edict_t *self)
{
	// MRB 022799 New sound dirs
	// RSP 102799 - switched to common filenames
	sound_sight = gi.soundindex ("gbot/sight.wav");
	sound_idle = gi.soundindex ("gbot/idle.wav");
	sound_pain1 = gi.soundindex ("gbot/pain1.wav");
	sound_pain2 = gi.soundindex ("gbot/pain2.wav");
	sound_attack = gi.soundindex ("gbot/attack.wav");
	sound_die = gi.soundindex ("gbot/death1.wav");

	self->s.sound = gi.soundindex ("gbot/flyby.wav");	// MRB 022799

	self->s.modelindex = gi.modelindex ("models/monsters/bots/Gbot/tris.md2");	// RSP 061899
}


/*QUAKED monster_bot_gbot (1 .5 0) (-16 -16 -8) (16 16 8) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_bot_gbot (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	monster_gbot_indexing (self);

//	self->model = "models/monsters/bots/Gbot/tris.md2";	// RSP 061899

	VectorSet (self->mins, -20, -20, -8);				// RSP 061700 - better fit to model
	VectorSet (self->maxs, 16, 20, 8);					// RSP 061700 - better fit to model
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	self->health = hp_table[HP_GBOT];	// RSP 032399
	self->mass = 50;

	self->pain = gbot_pain;
	self->pain_debounce_time = 0;       // RSP 062200
	self->die = gbot_die;

	self->monsterinfo.stand = gbot_stand;
	self->monsterinfo.walk = gbot_walk;
	self->monsterinfo.run = gbot_run;
	self->monsterinfo.attack = gbot_attack;
	self->monsterinfo.melee = gbot_attack;
	self->monsterinfo.sight = gbot_sight;
	self->monsterinfo.idle = gbot_idle;

	self->flags |= FL_PASSENGER;	// RSP 061899 - possible BoxCar passenger
	self->identity = ID_GBOT;		// RSP 042500

	gi.linkentity (self);

	self->monsterinfo.currentmove = &gbot_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

	flymonster_start (self);
}
