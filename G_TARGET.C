#include "g_local.h"
#include "luc_hbot.h"		// RSP 070700

/*QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)

Fire an origin-based temp entity event to the clients.

style: type byte
*/
void Use_Target_Tent (edict_t *ent, edict_t *other, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (ent->style);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);
}


void SP_target_temp_entity (edict_t *ent)
{
	ent->use = Use_Target_Tent;
}


//==========================================================

/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off reliable

noise: wav file to play
attenuation:
  -1 = none, send to whole level
   1 = normal fighting sounds
   2 = idle sound level
   3 = ambient sound level
volume:	0.0 to 1.0

Normal sounds play each time the target is used.  The reliable flag can be set for crucial voiceovers.

Looped sounds are always atten 3 / vol 1, and the use function toggles it on/off.
Multiple identical looping sounds will just increase volume without any speed cost.
*/
void Use_Target_Speaker (edict_t *ent, edict_t *other, edict_t *activator)
{
	int chan;

	if (ent->spawnflags & (SPAWNFLAG_TS_LOOPED_ON|SPAWNFLAG_TS_LOOPED_OFF))
	{	// looping sound toggles
		if (ent->s.sound)
			ent->s.sound = 0;	// turn it off
		else
			ent->s.sound = ent->noise_index;	// start it
	}
	else
	{	// normal sound
		if (ent->spawnflags & SPAWNFLAG_TS_RELIABLE)
			chan = CHAN_VOICE|CHAN_RELIABLE;
		else
			chan = CHAN_VOICE;
		// use a positioned_sound, because this entity won't normally be
		// sent to any clients because it is invisible
		gi.positioned_sound (ent->s.origin, ent, chan, ent->noise_index, ent->volume, ent->attenuation, 0);
	}
}


void SP_target_speaker (edict_t *ent)
{
	char buffer[MAX_QPATH];

	if (!st.noise)
	{
		gi.dprintf("target_speaker with no noise at %s\n", vtos(ent->s.origin));
		gi.dprintf("Using default sound\n");	// RSP 050499

		// Allow a default noise. This target_speaker may be ok with no noise
		// specified, for it may be the target of an SSM func_speaker_sequencer.
		strcpy (buffer,"lucworld/abmach14.wav");	// default noise
//		return;
	}
	else if (!strstr (st.noise, ".wav"))
		Com_sprintf (buffer, sizeof(buffer), "%s.wav", st.noise);
	else
		strncpy (buffer, st.noise, sizeof(buffer));

	ent->noise_index = gi.soundindex (buffer);

	if (!ent->volume)
		ent->volume = 1.0;

	if (!ent->attenuation)
		ent->attenuation = 1.0;
	else if (ent->attenuation == -1)	// use -1 so 0 defaults to 1
		ent->attenuation = 0;

	// check for prestarted looping sound
	if (ent->spawnflags & SPAWNFLAG_TS_LOOPED_ON)
		ent->s.sound = ent->noise_index;

	ent->use = Use_Target_Speaker;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity (ent);
}


//==========================================================

void Use_Target_Help (edict_t *ent, edict_t *other, edict_t *activator)
{
	if (ent->spawnflags & SPAWNFLAG_THELP_HELP1)
		strncpy (game.helpmessage1, ent->message, sizeof(game.helpmessage2)-1);
	else
		strncpy (game.helpmessage2, ent->message, sizeof(game.helpmessage1)-1);

	game.helpchanged++;
}

/*QUAKED target_help (1 0 1) (-16 -16 -24) (16 16 24) help1
When fired, the "message" key becomes the current personal
computer string, and the message light will be set on all
clients' status bars.
*/

