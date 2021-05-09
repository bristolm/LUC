// LUC Powerup Header File

// functions
qboolean luc_powerup_pickup(edict_t *self, edict_t *other);
void luc_powerup_use (edict_t *ent, gitem_t *item);
void luc_powerup_drop (edict_t *ent, gitem_t *item);
void luc_powerup_drop_dying (edict_t *ent, gitem_t *item);
qboolean luc_ent_has_powerup(edict_t *ent, int type);
int luc_ent_has_a_powerup(edict_t *ent);
void luc_ent_used_powerup(edict_t *ent, int type);
qboolean luc_powerup_init(void);
void luc_drop_powerups(edict_t *ent);
void luc_powerups_drop_dying (edict_t *ent);

// controlling parameters
// RSP 110799 - lettered icons so we can see they're missing

#define		POWERUP_MSHIELD_ICON		"p_mshiel"
#define		POWERUP_MSHIELD_NAME		"Matrix Shield"

#define		POWERUP_LAVABOOTS_ICON		"Tile1"
//#define		POWERUP_LAVABOOTS_ICON		"p_lavabt"
#define		POWERUP_LAVABOOTS_NAME		"Lava Boots"

#define		POWERUP_DECOY_ICON			"Tile2"
//#define		POWERUP_DECOY_ICON			"p_decoy"
#define		POWERUP_DECOY_NAME			"Holo Decoy"

// WPO 231099 - Vampire 
#define		POWERUP_VAMPIRE_ICON		"Tile3"
//#define		POWERUP_VAMPIRE_ICON		"p_vampir"
#define		POWERUP_VAMPIRE_NAME		"Vampire"
#define		VAMPIRE_MAX_HEALTH			200       // max health possible

// WPO 301099 - Personal Teleporter
#define		POWERUP_TELEPORTER_ICON		"Tile4"
//#define		POWERUP_TELEPORTER_ICON		"p_tport"
#define		POWERUP_TELEPORTER_NAME		"Teleporter"

// WPO 061199 - Super Speed
#define		POWERUP_SUPERSPEED_ICON		"Tile5"
//#define		POWERUP_SUPERSPEED_ICON		"p_super"
#define		POWERUP_SUPERSPEED_NAME		"SuperSpeed"

// WPO 061199 - Cloak
#define		POWERUP_CLOAK_ICON			"Tile6"
//#define		POWERUP_CLOAK_ICON			"p_cloak"
#define		POWERUP_CLOAK_NAME			"Cloak"

// WPO 071199 - Space Warp
#define		POWERUP_WARP_ICON			"Tile7"
//#define		POWERUP_WARP_ICON			"p_swarp"
#define		POWERUP_WARP_NAME			"Warp"

// enumerations

#define		POWERUP_MATRIXSHIELD	0	// defense only
#define		POWERUP_LAVABOOTS		1
#define		POWERUP_DECOY			2
#define		POWERUP_WARP			3	// WPO 071199 - Space Warp
#define		POWERUP_TELEPORTER		4	// WPO 301099 - Personal Teleporter
#define		POWERUP_SUPERSPEED		5	// WPO 061199 - Super Speed
#define		POWERUP_CLOAK			6	// WPO 061199 - Cloak - instant use
#define		POWERUP_VAMPIRE			7	// WPO 231099 - Vampire - instant use

#define		POWERUP_COUNT			8

typedef struct _LUCPowerups
{
	int index;
	char *name;
	char *icon;
	int itemIndex;		// quake2's index into item array
} LUCPowerups;

extern LUCPowerups luc_powerups[];	// RSP 091300




