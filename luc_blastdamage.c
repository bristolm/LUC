#include "g_local.h"
#include "luc_blastdamage.h"

static int blastCounter = 0;	// used to limit number of concurrent 
                                // blast entities

// RSP 062600
// Changed code to properly check for whether the decal lays entirely
// against a flat surface at all corners of the model.


// see if damage will be totally on something solid
qboolean CheckSolid(edict_t *ent,float radius)
{
    vec3_t  c,p,offset;
    vec3_t  forward,right,up;

    VectorCopy(ent->s.origin,c);
    AngleVectors (ent->s.angles,forward,right,up);
    
    // Test the upper, lower, and side midlines
    // of the model, and test -1 along the normal. Those
    // points should be in SOLID.

    VectorSet(offset,-1,0,radius);  // back 1 and up radius
    R_ProjectSource(c,offset,forward,right,up,p);
    if (!(gi.pointcontents(p) & CONTENTS_SOLID))
        return false;

    VectorSet(offset,-1,radius,0);  // back 1 and right radius
    R_ProjectSource(c,offset,forward,right,up,p);
    if (!(gi.pointcontents(p) & CONTENTS_SOLID))
        return false;

    VectorSet(offset,-1,0,-radius); // back 1 and down radius
    R_ProjectSource(c,offset,forward,right,up,p);
    if (!(gi.pointcontents(p) & CONTENTS_SOLID))
        return false;

    VectorSet(offset,-1,-radius,0); // back 1 and left radius
    R_ProjectSource(c,offset,forward,right,up,p);
    if (!(gi.pointcontents(p) & CONTENTS_SOLID))
        return false;

    // this is a good decal
    return true;
}


void BlastThink(edict_t *ent)
{
	// get rid of the blast damage

	G_FreeEdict(ent);

    if (deathmatch->value)
		--blastCounter;
}


void SpawnBlastDamage(char *model,trace_t *tr,float radius)
{
	edict_t *blast;

    if (tr->ent->s.modelindex != 1)
		return;

    if (deathmatch->value)
		if (blastCounter == MAX_BLASTS)
			return;
        else
			blastCounter++;

    blast = G_Spawn();
    vectoangles(tr->plane.normal,blast->s.angles);
    VectorCopy(tr->endpos,blast->s.origin);

    // If the model can't fit the surface correctly, don't display it.
	if (!(CheckSolid(blast,radius)))
	{
		G_FreeEdict(blast);
		if (deathmatch->value)
			blastCounter--;
	}

	VectorCopy(tr->endpos,blast->s.old_origin);
	VectorSet(blast->mins,-2,-2,-2);	// RSP 072000 - shrunk model
	VectorSet(blast->maxs,2,2,2);
	blast->s.renderfx = RF_TRANSLUCENT;
	blast->think = BlastThink;
	blast->nextthink = level.time + DAMAGE_DURATION;
	blast->s.modelindex = gi.modelindex(model);
	gi.linkentity(blast);
}
