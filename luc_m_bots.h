// All the worker-bot types use this single file for frames.
// should be included into each file separately.
// This helps keep the weapons synced.
//
// I'm going to try and limit the amount of creatures needed 

#include "luc_m_botframes.h"

#define FRAME_STAND_START	FRAME_stand1
#define FRAME_STAND_END		FRAME_stand18

#define FRAME_WALK_START	FRAME_walk1
#define FRAME_WALK_END		FRAME_walk12

#define FRAME_TRIGHT_START	FRAME_tright1
#define FRAME_TRIGHT_END	FRAME_tright12

#define FRAME_TLEFT_START	FRAME_tleft1
#define FRAME_TLEFT_END		FRAME_tleft12

#define FRAME_PAINA_START	FRAME_paina1
#define FRAME_PAINA_END		FRAME_paina7

#define FRAME_PAINB_START	FRAME_painb1
#define FRAME_PAINB_END		FRAME_painb5

#define FRAME_BRACE_START	FRAME_brace1
#define FRAME_BRACE_END		FRAME_brace6

#define FRAME_EXTEND_START	FRAME_extend1
#define FRAME_EXTEND_END	FRAME_extend4

#define FRAME_LIFT_START	FRAME_lift1
#define FRAME_LIFT_END		FRAME_lift6

//#define FRAME_WALKLOAD_START	FRAME_walk_load1	// RSP 072900 - not used
//#define FRAME_WALKLOAD_END	FRAME_walk_load12	// RSP 072900 - not used

#define FRAME_DEATHA_START		FRAME_deatha1
#define FRAME_DEATHA_END		FRAME_deatha6

#define FRAME_DEATHB_START		FRAME_deathb1
#define FRAME_DEATHB_END		FRAME_deathb5

#define FRAME_RUN_START			FRAME_run1
#define FRAME_RUN_END			FRAME_run6		// RSP 072900 - was FRAME_run5


// Setup 
void bot_spawnsetup (edict_t *self);
qboolean botEvalTarget(edict_t *self, vec3_t dest, int maxdist, int maxup, int maxdown, int maxside);

// standing - defined globally
void bot_stand (edict_t *self);
mframe_t bot_frames_stand[];
mmove_t bot_move_stand;

//pain - defined individually
void bot_pain (edict_t *self, edict_t *other, float kick, int damage);
mmove_t bot_move_paina;
mframe_t bot_frames_paina[];
mmove_t bot_move_painb;
mframe_t bot_frames_painb[];

//Running - defined locally
void bot_run (edict_t *self);

//Walking
void bot_walk (edict_t *self);

// Pivot
void bot_pivot (edict_t *self, float yaw);

//Die
void bot_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);
void bot_dead (edict_t *self);

//Grabbing and whatnot
void botUnlockFrom(edict_t *self, edict_t *locked);
qboolean botLockTo(edict_t *self, edict_t *locked);

