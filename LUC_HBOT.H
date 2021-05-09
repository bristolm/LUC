// LUC Dreadlock Header File

// functions
void Cmd_Dreadlock_f(edict_t *);
void Launch_Dreadlock(edict_t *, qboolean);	// RSP 092900 - added boolean

// controlling parameters
#define TARGETRADIUS				800		
#define FIRERADIUS					800
#define CLUERADIUS					500
#define SEPARATIONRADIUS			150
#define DLOCK_MIN_DISTANCE			100
#define DLOCK_WAYPOINT_RADIUS		200	
#define DLOCK_WAYPOINT_CLOSE_ENOUGH	 50
#define DLOCK_MAX_HEALTH			100
#define DLOCK_DAMAGE				  5	// how much damage a dreadlock's blaster does
#define GUARDBOT_DAMAGE				  2	// how much damage a guardbot's blaster does
#define ACTIVATION_SOUND			gi.soundindex("hbot/active.wav")
//#define FIRE_SOUND				gi.soundindex("hbot/fire.wav")	// RSP 091700 - not used

// dreadlock's range from owner
#define MINLEASH					150 // closest dreadlock should get to owner
#define MAXLEASH					400 // farthest dreadlock should get from owner

// traveling speeds
#define DLOCK_WANDER_SPEED			 50  // wandering
#define DLOCK_CATCHUP_SPEED			150  // catching up to owner
#define DLOCK_ATTACK_SPEED			150  // moving into attack position

// model animations
#define DLOCK_FIRSTFRAME 0
#define DLOCK_LASTFRAME  0

// RSP 070500 - dreadlock's state control, kept in self->state
#define DLOCK_WANDER		0   // wandering around, idle
#define DLOCK_ATTACK		1   // attacking an enemy
#define DLOCK_FOLLOW		2   // following waypoints
#define DLOCK_CATCHUP		3	// lost sight of owner, now catching up
#define DLOCK_TELEPORT		4	// have to teleport
#define DLOCK_RETURN_ATTACK	5	// go back and gib a freshly-killed enemy

// RSP 070700 - dreadlock availability and activation states, kept in player->client->pers.dreadlock
#define DLOCK_UNAVAILABLE	-1	// you don't have a dreadlock yet
#define DLOCK_DOCKED		 0	// you have a dreadlock, but it isn't launched
#define DLOCK_LAUNCHED		 1	// dreadlock is launched



