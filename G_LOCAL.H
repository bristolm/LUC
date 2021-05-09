// g_local.h -- local definitions for game module

#include "q_shared.h"

// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include "game.h"

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"baseq2"

// protocol bytes that can be directly added to messages
#define	svc_muzzleflash		1
#define	svc_muzzleflash2	2
#define	svc_temp_entity		3
#define	svc_layout			4
#define	svc_inventory		5
// 3.20
#define	svc_stufftext		11

//==================================================================

// view pitching times
#define DAMAGE_TIME		0.5
#define	FALL_TIME		0.3

// RSP 092000 - for ent->waterlevel
#define WL_DRY			0
#define WL_FEETWET		1
#define WL_WAISTWET		2
#define WL_HEADWET		3

// RSP 083100 - for teleportgun - # of seconds to store victim
#define TGUN_STORE_TIME 10

// RSP 071699 - moved from g_func.c to be more general
#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

// RSP 072100 - moved from luc_hbot.h to be more general
#define DEBRIS1_MODEL "models/objects/debris1/tris.md2"
#define DEBRIS2_MODEL "models/objects/debris2/tris.md2"
#define DEBRIS3_MODEL "models/objects/debris3/tris.md2"

// RSP 092500 - game-ending timers
#define SCENARIO_FADEOUT_LENGTH		50
#define SCENARIO_BLACKOUT_LENGTH	10
#define SCENARIO_FADEIN_LENGTH		30
#define CAPTURE_FADEOUT_LENGTH		50
#define CAPTURE_BLACKOUT_LENGTH		10
#define CAPTURE_FADEIN_LENGTH		30

// RSP 101400 - dreadlock implant timers
#define IMPLANT_FADEOUT_LENGTH		50
#define IMPLANT_BLACKOUT_LENGTH		50
#define IMPLANT_FADEIN_LENGTH		30

// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define	SPAWNFLAG_AMBUSH			0x00000001
#define	SPAWNFLAG_TRIGGER_SPAWN		0x00000002
#define	SPAWNFLAG_SIGHT				0x00000004
#define	SPAWNFLAG_PACIFIST			0x00000008

#define	SPAWNFLAG_NOT_EASY			0x00000100
#define	SPAWNFLAG_NOT_MEDIUM		0x00000200
#define	SPAWNFLAG_NOT_HARD			0x00000400
#define	SPAWNFLAG_NOT_DEATHMATCH	0x00000800
#define	SPAWNFLAG_NOT_COOP			0x00001000

// RSP 062699 - Add spawnflags for func_rotating
#define SPAWNFLAG_FROT_START_ON			0x01
#define SPAWNFLAG_FROT_REVERSE			0x02
#define SPAWNFLAG_FROT_X_AXIS			0x04
#define SPAWNFLAG_FROT_Y_AXIS			0x08
#define SPAWNFLAG_FROT_TOUCH_PAIN		0x10
#define SPAWNFLAG_FROT_STOP				0x20
#define SPAWNFLAG_FROT_ANIMATED			0x40
#define SPAWNFLAG_FROT_ANIMATED_FAST	0x80

// RSP 062699 - Add spawnflags for path_corner
#define SPAWNFLAG_PC_TELEPORT			0x01
#define SPAWNFLAG_PC_X_AXIS				0x02
#define SPAWNFLAG_PC_Y_AXIS				0x04
#define SPAWNFLAG_PC_REVERSE			0x08

// RSP 062699 - Add spawnflag for func_timer
#define SPAWNFLAG_FT_START_ON			0x01

// RSP 062699 - Add spawnflags for func_conveyor
#define SPAWNFLAG_FC_START_ON			0x01
#define SPAWNFLAG_FC_TOGGLE				0x02

// RSP 062899 - Add spawnflag for combat_point
#define SPAWNFLAG_CP_HOld				0x01

// RSP 062899 - Add spawnflags for func_wall
#define SPAWNFLAG_FWALL_TRIGGER_SPAWN	0x01
#define SPAWNFLAG_FWALL_TOGGLE			0x02
#define SPAWNFLAG_FWALL_START_ON		0x04
#define SPAWNFLAG_FWALL_ANIMATED		0x08
#define SPAWNFLAG_FWALL_ANIMATED_FAST	0x10

// RSP 062899 - Add spawnflags for func_object
#define SPAWNFLAG_FOBJ_TRIGGER_SPAWN	0x01
#define SPAWNFLAG_FOBJ_ANIMATED			0x02
#define SPAWNFLAG_FOBJ_ANIMATED_FAST	0x04

// RSP 062899 - Add spawnflags for func_explosive
#define SPAWNFLAG_FEXPL_TRIGGER_SPAWN	0x01
#define SPAWNFLAG_FEXPL_ANIMATED		0x02
#define SPAWNFLAG_FEXPL_ANIMATED_FAST	0x04

// RSP 062899 - Add spawnflags for func_clock
#define SPAWNFLAG_FCLK_TIMER_UP			0x01
#define SPAWNFLAG_FCLK_TIMER_DOWN		0x02
#define SPAWNFLAG_FCLK_START_OFF		0x04
#define SPAWNFLAG_FCLK_MULTI_USE		0x08

// RSP 062899 - Add spawnflags for misc_teleporter
#define SPAWNFLAG_MTEL_NO_FOUNTAIN_FX	0x01
#define SPAWNFLAG_MTEL_NO_SOUND			0x02
#define SPAWNFLAG_MTEL_NO_PAUSE			0x04
#define SPAWNFLAG_MTEL_NO_FLASH			0x08
#define SPAWNFLAG_MTEL_MAINTAIN_MOTION	0x10

// RSP 090199 - Add spawnflag for func_teleporter
#define SPAWNFLAG_FTEL_KILLMOMENTUM		0x01
#define SPAWNFLAG_FTEL_INACTIVE			0x02    // RSP 100499

// RSP 062899 - Add spawnflags for trigger_multiple
#define SPAWNFLAG_TMUL_MONSTER			0x01
#define SPAWNFLAG_TMUL_NOT_PLAYER		0x02
#define SPAWNFLAG_TMUL_TRIGGERED		0x04
#define SPAWNFLAG_COREAI				0x08	// RSP 101500 - use same bit for all triggers

// RSP 062899 - Add spawnflags for trigger_once
#define SPAWNFLAG_T1_OLDTRIGGER			0x01
#define SPAWNFLAG_T1_NEWTRIGGER			0x04
//#define SPAWNFLAG_COREAI				0x08	// RSP 101500

// RSP 062899 - Add spawnflags for trigger_counter
#define SPAWNFLAG_TC_NOMESSAGE			0x01
#define SPAWNFLAG_TC_RESET				0x02		// RSP 051100

// RSP 062899 - Add spawnflags for trigger_hurt
#define SPAWNFLAG_TH_START_OFF			0x01
#define SPAWNFLAG_TH_TOGGLE				0x02
#define SPAWNFLAG_TH_SILENT				0x04
#define SPAWNFLAG_TH_NO_PROTECTION		0x08
#define SPAWNFLAG_TH_SLOW				0x10

// WPO 051800 - Add spawnflags for trigger_push
#define SPAWNFLAG_TP_PUSH_ONCE			0x01
#define SPAWNFLAG_TP_START_OFF			0x02
#define SPAWNFLAG_TP_TOGGLE				0x04

// RSP 062899 - Add spawnflags for func_steam
#define SPAWNFLAG_FS_OFF_BY_TIMER		0x01

// RSP 062899 - Add spawnflags for target_help
#define SPAWNFLAG_THELP_HELP1			0x01

// RSP 062899 - Add spawnflags for target_blaster
#define SPAWNFLAG_TB_NOTRAIL			0x01
#define SPAWNFLAG_TB_NOEFFECTS			0x02

