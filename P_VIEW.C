
#include "g_local.h"
#include "m_player.h"

static	edict_t		*current_player;
static	gclient_t	*current_client;

static	vec3_t	forward, right, up;
float	xyspeed;

float	bobmove;
int		bobcycle;		// odd cycles are right foot going forward
float	bobfracsin;		// sin(bobfrac*M_PI)

extern char *LUC_single_statusbar;

/*
===============
SV_CalcRoll

===============
*/
float SV_CalcRoll (vec3_t angles, vec3_t velocity)
{
	float sign;
	float side;
	float value;
	
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);
	
	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;
	
	return side*sign;
}


/*
===============
P_DamageFeedback

Handles color blends and view kicks
===============
*/
void P_DamageFeedback (edict_t *player)
{
	gclient_t	*client;
	float	side;
	float	realcount, count, kick;
	vec3_t	v;
	int		r, l;
	static	vec3_t	power_color = {0.0, 1.0, 0.0};
	static	vec3_t	acolor = {1.0, 1.0, 1.0};
	static	vec3_t	bcolor = {1.0, 0.0, 0.0};

	client = player->client;

	client->ps.stats[STAT_FLASHES] = 0;

	// total points of damage shot at the player this frame
	count = (client->damage_blood + client->damage_armor + client->damage_parmor);
	if (count == 0)
		return;

	// flash the backgrounds behind the status numbers
	if (client->damage_blood)
		client->ps.stats[STAT_FLASHES] |= 1;

/* RSP 093000 - no armor in LUC
	if (client->damage_armor &&
		 (client->invincible_framenum <= level.framenum) &&
		 !(player->flags & FL_GODMODE))
	{
		client->ps.stats[STAT_FLASHES] |= 2;
	}
 */

	// start a pain animation if still in the player model
	if ((client->anim_priority < ANIM_PAIN) && (player->s.modelindex == 255))
	{
		static int i;

		client->anim_priority = ANIM_PAIN;
		if (client->ps.pmove.pm_flags & PMF_DUCKED)
		{
			player->s.frame = FRAME_crpain1-1;
			client->anim_end = FRAME_crpain4;
		}
		else
		{
			i = (i+1)%3;
			switch (i)
			{
			case 0:
				player->s.frame = FRAME_pain101-1;
				client->anim_end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201-1;
				client->anim_end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301-1;
				client->anim_end = FRAME_pain304;
				break;
			}
		}
	}

	realcount = count;
	if (count < 10)
		count = 10;	// always make a visible effect

	// play an appropriate pain sound
	if ((level.time > player->pain_debounce_time) &&
		 (client->invincible_framenum <= level.framenum) &&
		 !(player->flags & FL_GODMODE))
	{
		r = 1 + (rand()&1);
		player->pain_debounce_time = level.time + 0.7;
		if (player->health < 25)
			l = 25;
		else if (player->health < 50)
			l = 50;
		else if (player->health < 75)
			l = 75;
		else
			l = 100;
		gi.sound (player, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1, ATTN_NORM, 0);
	}

	// the total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0)
		client->damage_alpha = 0;
	client->damage_alpha += count*0.01;

	// RSP 032299: Cut blend saturation in half. It was getting tough to see.
	if (client->damage_alpha < 0.1)
		client->damage_alpha = 0.1;
	else if (client->damage_alpha > 0.3)
		client->damage_alpha = 0.3;		// don't go too saturated

	// the color of the blend will vary based on what's happened
	VectorClear (v);

	// RSP 061499 - if the FL_NOBLEND flag is set for this player, don't
	// add any color blend. For making screen shots that aren't foggy.

	if (!(player->flags & FL_NOBLEND))
	{
		if (client->damage_parmor)
			VectorMA (v, (float)client->damage_parmor/realcount, power_color, v);
		if (client->damage_armor)
			VectorMA (v, (float)client->damage_armor/realcount,  acolor, v);
		if (client->damage_blood)
			VectorMA (v, (float)client->damage_blood/realcount,  bcolor, v);
	}

	VectorCopy (v, client->damage_blend);

	//
	// calculate view angle kicks
	//
	kick = abs(client->damage_knockback);
	if (kick && (player->health > 0))	// kick of 0 means no view adjust at all
	{
		kick = kick * 100 / player->health;

		if (kick < count*0.5)
			kick = count*0.5;
		if (kick > 50)
			kick = 50;

		VectorSubtract (client->damage_from, player->s.origin, v);
		VectorNormalize (v);
		
		side = DotProduct (v, right);
		client->v_dmg_roll = kick*side*0.3;
		
		side = -DotProduct (v, forward);
		client->v_dmg_pitch = kick*side*0.3;

		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;
	client->damage_knockback = 0;
}




/*
===============
SV_CalcViewOffset

Auto pitching on slopes?

  fall from 128: 400 = 160000
  fall from 256: 580 = 336400
  fall from 384: 720 = 518400
  fall from 512: 800 = 640000
  fall from 640: 960 = 

  damage = deltavelocity*deltavelocity  * 0.0001

===============
*/
void SV_CalcViewOffset (edict_t *ent)
{
	float		*angles;
	float		bob;
	float		ratio;
	float		delta;
	vec3_t		v;

	// base angles
	angles = ent->client->ps.kick_angles;

	// if dead, fix the angle and don't add any kick
	if (ent->deadflag)
	{
		VectorClear (angles);

		ent->client->ps.viewangles[ROLL] = 40;
		ent->client->ps.viewangles[PITCH] = -15;
		ent->client->ps.viewangles[YAW] = ent->client->killer_yaw;
	}
	else
	{
		// add angles based on weapon kick
		VectorCopy (ent->client->kick_angles, angles);

		// add angles based on damage kick
		ratio = (ent->client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0)
		{
			ratio = 0;
			ent->client->v_dmg_pitch = 0;
			ent->client->v_dmg_roll = 0;
		}
		angles[PITCH] += ratio * ent->client->v_dmg_pitch;
		angles[ROLL] += ratio * ent->client->v_dmg_roll;

		// add pitch based on fall kick

		ratio = (ent->client->fall_time - level.time) / FALL_TIME;
		if (ratio < 0)
			ratio = 0;
		angles[PITCH] += ratio * ent->client->fall_value;

		// add angles based on velocity

		delta = DotProduct (ent->velocity, forward);
		angles[PITCH] += delta*run_pitch->value;
		
		delta = DotProduct (ent->velocity, right);
		angles[ROLL] += delta*run_roll->value;

		// add angles based on bob

		delta = bobfracsin * bob_pitch->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		angles[PITCH] += delta;
		delta = bobfracsin * bob_roll->value * xyspeed;
		if (ent->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6;		// crouching
		if (bobcycle & 1)
			delta = -delta;
		angles[ROLL] += delta;
	}

	// base origin

	VectorClear (v);

	// add view height

	v[2] += ent->viewheight;

	// add fall height

	ratio = (ent->client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0)
		ratio = 0;
	v[2] -= ratio * ent->client->fall_value * 0.4;

	// add bob height

	bob = bobfracsin * xyspeed * bob_up->value;
	if (bob > 6)
		bob = 6;
	//gi.DebugGraph (bob *2, 255);
	v[2] += bob;

	// add kick offset

	VectorAdd (v, ent->client->kick_origin, v);

	// absolutely bound offsets
	// so the view can never be outside the player box

	if (v[0] < -14)
		v[0] = -14;
	else if (v[0] > 14)
		v[0] = 14;
	if (v[1] < -14)
		v[1] = -14;
	else if (v[1] > 14)
		v[1] = 14;
	if (v[2] < -22)
		v[2] = -22;
	else if (v[2] > 30)
		v[2] = 30;

	VectorCopy (v, ent->client->ps.viewoffset);
}

/*
==============
SV_CalcGunOffset
==============
*/
void SV_CalcGunOffset (edict_t *ent)
{
	int		i;
	float	delta;

	// gun angles from bobbing
	ent->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005;
	ent->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01;
	if (bobcycle & 1)
	{
		ent->client->ps.gunangles[ROLL] = -ent->client->ps.gunangles[ROLL];
		ent->client->ps.gunangles[YAW] = -ent->client->ps.gunangles[YAW];
	}

	ent->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005;

	// gun angles from delta movement
	for (i = 0 ; i < 3 ; i++)
	{
		delta = ent->client->oldviewangles[i] - ent->client->ps.viewangles[i];
		if (delta > 180)
			delta -= 360;
		if (delta < -180)
			delta += 360;
		if (delta > 45)
			delta = 45;
		if (delta < -45)
			delta = -45;
		if (i == YAW)
			ent->client->ps.gunangles[ROLL] += 0.1*delta;
		ent->client->ps.gunangles[i] += 0.2 * delta;
	}

	// gun height
	VectorClear (ent->client->ps.gunoffset);
//	ent->ps->gunorigin[2] += bob;

	// gun_x / gun_y / gun_z are development tools
	for (i = 0 ; i < 3 ; i++)
	{
		ent->client->ps.gunoffset[i] += forward[i]*(gun_y->value);
		ent->client->ps.gunoffset[i] += right[i]*(gun_x->value);
		ent->client->ps.gunoffset[i] += up[i]*(-gun_z->value);
	}
}


/*
=============
SV_AddBlend
=============
*/
void SV_AddBlend (float r, float g, float b, float a, float *v_blend)
{
	float a2, a3;

	if (a <= 0)
		return;
	a2 = v_blend[3] + (1-v_blend[3])*a;	// new total alpha
	a3 = v_blend[3]/a2;		// fraction of color from old

	v_blend[0] = v_blend[0]*a3 + r*(1-a3);
	v_blend[1] = v_blend[1]*a3 + g*(1-a3);
	v_blend[2] = v_blend[2]*a3 + b*(1-a3);
	v_blend[3] = a2;
}


/*
=============
SV_CalcBlend
=============
*/
void SV_CalcBlend (edict_t *ent)
{
	int		contents;
	vec3_t	vieworg;
	int		remaining;
	float	alpha;	// RSP 100800
	int		state,fadeout_length,fadein_length,frame;	// RSP 101400

	ent->client->ps.blend[0] = ent->client->ps.blend[1] = 
		ent->client->ps.blend[2] = ent->client->ps.blend[3] = 0;

	// add for contents
	VectorAdd (ent->s.origin, ent->client->ps.viewoffset, vieworg);
	contents = gi.pointcontents (vieworg);
	if (contents & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER))
		ent->client->ps.rdflags |= RDF_UNDERWATER;
	else
		ent->client->ps.rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID|CONTENTS_LAVA))
		SV_AddBlend (1.0, 0.3, 0.0, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		SV_AddBlend (0.0, 0.1, 0.05, 0.6, ent->client->ps.blend);
	else if (contents & CONTENTS_WATER)
	{
//		SV_AddBlend (0.5, 0.3, 0.2, 0.4, ent->client->ps.blend);	// original settings
//		SV_AddBlend (0.5, 0.3, 0.2, 0.2, ent->client->ps.blend);	// RSP 051999 - cut alpha in half
//		SV_AddBlend (0.5, 0.3, 0.2, 0.3, ent->client->ps.blend);	// RSP 091500 - put some back
		// RSP 100800 - moved down to the capture_state section
	}

	if (ent->client->pers.scuba_active)	// RSP 032299
	{	// Check remaining air
		remaining = ent->client->pers.inventory[ITEM_INDEX(FindItem("Air"))]; // RSP 010399
		if (remaining == 50)	// 5 seconds of air left in tank
			gi.sound(ent, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1, ATTN_NORM, 0);

		// RSP 031799 Gray mask blend removed
//		if ((remaining > 50) || (remaining & 4))
//			SV_AddBlend (0.8, 0.8, 0.6, 0.04, ent->client->ps.blend);
	}
	else
	// WPO 241099 freeze gun
	if (ent->freeze_framenum > level.framenum)
	{
		remaining = ent->freeze_framenum - level.framenum;
		if ((remaining > 30) || (remaining & 4))
			SV_AddBlend (0, 0, 1, 0.08, ent->client->ps.blend);
	}
	else
	// WPO 231199 quad exists now in LUC
	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if ((remaining > 30) || (remaining & 4) )
			SV_AddBlend (0, 0, 1, 0.08, ent->client->ps.blend);
	}
	else
	// RSP 082300 - tri damage from megahealth
	if (ent->client->tri_framenum > level.framenum)
	{
		remaining = ent->client->tri_framenum - level.framenum;
		if ((remaining > 30) || (remaining & 4) )
			SV_AddBlend (0.3, 0.85, 0.06, 0.08, ent->client->ps.blend);
	}

	// add for damage
	if (ent->client->damage_alpha > 0)
		SV_AddBlend (ent->client->damage_blend[0],ent->client->damage_blend[1],
					 ent->client->damage_blend[2], ent->client->damage_alpha, ent->client->ps.blend);

	if (ent->client->bonus_alpha > 0)
		SV_AddBlend (0.85, 0.7, 0.3, ent->client->bonus_alpha, ent->client->ps.blend);

	// drop the damage value

	ent->client->damage_alpha -= 0.06;
	if (ent->client->damage_alpha < 0)
		ent->client->damage_alpha = 0;

	// drop the bonus value
	ent->client->bonus_alpha -= 0.1;
	if (ent->client->bonus_alpha < 0)
		ent->client->bonus_alpha = 0;

	// RSP 052599 - add the matrix alpha

	if (ent->client->matrix_alpha > 0)
		SV_AddBlend (0.3, ent->client->matrix_alpha, 0.06, ent->client->matrix_alpha, ent->client->ps.blend);

	ent->client->matrix_alpha -= 0.01;
	if (ent->client->matrix_alpha < 0)
		ent->client->matrix_alpha = 0;

	// RSP 101400 - determine specific values for blackout periods
	// These periods are mutually exclusive
	if (ent->implant_state)
	{
		state = ent->implant_state;
		fadeout_length = IMPLANT_FADEOUT_LENGTH;
		fadein_length = IMPLANT_FADEIN_LENGTH;
		frame = ent->client->implant_framenum;
	}
	else if (ent->scenario_state)
	{
		state = ent->scenario_state;
		fadeout_length = SCENARIO_FADEOUT_LENGTH;
		fadein_length = SCENARIO_FADEIN_LENGTH;
		frame = ent->client->scenario_framenum;
	}
	else if (ent->capture_state)
	{
		state = ent->capture_state;
		fadeout_length = CAPTURE_FADEOUT_LENGTH;
		fadein_length = CAPTURE_FADEIN_LENGTH;
		frame = ent->client->capture_framenum;
	}
	else
		state = 0;

	// Now handle the blackouts
	switch (state)
	{
	case 1:	// fading out
		if (frame > level.framenum)
		{
			alpha = 1.0 - ((float)(frame - level.framenum))/fadeout_length;
			SV_AddBlend (0,0,0,alpha,ent->client->ps.blend);
			if (contents & CONTENTS_WATER)
				SV_AddBlend (0.5, 0.3, 0.2, 0.3*(1.0 - alpha), ent->client->ps.blend);	// RSP 091500 - put some back
		}
		else
			SV_AddBlend (0,0,0,1,ent->client->ps.blend);
		break;
	case 2:	// blacked out
		SV_AddBlend (0,0,0,1,ent->client->ps.blend);
		break;
	case 3:	// fading in
		if (frame > level.framenum)
			SV_AddBlend (0,0,0,((float)(frame - level.framenum))/fadein_length,ent->client->ps.blend);
		break;
	default:
		if (contents & CONTENTS_WATER)
			SV_AddBlend (0.5, 0.3, 0.2, 0.3, ent->client->ps.blend);
		break;
	}
}


