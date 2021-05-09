/* Generic LUC routines - call anywhere */

#include "g_local.h"

extern qboolean enemy_vis;	// RSP 050199
extern int enemy_range;		// RSP 050199

edict_t *_findlifeinradius (edict_t *requester, edict_t *from, vec3_t org, float rad, 
							qboolean want_clients, qboolean want_monsters, qboolean want_deadstuff,
							float *distance)
{	// altered findradius function
	vec3_t	eorg;
	int		j;

	if (!from)
		from = g_edicts;
	else
		from++;

	for ( ; from < &g_edicts[globals.num_edicts] ; from++)
	{
		if (!from->inuse)
			continue;
		if (from->solid == SOLID_NOT)
			continue;

		if (from->client)
		{
			if (!want_clients)
				continue;
			else
			{
				if (requester != NULL && (deathmatch->value || coop->value) && 
					((int)(dmflags->value) & DF_NO_FRIENDLY_FIRE) && 
					OnSameTeam (requester, from))
					continue;
			}
		}

		if (!want_monsters && (from->svflags & SVF_MONSTER)) 
			continue;

		if (!want_deadstuff && from->deadflag != DEAD_NO)
			continue;

		for (j = 0 ; j < 3 ; j++)
			eorg[j] = org[j] - (from->absmin[j] + (from->size[j] *0.5));

		if ((*distance = VectorLength(eorg)) > rad)
			continue;

		return from;
	}

	return NULL;
}

void _AngleTranslation(vec3_t start, vec3_t offset, vec3_t result, int OffsetDirMod )
{
	vec3_t sines, cosines;
	int i;

	// Find matrix components
	for (i = 0 ; i < 3 ; i++)
	{
		float angle;

		angle = ( (i == PITCH) ? -OffsetDirMod : OffsetDirMod) * offset[i] * (M_PI/180);

		sines[i] = sin(angle);
		cosines[i] = cos(angle);
	}

	// apply matrix calculations
	VectorCopy(start,result);
	for (i = 0 ; i < 3 ; i++)
	{
		float tmp;
		int j = (i + 2) % 3;

		tmp =		(result[i] * cosines[i]) - (result[j] * sines[i]);
		result[j] =	(result[j] * cosines[i]) + (result[i] * sines[i]);
		result[i] = tmp;
	}
}
	
void _shuffleAngles(vec3_t angles)
{

	// Make things a little more understandable (to me at least ...)
	if (angles[PITCH] > 180)
		angles[PITCH] -= 360;

	if (angles[YAW] > 180)
		angles[YAW] -= 360;
	// Now everything is between -180 and 180

	// Pitch > 90 is confusing ...
	if (angles[PITCH] > 90)
	{
		angles[PITCH] = 180 - angles[PITCH];
		angles[YAW] += 180;
		angles[ROLL] += 180;
	}
	else if (angles[PITCH] < -90)
	{	// as is PITCH < -90
		angles[PITCH] = -180 - angles[PITCH];
		angles[YAW] += 180;
		angles[ROLL] += 180;
	}
}

void _DrawLaser(vec3_t start, vec3_t end)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_LASER);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (start, MULTICAST_PHS);
}

/*

  Pass in an origin (ent->s.origin) and extents (ent->mins, ent->maxs)
  and this will surround it with a green laser cage

*/

void _LaserBoxTraceMe(edict_t *self)
{
	_LaserTraceBBOX(self->s.origin,self->mins,self->maxs);
	self->nextthink = level.time + FRAMETIME;
}

void _LaserTraceBBOX(vec3_t origin, vec3_t mins, vec3_t maxs)
{
#define _VectorFromArray(v,i)   (v[0] = origin[0] + (i%2			? mins[0]:maxs[0]),		\
								 v[1] = origin[1] + ((i-1)%4 < 2	? mins[1]:maxs[1]),		\
								 v[2] = origin[2] + (i < 5			? mins[2]:maxs[2])		)
	int beams[16] = {	1,2,6,5,
						2,4,8,6,
						4,3,7,8,
						3,1,5,7};
	int i; 
	vec3_t lone, ltwo, lthree, lfour;

	for (i = 0 ; i < 16 ; i += 4)
	{
		_VectorFromArray(lone,beams[i]);
		_VectorFromArray(ltwo,beams[i+1]);
		_VectorFromArray(lthree,beams[i+2]);
		_VectorFromArray(lfour,beams[i+3]);

		_DrawLaser(lone,ltwo);
		_DrawLaser(ltwo,lthree);
		_DrawLaser(lthree,lfour);
	}
}