// RSP 062699 - Add spawnflags for target_laser
#define SPAWNFLAG_TLAS_START_ON			0x01
#define SPAWNFLAG_TLAS_RED				0x02
#define SPAWNFLAG_TLAS_GREEN			0x04
#define SPAWNFLAG_TLAS_BLUE				0x08
#define SPAWNFLAG_TLAS_YELLOW			0x10
#define SPAWNFLAG_TLAS_ORANGE			0x20
#define SPAWNFLAG_TLAS_FAT				0x40
#define SPAWNFLAG_TLAS_FLICKER_BOUND	0x80	// RSP 082400 - was 0x40, which is already *_FAT

// RSP 062699 - Add spawnflags for target_lightramp
#define SPAWNFLAG_TLR_TOGGLE			0x01

// RSP 062899 - Add spawnflag for misc_sprite
#define SPAWNFLAG_SP_TRANSLUCENT		0x01

// RSP 042799 - Add spawnflag defines for func_speaker_sequencer
#define SPAWNFLAG_FSS_START_ON			0x01
#define SPAWNFLAG_FSS_SSM				0x02
#define SPAWNFLAG_FSS_SSM_RELIABLE		0x04
#define SPAWNFLAG_FSS_SSM_LOOP_NOISE	0x08
#define SPAWNFLAG_FSS_SSM_OVERLAP		0x10

// RSP 050899 - Add spawnflag defines for func_event_sequencer
#define SPAWNFLAG_FES_SIMULTANEOUS		0x01
#define SPAWNFLAG_FES_REPEAT			0x02

// RSP 042799 - Add spawnflag defines for target_speaker
#define	SPAWNFLAG_TS_LOOPED_ON			0x01
#define	SPAWNFLAG_TS_LOOPED_OFF			0x02
#define	SPAWNFLAG_TS_RELIABLE			0x04

// RSP 042999 - Add spawnflag define for waypoints
#define SPAWNFLAG_WAYPOINT_RETREAT		0x01
#define SPAWNFLAG_WAYPOINT_DLOCK		0x02	// WPO 092699 Add dreadlock waypoints 

// RSP 052699 - Add spawnflag defines for item_deco
#define	SPAWNFLAG_ID_TRANSLUCENT		0x01
#define	SPAWNFLAG_ID_SOLID				0x02
#define	SPAWNFLAG_ID_START_OFF			0x04	// RSP 062199 - for animations
#define	SPAWNFLAG_ID_PUSH				0x08	// RSP 062299 - pushable item
#define	SPAWNFLAG_ID_EXPLODE			0x10	// RSP 062299 - explodable item
#define	SPAWNFLAG_ID_DROP				0x20	// RSP 062299 - drop to ground, else use given origin

// RSP 071699 - Add spawnflags for func_rotrains (rotating func_trains)
#define	SPAWNFLAG_RT_TRANSLATING		0x001
#define	SPAWNFLAG_RT_TOGGLE				0x002
#define	SPAWNFLAG_RT_BLOCK_STOPS		0x004
#define	SPAWNFLAG_RT_ROTATING			0x008	// 0 = stationary, 1 = rotating
#define	SPAWNFLAG_RT_TOUCH_PAIN			0x010
#define SPAWNFLAG_RT_ANIMATED			0x020	// RSP 080499 - moved here from path_corners
#define SPAWNFLAG_RT_ANIMATED_FAST		0x040	// RSP 080499 - moved here from path_corners
#define	SPAWNFLAG_RT_LATCHTRANS			0x080	// delay translation; not a spawnflag
#define SPAWNFLAG_RT_FINALROTATION		0x100	// final frame in a rotation
#define SPAWNFLAG_RT_FINALTRANS			0x200	// final frame in a translation
#define SPAWNFLAG_RT_WAITING			0x400	// in a waiting state at a path_corner

// RSP 062699 - Add spawnflags for func_searchlight
#define SPAWNFLAG_SL_START_ON			0x01
#define SPAWNFLAG_SL_REVERSE			0x02

// RSP 062000 - Add spawnflags for hand-thrown grenades
#define SPAWNFLAG_GR_HANDGRENADE		0x01	// A zero value for this flag means a launched grenade
#define SPAWNFLAG_GR_HELD				0x02	// Grenade hadn't been thrown when it exploded

// RSP 082400 - Add spawnflag for light
#define SPAWNFLAG_LT_START_OFF			0x01

// RSP 101600 - Add spawnflags for trigger_hbot
#define SPAWNFLAG_DL_NOMESSAGE			0x01

// RSP 103100 - Add spawnflags for trigger_scenario_end
#define SPAWNFLAG_TS_START_OFF			0x01
#define SPAWNFLAG_TS_TOGGLE				0x02


// RSP 072099 - Add style types for path_corners used with func_rotrains
#define PCSTYLE_TRANS				1
#define PCSTYLE_TRANS_ROT			2
#define PCSTYLE_TRANS_ROT_ARC		3
#define PCSTYLE_TRANS_ROT_NEXTPATH	4
#define PCSTYLE_ROT_ARC_TRANS		5

// RSP 081300 - Add style types for beetles
#define BEETLE_STYLE_PACIFIST		1
#define BEETLE_STYLE_SHOOT_MILD		2	// shoots randomly
#define BEETLE_STYLE_SHOOT_MED		3	// shoots more often, hurts more, moves faster
#define BEETLE_STYLE_SHOOT_HOT		4	// shoots even more, hurts a lot, moves even faster
#define BEETLE_STYLE_SHOOT_KILLER	5	// lethal shot

// edict->flags
#define	FL_FLY					0x00000001
#define	FL_SWIM					0x00000002	// implied immunity to drowning
//#define UNUSED				0x00000004	// was FL_IMMUNE_LASER, no longer used
#define	FL_INWATER				0x00000008
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
//#define UNUSED				0x00000040	// was FL_IMMUNE_SLIME, no longer used
//#define UNUSED				0x00000080	// was FL_IMMUNE_LAVA, no longer used
#define	FL_PARTIALGROUND		0x00000100	// not all corners are valid
//#define UNUSED				0x00000200	// was FL_WATERJUMP, no longer used
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_POWER_ARMOR			0x00001000	// power armor (if any) is active
#define FL_HEALTH_DECREMENT		0x00002000	// RSP 091900 - if set, decrement health to 100
#define FL_NODROWN				0x00004000	// player doesn't drown (cvar)
#define FL_ON					0x00008000	// RSP 062199 - general on/off flag
// MRB 19980909 - New flag for the 'extension' entity
#define FL_MASTER_EXT			0x00010000  // treat as if locked to master
#define FL_EFFECT_MASTER		0x00020000  // feedback on master if blocked
#define FL_AGGRESSIVE           0x00040000  // RSP 070500 - for dreadlock & Evil Guardbot
#define FL_SUPERAGGRESSIVE		0x00080000	// RSP 102700 - for Evil Guardbot
#define FL_PRINT_LOCATION		0x00100000	// RSP 032299: Print player location if set
#define FL_ROTATEONLY			0x00200000	// RSP 040899: Rotate Great White in place
#define FL_ATTACKING			0x00400000	// RSP 050199: Attacking monster
#define FL_RETREATING			0x00800000	// RSP 050199: Retreating monster
#define FL_SLOWDEATH			0x01000000	// RSP 110400: Slow endgame death
#define FL_SPARKDAMAGE			0x02000000	// RSP 051899: spark damage if set, blood if not
#define FL_FIRSTDLOCK			0x04000000	// RSP 101400: first dreadlock was picked up, if set
#define FL_FROZEN				0x08000000	// RSP 052599: matrix shot freezes monster
#define FL_NOBLEND				0x10000000	// RSP 061499: No damage screen blends if set
#define FL_CAPTURED				0x20000000	// RSP 092900: endgame capture state
#define FL_PASSENGER			0x40000000	// RSP 061899: BoxCar Passenger candidate
#define FL_RESPAWN				0x80000000	// used for item respawning

// RSP 062099 - Identity flags