/*
=================
P_FallingDamage
=================
*/
void P_FallingDamage (edict_t *ent)
{
	float	delta;
	int		damage;
	vec3_t	dir;

	if (ent->s.modelindex != 255)
		return;		// not in the player model

	if (ent->movetype == MOVETYPE_NOCLIP)
		return;

	if ((ent->client->oldvelocity[2] < 0) && (ent->velocity[2] > ent->client->oldvelocity[2]) && (!ent->groundentity))
	{
		delta = ent->client->oldvelocity[2];
	}
	else
	{
		if (!ent->groundentity)
			return;
		delta = ent->velocity[2] - ent->client->oldvelocity[2];
	}
	delta = delta*delta * 0.0001;

	// never take falling damage if completely underwater
	if (ent->waterlevel == WL_HEADWET)
		return;
	if (ent->waterlevel == WL_WAISTWET)
		delta *= 0.25;
	else if (ent->waterlevel == WL_FEETWET)
		delta *= 0.5;

	if (delta < 1)
		return;

	if (delta < 15)
	{
		ent->s.event = EV_FOOTSTEP;
		return;
	}

	ent->client->fall_value = delta*0.5;
	if (ent->client->fall_value > 40)
		ent->client->fall_value = 40;
	ent->client->fall_time = level.time + FALL_TIME;

	if (delta > 30)
	{
		if (ent->health > 0)
		{
			if (delta >= 55)
				ent->s.event = EV_FALLFAR;
			else
				ent->s.event = EV_FALL;
		}
		ent->pain_debounce_time = level.time;	// no normal pain sound
		damage = (delta-30)/2;
		if (damage < 1)
			damage = 1;
		VectorSet (dir, 0, 0, 1);

		if (!deathmatch->value || !((int)dmflags->value & DF_NO_FALLING) )
			T_Damage (ent, world, world, dir, ent->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
	}
	else
	{
		ent->s.event = EV_FALLSHORT;
		return;
	}
}



/*
=============
P_WorldEffects
=============
*/
void P_WorldEffects (void)
{
//	qboolean	breather;	// RSP 082400 - not used
//	qboolean	envirosuit;	// RSP 082400 - not used

	qboolean	scuba;		// RSP 032299
	int			waterlevel, old_waterlevel;
	vec3_t		bubble_origin;
	edict_t		*new_bubble;

	if (current_player->movetype == MOVETYPE_NOCLIP)
	{
		current_player->air_finished = level.time + 12;	// don't need air
		return;
	}

	waterlevel = current_player->waterlevel;
	old_waterlevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterlevel;

//	breather = current_client->breather_framenum > level.framenum;	// RSP 082400 - not used
//	envirosuit = current_client->enviro_framenum > level.framenum;	// RSP 082400 - not used
//	scuba = current_player->flags & FL_SCUBA_GEAR;

	scuba =	current_player->client->pers.scuba_active;	// RSP 032299

	// If we are using the scuba gear, decrease our air supply regardless of the water level.
	if (scuba)
		current_client->pers.inventory[ITEM_INDEX(FindItem("Air"))]--; // RSP 010399

	//
	// if just entered a water volume, play a sound
	//
	if (!old_waterlevel && waterlevel)
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		if (current_player->watertype & CONTENTS_LAVA)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_SLIME)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		else if (current_player->watertype & CONTENTS_WATER)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		current_player->flags |= FL_INWATER;

		// clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = level.time - 1;
	}

	//
	// if just completely exited a water volume, play a sound
	// RSP 100100 - don't play if captured in endgame
	if (old_waterlevel && !waterlevel	&&
		!current_player->capture_state	&&
		!current_player->scenario_state)	// RSP 102200 - for when trigger_scenario_end is underwater
	{
		PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
		current_player->flags &= ~FL_INWATER;
	}

	//
	// check for head just going under water
	//
	if ((old_waterlevel != WL_HEADWET) && (waterlevel == WL_HEADWET))
		gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1, ATTN_NORM, 0);

	//
	// check for head just coming out of water
	// RSP 100100 - don't play if captured in endgame
	if ((old_waterlevel == WL_HEADWET)	&&
		(waterlevel != WL_HEADWET)		&&
		!current_player->capture_state	&&
		!current_player->scenario_state)	// RSP 102200 - for when trigger_scenario_end is underwater
	{
		if (current_player->air_finished < level.time)
		{	// if player is not using scuba gear, gasp for air
			if (!scuba)
				gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		}
		else  if (current_player->air_finished < level.time + 11)
		{	// just break surface, but don't gulp if using scuba gear
			if (!scuba)
				gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1, ATTN_NORM, 0);
		}
	}

	// AJA 19980311 - Play "dry" scuba breathing sounds when the player's head is above
	// the water but he's still using the scuba gear.
	if ((waterlevel < WL_HEADWET) && scuba)
	{
		// AJA 19980312 - Check if we ran out of air (while not submerged)
		if (current_client->pers.inventory[ITEM_INDEX(FindItem("Air"))] <= 0) // RSP 010399
		{
			// disable the scuba gear
			current_player->client->pers.scuba_active = false;	// RSP 032299
//			current_player->flags &= ~FL_SCUBA_GEAR;
		}
		// do breathing
		else if (current_client->scuba_next_breathe <= level.framenum)
		{
			// set next breathe
			current_client->scuba_next_breathe = level.framenum + 25;

			if (!current_client->scuba_breathe_sound)
				gi.sound (current_player, CHAN_AUTO, gi.soundindex("items/scuba/d_breath1.wav"), 1, ATTN_NORM, 0);
			else
				gi.sound (current_player, CHAN_AUTO, gi.soundindex("items/scuba/d_breath2.wav"), 1, ATTN_NORM, 0);
			current_client->scuba_breathe_sound ^= 1;
			PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		}
	}

	//
	// check for drowning
	//
	if (waterlevel == WL_HEADWET)
	{
		// check scuba status
		if (scuba)
		{
			if (current_client->pers.inventory[ITEM_INDEX(FindItem("Air"))] <= 0) // RSP 010399
			{
				// give the player 5 seconds before starting to drown
				current_player->air_finished = level.time + 5;

				// disable the scuba gear from checking next time.
				current_player->client->pers.scuba_active = false;	// RSP 032299
//				current_player->flags &= ~FL_SCUBA_GEAR;
			}
			else
			{
				// let him live until the next frame when we check the air supply again.
				current_player->air_finished = level.time + 10;

				// do underwater breathing.
				if (current_client->scuba_next_breathe <= level.framenum)
				{
					// set next breathe
					current_client->scuba_next_breathe = level.framenum + 25;

					if (!current_client->scuba_breathe_sound)
						gi.sound (current_player, CHAN_AUTO, gi.soundindex("items/scuba/w_breath1.wav"), 1, ATTN_NORM, 0);
					else
						gi.sound (current_player, CHAN_AUTO, gi.soundindex("items/scuba/w_breath2.wav"), 1, ATTN_NORM, 0);
					PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);

					// AJA 19980720 - Release bubbles for 1 second if this was an exhale.
					if (current_client->scuba_breathe_sound)
						current_client->scuba_last_bubble = level.framenum + 9;

					// AJA 19980720 - (Moved down from above) Next breathe we'll do the
					// opposite (ie. exhale instead of inhale).
					current_client->scuba_breathe_sound ^= 1;
				}

				// AJA 19980720 - Spawn a bubble in the middle of the top plane
				// of the player's bounding box if time hasn't run out yet.
				if (level.framenum <= current_client->scuba_last_bubble)
				{
					VectorCopy(current_player->s.origin, bubble_origin);
					bubble_origin[2] += current_player->maxs[2];
					new_bubble = SpawnBubble(bubble_origin);
					// AJA 19981212 - Set the maximum lifetime of SCUBA-spawned
					// bubbles to 10 seconds (as opposed to the default 45).
					if (new_bubble)
						new_bubble->delay = level.time + 10.0;
				}
			}
		}
