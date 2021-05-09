#include "g_local.h"
#include "luc_m_bots.h"

#define WAD_MAX_DIST 600

// RSP 102499 - add sounds
void worker_sight(edict_t *self, edict_t *other);
void worker_idle (edict_t *self);
//static int	sound_attack;

// MRB 011198 - changed from coil to plasmawad attack.

// Local functions

/*
void  bot_workercoil_melee(edict_t *self);
void  bot_workercoil_sight(edict_t *self, edict_t *other);
void  bot_workercoil_idle(edict_t *self);
void  bot_workercoil_search(edict_t *self);
*/

void workerCoilEvalTarget(edict_t *self)
{
	vec3_t v;

	if (!botEvalTarget(self,self->enemy->s.origin,WAD_MAX_DIST,60,-60,30))
	{
		bot_run(self);
		return;
	}

	VectorCopy(self->enemy->s.origin, self->pos1);  //store aim point
	VectorSubtract(self->pos1, self->s.origin, v);
	self->random = VectorNormalize2(v,self->pos2);	//Store distance , unit toward target

	self->monsterinfo.pausetime = level.time + (self->random / 250);
}

void workerCoilPause(edict_t *self)
{
	//Depending on the distance of the enemy, allow a sufficient 'power up" time.

	if (level.time >= self->monsterinfo.pausetime)
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	else
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}


// standing frames for now ...
mframe_t bot_workercoil_frames_after []=
{
	ai_charge, 0, workerCoilPause, // FRAME_STAND_END - 2
	ai_charge, 0, NULL, 
	ai_charge, 0, NULL			   // FRAME_STAND_END
};
mmove_t bot_workercoil_move_after= {FRAME_STAND_END - 2, FRAME_STAND_END, bot_workercoil_frames_after, bot_run};

void workerCoilLaunchWad(edict_t *self)
{
	vec3_t start;

	// based on distance, give it a 'lofty' angle to throw
	self->pos2[2] += (.2 + (random() * 0.3));

	// Get start point for wad
	VectorMA(self->s.origin,10,self->pos2,start);
	start[2] += (self->maxs[2] - self->mins[2]);

	gi.sound (self, CHAN_VOICE,gi.soundindex ("worker/atakplas.wav"), 1, ATTN_NORM, 0);	// RSP 102499
	launch_plasma (self, start, self->pos2, skill->value + 1, 200 + (self->random/2), 2 + self->random/200, 48);
	
	// send muzzle flash
	gi.WriteByte (svc_muzzleflash2);	// RSP 063000
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_BLASTER);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->monsterinfo.pausetime = level.time + (self->random / 250);
	self->monsterinfo.currentmove = &bot_workercoil_move_after;
}

mframe_t bot_workercoil_frames_attack_frames [] =
{
	ai_charge, 0, workerCoilEvalTarget,	// 1
	ai_charge, 0, workerCoilPause,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, workerCoilLaunchWad	// 6
};
mmove_t bot_workercoil_attack_move = {FRAME_BRACE_START, FRAME_BRACE_END, bot_workercoil_frames_attack_frames, bot_run};

void bot_workercoil_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &bot_workercoil_attack_move;
	return;
}

/*QUAKED monster_bot_workercoil (1 .5 0) (-24 -24 0) (24 24 32) Ambush Trigger_Spawn Sight Pacifist
*/
void SP_monster_bot_workercoil (edict_t *self)
{
	self->s.modelindex2 = gi.modelindex ("models/monsters/bots/worker/w_Coil.md2");			// RSP 061899

	self->health = hp_table[HP_COILBOT];	// RSP 032399

	bot_spawnsetup(self);

	self->monsterinfo.stand  = bot_stand;
	self->monsterinfo.walk   = bot_walk;
	self->monsterinfo.run    = bot_run;
	self->monsterinfo.attack = bot_workercoil_attack;
	self->identity = ID_PLASMABOT;			// RSP 072900

	/* AJA 19980906 - Passive flag. This will mean that the bot will gather
	    resources instead of waiting to attack a player. The bot will not
	    react to the player unless the player harms it.
	   MRB 19981205 - walkmonster_start flags AI_PACIFIST, but we need to
	    feed it a currentmove to be consistant
	*/
	self->monsterinfo.currentmove = &bot_move_stand;

	gi.linkentity (self);

	self->monsterinfo.scale = MODEL_SCALE;

	walkmonster_start (self);
}