typedef enum
{
	ID_NULL,
	ID_WAYPOINT,
	ID_GREAT_WHITE,
	ID_JELLY,
	ID_LITTLEBOY,
	ID_TURRET,
	ID_FATMAN,
	ID_SUPERVISOR,
	ID_PLASMABOT,
	ID_DLOCK,
	ID_DECOY,
	ID_GBOT,
	ID_BEETLE,
	ID_BEAMBOT,
	ID_CLAWBOT,
	ID_PROBEBOT,
	ID_DRILLBOT,
	ID_ROCKETLIGHT,
	ID_AMBUSHBOT_DISK,
	ID_AMBUSHBOT_BOLT,
	ID_BATTLEBOT_DISK,
	ID_BATTLEBOT_BOLT,
	ID_REPEATER,
	ID_GUARDBOT			// RSP 100700
} identity_t;

// RSP 032699 - Shark turning flags
#define TD_LEFT			 0
#define TD_STRAIGHT		 1
#define TD_RIGHT		 2

// Matrix constants
#define MATRIX_EXPANSION_FRAMES	19
#define MATRIX_MIN_RAD			10
#define MATRIX_START_SKIN	 	 0
#define MATRIX_END_SKIN			14

// RSP 051399 - ammo type flags, initially for speargun
#define AMMO_TYPE_NORMAL_SPEAR	0
#define AMMO_TYPE_ROCKET_SPEAR	1

#define	FRAMETIME		0.1

// memory tags to allow dynamic memory to be cleaned up
#define	TAG_GAME	765		// clear when unloading the dll
#define	TAG_LEVEL	766		// clear when loading a new level


#define MELEE_DISTANCE	   80
#define GW_MELEE_DISTANCE 100	// RSP 040199 - shark center is 70 from his nose!
#define FM_MELEE_DISTANCE 128	// RSP 061799 - fatman center is 88 from his nose!

#define BODY_QUEUE_SIZE		8

typedef enum
{
	DAMAGE_NO,
	DAMAGE_YES,			// will take damage if hit
	DAMAGE_AIM			// auto targeting recognizes this
} damage_t;

