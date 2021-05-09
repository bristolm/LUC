#include "g_local.h"
#include "luc_m_bots.h"

#define MAX_CLAW_DIST			80

//static int sound_attack;	// RSP 110399

// Individual file for the 'Claw' worker bot
void  bot_workerclaw_keephold(edict_t *self);

void worker_sight(edict_t *self, edict_t *other);
void worker_idle (edict_t *self);


// Local functions

// Attack

qboolean bot_crushenemy(edict_t *self)
{
	vec3_t aim;

	if (self->enemy)
		if (self->enemy->deadflag == DEAD_DEAD)
			bot_stand(self);
		else
		{
			VectorSet (aim, MAX_CLAW_DIST, 0, 0);
			return (fire_hit(self,aim,( 5 + random()*5),0));
		}

	return false;
}

/* RSP 081800 - not used (contains bugs)
mframe_t bot_workerclaw_frames_attack_lift [] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t bot_workerclaw_attack_lift = {FRAME_LIFT_START, FRAME_LIFT_END, bot_workerclaw_frames_attack_lift, };
 */


void bot_workerclaw_lockhold(edict_t *self)
{
//	vec3_t	aim,aimang;	// RSP 073000 - not used
	trace_t	t;
	edict_t	*enemy = self->enemy;	// RSP 073000

	// damage enemy and check for grabbing
//	VectorSet (aim, MAX_CLAW_DIST, 0, 0);	// RSP 073000

	if (!(enemy->locked_to) && bot_crushenemy(self))
		enemy->locked_to = self;
	else if (!(enemy->locked_to == self))
	{ // someone else's got 'em.  what to do? ...
		bot_run(self);
	}

/* RSP 073000 - clawbot drags his butt below ground when this is allowed
	//re-orient ('cause it's sooo much fun!)
	VectorSubtract(self->enemy->s.origin, self->s.origin, aim);
	vectoangles(aim,aimang);
	self->s.angles[0] = aimang[0];
 */

	// now, for fun, drag 'em in as far as possible (don't screw with the vert part)
//	VectorCopy(self->s.origin,aim);
//	aim[2] = self->enemy->s.origin[2];	// RSP 073000 - have to screw with the vert part,
										// otherwise you can end up on top of the clawbot
	t = gi.trace(enemy->s.origin,enemy->mins,enemy->maxs,self->s.origin,enemy,MASK_MONSTERSOLID);

	VectorCopy(t.endpos,enemy->s.origin);
}

mframe_t bot_workerclaw_frames_attack_hold [] =
{
	ai_charge, 0, bot_crushenemy
};

mmove_t bot_workerclaw_attack_hold = {FRAME_EXTEND_END - 1, FRAME_EXTEND_END - 1, bot_workerclaw_frames_attack_hold, bot_workerclaw_keephold};

void bot_workerclaw_keephold(edict_t *self)
{
	self->monsterinfo.currentmove = &bot_workerclaw_attack_hold;
}

void bot_workerclaw_hold_or_lift(edict_t *self)
{
	// If this is debris, lift it, if it is a creature, hold it
	// Need to differentiate debris!
	// self->monsterinfo.currentmove = &bot_workerclaw_attack_lift;

	self->monsterinfo.currentmove = &bot_workerclaw_attack_hold;
}

mframe_t bot_workerclaw_frames_attack_extend [] =
{
	ai_charge, 0, NULL,	// 1
	ai_charge, 0, NULL,
	ai_charge, 0, bot_workerclaw_lockhold,
	ai_charge, 0, NULL	// 4
};
mmove_t bot_workerclaw_attack_extend = {FRAME_EXTEND_START, FRAME_EXTEND_END, bot_workerclaw_frames_attack_extend, bot_workerclaw_hold_or_lift};

void bot_workerclaw_extend(edict_t *self)
{
//	vec3_t v,vang;

/* RSP 073000 - clawbot drags his butt below ground when this is allowed
	// Reorient ourselves a bit so we're perfect - on target
	VectorSubtract(self->enemy->s.origin, self->s.origin, v);
	vectoangles(v,vang);
	self->s.angles[0] = vang[0];
 */

	self->monsterinfo.currentmove = &bot_workerclaw_attack_extend;
}

mframe_t bot_workerclaw_frames_attack_brace [] =
{
	ai_charge, 0, NULL,	// 1
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL	// 6
};
mmove_t bot_workerclaw_attack_brace = {FRAME_BRACE_START, FRAME_BRACE_END, bot_workerclaw_frames_attack_brace, bot_workerclaw_extend};

void bot_workerclaw_attack(edict_t *self)
{
	vec3_t	v;
//	vec3_t	vang;
	float	f;

	VectorMA(self->enemy->absmin,0.5,self->enemy->size,v);
	f = MAX_CLAW_DIST + ((self->enemy->maxs[0] + self->enemy->maxs[1]) /2);
	if (!(botEvalTarget(self,v,f,50,-30,20)))
	{
		bot_run(self);
		return;
	}

	gi.sound (self, CHAN_VOICE,gi.soundindex ("worker/atakclaw.wav"), 1, ATTN_NORM, 0);	// RSP 110399

/* RSP 073000 - clawbot drags his butt below ground when this is allowed
	// Reorient ourselves a bit so we're perfect - on target
	VectorSubtract(self->enemy->s.origin, self->s.origin, v);
	vectoangles(v,vang);
	self->s.angles[0] = vang[0];
 */

	// Start the attack
	self->monsterinfo.currentmove = &bot_workerclaw_attack_brace;
}

// Pain stuff

/*QUAKED monster_bot_workerclaw (1 .5 0) (-24 -24 0) (24 24 32) Ambush Trigger_Spawn Sight Pacifist
 */
void SP_monster_bot_workerclaw (edict_t *self)
{
	self->s.modelindex2 = gi.modelindex ("models/monsters/bots/worker/w_Claw.md2");			// RSP 061899

	self->health = hp_table[HP_CLAWBOT];	// RSP 032399

	bot_spawnsetup(self);

	self->monsterinfo.stand = bot_stand;
	self->monsterinfo.walk = bot_walk;
	self->monsterinfo.run = bot_run;
	self->monsterinfo.attack = bot_workerclaw_attack;
	self->identity = ID_CLAWBOT;			// RSP 072900

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