//AJA 19980223 - Added this function based on M_ChangeYaw
//MRB 19980929 - Added this back in ... with some friends
/*
===============
M_ChangePitch_ToIdeal
   - This sets the entities pitch based on the direction of the 'ideal_pitch' value
===============
*/
void M_ChangePitch_ToIdeal (edict_t *ent)
{
	float	ideal;
	float	current;
	float	move;
	float	speed;
	float	new_pitch;
	
	current = anglemod(ent->s.angles[PITCH]);
	ideal = ent->ideal_pitch;

	if (current == ideal)
		return;

	move = ideal - current;
	speed = ent->pitch_speed;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}

	// Don't pitch too far
	new_pitch = anglemod(current + move);
	if (new_pitch > 45)
		new_pitch = 45;
	else if (new_pitch < -45)
		new_pitch = -45;
	
	ent->s.angles[PITCH] = new_pitch;
}


void _AlignDirVelocity(edict_t *self)
{
	vec3_t move_dir;

	//Get direction of travel
	vectoangles(self->velocity, move_dir);
	VectorCopy(move_dir,self->s.angles);
}

void _MoveWithLockedEnt(edict_t *self)
{	// NOTE:  Need 'pos1' to be set to the offset from self->locked_to's wrt its foreward dir
	//        Need 'pos2' to be a 
	vec3_t  fwd,right,v;
	edict_t *anchor = self->locked_to;       // RSP 062000

	if (anchor == NULL)
	{	// MRB 011298 - if enemy is lost ...
		self->prethink = NULL;
		return;
	}

	// RSP 062000 - only reset origin and facing dir if entity you're
	//              locked to is still around. W/o this check, spears
	//              released from a jelly will end up at [0,0,0].
	if (Q_stricmp(anchor->classname, "freed") == 0)
	{
		self->prethink  = NULL;
		self->locked_to = NULL;
		return;
	}

	// set origin
	AngleVectors(anchor->s.angles,fwd,right,NULL);
	G_ProjectSource (anchor->s.origin, self->pos1, fwd, right, self->s.origin);

	//set 'facing' direction
	G_ProjectSource (anchor->s.origin, self->pos2, fwd, right,v);
	VectorSubtract(v,self->s.origin,v);
	vectoangles(v,self->s.angles);
}

void _LockToEnt(edict_t *lockee,edict_t *anchor)
{
	lockee->locked_to = anchor;
	if (lockee->movetype != MOVETYPE_FLYMISSILE)
		lockee->movetype = MOVETYPE_FLY;

	if (anchor != world)
	{
		vec3_t offset,myfwd,fwd,right,up;

		AngleVectors(anchor->s.angles,fwd,right,up);
		VectorSubtract(lockee->s.origin,anchor->s.origin, offset);

		// Find positional offset of origin from 'lockedto' entity
		lockee->pos1[0] = DotProduct(fwd,offset);
		lockee->pos1[1] = DotProduct(right,offset);
		lockee->pos1[2] = DotProduct(up,offset);

		// Find angular offset from 'lockedto' entity's angles (ignore roll for now)
		AngleVectors(lockee->s.angles,myfwd,NULL,NULL);
		VectorMA(lockee->s.origin,64,myfwd,offset);
		VectorSubtract(offset,anchor->s.origin, offset);

		lockee->pos2[0] = DotProduct(fwd,offset);
		lockee->pos2[1] = DotProduct(right,offset);
		lockee->pos2[2] = DotProduct(up,offset);

		// Set prethink action for alignment
		lockee->prethink = _MoveWithLockedEnt;
	}
	else
		lockee->prethink = NULL;

	VectorClear(lockee->velocity);
	VectorClear(lockee->avelocity);
}

void _SetBaseAnglesFromPlaneNormal(vec3_t normal, edict_t *self)
{
	vec3_t move_fwd, move_ang;

	vectoangles(normal,move_fwd);
// NOTE that typical 'flat' has a normal of PITCH = 90, YAW = heading, ROLL = 0.

	move_ang[PITCH] = fmod(move_fwd[PITCH] + 90 + 180,360) - 180;
	move_ang[YAW] = move_fwd[YAW];
	move_ang[ROLL] = 0;

	// MAX positive roll is equal to the deviation of initial PITCH from flat (0)

	if (move_ang[PITCH] > 0)		// Pointing DOWN, leaning FWD
		self->angle = move_ang[PITCH];
	else if (move_ang[PITCH] < 0)	// Pointing DOWN, leaning BACK
		self->angle = move_ang[PITCH] - (-180);

	// Store starting Normal and direction
	VectorCopy(move_ang,self->pos1);	// Starting (base) facing angles
	VectorCopy(normal,self->pos2);		// Starting plane normal
}