typedef enum 
{
	WEAPON_READY, 
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponstate_t;

typedef enum
{
	AMMO_GRENADES,
	AMMO_SPEARS,		// JBM 082998 Add support for spears
	AMMO_DISKS,			// RSP 121298 Add support for disks
	AMMO_PLASMA,		// MRB 031599 Add support for plasmawad (also used by matrix)
	AMMO_ROCKET_SPEARS,	// RSP 051399 Add support for rocket spears
	AMMO_BOLTS,			// RSP 051799 Add support for bolts
	AMMO_ICEPELLETS,	// WPO 111199 Add support for ice pellets
	AMMO_BULLETS,		// WPO 070100 Add support for bullets
	AMMO_RODS			// RSP 090100 Add support for teleport rods
} ammo_t;


//deadflag
#define DEAD_NO					0
#define DEAD_DYING				1
#define DEAD_DEAD				2
#define DEAD_RESPAWNABLE		3

//range
#define RANGE_MELEE				0
#define RANGE_NEAR				1
#define RANGE_MID				2
#define RANGE_FAR				3

//gib types
#define GIB_ORGANIC				0
#define GIB_METALLIC			1

//monster ai flags
#define AI_STAND_GROUND			0x00000001
#define AI_TEMP_STAND_GROUND	0x00000002
#define AI_SOUND_TARGET			0x00000004
#define AI_LOST_SIGHT			0x00000008
#define AI_PURSUIT_LAST_SEEN	0x00000010
#define AI_PURSUE_NEXT			0x00000020
#define AI_PURSUE_TEMP			0x00000040
#define AI_HOLD_FRAME			0x00000080
#define AI_GOOD_GUY				0x00000100
#define AI_BRUTAL				0x00000200
#define AI_NOSTEP				0x00000400  // MRB 19981001: With flyers and swimmers, this causes them to hover
#define AI_DUCKED				0x00000800
#define AI_COMBAT_POINT			0x00001000
#define AI_MEDIC				0x00002000
#define AI_RESURRECTING			0x00004000
// AJA 19980906 - AI_PACIFIST flag means that the monster will
// ignore the player unless shot.
#define AI_PACIFIST				0x00008000
// AJA 19980908 - AI_RECHARGING means that the monster is
// in the process of being recharged by the supervisor.
#define AI_RECHARGING			0x00010000

//monster attack state
#define AS_STRAIGHT				1
#define AS_SLIDING				2
#define	AS_MELEE				3
#define	AS_MISSILE				4

/* RSP 082500 - armor types not needed in LUC
// armor types
#define ARMOR_NONE				0
#define ARMOR_JACKET			1
#define ARMOR_COMBAT			2
#define ARMOR_BODY				3
#define ARMOR_SHARD				4
 */

// power armor types
#define POWER_ARMOR_NONE		0
#define POWER_ARMOR_SCREEN		1
#define POWER_ARMOR_SHIELD		2

// handedness values
#define RIGHT_HANDED			0
#define LEFT_HANDED				1
#define CENTER_HANDED			2

// game.serverflags values
#define SFL_CROSS_TRIGGER_1		0x00000001
#define SFL_CROSS_TRIGGER_2		0x00000002
#define SFL_CROSS_TRIGGER_3		0x00000004
#define SFL_CROSS_TRIGGER_4		0x00000008
#define SFL_CROSS_TRIGGER_5		0x00000010
#define SFL_CROSS_TRIGGER_6		0x00000020
#define SFL_CROSS_TRIGGER_7		0x00000040
#define SFL_CROSS_TRIGGER_8		0x00000080
#define SFL_CROSS_TRIGGER_MASK	0x000000ff

// noise types for PlayerNoise
#define PNOISE_SELF				0
#define PNOISE_WEAPON			1
#define PNOISE_IMPACT			2

// RSP 010399 - Generalized quantity of air you get with an air tank, and
//              when you're low on air.
#define AIR_QUANTITY     1500
#define AIR_LOW			  300

// edict->movetype values
typedef enum
{
MOVETYPE_NONE,			// never moves
MOVETYPE_NOCLIP,		// origin and angles change with no interaction
MOVETYPE_PUSH,			// no clip to world, push on box contact
MOVETYPE_STOP,			// no clip to world, stops on box contact

MOVETYPE_WALK,			// gravity
MOVETYPE_STEP,			// gravity, special edge handling
MOVETYPE_FLY,
MOVETYPE_TOSS,			// gravity
MOVETYPE_FLYMISSILE,	// extra size to monsters
MOVETYPE_BOUNCE,
MOVETYPE_RICOCHET		// RSP 121298 Added ricocheting disks
} movetype_t;

/* RSP 082500 - not used in LUC
typedef struct
{
	int		base_count;
	int		max_count;
	float	normal_protection;
	float	energy_protection;
	int		armor;
} gitem_armor_t;
 */

// gitem_t->flags
#define	IT_WEAPON		0x001	// use makes active weapon
#define	IT_AMMO			0x002
#define IT_ARMOR		0x004
#define IT_STAY_COOP	0x008
#define IT_KEY			0x010
#define IT_POWERUP		0x020
#define IT_DECO			0x040	// RSP 052699 - decoration
#define IT_DONTGIVE		0x080	// RSP 091200 - don't give with the 'give all' cheat
#define IT_NOTININV		0x100	// RSP 091800 - not droppable, so not really in inventory

// 3.20
// gitem_t->weapmodel for weapons indicates model index
#define WEAP_KNIFE				1 
#define WEAP_SPEARGUN			2 
#define WEAP_DISKGUN			3 
#define WEAP_PLASMAWAD			4
#define WEAP_BOLTGUN			5	// RSP 051799
#define WEAP_LUCSHOTGUN			6	// WPO 021199 LUC Shotgun
#define WEAP_TELEPORTGUN		7	// WPO 231099 Teleportgun
#define WEAP_FREEZEGUN			8	// WPO 241099 Freezegun
#define WEAP_RIFLE				9	// WPO 070100 Sniper Rifle
#define WEAP_MATRIX				10	// RSP 051999
#define WEAP_GRENADES			11	// WPO 071699
// 3.20 end 

typedef struct gitem_s
{
	char		*classname;	// spawning name
	qboolean	(*pickup)(struct edict_s *ent, struct edict_s *other);
	void		(*use)(struct edict_s *ent, struct gitem_s *item);
	void		(*drop)(struct edict_s *ent, struct gitem_s *item);
	void		(*weaponthink)(struct edict_s *ent);
	char		*pickup_sound;
	char		*world_model;
	int			world_model_flags;
	char		*view_model;

	// client side info
	char		*icon;
	char		*pickup_name;	// for printing on pickup
	int			count_width;	// number of digits to display by icon

	int			quantity;		// for ammo how much, for weapons how much is used per shot
	char		*ammo;			// for weapons
	int			flags;			// IT_* flags

	int			weapmodel;		// weapon model index (for weapons) 3.20

	void		*info;
	int			tag;

	char		*precaches;		// string of all models, sounds, and images this item will use
} gitem_t;

//
// this structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
//
typedef struct
{
	char		helpmessage1[512];
	char		helpmessage2[512];
	int			helpchanged;	// flash F1 icon if non 0, play sound
								// and increment only if 1, 2, or 3

	gclient_t	*clients;		// [maxclients]

	// can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char		spawnpoint[512];	// needed for coop respawns

	// store latched cvars here that we want to get at often
	int			maxclients;
	int			maxentities;

	// cross level triggers
	int			serverflags;

	// items
	int			num_items;

	qboolean	autosaved;
} game_locals_t;


//
// this structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
//
typedef struct
{
	int			framenum;
	float		time;

	char		level_name[MAX_QPATH];	// the descriptive name (Outer Base, etc)
	char		mapname[MAX_QPATH];		// the server name (base1, etc)
	char		nextmap[MAX_QPATH];		// go here when fraglimit is hit

	// intermission state
	float		intermissiontime;		// time the intermission was started
	char		*changemap;
	int			exitintermission;
	vec3_t		intermission_origin;
	vec3_t		intermission_angle;

	edict_t		*sight_client;			// changed once each frame for coop games

	edict_t		*sight_entity;
	int			sight_entity_framenum;
	edict_t		*sound_entity;
	int			sound_entity_framenum;
	edict_t		*sound2_entity;
	int			sound2_entity_framenum;

	int			pic_health;

	int			total_secrets;
	int			found_secrets;

	int			total_goals;
	int			found_goals;

	int			total_monsters;
	int			killed_monsters;

	edict_t		*current_entity;	// entity running from G_RunFrame
	int			body_que;			// dead bodies

	int			power_cubes;		// ugly necessity for coop
} level_locals_t;


// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actually present
// in edict_t during gameplay
typedef struct
{
	// world vars
	char		*sky;
	float		skyrotate;
	vec3_t		skyaxis;
	char		*nextmap;

	int			lip;
	int			distance;
	int			height;
	char		*noise;
	float		pausetime;
	char		*item;
	char		*gravity;

	float		minyaw;
	float		maxyaw;
	float		minpitch;
	float		maxpitch;

	// JBM 19981212 - Added the "times", "attenuations", "volumes", "noises", "speakers" fields for
	// the func_speaker_sequencer. These get stuffed into arrays with the same name w/ a trailing _
	// so we don't have to parse the string every time
	char		*noises;
	char		*times;
	char		*speakers;
	char		*attenuations;
	char		*volumes;

	// JBM 19980101 - Add the random_style spawntime temp for the func_speaker_sequencer
	int			random_style;

	char		*events;	// RSP 050899 - for f_e_s
	float		duration;	// RSP 050899 - for f_e_s

	char		*sizes;		// RSP 052699 - for item_deco
	int			frames;		// RSP 062099 - for item_deco animation
	int			skins;		// RSP 072500 - for beetle skin animation
} spawn_temp_t;


typedef struct
{
	// fixed data
	vec3_t		start_origin;
	vec3_t		start_angles;
	vec3_t		end_origin;
	vec3_t		end_angles;

	int			sound_start;
	int			sound_middle;
	int			sound_end;

	float		accel;
	float		speed;
	float		decel;
	float		distance;

	float		wait;

	// state data
	int			state;
	vec3_t		dir;
	float		current_speed;
	float		move_speed;
	float		next_speed;
	float		remaining_distance;
	float		decel_distance;
	void		(*endfunc)(edict_t *);
} moveinfo_t;


typedef struct
{
	void	(*aifunc)(edict_t *self, float dist);
	float	dist;
	void	(*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct
{
	int			firstframe;
	int			lastframe;
	mframe_t	*frame;
	void		(*endfunc)(edict_t *self);
} mmove_t;

typedef struct
{
	mmove_t		*currentmove;
	int			aiflags;
	int			nextframe;
	float		scale;

	void		(*stand)(edict_t *self);
	void		(*idle)(edict_t *self);
	void		(*search)(edict_t *self);
	void		(*walk)(edict_t *self);
	void		(*run)(edict_t *self);
// MRB 070498 added for pivot logic attempt
	void        (*pivot) (edict_t *ent, float yaw); 

	void		(*dodge)(edict_t *self, edict_t *other, float eta);
	void		(*attack)(edict_t *self);
	void		(*melee)(edict_t *self);
	void		(*sight)(edict_t *self, edict_t *other);
	qboolean	(*checkattack)(edict_t *self);

	float		pausetime;
	float		attack_finished;

	vec3_t		saved_goal;
	float		search_time;
	float		trail_time;
	vec3_t		last_sighting;
	int			attack_state;
	int			lefty;
	float		idle_time;
	int			linkcount;

	int			power_armor_type;
	int			power_armor_power;

	float		quad_framenum;		// 231199 Monsters can have quad
} monsterinfo_t;

// JBM 121698 - Added a struct to support some sequencer data
typedef struct
{
	char		**noises;
	char		**times;
	char		**speakers;
	char		**attenuations;
	char		**volumes;

	int			loopelement;
//	int			currenttime;		// RSP 050599 - recode f_s_s
	int			currentspeaker;
	int			currentnoise;
//	int			currentattenuation;	// RSP 050599 - recode f_s_s
//	int			currentvolume;		// RSP 050599 - recode f_s_s
	int			timeselements;
	int			speakerselements;
	int			noiseselements;
	int			attenuationselements;
	int			volumeselements;
	int			random_style;		// JBM 19980101 - Add the random_style
	int			prev_speaker_index;	// RSP 050599 - recode f_s_s
	int			noisecounter;		// RSP 050599 - how many noises we've played on this speaker

	// RSP 050899 - New fields for func_event_sequencer
	int			currentevent;
	char		**events;
	int			eventelements;
	float		duration;
} tnt_sequencer_info;


extern	game_locals_t	game;
extern	level_locals_t	level;
extern	game_import_t	gi;
extern	game_export_t	globals;
extern	spawn_temp_t	st;

//extern	int	sm_meat_index;	// RSP 092100 - no longer used
extern	int	snd_fry;

//extern	int	jacket_armor_index;	// RSP 082800 - not in LUC
//extern	int	combat_armor_index;	// RSP 082800 - not in LUC
//extern	int	body_armor_index;	// RSP 082800 - not in LUC

// RSP 100400 - endgame scenario flags get set when a scenario is completed.
//              use with ent->endgame_flags
//              remember to add extra flags to SCENARIO_ALL if more scenarios
//              are added.
#define SCENARIO_01					0x00000001
#define SCENARIO_02					0x00000002
#define SCENARIO_03					0x00000004
#define SCENARIO_ALL				(SCENARIO_01|SCENARIO_02|SCENARIO_03)

// RSP 102400 - more flags, but these represent which scenario
//				you're currently in.
//				use with ent->endgame_flags
#define CURRENT_SCENARIO_01			0x00010000
#define CURRENT_SCENARIO_02			0x00020000
#define CURRENT_SCENARIO_03			0x00040000
#define CURRENT_SCENARIO_MASK		(CURRENT_SCENARIO_01|CURRENT_SCENARIO_02|CURRENT_SCENARIO_03)

// means of death
#define MOD_UNKNOWN			0
#define MOD_BLASTER			1
//#define MOD_SHOTGUN		2		// RSP 091900 - not in LUC
#define MOD_SSHOTGUN		3		// WPO 021199 re-use this for LUC shotgun
//#define MOD_MACHINEGUN	4		// RSP 090100 - not in LUC
//#define MOD_CHAINGUN		5		// RSP 090100 - not in LUC
#define MOD_GRENADE			6
#define MOD_G_SPLASH		7
//#define MOD_ROCKET		8		// RSP 090100 - not in LUC
#define MOD_R_SPLASH		9
//#define MOD_HYPERBLASTER	10		// RSP 090100 - not in LUC
//#define MOD_RAILGUN		11		// RSP 090100 - not in LUC
#define MOD_MATRIX_EFFECT	14		// RSP 051599 - Matrix
#define MOD_HANDGRENADE		15
#define MOD_HG_SPLASH		16
#define MOD_WATER			17
#define MOD_SLIME			18
#define MOD_LAVA			19
#define MOD_CRUSH			20
#define MOD_TELEFRAG		21
#define MOD_FALLING			22
#define MOD_SUICIDE			23
#define MOD_HELD_GRENADE	24
#define MOD_EXPLOSIVE		25
#define MOD_BARREL			26
#define MOD_BOMB			27
#define MOD_EXIT			28
#define MOD_SPLASH			29
#define MOD_TARGET_LASER	30
#define MOD_TRIGGER_HURT	31
#define MOD_HIT				32
#define MOD_TARGET_BLASTER	33
#define MOD_SPEARGUN		34		// JBM 082998 Add support for speargun
#define MOD_DISKGUN			35		// RSP 121298 Add support for diskgun
#define MOD_KNIFE			37		// MRB 022299 Add support for knife
#define MOD_PLASMAWAD		38		// MRB 022299 Add support for plasmwad
#define MOD_ROCKET_SPEAR	39		// RSP 051399 Add support for rocket spears
#define MOD_BOLTGUN			40		// RSP 051799 Add support for boltgun
#define MOD_DREADLOCK		41		// WPO 081099 Add support for dreadlock
#define MOD_RIFLE			42		// WPO 070100 Add support for sniper rifle
#define MOD_BEETLE			43		// RSP 072100
#define MOD_FRIENDLY_FIRE	0x8000000

// RSP 062900 - Add a variety of explosion sounds
#define EXPLODE_SILENT                  0       // no sound
#define EXPLODE_LITTLEBOY               1
#define EXPLODE_LITTLEBOY_WATER         2
#define EXPLODE_GRENADE                 3
#define EXPLODE_GRENADE_WATER           4
#define EXPLODE_GBOT                    5
#define EXPLODE_FATMAN                  6
#define EXPLODE_DECO                    7
#define EXPLODE_ROCKETSPEAR             8
#define EXPLODE_ROCKETSPEAR_WATER       9
#define EXPLODE_TURRET                  10
#define EXPLODE_DLOCK                   11
#define EXPLODE_BIG                     EXPLODE_FATMAN
#define EXPLODE_SMALL                   EXPLODE_GRENADE

extern	int	meansOfDeath;

extern int hp_table[];				// RSP 051599
extern int wd_table[];				// RSP 051599

extern	edict_t				*g_edicts;
extern	tnt_sequencer_info	*g_tnt_seqs;

/*
	JBM 121498 - Just Documenting the below Offset functions as I was in here researching
				 I'm guessing on the FOFS but it would seem correct.
	
	FOFS - Field Offset
	STOFS - SpawnTemp Offset
	LLOFS - LevelLocals Offset
	CLOFS - Client Offset
	SQOFS - Sequencer Offset
*/
#define	FOFS(x)  (int)&(((edict_t *)0)->x)
#define	STOFS(x) (int)&(((spawn_temp_t *)0)->x)
#define	LLOFS(x) (int)&(((level_locals_t *)0)->x)
#define	CLOFS(x) (int)&(((gclient_t *)0)->x)
#define	SQOFS(x) (int)&(((tnt_sequencer_info *)0)->x)

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

extern	cvar_t	*maxentities;
extern	cvar_t	*deathmatch;
extern	cvar_t	*coop;
extern	cvar_t	*dmflags;
extern	cvar_t	*skill;
extern	cvar_t	*fraglimit;
extern	cvar_t	*timelimit;
extern	cvar_t	*password;
extern	cvar_t	*spectator_password;	// 3.20
extern	cvar_t	*needpass;				// 3.20
extern	cvar_t	*g_select_empty;
extern	cvar_t	*dedicated;

extern	cvar_t	*filterban;				// 3.20

extern	cvar_t	*sv_gravity;
extern	cvar_t	*sv_maxvelocity;

extern	cvar_t	*gun_x, *gun_y, *gun_z;
extern	cvar_t	*sv_rollspeed;
extern	cvar_t	*sv_rollangle;

extern	cvar_t	*run_pitch;
extern	cvar_t	*run_roll;
extern	cvar_t	*bob_up;
extern	cvar_t	*bob_pitch;
extern	cvar_t	*bob_roll;

extern	cvar_t	*sv_cheats;
extern	cvar_t	*maxclients;

// 3.20
extern	cvar_t	*maxspectators;
extern	cvar_t	*flood_msgs;
extern	cvar_t	*flood_persecond;
extern	cvar_t	*flood_waitdelay;
extern	cvar_t	*sv_maplist;
// 3.20 end

// RSP 042599 - Added CD cvar. "0" = get from CD. "1" = get from pak
extern	cvar_t	*cd_source;

#define CD_SOURCE_CD			0
#define CD_SOURCE_PAK			1
#define JELLY_IDLE				0.7	// RSP 070400

#define world (&g_edicts[0])

// item spawnflags
#define ITEM_TRIGGER_SPAWN		0x00000001
#define ITEM_NO_TOUCH			0x00000002
// 6 bits reserved for editor flags
//								0x00000004
//								0x00000008
//								0x00000010
//								0x00000020
//								0x00000040
//								0x00000080
// 8 bits used as power cube id bits for coop games
//								0x00000100
//								0x00000200
//								0x00000400
//								0x00000800
//								0x00001000
//								0x00002000
//								0x00004000
//								0x00008000
#define DROPPED_ITEM			0x00010000
#define	DROPPED_PLAYER_ITEM		0x00020000
#define ITEM_TARGETS_USED		0x00040000

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP		1
// 3.20
#define FFL_NOSPAWN			2

typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_FUNCTION,			// 3.20
	F_MMOVE,			// 3.20
	F_IGNORE,

	// JBM 121498 - Adding support for the speaker sequencer
	F_TNT_SEQUENCER_INFO,	//index on disk, pointer in memory

	// RSP 031199 - Adding support for speaker sequencer **char fields
	F_SEQUENCER_PPCHAR	// string on disk, pointer in memory, TAG_LEVEL
} fieldtype_t;

typedef struct
{
	char	*name;
	int		ofs;
	fieldtype_t	type;
	int		flags;
} field_t;


extern	field_t fields[];
extern	gitem_t	itemlist[];


//
// g_cmds.c
//
void Cmd_Help_f (edict_t *);
void Cmd_Score_f (edict_t *);
void SelectNextItem (edict_t *, int);	// RSP 091900
void SelectPrevItem (edict_t *, int);	// RSP 091900

//
// g_func.c
//
void Move_Calc (edict_t *ent, vec3_t dest, void(*func)(edict_t*));
void Move_Begin (edict_t *);
void Move_Final (edict_t *);
void rotating_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);

//
// g_items.c
//
void PrecacheItem (gitem_t *it);
void InitItems (void);
void SetItemNames (void);
void SetRespawn (edict_t *ent, float delay);
void ChangeWeapon (edict_t *ent);
void SpawnItem (edict_t *ent, gitem_t *item);
void Think_Weapon (edict_t *ent);
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf);
void droptofloor (edict_t *);	// RSP 081500
gitem_t	*FindItem (char *pickup_name);
gitem_t	*FindItemByClassname (char *classname);
gitem_t	*GetItemByIndex (int index);
qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count);
edict_t *Drop_Item (edict_t *ent, gitem_t *item);
int PowerArmorType (edict_t *ent);
#define	ITEM_INDEX(x) ((x)-itemlist)