void SP_target_help(edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	if (!ent->message)
	{
		gi.dprintf ("%s with no message at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}
	ent->use = Use_Target_Help;
}


//==========================================================

/*QUAKED target_secret (1 0 1) (-8 -8 -8) (8 8 8)

Counts a secret found.
These are single use targets.
*/

void use_target_secret (edict_t *ent, edict_t *other, edict_t *activator)
{
	gi.sound (ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

	level.found_secrets++;

	G_UseTargets (ent, activator);
	G_FreeEdict (ent);
}


void SP_target_secret (edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent->use = use_target_secret;
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent->noise_index = gi.soundindex (st.noise);
	ent->svflags = SVF_NOCLIENT;
	level.total_secrets++;
}


//==========================================================

/*QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8)

Counts a goal completed.
These are single use targets.
*/
void use_target_goal (edict_t *ent, edict_t *other, edict_t *activator)
{
	gi.sound (ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

	level.found_goals++;

	if (level.found_goals == level.total_goals)
		gi.configstring (CS_CDTRACK, "0");

	G_UseTargets (ent, activator);
	G_FreeEdict (ent);
}


void SP_target_goal (edict_t *ent)
{
	if (deathmatch->value)
	{	// auto-remove for deathmatch
		G_FreeEdict (ent);
		return;
	}

	ent->use = use_target_goal;
	if (!st.noise)
		st.noise = "misc/secret.wav";
	ent->noise_index = gi.soundindex (st.noise);
	ent->svflags = SVF_NOCLIENT;
	level.total_goals++;
}


//==========================================================
/*QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)

Spawns an explosion temporary entity when used.

delay: wait this long before going off
dmg:   how much radius damage should be done, defaults to 0
*/

void target_explosion_explode (edict_t *self)
{
	float save;

	// RSP 062900 - use generic explosion routine
	SpawnExplosion(self,TE_EXPLOSION1,MULTICAST_PHS,EXPLODE_SMALL);

//	gi.WriteByte (svc_temp_entity);
//	gi.WriteByte (TE_EXPLOSION1);
//	gi.WritePosition (self->s.origin);
//	gi.multicast (self->s.origin, MULTICAST_PHS);

	T_RadiusDamage (self, self->activator, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE);

	save = self->delay;
	self->delay = 0;
	G_UseTargets (self, self->activator);
	self->delay = save;
}


void use_target_explosion (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;

	if (!self->delay)
	{
		target_explosion_explode (self);
		return;
	}

	self->think = target_explosion_explode;
	self->nextthink = level.time + self->delay;
}


void SP_target_explosion (edict_t *ent)
{
	ent->use = use_target_explosion;
	ent->svflags = SVF_NOCLIENT;
}


//==========================================================

/*QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8)

Changes level to "map" when fired.

"target" is an optional func_BoxCar, whose contents you take with you.
*/
void use_target_changelevel (edict_t *self, edict_t *other, edict_t *activator)
{
    edict_t     *boxcar;    // RSP 061699
    gclient_t   *client;    // RSP 070600
	int			i;			// RSP 070600

	if (level.intermissiontime)
		return;		// already activated

	if (!deathmatch->value && !coop->value)
	{
		if (g_edicts[1].health <= 0)
			return;
	}

	// if noexit, do a ton of damage to other
	if (deathmatch->value && !( (int)dmflags->value & DF_ALLOW_EXIT) && other != world)
	{
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 10 * other->max_health, 1000, 0, MOD_EXIT);
		return;
	}

	// if multiplayer, let everyone know who hit the exit
	if (deathmatch->value)
	{
		if (activator && activator->client)
			gi.bprintf (PRINT_HIGH, "%s exited the level.\n", activator->client->pers.netname);
	}

	// RSP 061699 - If "target" is filled in, it belongs to a func_BoxCar, so process it.
	if (self->target)
	{
		boxcar = G_Find (NULL, FOFS(targetname), self->target);
		if (boxcar)
		{
			if (Q_stricmp(boxcar->classname,"func_BoxCar") != 0)
				gi.dprintf("target_changelevel with invalid func_BoxCar target %s\n",boxcar->classname);
			else
				boxcar->use(boxcar,NULL,NULL);	// record all travelling entities
		}
		else
			gi.dprintf("No func_BoxCar with targetname %s\n",self->target);
	}

    // RSP 070600 - If there are dreadlocks about, remove them so they
    // don't appear if the player comes back here.
	for (i = 0 ; i < game.maxclients ; i++)
    {
        client = g_edicts[i+1].client;
        if (client->pers.Dreadlock)
            if (client->pers.Dreadlock->inuse)
            {
                gi.unlinkentity(client->pers.Dreadlock);
                client->pers.dreadlock = DLOCK_DOCKED; // has one, but inactive
            }
    }

	// if going to a new unit, clear cross triggers
	if (strstr(self->map, "*"))	
		game.serverflags &= ~(SFL_CROSS_TRIGGER_MASK);

	BeginIntermission (self);
}


void SP_target_changelevel (edict_t *ent)
{
	if (!ent->map)
	{
		gi.dprintf("target_changelevel with no map at %s\n", vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	ent->use = use_target_changelevel;
	ent->svflags = SVF_NOCLIENT;
}


//==========================================================

/*QUAKED target_splash (1 0 0) (-8 -8 -8) (8 8 8)
Creates a particle splash effect when used.

sounds:
  1) sparks
  2) blue water
  3) brown water
  4) slime
  5) lava
  6) blood

count: how many pixels in the splash
dmg:   if set, does a radius damage at this location when it splashes,
       which is useful for lava/sparks
*/

void use_target_splash (edict_t *self, edict_t *other, edict_t *activator)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPLASH);
	gi.WriteByte (self->count);
	gi.WritePosition (self->s.origin);
	gi.WriteDir (self->movedir);
	gi.WriteByte (self->sounds);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	if (self->dmg)
		T_RadiusDamage (self, activator, self->dmg, NULL, self->dmg+40, MOD_SPLASH);
}


void SP_target_splash (edict_t *self)
{
	self->use = use_target_splash;
	G_SetMovedir (self->s.angles, self->movedir);

	if (!self->count)
		self->count = 32;

	self->svflags = SVF_NOCLIENT;
}


//==========================================================

/*QUAKED target_spawner (1 0 0) (-8 -8 -8) (8 8 8)
Useful for spawning monsters and gibs.

Set target to the type of entity you want spawned.

For monsters:
	The monster will spawn facing the direction of "angle".
	Set pathtarget to the targetname the monster will have after spawning.

For gibs:
	Set "angle" if you want it moving.
	"speed" is how fast it should be moving, otherwise it
	will just be dropped
*/
void ED_CallSpawn (edict_t *ent);

void use_target_spawner (edict_t *self, edict_t *other, edict_t *activator)
{
	edict_t	*ent = G_Spawn();

	ent->classname = self->target;
	ent->targetname = self->pathtarget;			// RSP 103100 - for endgame
	VectorCopy (self->s.origin, ent->s.origin);
	VectorCopy (self->s.angles, ent->s.angles);
	ED_CallSpawn (ent);
	gi.unlinkentity (ent);
	KillBox (ent);
	gi.linkentity (ent);
	if (self->speed)
		VectorCopy (self->movedir, ent->velocity);
}


void SP_target_spawner (edict_t *self)
{
	self->use = use_target_spawner;
	self->svflags = SVF_NOCLIENT;
	if (self->speed)
	{
		G_SetMovedir (self->s.angles, self->movedir);
		VectorScale (self->movedir, self->speed, self->movedir);
	}
}

//==========================================================
// WPO 042800 Remove target_blaster reference
/*target_blaster (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
Fires a blaster bolt in the set direction when triggered.

dmg		default is 15
speed	default is 1000

void use_target_blaster (edict_t *self, edict_t *other, edict_t *activator)
{
	int effect;

	if (self->spawnflags & SPAWNFLAG_TB_NOEFFECTS)
		effect = 0;
	else if (self->spawnflags & SPAWNFLAG_TB_NOTRAIL)
		effect = EF_HYPERBLASTER;
	else
		effect = EF_BLASTER;

	fire_blaster (self, self->s.origin, self->movedir, self->dmg, self->speed, EF_BLASTER, MOD_TARGET_BLASTER);
	gi.sound (self, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);
}

void SP_target_blaster (edict_t *self)
{
	self->use = use_target_blaster;
	G_SetMovedir (self->s.angles, self->movedir);
	// WPO 042800 Remove target_blaster reference
//	self->noise_index = gi.soundindex ("weapons/laser2.wav");

	if (!self->dmg)
		self->dmg = 15;
	if (!self->speed)
		self->speed = 1000;

	self->svflags = SVF_NOCLIENT;
}
*/


//==========================================================

/*QUAKED target_crosslevel_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Once this trigger is touched/used, any trigger_crosslevel_target
with the same trigger number is automatically used when a level
is started within the same unit.  It is OK to check multiple triggers.
Message, delay, target, and killtarget also work.
*/
void trigger_crosslevel_trigger_use (edict_t *self, edict_t *other, edict_t *activator)
{
	game.serverflags |= self->spawnflags;
	G_FreeEdict (self);
}


void SP_target_crosslevel_trigger (edict_t *self)
{
	self->svflags = SVF_NOCLIENT;
	self->use = trigger_crosslevel_trigger_use;
}


/*QUAKED target_crosslevel_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
Triggered by a target_crosslevel_trigger elsewhere within a unit.
If multiple triggers are checked, all must be true.  Delay, target and
killtarget also work.

delay: delay before using targets if the trigger has been activated (default 1)
*/
void target_crosslevel_target_think (edict_t *self)
{
	if (self->spawnflags == (game.serverflags & SFL_CROSS_TRIGGER_MASK & self->spawnflags))
	{
		G_UseTargets (self, self);
		G_FreeEdict (self);
	}
}


void SP_target_crosslevel_target (edict_t *self)
{
	if (!self->delay)
		self->delay = 1;
	self->svflags = SVF_NOCLIENT;

	self->think = target_crosslevel_target_think;
	self->nextthink = level.time + self->delay;
}


//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT FLICKER_BOUND
When triggered, fires a laser.  You can either set
a target or a direction. If you want the laser to
move around, set pathtarget to the moving entity.

pathtarget: entity you'd like the laser's origin to follow
message: String defining the custom laser colors.  String format is:
  	 <palette start> <palette end> <flicker delta>
	Setting this message will SUPERIMPOSE its settings over the various RED GREEN ... color flags
dmg: the amount of damage to do
*/

/* MRB 071798 - added new color cycling options to this entity
   I'm using pos1 and pos2 (my favorite members!) to store the 'skin' colors.
        self->pos1 = [color1 start][color1 message pointer][color1 flicker delta]
        self->pos2 = [color2 start][color2 message pointer][color2 flicker delta]
	short inner_two : 8;
	short outer_two : 8;
	short inner_one : 8;
	short outer_one : 8;
	This structure helps deal with the skinnum values */

typedef struct skin_color {
	short inner_two : 8;
	short inner_one : 8;
	short outer_two : 8;
	short outer_one : 8;
} SKIN_COLOR;


/* function to handle the color string */
//static int target_laser_walk_string (edict_t *self, int cursor, float start, int *result)
static float target_laser_walk_string (edict_t *self, vec3_t v, int *result, qboolean OKtoClearFlicker)
{
	int		target = 0;
	int		cursor = (int) v[1];
	float	start = v[0];
	qboolean ClearCursor = false;

	if (cursor == 0)
		return 0;

	while (1)
	{
		if (self->message[cursor] == '_')			//"..?>>_<<?..."
			// future Break, check upcoming values and ignore
		{
			if (self->message[cursor + 2] == ' ')    //"..?>>_<<?"	// RSP 082300 - was "=", should be "=="
				ClearCursor = true;  // No further need to come in here!
			++cursor;
		}
		else if (self->message[cursor] == '*')		//"...?>>*<<?..."
		{
			if (OKtoClearFlicker)
				v[2] = 0;
			++cursor;
		}
		else if (self->message[cursor] == '-')		//"...X>>-<<?..."
		{	// In the middle of a gradient to another value
			if (self->message[cursor +1] >= 'a')	//"...X>>-<<x..."
				target = start + 26 + (self->message[cursor +1] - 'a');
			else									//"...X>>-<<X..."
				target = start + (self->message[cursor +1] - 'A');

			if (*result < target)
				++*result;
			else
				--*result;

			if (*result == target)
				cursor += 2;
			break;
		}
		else if (self->message[cursor] == ' ')			//"...X>> <<"
		{	// end of row - rewind
			while (self->message[cursor -1] != ' ' && self->message[cursor -1] != '_')
				cursor--;								//" >>?<<?..."
		}											//or  "_>>?<<?..."
		else
		{	// Absolute color 
			if (self->message[cursor] >= 'a')			//"...?>>x<<?..."
				*result = start + 26 + (self->message[cursor] - 'a');
			else										//"...?>>X<<?..."
				*result = start + (self->message[cursor] - 'A');
			++cursor;
			break;
		}
	}
	if (ClearCursor)
		return 0;
	else
		return (float)cursor;
}


/* MRB 071898 - assign values based on entered message
   Must be called to reset thing to the way they were at the outset
   if everything is to be restarted */

void target_laser_devalue_message(edict_t *self)
{
	int		i, iInner[3], iOuter[3];
	char	sInner[64], sOuter[64];

	if (!(self->message))
		return;
	
	for (i = 0 ; i < 3 ; i++)
		iInner[i] = iOuter[i] = 0;

	i = sscanf(self->message,"%d %s %d %d %s %d",
					&iInner[0], sInner, &iInner[2],
					&iOuter[0], sOuter, &iOuter[2]);
	if (i > 0)
	{	// Set Inner laser values
		if (strlen(sInner) == 1)
			iInner[1] = 0;
		else
			for (iInner[1] = 0; 
				 Q_strncasecmp(self->message + iInner[1], sInner, strlen(sInner));
				 iInner[1]++)
			{
				;
			}

		// Fwd flixker only until I fix the string walking function to work better
		if (iInner[2] < 0)
			iInner[2] = 2;

		VectorSet(self->pos1,iInner[0],iInner[1],iInner[2]);

		// Copy to second one for now
		VectorCopy (self->pos1, self->pos2);
	}

/* parse second set of values - add later maybe?
	if (i > 3)
	{	// Set Outer laser values
		if (strlen(sOuter) == 1)
			iOuter[1] = 0;
		else
			for (iOuter[1] = iInner[1] + strlen(sInner) + 1; 
				Q_strncasecmp(self->message + iOuter[1], sOuter, strlen(sOuter));
				iOuter[1]++)
			{
				;
			}
		VectorSet(self->pos2,iOuter[0],iOuter[1],iOuter[2]);
	}
*/
}


/* MRB 071898 - function to construct the 
   skinnum from the current state self->message. */
void target_laser_gen_skinnum (edict_t *self, qboolean flushvalues)
{
	SKIN_COLOR	*skinmask = (SKIN_COLOR *) &(self->s.skinnum);

	int		inner[2] = {skinmask->inner_one&0xff,skinmask->inner_two&0xff};
	int		outer[2] = {skinmask->outer_one&0xff,skinmask->outer_two&0xff};
	float	Cursor1 = 0, Cursor2 = 0;
	int		i;

	if ( !(flushvalues) &&				// not setting values
		  (self->pos1[1] == 0) &&		// no inner range
		  (self->pos2[1] == 0))			// no outer range
	{
		return;							// then don't bother!
	}

	if (flushvalues)
	{	// First time in or starting over, set up values at start
		inner[0] = self->pos1[0];
		outer[0] = self->pos2[0];
	}
	else
	{	// Advance values if needed
		if (self->pos1[1] != 0)
			self->pos1[1] = target_laser_walk_string(self, self->pos1, &inner[0], true);

		if (self->pos2[1] != 0)
			self->pos2[1] = target_laser_walk_string(self, self->pos2, &outer[0], true);
	}

	// Copy over second values
	inner[1] = inner[0];
	outer[1] = outer[0];

	// Set flicker values
	if (self->pos1[2])
		if (self->spawnflags & SPAWNFLAG_TLAS_FLICKER_BOUND)
		{	// BOUND by regular rules - these values must walk the string again
			for (i = (int)(self->pos1[2]) ; i >= 0 ; i--)
				Cursor1 = target_laser_walk_string(self, self->pos1, &inner[1], false);
		}
		else
			inner[1] += self->pos1[2];

	if (self->pos2[2])
		if (self->spawnflags & SPAWNFLAG_TLAS_FLICKER_BOUND)
		{	// BOUND by regular rules - these values must walk the string again
			for (i = (int)(self->pos2[2]) ; i >= 0 ; i--)
				Cursor2 = target_laser_walk_string(self, self->pos2, &outer[1], false);
		}
		else
			outer[1] += self->pos2[2];

	//The values only must exist in the 0-255 range
	if (inner[1] > 255)
		inner[1] = 255;
	else if (outer[1] < 0)
		inner[1] = 0;

	if (outer[1] > 255)
		outer[1] = 255;
	else if (outer[1] < 0)
		outer[1] = 0;

	// Store these babies
	skinmask->inner_one = inner[0];
	skinmask->inner_two = inner[1];
	skinmask->outer_one = outer[0];
	skinmask->outer_two = outer[1];
}


void target_laser_think (edict_t *self)
{
	edict_t	*ignore;
	vec3_t	start;
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;
	vec3_t	last_movedir;
	int		count;

	// JBM 073098 Need some vars
	vec3_t	neworigin;		// The center point of the func_train we're following

	if (self->spawnflags & 0x80000000)
		count = 8;
	else
		count = 4;

	//MRB 071598 - regen the skin number ...
	target_laser_gen_skinnum(self, false);

	// JBM 073098 Move the originating point of the laser if the movetarget is set
	// Incidentally, movetarget is set if you specify a direction for the beam... So don't if you
	// want it to move its base...
	if (self->movetarget)
	{
		VectorMA (self->movetarget->absmin, 0.5, self->movetarget->size, neworigin);
		VectorCopy(neworigin, self->s.origin);
	}

	if (self->enemy)
	{
		VectorCopy (self->movedir, last_movedir);
		VectorMA (self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
		if (!VectorCompare(self->movedir, last_movedir))
			self->spawnflags |= 0x80000000;
	}

	// RSP 032699 - ignore the func_train you moved your start spot to. It's a solid and
	// gi.trace() will otherwise find it and cause the laser to draw incorrectly.
//	ignore = self;
	ignore = self->movetarget;
	VectorCopy (self->s.origin, start);

//	VectorMA (start, 8000, self->movedir, end);
	_ExtendWithBounds(start, 8000, self->movedir, NULL, NULL, end);
	while (1)
	{
		tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);

		if (!tr.ent)
			break;

		// hurt it if we can
		if (tr.ent->takedamage /* && !(tr.ent->flags & FL_IMMUNE_LASER) */)	// RSP 092100
			T_Damage (tr.ent, self, self->activator, self->movedir, tr.endpos, vec3_origin, self->dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);

		// if we hit something that's not a monster or player or is immune to lasers, we're done
		if (!(tr.ent->svflags & SVF_MONSTER) && !tr.ent->client)
		{
			if (self->spawnflags & 0x80000000)
			{
				self->spawnflags &= ~0x80000000;
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_LASER_SPARKS);
				gi.WriteByte (count);
				gi.WritePosition (tr.endpos);
				gi.WriteDir (tr.plane.normal);
				gi.WriteByte (self->s.skinnum);
				gi.multicast (tr.endpos, MULTICAST_PVS);
			}
			break;
		}

		ignore = tr.ent;
		VectorCopy (tr.endpos, start);
	}

	VectorCopy (tr.endpos, self->s.old_origin);

	self->nextthink = level.time + FRAMETIME;
}


void target_laser_on (edict_t *self)
{
	if (!self->activator)
		self->activator = self;
	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
	self->nextthink = level.time + FRAMETIME;

	target_laser_think (self);

	//MRB 071898 - reset the color numbers ...
	target_laser_devalue_message(self);
	target_laser_gen_skinnum(self, true);
}


void target_laser_off (edict_t *self)
{
	self->spawnflags &= ~SPAWNFLAG_TLAS_START_ON;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
}


void target_laser_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->activator = activator;
	if (self->spawnflags & SPAWNFLAG_TLAS_START_ON)
		target_laser_off (self);
	else
		target_laser_on (self);
}


void target_laser_start (edict_t *self)
{
	edict_t *ent;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= (RF_BEAM|RF_TRANSLUCENT);
	self->s.modelindex = 1;			// must be non-zero

	// MRB 071798 - clear just to be safe 
	VectorClear(self->pos1);		// Inner color
	VectorClear(self->pos2);		// Outer color

	// JBM 080298 - If the pathtarget key is set, and self->movetarget has not been set yet, set it
	// self->movetarget will hold the func_train ent specified in the pathtarget key
	if (self->pathtarget && !self->movetarget)
		self->movetarget = G_Find (NULL, FOFS(targetname), self->pathtarget);

	// set the beam diameter
	if (self->spawnflags & SPAWNFLAG_TLAS_FAT)
		self->s.frame = 16;
	else
		self->s.frame = 4;

	// set the color
	// MRB 071798 - color is now set in a function above.  Now this one stores the start colors.
	// Apparently the skinnum is set up in 4 2byte couplets
	//    [00][00][00][00] = [Outer color1][Inner color1][Outer color2][Inner color2]

	if (self->spawnflags & SPAWNFLAG_TLAS_RED)
	{	//	self->s.skinnum = 0xf2f2f0f0;
		VectorSet(self->pos1, 242, 0, -2);
		VectorSet(self->pos2, 242, 0, -2);
	}
	else if (self->spawnflags & SPAWNFLAG_TLAS_GREEN)
	{	//	self->s.skinnum = 0xd0d1d2d3;
		VectorSet(self->pos2, 208, 0, 2);
		VectorSet(self->pos1, 209, 0, 2);
	}	
	else if (self->spawnflags & SPAWNFLAG_TLAS_BLUE)
	{	//	self->s.skinnum = 0xf3f3f1f1;
		VectorSet(self->pos2, 243, 0, -2);
		VectorSet(self->pos1, 243, 0, -2);
	}	
	else if (self->spawnflags & SPAWNFLAG_TLAS_YELLOW)
	{	//	self->s.skinnum = 0xdcdddedf;
		VectorSet(self->pos2, 220, 0, 2);
		VectorSet(self->pos1, 222, 0, 2);
	}	
	else if (self->spawnflags & SPAWNFLAG_TLAS_ORANGE)
	{	//	self->s.skinnum = 0xe0e1e2e3;
		VectorSet(self->pos2, 224, 0, 2);
		VectorSet(self->pos1, 225, 0, 2);
	}

	target_laser_devalue_message(self);
	target_laser_gen_skinnum (self, true);

	if (!self->enemy)
		if (self->target)
		{
			ent = G_Find (NULL, FOFS(targetname), self->target);
			if (!ent)
				gi.dprintf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
			self->enemy = ent;
		}
		else
			G_SetMovedir (self->s.angles, self->movedir);

	self->use = target_laser_use;
	self->think = target_laser_think;

	if (!self->dmg)
		self->dmg = 1;

	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);
	gi.linkentity (self);

	if (self->spawnflags & SPAWNFLAG_TLAS_START_ON)
		target_laser_on (self);
	else
		target_laser_off (self);
}