/* RSP 072100 - not used
#define MAX_YAW_DELTA 20

qboolean _MassageAngles(vec3_t self_angles, vec3_t self_normal, vec3_t new_angles,
					 float base_yaw, float max_pitch_inc)
{
	vec3_t delta_angles;
	float angle_range;
	qboolean ret_val = true;

//--YAW----

	delta_angles[YAW] = fmod(new_angles[YAW] - self_angles[YAW] + 180,360) - 180;

	if (delta_angles[YAW] > MAX_YAW_DELTA)
	{
		delta_angles[YAW] = MAX_YAW_DELTA;
		ret_val = false;
	}
	else if (delta_angles[YAW] < -MAX_YAW_DELTA)
	{
		delta_angles[YAW] = -MAX_YAW_DELTA;
		ret_val = false;
	}

	self_angles[YAW] = anglemod(self_angles[YAW] + delta_angles[YAW]);
	
//--PITCH--
	// If any angle goes from like 0 to 359, we'll see a big change in angle.  Not good.  Try to be incremental ...
	delta_angles[PITCH] = fmod(new_angles[PITCH] - self_angles[PITCH] + 180,360) - 180;
	self_angles[PITCH] += delta_angles[PITCH];

//--ROLL---
	// This is the deviation in angle from the 'ideal' angle in radians.
	angle_range = (fmod(base_yaw - self_angles[YAW] + 180,360) - 180) * (M_PI*2 / 360); 
	new_angles[ROLL] = sin(angle_range) * max_pitch_inc;

	// If 'Upside down' things are a bit different (it's doing funny things if I'm above it )
	if (self_normal[2] < 0)  //<-- attached surface normal
		new_angles[ROLL] += 180;

	delta_angles[ROLL] = fmod(new_angles[ROLL] - self_angles[ROLL] + 180,360) - 180;
	self_angles[ROLL] += delta_angles[ROLL];

	return(ret_val);
}
 */

/*
	JBM 112209 - Added this for use in the speaker sequencer
	expandarray : Expand a string containing strings seperated by commas
		(ex. : "speaker1,speaker2,speaker3"

   DO NOT send this function a null string!!!
*/
char** expandarray(char *delimitedstring, int elementsize)
{	
	// JBM 010199 - Fixed a bug... Need to make room for the \0 character in *temp :-)
	// JBM 010199 - Converted the mallocs over to gi.TagMalloc for conformity
	char *p,*temp;  // useful pointers
	int i; // to count the elements of the array
	int currentelement;
	char **thearray; // The array we will return
	size_t jtest;

	// allocate enough space for elementsize pointers
	// thearray = malloc(sizeof(*thearray) * elementsize);
	// RSP 031799: Allocate one more entry to hold a delimiting NULL
	thearray = gi.TagMalloc(sizeof(*thearray) * (elementsize + 1), TAG_LEVEL);
	
	i = 0;
	currentelement = 0;
	
	p=delimitedstring;
	jtest = strlen(p);

	//temp = malloc(sizeof(*temp) * jtest + 1);
	temp = gi.TagMalloc(sizeof(*temp) * jtest + 1, TAG_LEVEL);

	temp[0] = '\0';
//	p=delimitedstring;      // RSP 041499 - redundant code; already done above
	while (*p)
	{
		if (*p == ',') 
		{
			temp[i] = '\0';

            // RSP 041499 - change strdup to tagged malloc
//          thearray[currentelement] = strdup(temp);
            thearray[currentelement] = gi.TagMalloc(strlen(temp) + 1, TAG_LEVEL);	// RSP 050599 - corrected string length
            strcpy(thearray[currentelement],temp);
			temp[0] = '\0';
			i = -1;
			currentelement++;
		}
		else
			temp[i] = *p;
		p++;
		i++;
	}
	
	// The loop exits before storing the last string, so do that now.
	temp[i] = '\0';

	// RSP 041499 - change strdup to tagged malloc
//	thearray[currentelement] = strdup(temp);
	thearray[currentelement] = gi.TagMalloc(strlen(temp) + 1, TAG_LEVEL);	// RSP 050599 - corrected string length
	strcpy(thearray[currentelement],temp);

	// RSP 031799: Now set the final NULL delimiter
	// This is used during savegames, and is the fix for the 1a/1b savegame problem drake found
	thearray[currentelement+1] = NULL;

	//free (temp);

	return thearray;
}