//
// g_save.c
//
void ReadGame(char *);
void WriteGame(char *,qboolean);
void WriteLevel (char *);
void ReadLevel (char *);
qboolean ClientConnect (edict_t *, char *);
void ClientBegin (edict_t *);

//
// g_trigger.c
//
void InitTrigger (edict_t *);	// RSP 092900

//
// g_utils.c
//
qboolean	KillBox (edict_t *ent);
void	G_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);
edict_t *G_Find (edict_t *from, int fieldofs, char *match);
edict_t *findradius (edict_t *from, vec3_t org, float rad);
edict_t *G_PickTarget (char *targetname);
void	G_UseTargets (edict_t *ent, edict_t *activator);
void	G_SetMovedir (vec3_t angles, vec3_t movedir);
qboolean Is_Projectile(edict_t *);

void	G_InitEdict (edict_t *e);
edict_t	*G_Spawn (void);
void	G_FreeEdict (edict_t *e);

void	G_TouchTriggers (edict_t *ent);
void	G_TouchSolids (edict_t *ent);

char	*G_CopyString (char *in);

float	*tv (float x, float y, float z);
char	*vtos (vec3_t v);

float	vectoyaw (vec3_t vec);
void	vectoangles (vec3_t vec, vec3_t angles);
void	r_vectoangles (vec3_t vec, vec3_t angles);	// RSP 070400