/* RSP 082400 - not used
		// breather or envirosuit give air
		else if (breather || envirosuit)
		{
			current_player->air_finished = level.time + 10;

			if (((int)(current_client->breather_framenum - level.framenum) % 25) == 0)
			{
				// WPO 042800 Removed sounds
//				if (!current_client->breather_sound)
//					gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1, ATTN_NORM, 0);
//				else
//					gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1, ATTN_NORM, 0);
				current_client->breather_sound ^= 1;
				PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
				//FIXME: release a bubble?
			}
		}
 */

		// if out of air, start drowning
		if (current_player->air_finished < level.time)
		{	// drown!
			// AJA 19980308 - Don't drown even if we've run out of air if the cvar
			// "nodrown" is set.
			if ((current_player->client->next_drown_time < level.time) &&
				(current_player->health > 0) &&
				!(current_player->flags & FL_NODROWN))
			{
				current_player->client->next_drown_time = level.time + 1;

				// take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"), 1, ATTN_NORM, 0);
				else if (rand()&1)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1, ATTN_NORM, 0);

				current_player->pain_debounce_time = level.time;

				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else
	{
		current_player->air_finished = level.time + 12;
		current_player->dmg = 2;
	}

	//
	// check for sizzle damage
	//
	if (waterlevel && (current_player->watertype & (CONTENTS_LAVA|CONTENTS_SLIME)))
	{
		if (current_player->watertype & CONTENTS_LAVA)
		{
			if ((current_player->health > 0) &&
				(current_client->invincible_framenum < level.framenum) &&
				!(current_player->flags & FL_GODMODE) &&					// RSP 083000 - no sound in God mode
				(current_player->pain_debounce_time <= level.time))
			{
				if (rand()&1)
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1, ATTN_NORM, 0);
				current_player->pain_debounce_time = level.time + 1;
			}

// RSP 082400 - no envirosuit in LUC
//			if (envirosuit)	// take 1/3 damage with envirosuit
//				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterlevel, 0, 0, MOD_LAVA);
//			else
				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 3*waterlevel, 0, 0, MOD_LAVA);
		}
		else if (current_player->watertype & CONTENTS_SLIME)
		{
// RSP 082400 - no envirosuit in LUC
//			if (!envirosuit)
//			{	// no damage from slime with envirosuit
				T_Damage (current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterlevel, 0, 0, MOD_SLIME);
//			}
		}
	}
}