void SP_target_laser (edict_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

/*QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE

speed:   how many seconds the ramping will take
message: two letters - starting lightlevel and ending lightlevel
*/

void target_lightramp_think (edict_t *self)
{
	char style_setting[2];

	style_setting[0] = 'a' + self->movedir[0] + (level.time - self->timestamp) / FRAMETIME * self->movedir[2];
	style_setting[1] = 0;

	// self->enemy points to the light entity that this lightramp is controlling
	// Now set the style setting for all lights with that light's 'style' value

	gi.configstring (CS_LIGHTS+self->enemy->style, style_setting);

	if ((level.time - self->timestamp) < self->speed)
		self->nextthink = level.time + FRAMETIME;
	else if (self->spawnflags & SPAWNFLAG_TLR_TOGGLE)
	{
		char temp;

		temp = self->movedir[0];
		self->movedir[0] = self->movedir[1];
		self->movedir[1] = temp;
		self->movedir[2] *= -1;
	}
}


void target_lightramp_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self->enemy)
	{
		edict_t *e;

		// check all the targets
		e = NULL;
		while (1)
		{
			e = G_Find (e, FOFS(targetname), self->target);
			if (!e)
				break;
			if (strcmp(e->classname, "light") != 0)
			{
				gi.dprintf("%s at %s ", self->classname, vtos(self->s.origin));
				gi.dprintf("target %s (%s at %s) is not a light\n", self->target, e->classname, vtos(e->s.origin));
			}
			else
				self->enemy = e;
		}

		if (!self->enemy)
		{
			gi.dprintf("%s target %s not found at %s\n", self->classname, self->target, vtos(self->s.origin));
			G_FreeEdict (self);
			return;
		}
	}

	self->timestamp = level.time;
	target_lightramp_think (self);
}