//
// g_spawn.c
//
void	Add_BoxCar();	// RSP 042000

//
// g_combat.c
//
qboolean OnSameTeam (edict_t *ent1, edict_t *ent2);
qboolean CanDamage (edict_t *targ, edict_t *inflictor);
void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod);
// RSP 010399: T_RadiusDamage now returns true/false
qboolean T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);

// damage flags
#define DAMAGE_RADIUS			0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR			0x00000002	// armour does not protect from this damage
#define DAMAGE_ENERGY			0x00000004	// damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK		0x00000008	// do not affect velocity, just view angles
#define DAMAGE_BULLET			0x00000010  // damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION	0x00000020  // armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD	300
#define DEFAULT_BULLET_VSPREAD	500
#define DEFAULT_SHOTGUN_HSPREAD	1000
#define DEFAULT_SHOTGUN_VSPREAD	500
#define DEFAULT_SSHOTGUN_COUNT	20

//
// g_monster.c
//
void monster_fire_bullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hspread, int vspread, int flashtype);
void monster_fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int flashtype);
edict_t *monster_fire_blaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype, int effect);
void M_SetEffects (edict_t *);

// RSP 121298 - Add diskgun support
void monster_fire_diskgun (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashtype);	// MRB 071299 - for battlebot

void M_droptofloor (edict_t *ent);
void monster_think (edict_t *self);
void walkmonster_start (edict_t *self);
void swimmonster_start (edict_t *self);
void flymonster_start (edict_t *self);
void AttackFinished (edict_t *self, float time);
void monster_death_use (edict_t *self);
void M_CatagorizePosition (edict_t *ent);
void M_FlyCheck (edict_t *self);
void M_CheckGround (edict_t *ent);
qboolean M_CheckAttack (edict_t *self);

//
// g_misc.c
//
void ThrowHead (edict_t *self, char *gibname, int damage, int type);
void ThrowClientHead (edict_t *self, int damage);
void ThrowGib (edict_t *self, char *gibname, int damage, int type);
void SpawnExplosion (edict_t *self,int explosion_type,int multicast_type,int sound);
void SpawnExplosion1(edict_t *self,int sound);	// RSP 062900
void SpawnExplosion2(edict_t *self,int sound);	// RSP 062299,062900

//
// g_ai.c
//
void AI_SetSightClient (void);

void ai_stand (edict_t *self, float dist);
void ai_move (edict_t *self, float dist);
void ai_walk (edict_t *self, float dist);
void ai_turn (edict_t *self, float dist);
void ai_run (edict_t *self, float dist);
void ai_charge (edict_t *self, float dist);
int range (edict_t *self, edict_t *other);
void FoundTarget (edict_t *self);
qboolean infront (edict_t *self, edict_t *other);
qboolean visible (edict_t *self, edict_t *other);
qboolean FacingIdeal(edict_t *self);
qboolean FindTarget (edict_t *self);

//
// g_weapon.c
//
void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin);
qboolean fire_hit (edict_t *self, vec3_t aim, int damage, int kick);
void fire_bullet (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int mod);
void fire_shotgun (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int kick, int hspread, int vspread, int count, int mod);
edict_t *fire_blaster (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect, qboolean hyper);	// RSP 081700 - now returns pointer to bolt edict

// RSP 121298 - Add support for Diskgun
void launch_disk (edict_t *self, vec3_t start, vec3_t aimdir, int damage, int speed, int effect);

//
// p_trail.c
//
void PlayerTrail_Init (void);
void PlayerTrail_Add (vec3_t spot);
void PlayerTrail_New (vec3_t spot);
edict_t *PlayerTrail_PickFirst (edict_t *self);
edict_t *PlayerTrail_PickNext (edict_t *self);
edict_t	*PlayerTrail_LastSpot (void);
edict_t *PlayerTrail_PickPrev(edict_t *spot);	// RSP 070600


//
// g_client.c
//
void respawn (edict_t *ent);
void BeginIntermission (edict_t *targ);
void PutClientInServer (edict_t *ent);
void InitClientPersistant (gclient_t *client);
void InitClientResp (gclient_t *client);
void InitBodyQue (void);
void ClientBeginServerFrame (edict_t *ent);

//
// g_player.c
//
void player_pain (edict_t *self, edict_t *other, float kick, int damage);
void player_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

//
// g_svcmds.c
//
void	ServerCommand (void);
qboolean SV_FilterPacket (char *from);	// 3.20

//
// p_view.c
//
void ClientEndServerFrame (edict_t *ent);

//
// p_hud.c
//
void MoveClientToIntermission (edict_t *client);
void G_SetStats (edict_t *ent);
void G_SetSpectatorStats (edict_t *ent);	// 3.20
void G_CheckChaseStats (edict_t *ent);	// 3.20
void ValidateSelectedItem (edict_t *ent);
void DeathmatchScoreboardMessage (edict_t *client, edict_t *killer);
void HelpMessage (edict_t *, char *);	// RSP 091600
void DisplayHelpMessage (edict_t *);	// RSP 091600

//
// g_pweapon.c
//
void PlayerNoise(edict_t *who, vec3_t where, int type);

//
// m_move.c
//
qboolean M_CheckBottom (edict_t *ent);
qboolean M_walkmove (edict_t *ent, float yaw, float dist);
void M_MoveToGoal (edict_t *ent, float dist);
qboolean M_ChangeYaw (edict_t *ent);	// RSP 012099 Need to know if successful or not
//void M_ChangeBBox(edict_t *ent);		// RSP 012099: Change Bounding Box for shark
qboolean SV_movestep (edict_t *ent, vec3_t move, qboolean relink,qboolean forceit); // RSP 022099
void SV_NewChaseDir (edict_t *,edict_t *,float);	// RSP 032299

//
// g_phys.c
//
void G_RunEntity (edict_t *ent);
int ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce);	// RSP 030599
int SV_FlyMove (edict_t *ent, float time, int mask);	// RSP 070800

//
// g_main.c
//
void SaveClientData (void);
void FetchClientEntData (edict_t *ent);

// 3.20
// g_chase.c
//
void UpdateChaseCam(edict_t *ent);
void ChaseNext(edict_t *ent);
void ChasePrev(edict_t *ent);
void GetChaseTarget(edict_t *ent);
// 3.20 end


//
// luc_hbot.c
//
qboolean CheckSpawnPoint(edict_t *);
void Dreadlock_recall(edict_t *, qboolean);	// RSP 092900 

//
// luc_g_bubbles.c
//
// AJA 19980720 - Made SpawnBubble() usable by all.
edict_t *SpawnBubble (vec3_t origin);