/*
===============
G_SetClientEffects
===============
*/
void G_SetClientEffects (edict_t *ent)
{
//	int pa_type;
	int remaining;		// WPO 241099 put back in for freezegun

	ent->s.effects = 0;
	ent->s.renderfx = 0;

	if ((ent->health <= 0) || level.intermissiontime)
		return;

	// WPO 241099 show blue shell around freezegun victim
	if (ent->freeze_framenum > level.framenum)
	{
		remaining = ent->freeze_framenum - level.framenum;
		// WPO 231199 thawed player gets quad
		if (remaining == 1)
		{
			ent->client->quad_framenum = level.framenum + 300;
			ent->s.effects |= EF_QUAD;
			gi.sound(ent, CHAN_RELIABLE+CHAN_ITEM, gi.soundindex("items/quaddamage.wav"), 1, ATTN_NONE, 0);
		}
		else
		if ((remaining > 30) || (remaining & 4))
			ent->s.effects |= EF_QUAD;
	}
	// WPO 231199 thawed player gets quad
	else
	if (ent->client->quad_framenum > level.framenum)
	{
		remaining = ent->client->quad_framenum - level.framenum;
		if ((remaining > 30) || (remaining & 4))
			ent->s.effects |= EF_QUAD;
	}

	// show cheaters!!!
	if (ent->flags & FL_GODMODE)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	}
}