/*
	JBM 112209 - Added this for use in the speaker sequencer
	countcommas : counts the number of commas in a string
*/
int countcommas(char *delimitedstring)
{
	char *p;  // useful pointers
	int i,c; // to count the elements of the array
	int maxlength; // the longest string's length
	
	// AJA 19990118 - Return -1 if passed a NULL string.
	if (!delimitedstring) return -1;

	p=delimitedstring;
	i = 0;
	c = 0;
	maxlength = 0;
	while (*p)
	{
		if (*p == ',')
		{
			if (c > maxlength)
				maxlength = c;
			c = -1;
			i++;
		}
		p++;
		c++;
	}

	return i;
}

// clips a projection vector inside a box (or the world if extents are NULL)
//  assumes dist != 0 and start is a valid point

void _ExtendWithBounds(vec3_t start, float dist, vec3_t dir, vec3_t max, vec3_t min, vec3_t end)
{	// little under real MAX/MIN to avoid rounding over ...
	vec3_t MAX = { 4094,4094,4094 };
	vec3_t MIN = {-4094,-4094,-4094};
	int i;
	
	VectorMA (start, dist, dir, end);

	if (dist == 0)
		return;

	for (i = 0; i < 3; i++)
	{
		float f;
		float r = 1;

		if (end[i] > (f = (max ? max[i] : MAX[i])) )
		{
			r = (f - start[i]) / (end[i] - start[i]);
			if (r > 0 && r < 1)
				VectorMA (start, (dist *= r), dir, end);
		}
		else if (end[i] < (f = (min ? min[i] : MIN[i])) )
		{
			r = (f - start[i]) / (end[i] - start[i]);
			if (r > 0 && r < 1)
				VectorMA (start, (dist *= r), dir, end);
		}
	}
}

// RSP 050199 - Generalized Forget Player routine. Used when the monster needs to go
// do something else for a while, like retreat or seek out a jelly.
void Forget_Player(edict_t *self)
{
	self->enemy = self->goalentity = self->oldenemy = NULL;	// forget the player
	self->monsterinfo.aiflags &= ~AI_SOUND_TARGET;
	enemy_vis = false;
	enemy_range = RANGE_FAR;	// RSP 041099 - should stop shark far lunges
}


// RSP 050199 - See if this waypoint is on the given path of waypoints
qboolean WaypointOnPath(edict_t *candidate,edict_t *path)
{
	edict_t *e = path;
	edict_t *h;

	if (path == NULL)	// RSP 060599
		return false;

	if (candidate == path)
		return true;	// quick check

	while (e->target)
	{
		h = e;
		e = G_Find(NULL,FOFS(targetname),e->target);
		if (e == NULL)
		{
			gi.dprintf("Invalid target for %s at %s\n",h->classname,vtos(h->s.origin));
			return false;
		}
		if (candidate == e)
			return true;	// Found it
	}
	return false;	// No match
}


// RSP 042999 - Find the closest waypoint that belongs to you
edict_t *G_Find_Waypoint(edict_t *self,qboolean retreat)
{
	edict_t	*candidate = NULL;
	edict_t *best_candidate = NULL;
	float	shortest_distance = 1000000;	// shortest distance from self to candidate
	float	distance;
	vec3_t	v;
	edict_t *waypoint_start = NULL;	// RSP 060599

	if (retreat)
	{
		if (self->retreat)
			waypoint_start = G_Find(NULL,FOFS(targetname),self->retreat);
	}
	else
	{
		if (self->attack)
			waypoint_start = G_Find(NULL,FOFS(targetname),self->attack);
	}

	if (waypoint_start)
		while (true)
		{
			candidate = G_Find(candidate,FOFS(classname),"waypoint");
			if (!candidate)
				break;
			// See if this waypoint is on the path we're interested in (attack or retreat)
			if (WaypointOnPath(candidate,waypoint_start))
			{
				VectorSubtract(self->s.origin,candidate->s.origin,v);
				if ((distance = VectorLength(v)) < shortest_distance)
				{
					best_candidate = candidate;
					shortest_distance = distance;
				}
			}
		}

	return(best_candidate);
}

// Almost a copy of the infront() function - only this allws you to specify
// your range.
qboolean wellInfront (edict_t *self, edict_t *other, float fraction)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;
	
	AngleVectors (self->s.angles, forward, NULL, NULL);
	VectorSubtract (other->s.origin, self->s.origin, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);
	
	if (dot > fraction)
		return true;
	return false;
}