//
// luc_g_misc.c
//
// RSP 052699 - Generalize enemy targetting spot
void Pick_Client_Target(edict_t *,vec3_t);
void waypoint_check(edict_t *);
void SP_item_deco (edict_t *);

//
// luc_g_weapon.c
//
// MRB 19981210 - added weapon definitions in here
//
void launch_plasma (edict_t *, vec3_t, vec3_t , int , int , float, int);
qboolean IsHeadShot(edict_t *, vec3_t);				// RSP 070999
edict_t *spear_impale(edict_t *, qboolean);			// RSP 081500
void spear_throw_backwards(edict_t *);				// RSP 081500

// RSP 012099: Added declaration for new routine
//
// luc_m_gwhite.c
//
void gwhite_straightenout(edict_t *);
void gwhite_ungrab (edict_t *);		// RSP 030999

//
// luc_m_bot.c
//
void botUnlockFrom(edict_t *, edict_t *);	// RSP 030999
void workerbot_straightenout(edict_t *);	// RSP 072900

//
// luc_m_jelly.c
//
void jelly_touch(edict_t *, edict_t *, cplane_t *, csurface_t *);	// RSP 032699
void gwhite_checkhealth(edict_t *);									// RSP 032699

//
// luc_m_littleboy.c
//
void bomb_touch (edict_t *, edict_t *, cplane_t *, csurface_t *);	// RSP 052199
void bomb_seektarget(edict_t *);									// RSP 090100
void fire_bomb (edict_t *, vec3_t, vec3_t, int);	// RSP 050999 - damage values not passed
void spawn_beacon(edict_t *);						// RSP 051299 - to add player flashlight

//
// luc_m_bot_wbeam.c
//
void workerbeam_laser_off(edict_t *);								// RSP 090100

//
// luc_beetle.c
//
void AlignBeetle(edict_t *);	// RSP 090100
void Beetle_Stop(edict_t *);	// RSP 090100

//
// various indexing routines - RSP 092500
//
void monster_supervisor_indexing (edict_t *);
void monster_ambush_indexing (edict_t *);
void monster_battle_indexing (edict_t *);
void monster_bot_indexing (edict_t *);
void monster_fatman_indexing (edict_t *);
void monster_gbot_indexing (edict_t *);
void monster_gwhite_indexing (edict_t *);
void monster_jelly_indexing (edict_t *);
void monster_landjelly_indexing (edict_t *);
void monster_repeater_indexing (edict_t *);


//============================================================================

// RSP 032399 - Indices for the Monster hit point table hp_table[].
// Setting these values globally allows us to read them from a preset file so
// we can evaluate different settings.

typedef enum
{
	HP_LANDJELLY,
	HP_WATERJELLY,
	HP_GBOT,
	HP_COILBOT,
	HP_BEAMBOT,
	HP_CLAWBOT,
	HP_SHARK,
	HP_SUPERVISOR,
	HP_FATMAN,
	HP_REPEATER,
	HP_PROBEBOT,
	HP_TURRET,
	HP_LITTLEBOY,
	HP_AMBUSH,		// RSP 071299
	HP_BATTLE,		// RSP 071299
	HP_BEETLE,		// RSP 072400
	HP_NUMMONSTERS
} hitpoints_t;

//============================================================================

// RSP 032399 - Indices for the Weapon damage table wd_table[].
// Setting these values globally allows us to read them from a preset file so
// we can evaluate different settings.

typedef enum
{
	WD_KNIFE,
	WD_SPEARGUN,
	WD_DISKGUN,
	WD_PLASMAWAD,
	WD_DISKGUNBLAST,
	WD_LITTLEBOY,
	WD_LITTLEBOYBLAST,
	WD_ROCKET_SPEAR,
	WD_ROCKET_SPEAR_BLAST,
	WD_BOLTGUN,				// RSP 051799
	WD_MATRIX,				// RSP 051999
	WD_BATTLE_KICK,			// MRB 091699
	WD_AMBUSH_HIT,			// MRB 091699
	WD_RIFLE,				// RSP 091900
	WD_NUMWEAPONS
} weapon_damage_t;

//============================================================================


// client_t->anim_priority
#define	ANIM_BASIC		0	// stand / run
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define	ANIM_REVERSE	6	// 3.20

// client data that stays across multiple level loads
typedef struct
{
	char		userinfo[MAX_INFO_STRING];
	char		netname[16];
	int			hand;

	qboolean	connected;			// a loadgame will leave valid entities that
									// just don't have a connection yet

	// values saved and restored from edicts when changing levels
	int			health;
	int			max_health;

//	qboolean	powerArmorActive;		// 3.20
	int			savedFlags;		// 3.20

	int			selected_item;
	int			inventory[MAX_ITEMS];

	// ammo capacities
	int			max_bullets;
	int			max_grenades;
	int			max_spears;		// JBM 082998 Add support for spears and the speargun
	int			max_disks;		// RSP 121298 Add support for disks and the disk gun
	int			max_plasma;		// MRB 031599 Add support for plasmawad
	int			max_rocket_spears;// RSP 051399 Add support for rocket spears
	int			max_bolts;		// RSP 051799 Add support for bolts
	int			max_icepellets;	// WPO 111199 Add ice pellets
	int			max_rods;		// RSP 090100 Add teleport rods

	gitem_t		*weapon;
	gitem_t		*lastweapon;
//	gitem_t		*extratoy;		// RSP 092100 - not used

	int			power_cubes;	// used for tracking the cubes in coop games
	int			score;			// for calculating total unit score in coop games

	int			game_helpchanged;	// 3.20
	int			helpchanged;		// 3.20

	qboolean	spectator;			// client is a spectator (3.20)

	qboolean	scuba_active;		// RSP 032299: Moved from flags so it's persistant
//	edict_t		*currenttoy;		// RSP 092100 - not used
	qboolean	show_air_gauge;		// RSP 032299
	qboolean	flashlight_on;		// RSP 051299: false = off, true = on
	int			ammo_type;			// RSP 051399: for multiple ammo types w/same weapon
	float		bolt_heat;			// RSP 051799: track heating level for bolt gun
	// WPO 081099: helper bot
	int			dreadlock; 			// 1 = ON, 0 = OFF, -1 = not available
	int			dreadlock_health;
	edict_t		*Dreadlock;
} client_persistant_t;

// client data that stays across deathmatch respawns
typedef struct
{
	client_persistant_t	coop_respawn;	// what to set client->pers to on a respawn
	int			enterframe;			// level.framenum the client entered the game
	int			score;				// frags, etc
	vec3_t		cmd_angles;			// angles sent over in the last command
//	int			game_helpchanged;	// 3.20
//	int			helpchanged;		// 3.20

	qboolean	spectator;			// client is a spectator (3.20)
	int			rank;				// WPO 281099 Player rank
} client_respawn_t;