/*
===============
G_SetClientEvent
===============
*/
void G_SetClientEvent (edict_t *ent)
{
	if (ent->s.event)
		return;

	if (ent->groundentity && (xyspeed > 225))
		if ((int)(current_client->bobtime+bobmove) != bobcycle)
			ent->s.event = EV_FOOTSTEP;
}


/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound (edict_t *ent)
{
	char *weap;

	// 3.20
	if (ent->client->pers.game_helpchanged != game.helpchanged)
	{
		ent->client->pers.game_helpchanged = game.helpchanged;
		ent->client->pers.helpchanged = 1;
	}

	// help beep (no more than three times)
	if (ent->client->pers.helpchanged && (ent->client->pers.helpchanged <= 3) && !(level.framenum&63))
	{
		ent->client->pers.helpchanged++;
		gi.sound (ent, CHAN_VOICE, gi.soundindex ("misc/pc_up.wav"), 1, ATTN_STATIC, 0);
	}
	// 3.20 end


	if (ent->client->pers.weapon)
		weap = ent->client->pers.weapon->classname;
	else
		weap = "";

	if (ent->waterlevel && (ent->watertype & (CONTENTS_LAVA|CONTENTS_SLIME)))
		ent->s.sound = snd_fry;
	else if (strcmp(weap, "weapon_matrix") == 0)						// RSP 051999
		ent->s.sound = gi.soundindex("weapons/matrix/matrix_hum.wav");	// RSP 060700
	else if (ent->client->weapon_sound)
		ent->s.sound = ent->client->weapon_sound;
	else
		ent->s.sound = 0;

	// RSP 090100 - play teleportgun storage sound if there's a victim being held,
	// even if the teleportgun is not raised.
	if (ent->teleportgun_victim)
		if (level.framenum >= ent->tportding_framenum)
		{
			int delta;

			gi.sound(ent, CHAN_WEAPON, gi.soundindex("weapons/teleportgun/tport_ding.wav"), 1, ATTN_NORM, 0);

			// set frame for next time to play sound
			delta = (int)((ent->teleportgun_framenum - level.framenum)/10);
			if (delta < 2)
				delta = 2;	// no more than 5 per second
			ent->tportding_framenum = level.framenum + delta;
		}
}