void SP_target_lightramp (edict_t *self)
{
	if (!self->message					||
		(strlen(self->message) != 2)	||
		(self->message[0] < 'a') 		||
		(self->message[0] > 'z') 		||
		(self->message[1] < 'a') 		||
		(self->message[1] > 'z') 		||
		(self->message[0] == self->message[1]))
	{
		gi.dprintf("target_lightramp has bad ramp (%s) at %s\n", self->message, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname, vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	self->svflags |= SVF_NOCLIENT;
	self->use = target_lightramp_use;
	self->think = target_lightramp_think;

	self->movedir[0] = self->message[0] - 'a';
	self->movedir[1] = self->message[1] - 'a';
	self->movedir[2] = (self->movedir[1] - self->movedir[0]) / (self->speed / FRAMETIME);
}

//==========================================================

/*QUAKED target_earthquake (1 0 0) (-8 -8 -8) (8 8 8)
When triggered, this initiates a level-wide earthquake.
All players and monsters are affected.

speed: severity of the quake, default 200
count: duration of the quake, default 5
*/

void target_earthquake_think (edict_t *self)
{
	int		i;
	edict_t	*e;

	if (self->last_move_time < level.time)
	{
		gi.positioned_sound (self->s.origin, self, CHAN_AUTO, self->noise_index, 1.0, ATTN_NONE, 0);
		self->last_move_time = level.time + 0.5;
	}

	for (i = 1, e = g_edicts+i ; i < globals.num_edicts ; i++, e++)
	{
		if (!e->inuse)
			continue;
		if (!e->client)
			continue;
		if (!e->groundentity)
			continue;

		e->groundentity = NULL;
		e->velocity[0] += crandom()* 150;
		e->velocity[1] += crandom()* 150;
		e->velocity[2] = self->speed * (100.0 / e->mass);
	}

	if (level.time < self->timestamp)
		self->nextthink = level.time + FRAMETIME;
}


void target_earthquake_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->timestamp = level.time + self->count;
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}


void SP_target_earthquake (edict_t *self)
{
	if (!self->targetname)
		gi.dprintf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	if (!self->count)
		self->count = 5;

	if (!self->speed)
		self->speed = 200;

	self->svflags |= SVF_NOCLIENT;
	self->think = target_earthquake_think;
	self->use = target_earthquake_use;

	self->noise_index = gi.soundindex ("world/quake.wav");
}


/*
==============================================================================

target_clue

==============================================================================
*/

/*QUAKED target_clue (1 0 0) (-8 -8 -8) (8 8 8)
Prints a message to the screen when used by a Dreadlock.
Can not be used or triggered by anything else.

message: message to print
*/

void use_target_clue(edict_t *self,edict_t *other,edict_t *activator)
{
	HelpMessage(other,self->message);	// RSP 091600 - replaces gi.centerprintf
//	gi.centerprintf(other,"%s",self->message);
	G_FreeEdict(self);
}

void SP_target_clue(edict_t *self)
{
    self->use = use_target_clue;
	self->svflags |= SVF_NOCLIENT;
}

/*
==============================================================================

target_blinkout

==============================================================================
*/

// RSP 100800
/*QUAKED target_blinkout (1 0 0) (-8 -8 -8) (8 8 8)
When used, this makes its target blink on and off at an increasing rate, then
removes itself and the target.

delay: wait this long before starting blink sequence
map:   *.cin to play if the player loses scenario 1
*/

int switchit[] = {
	1,		// placeholder, not used
	1,9,	// FRAMETIMES invisible, FRAMETIMES visible
	1,4,
	1,4,
	1,3,
	1,2,
	1,2,
	1,2,
	1,2,
	1,1,
	1,1,
	1,1,
	1,1,
	1,1,
	1,1,
	1,1,
	1,1,
	1,1,
	2,1,
	2,1,
	2,1,
	2,1,
	3,1,
	4,1,
	4,1,
	9,1,
	0		// NULL end of table
};

void target_blinkout_think(edict_t *self)
{
	int i;

	self->flags ^= FL_ON;		// toggle the on/off flag
	if (self->flags & FL_ON)	// turned it on
		self->target_ent->svflags &= ~SVF_NOCLIENT;	// target becomes visible
	else
		self->target_ent->svflags |= SVF_NOCLIENT;	// target becomes invisible

	i = switchit[++(self->count)];	// next entry in switchit[] table
	if (i <= 1)						// target becomes translucent after a while
		self->target_ent->s.renderfx |= RF_TRANSLUCENT;
	else
		self->target_ent->s.renderfx &= ~RF_TRANSLUCENT;

	if (i == 0)	// hit the end of the switchit[] table?
	{
		G_FreeEdict(self->target_ent);	// remove target
		G_FreeEdict(self);				// remove self
	}
	else	// keep going
		self->nextthink = level.time + i*FRAMETIME;
}

void use_target_blinkout(edict_t *self,edict_t *other,edict_t *activator)
{
	self->target_ent = G_Find (NULL, FOFS(targetname), self->target);
	if (!self->target_ent || !self->target_ent->inuse)
	{	// player escaped
		G_FreeEdict(self);
		return;
	}
		
	if (Q_stricmp(self->target_ent->classname,"path_corner") == 0)
	{	// player loses. play the "map" *.cin
		BeginIntermission (self);
		return;
	}
	self->count = 0;					// index into switchit[] table
	self->flags |= FL_ON;				// target starts ON
	if (self->delay)
		self->nextthink = level.time + self->delay;
	else
		self->nextthink = level.time + FRAMETIME;
}

void SP_target_blinkout (edict_t *self)
{
    self->use = use_target_blinkout;
	self->think = target_blinkout_think;
	self->svflags |= SVF_NOCLIENT;
}