// this structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s
{
	// known to server
	player_state_t	ps;				// communicated by server to clients
	int				ping;

	// private to game
	client_persistant_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;	// for detecting out-of-pmove changes

	qboolean	showscores;			// set layout stat
	qboolean	showinventory;		// set layout stat
	qboolean	showhelp;
	qboolean	showhelpicon;

	int			ammo_index;

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	qboolean	weapon_thunk;

	gitem_t		*newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_parmor;		// damage absorbed by power armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation

	float		killer_yaw;			// when dead, look at killer

	weaponstate_t	weaponstate;
	vec3_t		kick_angles;		// weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
	float		fall_time, fall_value;		// for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			// aiming direction
	float		bobtime;			// so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;

	float		next_drown_time;
	int			old_waterlevel;
//	int			breather_sound;		// RSP 082400 - not used

	int			machinegun_shots;	// for weapon raising in Q2, for boltgun heat level in LUC

	// animation vars
	int			anim_end;
	int			anim_priority;
	qboolean	anim_duck;
	qboolean	anim_run;

	// powerup timers

	float		invincible_framenum;	// Used with Megahealth 5 second GOD mode
//	float		breather_framenum;		// RSP 082400 - not used
//	float		enviro_framenum;		// RSP 082400 - not used

	float		quad_framenum;
	float		superspeed_framenum;	// WPO 241099 super speed powerup
	float		PlayerSpeed;			// WPO 281099 super speed bug fix
	float		lavaboots_framenum;		// WPO 061199 lava boots powerup
	float		cloak_framenum;			// WPO 061199 cloak powerup
	float		vampire_framenum;		// WPO 141199 vampire powerup
	float		tri_framenum;			// RSP 082300 megahealth powerup - tridamage effect

	float		decoy_timeleft;			// WPO 141199 time left for decoy
	int			scenario_framenum;		// RSP 092500 used with endgame scenario
	int			capture_framenum;		// RSP 092900 used with endgame scenario
	int			implant_framenum;		// RSP 101400 for initial dreadlock implant

	qboolean	grenade_blew_up;
	float		grenade_time;
//	int			silencer_shots;			// RSP 083000 - no silencer in LUC
	int			weapon_sound;

	float		pickup_msg_time;

	float		flood_locktill;		// locked from talking (3.20)
	float		flood_when[10];		// when messages were said (3.20)
	int			flood_whenhead;		// head pointer for when said (3.20)

	float		respawn_time;		// can respawn when time > this

	edict_t		*chase_target;		// player we are chasing (3.20)
	qboolean	update_chase;		// need to update chase info?	(3.20)

	// AJA 19980310 - Scuba variables.
	// RSP 010399: scuba_air_supply is obsolete. air supply is now in the persistent inventory list
//	int			scuba_air_supply;		// in frames (300 == 30 seconds)
	int			scuba_next_breathe;		// time to make the next noise
	int			scuba_breathe_sound;	// inhale (0) / exhale (1)

	// AJA 19980720 - Time to spawn the last bubble while exhaling underwater.
	int			scuba_last_bubble;

	// AJA 19980314 - Show air gauge flag. This doesn't affect the client's
	// behaviour, so I figure it's better put here as a variable than a bit
	// in the flag variable.
//	qboolean	show_air_gauge;

	// RSP 052599: alpha value for matrix detonation
	float		matrix_alpha;

	// WPO 301099 Personal Teleport
	qboolean	teleport_stored;
	vec3_t		teleport_angles;
	vec3_t		teleport_origin;

	// RSP 091600 - help message timer
	float		help_timer;	// set this to the amount of time you want the message displayed
};


struct edict_s
{
	entity_state_t		s;
	struct gclient_s	*client;	// NULL if not a player
									// the server expects the first part
									// of gclient_s to be a player_state_t
									// but the rest of it is opaque

	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf
	
	int			num_clusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			// unused if num_clusters != -1
	int			areanum, areanum2;

	//================================

	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;


	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================
	int			movetype;
	int			flags;

	char		*model;
	float		freetime;			// sv.time when the object was freed
	
	//
	// only used locally in game, not by server
	//
	char		*message;
	char		*classname;
	int			spawnflags;

	float		timestamp;

	float		angle;			// set in map editor, -1 = up, -2 = down
	char		*target;
	char		*targetname;
	char		*killtarget;
	char		*team;
	char		*pathtarget;
	char		*deathtarget;
	char		*combattarget;
	edict_t		*target_ent;
	edict_t		*cell;			// RSP 100800 - for endgame

	float		speed, accel, decel;
	vec3_t		movedir;
	vec3_t		pos1, pos2, pos3;	// RSP 072200 - added pos3 for turret use

	vec3_t		velocity;
	vec3_t		avelocity;
	int			mass;
	float		air_finished;
	float		gravity;		// per entity gravity multiplier (1.0 is normal)
								// use for lowgrav artifact, flares

	edict_t		*goalentity;
	edict_t		*movetarget;

	float		yaw_speed;
	float		ideal_yaw;

	// MRB 092998 - added back in these values for pitch aware entities.
	float		pitch_speed;
	float		ideal_pitch;

	float		nextthink;
	void		(*prethink) (edict_t *ent);
	void		(*think)(edict_t *self);
	void		(*blocked)(edict_t *self, edict_t *other);	//move to moveinfo?
	void		(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void		(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void		(*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void		(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

	float		touch_debounce_time;		// are all these legit?  do we need more/less of them?
	float		pain_debounce_time;
	float		damage_debounce_time;
	float		fly_sound_debounce_time;	//move to clientinfo
	float		last_move_time;

	int			health;
	int			max_health;
	int			gib_health;
	int			deadflag;
	qboolean	show_hostile;

	float		powerarmor_time;

	char		*map;			// target_changelevel

	int			viewheight;		// height above origin where eyesight is determined
	int			takedamage;
	int			dmg;
	int			radius_dmg;
	float		dmg_radius;
	int			sounds;			// make this a spawntemp var?
								// RSP 042599 - Also used for func_CD track
	int			count;			// various things, now including available air for scuba gear

	edict_t		*chain;
	edict_t		*enemy;
	edict_t		*oldenemy;
	edict_t		*activator;
	edict_t		*groundentity;
	int			groundentity_linkcount;
	edict_t		*teamchain;
	edict_t		*teammaster;

	edict_t		*mynoise;		// can go in client only
	edict_t		*mynoise2;

	int			noise_index;
	int			noise_index2;
	float		volume;
	float		attenuation;

	// timing variables
	float		wait;
	float		delay;			// before firing targets
	float		random;

	float		teleport_time;

	int			watertype;
	int			waterlevel;

	vec3_t		move_origin;
//	vec3_t		move_angles;	// RSP 100200 - not used

	// move this to clientinfo?
	int			light_level;

	int			style;			// also used as areaportal number and path_corner style

	gitem_t		*item;			// for bonus items

	// common data blocks
	moveinfo_t		moveinfo;
	monsterinfo_t	monsterinfo;

	// JBM 19980823 - Added the "radius" field for the supervisor bot. Defines it's "field of
	// influence" for bot detection and such.
	float		radius;

	// AJA 19980613 - Added the "name" and "z_offset" fields for
	// the misc_sprite entity. "z_offset" is also used to hold the (int) z
	// componenent of landing places for beetle jumps
	char		*name;
	int			z_offset;

	// MRB 19980901 - Added entity that this guy's reliant on for movement.
	edict_t		*locked_to;

	// JBM 19981216 - Added a struct for func_speaker_sequencer specific data
	tnt_sequencer_info	*sequencer_data;

	// RSP 032699 - Add this for quick check on shark turning direction
	int			turn_direction;

	// RSP 041099 - Add this for timing how long the shark is stuck in a corner
	// RSP 072500 - Also used to keep track of beetle model frames
	int			count2;

	// RSP 050199 - First waypoint in the attack and retreat paths
	char		*attack;
	char		*retreat;

	float		stopthink;	// When to stop a func_event_sequencer from thinking

	// RSP 061699 - For use with func_BoxCar.
	int			boxcar_index;
	char		*model2;
	char		*model3;	// MRB 071299 - battlebot gun model
//	char		*model4;

	// RSP 062099 - identity values
	int			identity;

	// RSP 072499 - current path_corner for func_rotrain
	edict_t		*curpath;

	// WPO 083199 - dreadlock
	edict_t		*dlockOwner;		// can't use owner field
	int			state;				// RSP 070700 - state of dreadlock; also used with other actions

	// WPO 171099 - decoy
	edict_t		*decoy;
	int			decoy_animation;
	float		freeze_framenum;		// WPO 241099 freeze gun

	// RSP 083100: track time for stored teleport gun victim
	edict_t		*teleportgun_victim;
	float		teleportgun_framenum;             
	float		tportding_framenum;		// RSP 090100 next frame to sound teleportgun storage sound

	int			scenario_state;			// RSP 092500 - for endgame scenario
	int			capture_state;			// RSP 092900 - for endgame scenario
	int			implant_state;			// RSP 101400 - for initial dreadlock implant
	int			endgame_flags;			// RSP 101500 - for endgame scenario
};

#include "luc_g_local.h"