/*
===============
G_SetClientFrame
===============
*/
void G_SetClientFrame (edict_t *ent)
{
	gclient_t	*client;
	qboolean	duck, run;

	if (ent->s.modelindex != 255)
		return;		// not in the player model

	client = ent->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;

	if (xyspeed)
		run = true;
	else
		run = false;

	// check for stand/duck and stop/go transitions
	if ((duck != client->anim_duck) && (client->anim_priority < ANIM_DEATH))
		goto newanim;
	if ((run != client->anim_run) && (client->anim_priority == ANIM_BASIC))
		goto newanim;
	if (!ent->groundentity && (client->anim_priority <= ANIM_WAVE))
		goto newanim;

	// 3.20
	if (client->anim_priority == ANIM_REVERSE)
	{
		if (ent->s.frame > client->anim_end)
		{
			ent->s.frame--;
			return;
		}
	}
	else if (ent->s.frame < client->anim_end)
	{	// continue an animation
		ent->s.frame++;
		return;
	}
	// 3.20 end

	if (client->anim_priority == ANIM_DEATH)
		return;		// stay there
	if (client->anim_priority == ANIM_JUMP)
	{
		if (!ent->groundentity)
			return;		// stay there
		ent->client->anim_priority = ANIM_WAVE;
		ent->s.frame = FRAME_jump3;
		ent->client->anim_end = FRAME_jump6;
		return;
	}

newanim:
	// return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!ent->groundentity)
	{
		client->anim_priority = ANIM_JUMP;
		if (ent->s.frame != FRAME_jump2)
			ent->s.frame = FRAME_jump1;
		client->anim_end = FRAME_jump2;
	}
	else if (run)
	{	// running
		if (duck)
		{
			ent->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else
		{
			ent->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else
	{	// standing
		if (duck)
		{
			ent->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else
		{
			ent->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}


#define MAX_FLASHLIGHT_DIST	128

void P_Flashlight(edict_t *self)
{
	vec3_t	forward,end,v;
	trace_t	tr;
	float	len;

	// Move the temporary light entity if flashlight is on

	if (self->client->pers.flashlight_on)
		if (self->target_ent)
		{
			AngleVectors (self->client->v_angle, forward, NULL, NULL);	// forward direction
			VectorNormalize(forward);
			VectorMA(self->s.origin,8000,forward,end);		// look for something to shine on
			tr = gi.trace (self->s.origin,NULL,NULL,end,self,MASK_SOLID);
			VectorSubtract(self->s.origin, tr.endpos, v);	// find vector
			len = VectorLength(v);							// find distance
			if (len > MAX_FLASHLIGHT_DIST)					// too far away?
				len = MAX_FLASHLIGHT_DIST;
			VectorMA(self->s.origin,len,forward,end);		// determine entity location
			VectorCopy(end,self->target_ent->s.origin);
			gi.linkentity(self->target_ent);
		}
}


/*
=================
ClientEndServerFrame

Called for each player at the end of the server frame
and right after spawning
=================
*/
void ClientEndServerFrame (edict_t *ent)
{
	float	bobtime;
	int		i;

	current_player = ent;
	current_client = ent->client;

	//
	// If the origin or velocity have changed since ClientThink(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.
	// 
	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	//
	for (i = 0 ; i < 3 ; i++)
	{
		current_client->ps.pmove.origin[i] = ent->s.origin[i]*8.0;
		current_client->ps.pmove.velocity[i] = ent->velocity[i]*8.0;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if (level.intermissiontime)
	{
		// FIXME: add view drifting here?
		current_client->ps.blend[3] = 0;
		current_client->ps.fov = 90;
		G_SetStats (ent);
		return;
	}

	AngleVectors (ent->client->v_angle, forward, right, up);

	// burn from lava, etc
	P_WorldEffects ();

	//
	// set model angles from view angles so other things in
	// the world can tell which direction you are looking
	//
	if (ent->client->v_angle[PITCH] > 180)
		ent->s.angles[PITCH] = (-360 + ent->client->v_angle[PITCH])/3;
	else
		ent->s.angles[PITCH] = ent->client->v_angle[PITCH]/3;
	ent->s.angles[YAW] = ent->client->v_angle[YAW];

	ent->s.angles[ROLL] = 0;
	ent->s.angles[ROLL] = SV_CalcRoll (ent->s.angles, ent->velocity)*4;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	xyspeed = sqrt(ent->velocity[0]*ent->velocity[0] + ent->velocity[1]*ent->velocity[1]);

	if (xyspeed < 5)
	{
		bobmove = 0;
		current_client->bobtime = 0;	// start at beginning of cycle again
	}
	else if (ent->groundentity)
	{	// so bobbing only cycles when on ground
		if (xyspeed > 210)
			bobmove = 0.25;
		else if (xyspeed > 100)
			bobmove = 0.125;
		else
			bobmove = 0.0625;
	}
	
	bobtime = (current_client->bobtime += bobmove);

	if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
		bobtime *= 4;

	bobcycle = (int)bobtime;
	bobfracsin = fabs(sin(bobtime*M_PI));

	// detect hitting the floor
	P_FallingDamage (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// determine the view offsets
	SV_CalcViewOffset (ent);

	// determine the gun offsets
	SV_CalcGunOffset (ent);

	// RSP 051299 - add flashlight if on
	P_Flashlight(ent);

	// RSP 032299: Display coordinates if FL_PRINT_LOCATION is set
	if (ent->flags & FL_PRINT_LOCATION)
		gi.dprintf("[%s]\n",vtos(ent->s.origin));

	// determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	// FIXME: with client prediction, the contents
	// should be determined by the client
	SV_CalcBlend (ent);

	// chase cam stuff (3.20)
	if (ent->client->resp.spectator)
		G_SetSpectatorStats(ent);
	else
		G_SetStats (ent);
	G_CheckChaseStats(ent);

	G_SetClientEvent (ent);

	G_SetClientEffects (ent);

	G_SetClientSound (ent);

	G_SetClientFrame (ent);

	VectorCopy (ent->velocity, ent->client->oldvelocity);
	VectorCopy (ent->client->ps.viewangles, ent->client->oldviewangles);

	// clear weapon kicks
	VectorClear (ent->client->kick_origin);
	VectorClear (ent->client->kick_angles);

	// if the scoreboard is up, update it
	if (ent->client->showscores && !(level.framenum & 31))
	{
		DeathmatchScoreboardMessage (ent, ent->enemy);
		gi.unicast (ent, false);
	}

	// RSP 091600 - Show help message if there's one
	if (ent->client->showhelp)
	{
		ent->client->help_timer -= FRAMETIME;
		if (ent->client->help_timer <= 0)
			ent->client->showhelp = false;
		else
			DisplayHelpMessage (ent);
	}
}

