#include "g_local.h"

// AJA 19981222 Forward decl of Use_Target_Speaker to get this file to compile.
// JBM 19990122 Moved this here with the funct_speaker_sequencer
void Use_Target_Speaker (edict_t *ent, edict_t *other, edict_t *activator);

// =================================================================================
// RSP 042599 - Added func_CD

/*QUAKED func_CD (1 0 0) (-8 -8 -8) (8 8 8)

To play tracks from CD:

You must have 'set cd_nocd "0"' in your config.cfg file.

"sounds"    Track number on CD to play.
            If "sounds" is 0, the CD is turned off.

To play sounds from the sounds/CD directory:

You must have 'set cd_source "1"' in your autoexec.cfg file.

"sounds"	sound file name. If you use numbers as file names, then
            you can alternate between using the CD or using files by
            changing the setting of your cd_source variable. There's
            no way to turn off a sound.
 */

void func_CD_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (cd_source->value == CD_SOURCE_CD)	// Use CD sounds
		gi.configstring(CS_CDTRACK,va("%i",self->sounds));
	else if (self->sounds)					// Use sounds stored in PAK
		gi.sound(self,CHAN_AUTO,self->sounds,1,ATTN_NONE,0);
}

void SP_func_CD (edict_t *ent)
{
	char buffer[MAX_QPATH];

	ent->use = func_CD_use;		// Gets called when triggered

	if (ent->sounds && (cd_source->value == CD_SOURCE_PAK))	// Take sounds from PAK?
	{
		Com_sprintf(buffer,sizeof(buffer),"CD/%i.wav",ent->sounds);
		ent->sounds = gi.soundindex(buffer);
	}

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity (ent);
}
// =================================================================================


/*QUAKED func_speaker_sequencer (1 0 0) (-8 -8 -8) (8 8 8) START_ON SINGLE_SPEAKER_MODE SSM_RELIABLE SSM_LOOP_NOISE SSM_OVERLAP

Single Speaker Mode (SSM)
--------------------------------------
A chain of sounds is played at a single speaker at specified time intervals.
If more then 1 speaker is used in SSM it will play the sequence at 1 speaker
then move to the next and play the sequence there. If you don't supply any
speakers in the speaker list, they will be spawned for you depending on the
SSM_OVERLAP spawnflag. If SSM_OVERLAP is set, 1 speaker will be
spawned for each sound specified.

"speakers"     Speaker to use
"times"         Time to wait before triggering the next sound
"noises"        Wav files to play
"attenuations"
   -1 = none, send to whole level
    1 = normal fighting sounds
    2 = idle sound level
    3 = ambient sound level
"volumes"       Volume 0.0 to 1.0
"random_style"
    1 = Random Speaker - Play a sequence at a random speaker
    2 = Random Sound   - Play a random sound in the sequence at the current
                                 speaker
    3 = Random Both	    - Play a random sound at a random speaker

The SSM_RELIABLE spawnflag can be set for crucial voiceovers.

If you don't specify a "times" value, then a default of "1" is assigned.
If you specify one value, then it will be used for every noise.
If you specify several values, then the number of these must match the
number of noises.

"Attenuations" and "volumes" values follow the same rules as "times". If
"attenuations" is missing, a default value of "3" is used. If "volumes"
is missing, a default value of "1" is used.

If you don't specify a list of speakers, one speaker is spawned for
you unless you have the SSM_OVERLAP spawnflag on in which case 1 speaker for
each sound will be spawned.

If multiple speakers are supplied the sequences will alternate between the
specified speakers. A list of speakers MUST be supplied to use this
method. If not, they will all play at the same speaker.

SSM_OVERLAP allows you to start playing a new sound while letting the old
sound finish on its own instead of being ended when the new sound starts.
Helps reduce snaps and pops that occur when using the sequencer.

SSM_LOOP_NOISE will set the speaker(s)' LOOPED_OFF spawnflag
that is/are used in the sequence. This allows the sound to loop until it's
time to trigger a new sound.

Random_Style 1 : Place multiple speakers by hand. It will then play the
sequence at a random speaker.

Random_Style 2 : Plays a random noise in the sequence at the current speaker.
When it plays as many sounds as there are in the sequence, it will switch
speakers. If only 1 speaker is being used it will switch to the same speaker
and start again.

Random_Style 3 : Place multiple speakers by hand. It will then play a random
sound in the sequence at a random speaker. If only 1 speaker is being used
the random speaker will always be this single speaker.

Multi Speaker Mode (MSM)
------------------------------------
A chain of sounds is played by triggering speakers at a specified time
interval. The func_speaker_sequencer is the controller.

"speakers"     List of speakers to activate 
"times"         Time to wait before triggering the next speaker
"random_style"  In MSM, any non 0 (zero) value will trigger a random speaker
                      in the list,

The SSM_RELIABLE spawnflag can be set for crucial voiceovers. The speaker
attributes are applied on the individual speaker to be triggered.

If you don't specify a "times" value, then a default of "1" is assigned.
If you specify one value, then it will be used for every speaker.
If you specify several values, then the number of these must match the
number of speakers.

Note on Speakers
--------------------------
If you want speakers to be spawned for you, you must leave the speakers
field empty, if you supply 1 speaker, you must supply ALL the speakers you
want to use.

When you supply a speaker or a list of speakers, ALL speakers with that
targetname will be triggered.

*/

// JBM 112298 - Finished Adding Func_Speaker_Sequencer
// RSP 050599 - Rearranged and corrected to match specification

void func_speaker_sequencer_think (edict_t *self)
{
	int current_speaker_index;	// index of current speaker in speaker list
	int current_noise_index;	// index of current noise in noise list (SSM only)
	char buffer[MAX_QPATH];		// a buffer for stuff
	int random_style = self->sequencer_data->random_style;
	char *eventname;

	edict_t *target_speaker,*previous_speaker;

	if (!(self->spawnflags & SPAWNFLAG_FSS_SSM))
	{
		// MSM
		// set the current speaker element

		if (random_style != 0)	// Use speakers randomly
			self->sequencer_data->currentspeaker = rand() % (self->sequencer_data->speakerselements + 1);

		current_speaker_index = self->sequencer_data->currentspeaker;

		// find the speaker's edict_t
		// RSP 080399 - changed to target _every_ edict that has this targetname
		target_speaker = NULL;
		eventname = self->sequencer_data->speakers[current_speaker_index];
		while (target_speaker = G_Find(target_speaker,FOFS(targetname),eventname))
			target_speaker->use(target_speaker,self,self);

		// If you're not using speakers randomly, select the next speaker element in the sequence

		if (random_style == 0)
			if (current_speaker_index == self->sequencer_data->speakerselements)
				self->sequencer_data->currentspeaker = 0;
			else
				self->sequencer_data->currentspeaker++;

		// Set the time the next speaker should be triggered
		// We might only have one times value. If that's the case, always use the one we were
		// given. If there is more than one, you can take a value from the correct index in the list.

		if (self->sequencer_data->times[1] == NULL)
			self->nextthink = level.time + atof(self->sequencer_data->times[0]);
		else
			self->nextthink = level.time + atof(self->sequencer_data->times[current_speaker_index]);
	}
	else
	{
		// SSM
		// Set the next speaker and noise based on random_style

		switch (random_style)
		{
		default:
		case 0: // no random_style, use speaker and noise selected the last time through
			break;
		case 1: // random speaker
			self->sequencer_data->currentspeaker = rand() % (self->sequencer_data->speakerselements + 1);
			break;
		case 2: // random sound
			self->sequencer_data->currentnoise = rand() % (self->sequencer_data->noiseselements + 1);
			break;
		case 3: // random sound and speaker
			self->sequencer_data->currentspeaker = rand() % (self->sequencer_data->speakerselements + 1);
			self->sequencer_data->currentnoise = rand() % (self->sequencer_data->noiseselements + 1);
			break;
		}
		
		current_speaker_index = self->sequencer_data->currentspeaker;
		current_noise_index = self->sequencer_data->currentnoise;

		// find the speaker's edict_t
		// RSP 080399 - changed to target _every_ edict that has this targetname
		target_speaker = NULL;
		eventname = self->sequencer_data->speakers[current_speaker_index];
		while (target_speaker = G_Find(target_speaker,FOFS(targetname),eventname))
		{
			// Turn off the previous speaker if overlap isn't specified
			// If this is the first time we're playing a noise, there's no previous speaker, indicated by a
			// value of -1.

			if (!(self->spawnflags & SPAWNFLAG_FSS_SSM_OVERLAP) && (self->sequencer_data->prev_speaker_index != -1))
			{
				previous_speaker = NULL;
				eventname = self->sequencer_data->speakers[self->sequencer_data->prev_speaker_index];
				while (previous_speaker = G_Find(previous_speaker,FOFS(targetname),eventname))
				{
					previous_speaker->s.sound = 0;	// turn off the speaker
					gi.unlinkentity (previous_speaker);
				}
			}

			// append .wav if needed

			if (!strstr(self->sequencer_data->noises[current_noise_index],".wav"))
				Com_sprintf(buffer,sizeof(buffer),"%s.wav", self->sequencer_data->noises[current_noise_index]);
			else
				strcpy(buffer,self->sequencer_data->noises[current_noise_index]);

			target_speaker->noise_index = gi.soundindex(buffer);
		
			// We might only have one volume value. If that's the case, always use the one we were
			// given. If there is more than one, you can take a value from the correct index in the list.

			if (self->sequencer_data->volumes[1] == NULL)
				target_speaker->volume = atof(self->sequencer_data->volumes[0]);
			else
				target_speaker->volume = atof(self->sequencer_data->volumes[current_noise_index]);

			// We might only have one attenuation value. If that's the case, always use the one we were
			// given. If there is more than one, you can take a value from the correct index in the list.

			if (self->sequencer_data->attenuations[1] == NULL)
				target_speaker->attenuation = atof(self->sequencer_data->attenuations[0]);
			else
				target_speaker->attenuation = atof(self->sequencer_data->attenuations[current_noise_index]);

			// must link the entity so we get areas and clusters so
			// the server can determine who to send updates to

			gi.linkentity (target_speaker);

			// Use the speaker

			target_speaker->use(target_speaker,self,self);
		}

		// Set the next time a sound should be triggered.
		// We might only have one times value. If that's the case, always use the one we were
		// given. If there is more than one, you can take a value from the correct index in the list.

		if (self->sequencer_data->times[1] == NULL)
			self->nextthink = level.time + atof(self->sequencer_data->times[0]);
		else
			self->nextthink = level.time + atof(self->sequencer_data->times[current_noise_index]);

		// Save the current speaker number so we can turn it off the next time through if we're
		// not allowing overlapped sounds.

		self->sequencer_data->prev_speaker_index = current_speaker_index;

		// Change the speaker index for the next time through based on the random_style.
		// Increment the noise index if not choosing noises randomly.

		switch (random_style)
		{
		default:
		case 0: // no random_style (speakers and noises used in sequence)
			if (current_noise_index == self->sequencer_data->noiseselements)
				self->sequencer_data->currentnoise = 0;
			else
				self->sequencer_data->currentnoise++;
			if (self->sequencer_data->currentspeaker == self->sequencer_data->speakerselements)
				self->sequencer_data->currentspeaker = 0;
			else
				self->sequencer_data->currentspeaker++;
			break;
		case 1: // random speaker  (noises used in sequence)
			if (current_noise_index == self->sequencer_data->noiseselements)
				self->sequencer_data->currentnoise = 0;
			else
				self->sequencer_data->currentnoise++;
			break;
		case 2: // random sound

			// Once we've played N noises at a speaker, where N is
			// the number of noise elements, we can move to another speaker. These noises are
			// being selected randomly. When noisecounter has reset to 0, then we can switch
			// speakers. Otherwise, leave the speaker index where it is.

			// Increment the count of the number of noises we've used on this speaker

			if (++self->sequencer_data->noisecounter > self->sequencer_data->noiseselements)
				{
				self->sequencer_data->noisecounter = 0;
				if (self->sequencer_data->currentspeaker == self->sequencer_data->speakerselements)
					self->sequencer_data->currentspeaker = 0;
				else
					self->sequencer_data->currentspeaker++;
				}
			break;
		case 3: // random sound and speaker. Do nothing. They'll be set randomly the next time through.
			break;
		}
	}
}

void func_speaker_sequencer_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// pass along the activator
	self->activator = activator;

	// if on, turn it off
	if (self->nextthink)
	{
		self->nextthink = 0;
		
		// Set the sequence back to the 0th element for when it starts again
//		self->dmg = 0;	// RSP 050799 - Huh? What's this for?
		return;
	}

	func_speaker_sequencer_think (self);
}

// JBM 112298 - Finished adding Func_Speaker_Sequencer
// TODO : Single Speaker Mode
void SP_func_speaker_sequencer (edict_t *ent)
{
	edict_t *spawnspeaker;
//	char	buffer[MAX_QPATH];	// RSP 042799 - not needed
	char	*defaultspeaker = "lucjbm";
	char	*defaultsound = "lucworld/abmach14.wav";	// RSP 042799 - abmach4 -> abmach14
	char	*newspkname;
	char	index_as_char[6];
	int		chars_to_append;
	int		current_length;
	int		i,j;
	int		time_count,noise_count,speaker_count,attenuation_count,volume_count;	// RSP 050699
	
	// seed the rnd function
	srand( (unsigned)time( NULL ) );	

	// RSP 031199
	// This used to allocate memory, fill in the structure data, and set a pointer
	// in the edict to point to that memory.
	//
	// Since we already have an array of structs to hold sequencer data, we'll just
	// change this pointer to point to one of the items in that array. The index of
	// the struct in that array will be the same as the index of this edict in its own
	// edict array. This uses up a lot of memory, but is the simplest solution at this
	// point. We can look at it again for phase 2.

	ent->sequencer_data = &g_tnt_seqs[ent - g_edicts];
	// End of allocation changes

	// Check to see if we have times... If not, use a default
	if (!st.times)
	{
		gi.dprintf("func_speaker_sequencer with no times set at %s [%s] using default time of 1\n",
			vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
		st.times = "1";
	}

	//If we are in single speaker mode we have a few more checks/defaults to make/use...
	if (ent->spawnflags & SPAWNFLAG_FSS_SSM)
	{
		// Check for noises we need that!
		if (!st.noises)
		{
			gi.dprintf("func_speaker_sequencer with no noises set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}

		// Carve up the noises.
		noise_count = countcommas(st.noises);				// RSP 050699
		ent->sequencer_data->noiseselements = noise_count;	// RSP 050699
		ent->sequencer_data->noises = expandarray(st.noises, noise_count + 1);

		// Use a default attenuation if it isn't present
		if (!st.attenuations)
			st.attenuations = "3";
			
		// Use a default volume if it isn't present
		if (!st.volumes)
			st.volumes = "1";

		//Check to see if a speaker was listed and if not, spawn them
		if (!st.speakers)
		{
			// Check if SSM_OVERLAP is on to find out how many to spawn
			if (ent->spawnflags & SPAWNFLAG_FSS_SSM_OVERLAP)
				speaker_count = ent->sequencer_data->noiseselements + 1;
			else
				speaker_count = 1;
			for (i = 1 ; i <= speaker_count ; i++)
			{
				// Generate a speaker name based on the default speaker name and an incremented index
				itoa(i,index_as_char,10);
				newspkname = gi.TagMalloc(MAX_QPATH, TAG_LEVEL);
				strcpy(newspkname, defaultspeaker);
				strcat(newspkname, index_as_char);

				// generate a random number in the range of 5-8 to figure out how many characters to 
				// append to the defaultspeaker name
				chars_to_append = (rand() % 4) + 5;

				// Get the length of the newspkname without the \0 terminator
				current_length = strlen(newspkname);
				//current_length++;
				for (j = 1 ; j <= chars_to_append ; j++)
				{
					newspkname[current_length] = (rand() % 26) + 97;
					current_length++;
				}
				
				// Add the \0 terminator back on because we overwrote it.
				newspkname[current_length] = '\0';

				// Spawn a default speaker. Defaultsound and defaultspeaker are declared and assigned to above
				// there... What can I say, I needed defaults. I think the defaultspeaker is pretty safe...
				spawnspeaker = G_Spawn();
//				strcpy (buffer, defaultsound);						// RSP 042799 - simplify
				spawnspeaker->noise_index = gi.soundindex (defaultsound);	// RSP 042799 - simplify
				spawnspeaker->volume = 1.0;
				spawnspeaker->attenuation = 1.0;

				// Same origin as sequencer
//				spawnspeaker->s.origin[0] = ent->s.origin[0];		// RSP 042799 - simplify
//				spawnspeaker->s.origin[1] = ent->s.origin[1];		// RSP 042799 - simplify
//				spawnspeaker->s.origin[2] = ent->s.origin[2];		// RSP 042799 - simplify
				VectorCopy (ent->s.origin, spawnspeaker->s.origin);	// RSP 042799 - simplify

				// Well, it does need a name

				spawnspeaker->targetname = newspkname;

				// Add the new speaker name into the speakers list

				if (i == 1)
				{
					// I'm thinking if a spawntemp * field is null, it doesn't declare
					// memory for it, So declare it here. This makes sense to me because
					// I am using the spawntemp values I added in a odd way...
					// AJA 19990118 - Not enough memory was being allocated before, causing
					// a memory overwrite of the second speaker's targetname!! Now allocate
					// 18 bytes per speaker we spawn (should be better).

					st.speakers = gi.TagMalloc(18*speaker_count, TAG_LEVEL);	// RSP 042799 - 17->18
					strcpy(st.speakers,newspkname);
				}
				else
				{
					strcat(st.speakers,",");
					strcat(st.speakers,newspkname);
				}

				// Set the reliable flag for the target_speaker if it has been set for the sequencer
				if (ent->spawnflags & SPAWNFLAG_FSS_SSM_RELIABLE)		// RSP 042799 - Simplify
					spawnspeaker->spawnflags |= SPAWNFLAG_TS_RELIABLE;

				// set looped-off spawnflag if needed
				if (ent->spawnflags & SPAWNFLAG_FSS_SSM_LOOP_NOISE)		// RSP 042799 - Simplify
					spawnspeaker->spawnflags |= SPAWNFLAG_TS_LOOPED_OFF;

				// Set the speakers use function
				spawnspeaker->use = Use_Target_Speaker;

				// must link the entity so we get areas and clusters so
				// the server can determine who to send updates to
				gi.linkentity (ent);
			}
			
		}
	}

	// copy the spawntemp random_style in to the sequencer_data struct for later use
	ent->sequencer_data->random_style = st.random_style;

	// Check to see if we are in Single_Speaker_Mode or not and act accordingly
	if (ent->spawnflags & SPAWNFLAG_FSS_SSM)
	{
		// REQUIRED Fields for SSM Mode: (must be filled in by this point,
		// either by the level designer or by default values in the code
		// above)
		// - times
		// - speakers
		// - attenuations
		// - volumes

		// get the elements for making the array. The number returned is 1 less then the number of
		// elements, thus 3 commas means 4 elements : [0th],[1st],[2nd],[3rd]
		if ((time_count = countcommas(st.times)) < 0)
		{
			// Should have been initialized by this point.
			gi.dprintf("func_speaker_sequencer in SSM with no times set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}
		ent->sequencer_data->timeselements = time_count;
		ent->sequencer_data->times = expandarray(st.times, time_count + 1);

		if ((speaker_count = countcommas(st.speakers)) < 0)
		{
			// Should have been initialized by this point.
			gi.dprintf("func_speaker_sequencer in SSM with no speakers set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}
		ent->sequencer_data->speakerselements = speaker_count;
		ent->sequencer_data->speakers = expandarray(st.speakers, speaker_count + 1);

		if ((attenuation_count = countcommas(st.attenuations)) < 0)
		{
			// Should have been initialized by this point.
			gi.dprintf("func_speaker_sequencer in SSM with no attenuations set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}
		ent->sequencer_data->attenuationselements = attenuation_count;
		ent->sequencer_data->attenuations = expandarray(st.attenuations, attenuation_count + 1);

		if ((volume_count = countcommas(st.volumes)) < 0)
		{
			// Should have been initialized by this point.
			gi.dprintf("func_speaker_sequencer in SSM with no volumes set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}
		ent->sequencer_data->volumeselements = volume_count;
		ent->sequencer_data->volumes = expandarray(st.volumes, volume_count + 1);

		// RSP 050599 - Make sure the times, attenuations, and volumes counts are valid. If any of these is
		// greater than 0, then it has to equal the number of noises.

		if ((time_count > 0) && (time_count != noise_count))
		{
			gi.dprintf("Noise and Time counts don't match\n");
			gi.dprintf("for SSM f_s_s at %s\n",vtos(ent->s.origin));
			G_FreeEdict(ent);
			return;
		}
		if ((attenuation_count > 0) && (attenuation_count != noise_count))
		{
			gi.dprintf("Noise and Attenuation counts don't match\n");
			gi.dprintf("for SSM f_s_s at %s\n",vtos(ent->s.origin));
			G_FreeEdict(ent);
			return;
		}
		if ((volume_count > 0) && (volume_count != noise_count))
		{
			gi.dprintf("Noise and Volume counts don't match\n");
			gi.dprintf("for SSM f_s_s at %s\n",vtos(ent->s.origin));
			G_FreeEdict(ent);
			return;
		}
	}
	else
	{
		// Multiple Speaker Mode

		// REQUIRED Fields for MSM Mode: (must be filled in by this point,
		// either by the level designer or by default values in the code above)
		// - times
		// - speakers

		// initialize some vars
		// get the elements for making the array. The number returned is 1 less then the number of
		// elements, thus 3 commas means 4 elements : [0th],[1st],[2nd],[3rd]

		if ((time_count = countcommas(st.times)) < 0)
		{
			// Should have been initialized by this point.
			gi.dprintf("func_speaker_sequencer in MSM with no times set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}

		ent->sequencer_data->timeselements = time_count;
		ent->sequencer_data->times = expandarray(st.times, time_count + 1);

		if ((speaker_count = countcommas(st.speakers)) < 0)
		{
			// Should have been initialized by this point.
			gi.dprintf("func_speaker_sequencer in MSM with no speakers set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
			G_FreeEdict(ent);
			return;
		}

		ent->sequencer_data->speakerselements = speaker_count;
		ent->sequencer_data->speakers = expandarray(st.speakers, speaker_count + 1);

		// RSP 050599 - Make sure the times count is valid. If 
		// greater than 0, then it has to equal the number of speakers.

		if ((time_count > 0) && (time_count != speaker_count))
		{
			gi.dprintf("Speaker and Time counts don't match\n");
			gi.dprintf("for MSM f_s_s at %s\n",vtos(ent->s.origin));
			G_FreeEdict(ent);
			return;
		}
	}

	ent->use = func_speaker_sequencer_use;
	ent->think = func_speaker_sequencer_think;

	// RSP 050599 - Make sure the random_style is valid

	if ((ent->sequencer_data->random_style < 0) || (ent->sequencer_data->random_style > 3))
		{
		gi.dprintf("Sequencer %s random_style is invalid\n",vtos(ent->s.origin));
		ent->sequencer_data->random_style = 0;
		}

	// See if it should start on

	if (ent->spawnflags & SPAWNFLAG_FSS_START_ON)
	{
		ent->nextthink = level.time + 1.0;
		ent->activator = ent;
	}

	// RSP 050599 - initialize the "previous speaker" index so we know when we're
	// looking for one for the first time

	ent->sequencer_data->prev_speaker_index = -1;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to

	gi.linkentity (ent);
}

/*QUAKED func_teleport (1 0 0) ? KILL_MOMENTUM START_OFF
You need to have a brush as part of this entity to define its area of effect.

If the KILL_MOMENTUM spawnflag is set, momentum goes to zero at exit.
If the START_OFF spawn flag is set, this teleporter has no effect. Must
be triggered ACTIVE. Give the teleporter a targetname so it can be
triggered.
*/
void teleport_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t		*dest;
	vec3_t		v;	// RSP 062499

	if (!other->client)
		return;

	// RSP 100499 - added inactive/active states
	if (self->spawnflags & SPAWNFLAG_FTEL_INACTIVE)
		return;

	dest = G_Find (NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf ("Couldn't find destination\n");
		return;
	}

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity (other);

	// Move the player to the destination

	if (Q_stricmp(dest->classname,"func_wall") == 0)	// RSP 062499 - try alternate teleport code for better landing accuracy
	{
		VectorSubtract(other->s.origin,self->absmin,v);
		VectorAdd(v,dest->absmin,other->s.origin);
		VectorSubtract(other->s.old_origin,self->absmin,v);
		VectorAdd(v,dest->absmin,other->s.old_origin);
	}
	else	// old way
	{
		VectorCopy (dest->s.origin, other->s.origin);
		VectorCopy (dest->s.origin, other->s.old_origin);
	}

	//  RSP 090199 - if the Kill Momentum spawnflag is set, clear the velocity
	if (self->spawnflags & SPAWNFLAG_FTEL_KILLMOMENTUM)
		VectorClear (other->velocity);

	// kill anything at the destination
	KillBox (other);

	gi.linkentity (other);
}

// RSP 100499 - make teleports triggered
void func_teleport_use(edict_t *self, edict_t *other, edict_t *activator)
{
	// If active, make inactive.
	// If inactive, make active.
	self->spawnflags ^= SPAWNFLAG_FTEL_INACTIVE;
}

void SP_func_teleport (edict_t *ent)
{
	if (!ent->target)
	{
		gi.dprintf ("teleporter without a target.\n");
		G_FreeEdict (ent);
		return;
	}

	ent->solid = SOLID_TRIGGER;
	ent->svflags |= SVF_NOCLIENT;
	ent->touch = teleport_touch;
	gi.setmodel (ent, ent->model);

	// RSP 100499 - check for targetname. If no targetname,
	// can't be triggered ACTIVE/INACTIVE.
	if (!ent->targetname)
		ent->spawnflags &= ~SPAWNFLAG_FTEL_INACTIVE;
	else
		ent->use = func_teleport_use;  // Gets called when triggered

	gi.linkentity (ent);
}


// RSP 050899 - New entity

/*QUAKED func_event_sequencer (1 0 0) (-8 -8 -8) (8 8 8) SIMULTANEOUS REPEAT

A chain of events is activated by triggering entities at a specified time
interval. The func_event_sequencer is the controller.

"events"     List of events to activate
"delay"      Wait this long before the first event
"times"       Times to wait before triggering the next event
"duration"  When to stop the sequence after it's begun. Most useful with REPEAT set.

Set the REPEAT spawnflag if you want the sequence to be repeated.

If you don't specify a "times" value, then a default of "1" is assigned.
If you specify one value, then it will be used for every event.
If you specify several values, then the number of these must match the
number of events.

Set the SIMULTANEOUS spawnflag if you want all "events" to trigger at the same
time. In this case, only the first number in "times" has meaning, and that's
how long you wait until you trigger these events again, assuming the REPEAT
spawnflag is set.
*/


void func_event_sequencer_think (edict_t *self)
{
	int		current_event_index;	// index of current event in event list
	char	*eventname;
	edict_t	*event;

	// If we've used up our duration time, then stop triggering these events.
	if (self->stopthink > 0)
		if (self->stopthink <= level.time)
		{
			self->nextthink = 0;
			return;
		}

	if (self->spawnflags & SPAWNFLAG_FES_SIMULTANEOUS)
	{
		// trigger all listed events at the same time

		current_event_index = 0;
		while (self->sequencer_data->events[current_event_index])
		{
			// trigger every event with the current eventname from the list of events

			event = NULL;
			eventname = self->sequencer_data->events[current_event_index];
			while (event = G_Find(event,FOFS(targetname),eventname))
				if (self->activator)	// RSP 110400
					event->use(event,self,self->activator);
				else
					event->use(event,self,self);

			current_event_index++;
		}

		// If the repeat flag is not on, then stop triggering this list of events

		if (!(self->spawnflags & SPAWNFLAG_FES_REPEAT))
		{
			self->nextthink = 0;
			return;
		}

		// Set the time the next event should be triggered.

		self->nextthink = level.time + atof(self->sequencer_data->times[0]);
	}
	else	// trigger the events in sequence using the "times" values
	{

		// set the current event element

		current_event_index = self->sequencer_data->currentevent;

		// trigger every event with the current eventname from the list of events

		event = NULL;
		eventname = self->sequencer_data->events[current_event_index];
		while (event = G_Find(event,FOFS(targetname),eventname))
			if (self->activator)	// RSP 110400
				event->use(event,self,self->activator);
			else
				event->use(event,self,self);

		// Select the next event element in the sequence

		if (current_event_index == self->sequencer_data->eventelements)
		{
			self->sequencer_data->currentevent = 0;

			// If the repeat flag is not on, then stop triggering this list of events

			if (!(self->spawnflags & SPAWNFLAG_FES_REPEAT))
			{
				self->nextthink = 0;
				return;
			}
		}
		else
			self->sequencer_data->currentevent++;

		// Set the time the next event should be triggered.
		// We might only have one times value. If that's the case, always use the one we were
		// given. If there is more than one, you can take a value from the correct index in the list.

		if (self->sequencer_data->times[1] == NULL)
			self->nextthink = level.time + atof(self->sequencer_data->times[0]);
		else
			self->nextthink = level.time + atof(self->sequencer_data->times[current_event_index]);
	}
}


void func_event_sequencer_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// pass along the activator
	self->activator = activator;

	// if on, turn it off
	if (self->nextthink)
	{
		self->nextthink = 0;
		return;
	}

	// RSP 052299 - fixed bug where this wasn't activating the think function

	if (self->delay)
		self->nextthink = level.time + self->delay;
	else if (self->sequencer_data->duration)
		self->stopthink = self->nextthink + self->sequencer_data->duration;
	else
		self->think(self);
}


void SP_func_event_sequencer (edict_t *ent)
{
	int time_count,event_count;
	
	// Since we already have an array of structs to hold sequencer data, we'll just
	// change this pointer to point to one of the items in that array. The index of
	// the struct in that array will be the same as the index of this edict in its own
	// edict array. This uses up a lot of memory, but is the simplest solution at this
	// point. We can look at it again for phase 2.

	ent->sequencer_data = &g_tnt_seqs[ent - g_edicts];

	// Check to see if we have times... If not, use a default
	if (!st.times)
	{
		gi.dprintf("func_event_sequencer with no times set at %s [%s] using default time of 1\n",
			vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
		st.times = "1";
	}

	// REQUIRED Fields (must be filled in by this point,
	// either by the level designer or by default values in the code above)
	// - times
	// - events

	// initialize some vars
	// get the elements for making the array. The number returned is 1 less then the number of
	// elements, thus 3 commas means 4 elements : [0th],[1st],[2nd],[3rd]

	if ((time_count = countcommas(st.times)) < 0)
	{
		// Should have been initialized by this point.
		gi.dprintf("func_event_sequencer with no times set at %s [%s]\n",
				vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
		G_FreeEdict(ent);
		return;
	}

	ent->sequencer_data->timeselements = time_count;
	ent->sequencer_data->times = expandarray(st.times, time_count + 1);

	if ((event_count = countcommas(st.events)) < 0)
	{
		// Should have been initialized by this point.
		gi.dprintf("func_event_sequencer with no events set at %s [%s]\n",
			vtos(ent->s.origin), ent->targetname ? ent->targetname : "unnamed");
		G_FreeEdict(ent);
		return;
	}

	ent->sequencer_data->eventelements = event_count;
	ent->sequencer_data->events = expandarray(st.events, event_count + 1);
	ent->sequencer_data->duration = st.duration;

	// Make sure the times count is valid. If 
	// greater than 0, then it has to equal the number of events.

	if ((time_count > 0) && (time_count != event_count))
	{
		gi.dprintf("Event and Time counts don't match\n");
		gi.dprintf("f_e_s at %s\n",vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	ent->use = func_event_sequencer_use;
	ent->think = func_event_sequencer_think;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to

	gi.linkentity (ent);
}


// RSP 061699 - Added func_BoxCar

/*QUAKED func_BoxCar (0 .5 .8) ?

Transports every item, client, and monster whose origin is inside
the func_BoxCar to the next map, preserving state and relative position.

func_BoxCar should be the target of a target_changelevel. When the
target_changelevel is triggered, the func_BoxCar use() function is
invoked.

You need an identically-sized BoxCar in the destination map,
and both BoxCars should have the same targetname. Both should also have
the same orientation wrt x,y,z.

Turrets are not transported. They're considered part of the scenery. If
you have a turret inside the func_BoxCar, you need to replicate it at the
destination.

Anything else--like ramping lights, switched lights, target_speakers--are
not transported. Keep these things out of the func_BoxCar and you should be
able to preserve the "seamlessness".

Since the player will be spawned at his relative position w/in the func_BoxCar,
a player_start in the destination map will be ignored.

Any entities that are outside the func_BoxCar that are referenced by 
entities inside the func_BoxCar will be ignored and all references will be
dropped.

Monsters will lose any waypoint paths.

The boundaries of the func_BoxCar should--as much as possible--align with
solid brushes to eliminate things like a supervisor inside the func_BoxCar
who is restoring health to a monster outside the func_BoxCar. Supervisor
transports, monster doesn't.
 */

edict_t *BoxCar_Passengers = NULL;	// Array of transported entities
						// The func_BoxCar itself is always first on the list

vec3_t boxcarmin,boxcarmax;
char BoxCarName[256];	// RSP 042000 - Name of func_BoxCar

// See if this entity is a valid passenger

qboolean BoxCar_ValidPassenger(edict_t *e)
{
	if ((e->flags & FL_PASSENGER) || e->client)	// Take live clients and passengers
	{
		if ((e->inuse) && // RSP 091500 - can't have dead things winking in and out
//		if ((e->inuse) && (e->deadflag == DEAD_NO) &&	// RSP 042000 - added DEAD_NO check
		    ((e->s.origin[0] >= boxcarmin[0]) && (e->s.origin[0] <= boxcarmax[0])) &&
		    ((e->s.origin[1] >= boxcarmin[1]) && (e->s.origin[1] <= boxcarmax[1])) &&
		    ((e->s.origin[2] >= boxcarmin[2]) && (e->s.origin[2] <= boxcarmax[2])))
		{
			return true;
		}
	}
	return false;
}


// Convert a pointer to an edict into an index into the BoxCar_Passengers array if
// the edict being pointed to is a passenger. If it's not, clear the link. When the
// passengers are restored in the next map, the links will be properly set to the
// new entity structure addresses.

void BoxCar_ConvertLink(edict_t **link)
{
	if ((*link)->boxcar_index)
		(*link) = (edict_t *)((*link)->boxcar_index);	// store the index
	else
		(*link) = NULL;	// Clear the link
}

void BoxCar_ConvertString(char **name)
{
	edict_t *t = NULL;

	while ((t = G_Find (t, FOFS(targetname), *name)))
		if (t->boxcar_index)
			return;	// There's a passenger with this targetname, so keep the name

	*name = NULL;	// Clear the name; there are no passengers with this name
}

// Add a new entity to the list being transported

void BoxCarSave(edict_t *ent)
{
	edict_t	*b;

	BoxCar_Passengers[ent->boxcar_index] = *ent; 	// Make a copy

	b = &BoxCar_Passengers[ent->boxcar_index];

	b->attack = b->retreat = NULL;	// Remove waypoints

	// Convert all entity pointers to passenger indices

	if (b->locked_to)
	{
		if (b->locked_to == world)
			b->locked_to = (edict_t *) -1;	// world is special
		else
			BoxCar_ConvertLink(&(b->locked_to));
	}
	if (b->chain)
		BoxCar_ConvertLink(&(b->chain));
	if (b->enemy)
		BoxCar_ConvertLink(&(b->enemy));
	if (b->oldenemy)
		BoxCar_ConvertLink(&(b->oldenemy));
	if (b->activator)
		BoxCar_ConvertLink(&(b->activator));
	if (b->groundentity)
		BoxCar_ConvertLink(&(b->groundentity));
	if (b->teamchain)
		BoxCar_ConvertLink(&(b->teamchain));
	if (b->teammaster)
		BoxCar_ConvertLink(&(b->teammaster));
	if (b->mynoise)
		BoxCar_ConvertLink(&(b->mynoise));
	if (b->mynoise2)
		BoxCar_ConvertLink(&(b->mynoise2));
	if (b->goalentity)
		BoxCar_ConvertLink(&(b->goalentity));
	if (b->movetarget)
		BoxCar_ConvertLink(&(b->movetarget));
	if (b->target_ent)
		BoxCar_ConvertLink(&(b->target_ent));
	if (b->cell)	// RSP 100800
		BoxCar_ConvertLink(&(b->cell));
	if (b->owner)
		BoxCar_ConvertLink(&(b->owner));
	if (b->curpath)	// RSP 072499
		BoxCar_ConvertLink(&(b->curpath));
	if (b->dlockOwner)	// RSP 090200
		BoxCar_ConvertLink(&(b->dlockOwner));
	if (b->decoy)	// RSP 090200
		BoxCar_ConvertLink(&(b->decoy));
	if (b->teleportgun_victim)	// RSP 090200
		BoxCar_ConvertLink(&(b->teleportgun_victim));

	// Client structure data (game.clients) isn't wiped at the start of a level.
	// The b->client pointer is still good across levels, because game.clients
	// doesn't move and the client always has the same index number.

/* RSP 092100 - not used
	if (b->client && b->client->pers.currenttoy)
		BoxCar_ConvertLink(&(b->client->pers.currenttoy));
 */
	// Look at the fields that contain the targetnames of entities. If there are
	// any passengers with this targetname, keep the field as is. If there are no
	// passengers with this targetname, clear the field. You don't need to process
	// these at the destination.

	if (b->killtarget)
		BoxCar_ConvertString(&(b->killtarget));
	if (b->target)
		BoxCar_ConvertString(&(b->target));
	if (b->pathtarget)
		BoxCar_ConvertString(&(b->pathtarget));
	if (b->deathtarget)
		BoxCar_ConvertString(&(b->deathtarget));
	if (b->combattarget)
		BoxCar_ConvertString(&(b->combattarget));

//	G_FreeEdict(ent);	// RSP 042000 - this entity isn't here if we come back
}


void func_BoxCar_use (edict_t *ent, edict_t *other, edict_t *activator)
{
	edict_t	*e;
	int		counter = 0;	// count of transported entities
	int		i;

	// Save all items, clients, and monsters inside the BoxCar for transport
	// to the next map.

	// Count the number of passengers

	VectorCopy(ent->absmin,boxcarmin);
	VectorCopy(ent->absmax,boxcarmax);
	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		e = &g_edicts[i];
		if (e->boxcar_index)
			continue;	// already got this one; see mynoise discussion below
		if (BoxCar_ValidPassenger(e))
		{
			counter++;
			e->boxcar_index = counter;	// entity is inside the func_BoxCar

			// Special handling for mynoise and mynoise2 for clients. When the player makes
			// a noise, edicts are spawned for mynoise and mynoise2. But only one of them
			// is linked into the game, depending on whether the noise came from the player
			// or from the player's weapon. Noise edicts are given to monsters as enemies so
			// they can track the player, if the player himself can't be seen. Since only one
			// is used at a time, the other is "left behind" at the coordinates where it was
			// last used. We _have_ to carry these edicts in the BoxCar, even if their locations
			// are outside the BoxCar. We do that by giving them the same location of the player.
			//
			// If mynoise exists, then mynoise2 exists

			if (e->client && e->mynoise)
			{
				VectorCopy(e->s.origin,e->mynoise->s.origin);
				VectorCopy(e->s.origin,e->mynoise2->s.origin);

				// Now mark these edicts as passengers, unless they were previously marked.

				if (e->mynoise->boxcar_index == 0)
				{
					counter++;
					e->mynoise->boxcar_index = counter;
				}
				if (e->mynoise2->boxcar_index == 0)
				{
					counter++;
					e->mynoise2->boxcar_index = counter;
				}
			}

			// RSP 090100
			// Special handling for teleportgun victims that are being "carried" w/in the
			// teleportgun the player own. The victim's entity was left at the spot where
			// it was when grabbed, which might be outside the BoxCar. We have to bring
			// that entity into the BoxCar, because if we launch them after spawning in the
			// new map, it has to be there to launch. So we copy the player's coordinates to
			// the victim's, and mark the victim as being inside the BoxCar.

			if (e->client && e->teleportgun_victim)
			{
				VectorCopy(e->s.origin,e->teleportgun_victim->s.origin);
				VectorCopy(e->s.origin,e->teleportgun_victim->s.origin);

				// Now mark this edict as a passenger, unless it was previously marked.

				if (e->teleportgun_victim->boxcar_index == 0)
				{
					counter++;
					e->teleportgun_victim->boxcar_index = counter;
				}
			}
		}
	}

	// Create the array to store the passengers. The zeroth entry in the array
	// is only used to store general information about the func_BoxCar. This is used when
	// setting up the new map.

	BoxCar_Passengers = gi.TagMalloc((counter+1)*sizeof(g_edicts[0]),TAG_GAME);
	strcpy(BoxCarName,ent->targetname);	// RSP 042000
	BoxCar_Passengers[0].count = counter;			// the number of passengers
	BoxCar_Passengers[0].nextthink = level.time;	// current timestamp
	BoxCar_Passengers[0].count2 = level.framenum;	// current framenum
	VectorCopy(ent->absmin,BoxCar_Passengers[0].absmin);		// reference point for source BoxCar

	// Remove any matrix shots that are targeted to monsters that aren't making the trip.

	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		e = &g_edicts[i];

		if (e->classname && (strcmp(e->classname,"matrix blast") == 0))	// RSP 042000
			if (e->enemy->boxcar_index == 0)
			{	// The targeted monster isn't traveling, so neither is the matrix
				e->boxcar_index = 0;
				e->locked_to->boxcar_index = 0;
				BoxCar_Passengers[0].count -= 2;			// the number of passengers
			}
	}

	// Save the information for each passenger. As you do so, it's OK to alter
	// entity information because these entities are about to be destroyed anyway
	// as the level ends. 

	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		e = &g_edicts[i];

		if (e->boxcar_index)	// entity is inside the func_BoxCar?
			BoxCarSave(e);
	}

	// RSP 092500 free all passenger edicts afterwards
	for (i = 0 ; i < globals.num_edicts ; i++)
	{
		e = &g_edicts[i];

		if (e->boxcar_index)	// entity is inside the func_BoxCar?
			G_FreeEdict(e);
	}
}

void SP_func_BoxCar (edict_t *ent)
{
	gi.setmodel (ent, ent->model);
	ent->use = func_BoxCar_use;		// Gets called when triggered
	ent->svflags = SVF_NOCLIENT;	// Invisible
	gi.linkentity(ent);
}


/*QUAKED func_rotrain (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS START_ROT_ON TOUCH_PAIN ANIMATED ANIMATED_FAST

Trains are moving platforms that players can ride. A func_rotrain is a
func_train that can rotate.

The min point of the train at each corner is defined by the origins of path_corners.

The train spawns at the first target (path_corner) it is pointing at.

If the train is the target of a button or trigger, it will not begin
moving until activated.

A train can rotate while stationary or while moving. See path_corner.

Keys:

"speed"	translation speed (default 100)
"dmg"	damage done when blocked (default 2)
"noise"	looping sound to play when the train is in motion

Spawnflags:

"start_on"		start moving when spawned
"toggle"		turn on/off with triggers
"block_stops"	stop when blocked
"start_rot_on"	start rotating when spawned
"touch_pain"	painful if it touches you when rotating
"animated"		?
"animated_fast"	?
*/

void rotrain_readpathcorner (edict_t *);
void rotrain_wait (edict_t *);
void rotrain_final(edict_t *);
void rotrain_begin(edict_t *);

void rotrain_done (edict_t *self)
{
	// did we just finish the final frame in a rotation?

	if (self->spawnflags & SPAWNFLAG_RT_FINALROTATION)
	{
		// nail the desired arc in the last frame for styles that need it
		VectorCopy(self->moveinfo.end_angles,self->s.angles);
		self->spawnflags &= ~(SPAWNFLAG_RT_ROTATING|SPAWNFLAG_RT_FINALROTATION);
		VectorClear (self->avelocity);
		self->touch = NULL;

	// check for an ongoing translation. this will occur if we've reached a desired
	// arc before reaching the next path_corner.

	// if self->spawnflags & SPAWNFLAG_RT_LATCHTRANS is set, a delayed
	// translation must be started if this is style PCSTYLE_ROT_ARC_TRANS.

	// if self->spawnflags & SPAWNFLAG_RT_LATCHTRANS is not set,
	// self->timestamp has the time when the translation is to stop.
	// if non-zero, set the translation stop routine and time when to
	// invoke it.

		if ((self->curpath->style == PCSTYLE_TRANS_ROT_ARC) ||
			(self->curpath->style == PCSTYLE_ROT_ARC_TRANS))
			if (self->spawnflags & SPAWNFLAG_RT_LATCHTRANS)	// if latched
			{
				self->spawnflags |= SPAWNFLAG_RT_TRANSLATING;	// turn on translation
				self->spawnflags &= ~SPAWNFLAG_RT_LATCHTRANS;	// turn off latch
				rotrain_begin(self);
			}
			else if (self->spawnflags & SPAWNFLAG_RT_TRANSLATING)
			{
				self->nextthink = self->timestamp;
				self->think = rotrain_final;
			}
	}

	// did we just finish the final frame in a translation?

	if (self->spawnflags & SPAWNFLAG_RT_FINALTRANS)
	{
		// nail the desired arc in the last frame for style PCSTYLE_TRANS_ROT_NEXTPATH
		if (self->spawnflags & SPAWNFLAG_RT_ROTATING)
			if (self->curpath->style == PCSTYLE_TRANS_ROT_NEXTPATH)
				VectorCopy(self->moveinfo.end_angles,self->s.angles);
		self->spawnflags &= ~(SPAWNFLAG_RT_TRANSLATING|SPAWNFLAG_RT_FINALTRANS);
		VectorClear (self->velocity);
		self->moveinfo.endfunc (self);	// call end function
	}
}

// Set up the final one-FRAMETIME move. The velocity may be different in this
// final frame than it's been in previous frames. This accounts for roundoff
// errors. This applies to both rotation and translation.

void rotrain_final (edict_t *self)
{
	vec3_t	move;
	float	len;

	// Set up final rotation frame if stopping rotation at a specific arc

	if (self->spawnflags & SPAWNFLAG_RT_ROTATING)
		switch(self->curpath->style)
		{
		default:
		case PCSTYLE_TRANS:
		case PCSTYLE_TRANS_ROT:
		case PCSTYLE_TRANS_ROT_NEXTPATH:
			break;
		case PCSTYLE_TRANS_ROT_ARC:
		case PCSTYLE_ROT_ARC_TRANS:
			VectorSubtract (self->moveinfo.end_angles, self->s.angles, move);
			len = VectorLength(move);
			if (len == 0)
			{
				self->spawnflags |= SPAWNFLAG_RT_FINALROTATION;
				rotrain_done (self);
			}
			else
			{
				VectorScale (move, 1.0/FRAMETIME, self->avelocity);
				self->spawnflags |= SPAWNFLAG_RT_FINALROTATION;
				self->think = rotrain_done;
				self->nextthink = level.time + FRAMETIME;
			}
			return;
		}

	// Set up final translation frame if stopping translation

	if (self->spawnflags & SPAWNFLAG_RT_TRANSLATING)
	{
		if (self->moveinfo.remaining_distance == 0)
		{
			self->spawnflags |= SPAWNFLAG_RT_FINALTRANS;
			rotrain_done (self);
		}
		else
		{
			VectorScale (self->moveinfo.dir, self->moveinfo.remaining_distance / FRAMETIME, self->velocity);
			self->spawnflags |= SPAWNFLAG_RT_FINALTRANS;
			self->think = rotrain_done;
			self->nextthink = level.time + FRAMETIME;
		}
	}
}

void rotrain_begin (edict_t *self)
{
	float	frames;
	vec3_t	delta_distance,delta_angle;
	float	traveltime,nextthink;
	edict_t	*path = self->curpath;
	edict_t	*t;

	self->nextthink = 0;	// initialize, set later

	// Set up rotation
	
	if (self->spawnflags & SPAWNFLAG_RT_ROTATING)
	{
		switch(path->style)
		{
		case PCSTYLE_TRANS_ROT:		// constant rotation at the given speed while translating
			VectorScale (self->movedir, path->speed, self->avelocity);	// set angular velocity based on speed
			break;

		case PCSTYLE_ROT_ARC_TRANS:	// rotate at given speed until arc reached, then translate
			self->spawnflags &= ~SPAWNFLAG_RT_TRANSLATING;
		case PCSTYLE_TRANS_ROT_ARC:	// rotate at given speed while translating until arc reached
			VectorCopy(self->s.angles,self->moveinfo.start_angles);
			VectorScale(self->movedir,path->s.angles[1],self->moveinfo.end_angles);
			VectorAdd(self->s.angles,self->moveinfo.end_angles,self->moveinfo.end_angles);

			// divide by angular speed to get time to reach dest
			traveltime = path->s.angles[1]/path->speed;
			VectorScale (self->movedir,path->speed,self->avelocity);
			// RSP DEBUG: you can't return here if this is true, because you still have to set
			// up translation. Check it if a bug appears.
			if (traveltime < FRAMETIME)
			{
				self->spawnflags |= SPAWNFLAG_RT_FINALROTATION;
				self->think = rotrain_done;
				self->nextthink = level.time + FRAMETIME;
				return;
			}
			else
			{	// set nextthink to trigger a think when dest is reached
				frames = floor(traveltime / FRAMETIME);
				self->nextthink = level.time + frames * FRAMETIME;
				self->think = rotrain_final;
			}
			break;

		case PCSTYLE_TRANS_ROT_NEXTPATH:	// complete arc at next path_corner while translating
			VectorCopy(self->s.angles,self->moveinfo.start_angles);
			t = G_PickTarget(path->target);
			VectorSubtract(path->s.origin, t->s.origin, delta_distance);	// find vector from start to end
			traveltime = VectorLength(delta_distance)/self->moveinfo.speed;	// time to translate
			path->speed = path->s.angles[1] / traveltime;
			VectorScale(self->movedir,path->s.angles[1],delta_angle);
			VectorScale (delta_angle, 1.0 / traveltime, self->avelocity);
			VectorAdd(self->s.angles,delta_angle,self->moveinfo.end_angles);
			// Since the arc finishes at the next path_corner, there's no need to
			// set a separate think time to stop rotation. All rotation stops at the
			// next path_corner by default.
			break;
		}

		if (self->spawnflags & SPAWNFLAG_RT_TOUCH_PAIN)
			self->touch = rotating_touch;
	}

	// Set up translation if not latched

	if ((self->spawnflags & SPAWNFLAG_RT_TRANSLATING) && (!(self->spawnflags & SPAWNFLAG_RT_LATCHTRANS)))
	{
		if ((self->moveinfo.speed * FRAMETIME) >= self->moveinfo.remaining_distance)
		{
			rotrain_final (self);
			return;
		}
		VectorScale (self->moveinfo.dir, self->moveinfo.speed, self->velocity);
		frames = floor((self->moveinfo.remaining_distance / self->moveinfo.speed) / FRAMETIME);
		self->moveinfo.remaining_distance -= frames * self->moveinfo.speed * FRAMETIME;
		nextthink = level.time + (frames * FRAMETIME);	// when rotrain_final gets called to setup final translation frame
		if (self->nextthink && (self->nextthink < nextthink))
			self->timestamp = nextthink; // save translation stop for later
		else if (self->nextthink)
		{
			self->timestamp = self->nextthink;
			self->nextthink = nextthink;
			self->think = rotrain_final;
		}
		else	// translation stop earlier than rotation stop?
		{
			self->nextthink = nextthink;
			self->think = rotrain_final;
		}
	}

	if (!(self->flags & FL_TEAMSLAVE))
		self->s.sound = self->moveinfo.sound_middle;
}

void rotrain_calc (edict_t *self, vec3_t dest, void(*func)(edict_t*))
{
	edict_t *path = self->curpath;	// path_corner providing the directions

	// Set up rotation
	
	switch(path->style)
	{
	case PCSTYLE_TRANS_ROT:
	case PCSTYLE_TRANS_ROT_NEXTPATH:
		break;
	case PCSTYLE_TRANS_ROT_ARC:
	case PCSTYLE_ROT_ARC_TRANS:
		if (path->s.angles[1] == 0)
			self->spawnflags &= ~SPAWNFLAG_RT_ROTATING;
		break;
	}

	if (self->spawnflags & SPAWNFLAG_RT_ROTATING)
	{
		VectorClear (self->avelocity);

		// set the axis of rotation

		VectorClear(self->movedir);
		if (path->spawnflags & SPAWNFLAG_PC_X_AXIS)
			self->movedir[2] = 1.0;
		else if (path->spawnflags & SPAWNFLAG_PC_Y_AXIS)
			self->movedir[0] = 1.0;
		else // Z_AXIS
			self->movedir[1] = 1.0;

		// check for reverse rotation
		if (path->spawnflags & SPAWNFLAG_PC_REVERSE)
			VectorNegate (self->movedir, self->movedir);
		VectorCopy(self->s.angles,self->moveinfo.start_angles);
		VectorScale(self->movedir,self->s.angles[1],self->moveinfo.end_angles);
	}

	// Set up translation

	if (self->spawnflags & SPAWNFLAG_RT_TRANSLATING)
	{
		VectorClear (self->velocity);
		VectorSubtract (dest, self->s.origin, self->moveinfo.dir);
		self->moveinfo.remaining_distance = VectorNormalize (self->moveinfo.dir);

	// for func_rotrains, if we're sitting at the destination, but we still had some
	// wait time left when we were paused, that time is stored in last_move_time. if we're NOT
	// sitting at the destination, then we were inside a translation time and not a
	// wait time, so we can clear last_move_time.

		if (self->moveinfo.remaining_distance > 0)
			self->last_move_time = 0;	// not needed, so clear it out
	}

	self->moveinfo.endfunc = func;
	if (level.current_entity == ((self->flags & FL_TEAMSLAVE) ? self->teammaster : self))
		rotrain_begin (self);
	else
	{
		self->nextthink = level.time + FRAMETIME;
		self->think = rotrain_begin;
	}
}


void rotrain_blocked (edict_t *self, edict_t *other)
{
	if (!(other->svflags & SVF_MONSTER) && (!other->client) )
	{
		// give it a chance to go away on its own terms (like gibs)
		T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, 100000, 1, 0, MOD_CRUSH);
		// if it's still there, nuke it
		if (other)
		{
			SpawnExplosion1(other,EXPLODE_SMALL);      // RSP 062900
			G_FreeEdict(self);
		}
		return;
	}

	if (level.time < self->touch_debounce_time)
		return;

	if (!self->dmg)
		return;
	self->touch_debounce_time = level.time + 0.5;
	T_Damage (other, self, self, vec3_origin, other->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}


// RSP 062999 - rotrain_wait() is called when a func_rotrain hits its destination path_corner
// It doesn't actually _hit_ the path_corner. rotrain_wait() gets called once the func_rotrain
// has traveled the distance needed to get to the next path_corner. At that point,
// self->target_ent is the path_corner the func_rotrain just "touched".

void rotrain_wait (edict_t *self)
{
	// stop any continuous rotation
	if (self->spawnflags & SPAWNFLAG_RT_ROTATING)
	{
		self->spawnflags &= ~SPAWNFLAG_RT_ROTATING;
		VectorClear (self->avelocity);
		self->touch = NULL;
	}

	if (self->target_ent->pathtarget)
	{
		char	*savetarget;
		edict_t	*ent;

		ent = self->target_ent;
		savetarget = ent->target;
		ent->target = ent->pathtarget;
		G_UseTargets (ent, self->activator);
		ent->target = savetarget;

		// make sure we didn't get killed by a killtarget
		if (!self->inuse)
			return;
	}

	if (self->moveinfo.wait)
	{
		if (self->moveinfo.wait > 0)
		{
			self->nextthink = level.time + self->moveinfo.wait;
			self->curpath = self->target_ent;
			self->think = rotrain_readpathcorner;
			self->spawnflags |= SPAWNFLAG_RT_WAITING;
		}
		else if (self->spawnflags & SPAWNFLAG_RT_TOGGLE)  // && wait < 0
		{
			self->curpath = self->target_ent;
			self->spawnflags &= ~(SPAWNFLAG_RT_TRANSLATING|SPAWNFLAG_RT_ROTATING|SPAWNFLAG_RT_WAITING);
			VectorClear (self->velocity);
			VectorClear (self->avelocity);
			self->nextthink = 0;
			self->touch = NULL;
		}

		if (!(self->flags & FL_TEAMSLAVE))
			self->s.sound = 0;
	}
	else
	{
		self->curpath = self->target_ent;
		rotrain_readpathcorner (self);
	}
}

// rotrain_readpathcorner(): Find your next translation stop and get started toward it. If there's
// rotation, then get that started too.

void rotrain_readpathcorner (edict_t *self)
{
	edict_t		*destination_edict;
	vec3_t		dest;
	qboolean	first;
	edict_t		*source_edict = self->curpath;	// where you're leaving from
	char		*destination_name;				// where you're stopping next.

	self->spawnflags &= ~SPAWNFLAG_RT_WAITING;
	first = true;
	VectorClear(dest);

again:
	destination_name = self->target;	// where you're stopping next.
	if (!destination_name)
	{
		self->spawnflags &= ~(SPAWNFLAG_RT_TRANSLATING|SPAWNFLAG_RT_ROTATING);
		return;
	}

	destination_edict = G_PickTarget (destination_name);
	if (!destination_edict)
	{
		gi.dprintf ("rotrain_readpathcorner: bad destination %s\n", destination_name);
		self->spawnflags &= ~(SPAWNFLAG_RT_TRANSLATING|SPAWNFLAG_RT_ROTATING);
		return;
	}

	// Set self->target to the stop after the current destination

	self->target = destination_edict->target;

	// check for a teleport path_corner at the destination
	if (destination_edict->spawnflags & SPAWNFLAG_PC_TELEPORT)
	{
		if (!first)
		{
			gi.dprintf ("connected teleport path_corners, see %s at %s\n", destination_edict->classname, vtos(destination_edict->s.origin));
			self->spawnflags &= ~(SPAWNFLAG_RT_TRANSLATING|SPAWNFLAG_RT_ROTATING);
			return;
		}
		first = false;
		VectorSubtract (destination_edict->s.origin, self->mins, self->s.origin);
		VectorCopy (self->s.origin, self->s.old_origin);
		self->s.event = EV_OTHER_TELEPORT;	// 3.20
		gi.linkentity (self);
		goto again;
	}

	// moveinfo.wait gets the amount of time you're to wait once you arrive at your
	// destination. Think of a button. This is _not_ the amount of time you're to wait
	// before doing this translation.

	self->moveinfo.wait = destination_edict->wait;

	self->target_ent = destination_edict;	// self->target_ent now points to the current destination

	VectorSubtract (destination_edict->s.origin, self->mins, dest);
	VectorCopy (self->s.origin, self->moveinfo.start_origin);
	VectorCopy (dest, self->moveinfo.end_origin);

	// Any rotation?

	switch(source_edict->style)
	{
	default:
	case PCSTYLE_TRANS:
		self->spawnflags |= SPAWNFLAG_RT_TRANSLATING;
		self->spawnflags &= ~SPAWNFLAG_RT_ROTATING;
		break;
	case PCSTYLE_ROT_ARC_TRANS:
		self->spawnflags |= SPAWNFLAG_RT_LATCHTRANS;	// set latch to delay translation
	case PCSTYLE_TRANS_ROT:
	case PCSTYLE_TRANS_ROT_ARC:
	case PCSTYLE_TRANS_ROT_NEXTPATH:
		self->spawnflags |= (SPAWNFLAG_RT_TRANSLATING|SPAWNFLAG_RT_ROTATING);
		break;
	}

	rotrain_calc(self,dest,rotrain_wait);	// does rotation and translation
}


void rotrain_resume (edict_t *self)
{
	float	frames;
	vec3_t	destdelta,v,dest;
	float	len;
	float	traveltime,nextthink;
	edict_t	*path = self->curpath;
	edict_t	*t;

	self->nextthink = 0;	// initialize, set later

	// resume rotation
	
	if (self->spawnflags & SPAWNFLAG_RT_ROTATING)
	{
		switch(path->style)
		{
		case PCSTYLE_TRANS_ROT:		// constant rotation at the given speed while translating
			VectorScale (self->movedir, path->speed, self->avelocity);	// set angular velocity based on speed
			break;

		case PCSTYLE_ROT_ARC_TRANS:	// rotate at given speed until arc reached, then translate
		case PCSTYLE_TRANS_ROT_ARC:	// rotate at given speed while translating until arc reached

			// set destdelta to the vector needed to move
			VectorSubtract (self->moveinfo.end_angles, self->s.angles, destdelta);
	
			// calculate length of vector
			len = VectorLength (destdelta);
	
			// divide by angular speed to get time to reach dest
			traveltime = len/path->speed;
			VectorScale (destdelta, 1.0 / traveltime, self->avelocity);

			// RSP DEBUG: you can't return here if this is true, because you still have to set
			// up translation. Watch for bugs to turn up here.
			if (traveltime < FRAMETIME)
			{
				self->spawnflags |= SPAWNFLAG_RT_FINALROTATION;
				self->think = rotrain_done;
				self->nextthink = level.time + FRAMETIME;
				return;
			}
			else
			{	// set nextthink to trigger a think when target arc is reached
				frames = floor(traveltime / FRAMETIME);
				self->nextthink = level.time + frames * FRAMETIME;
				self->think = rotrain_final;
			}
			break;

		case PCSTYLE_TRANS_ROT_NEXTPATH:	// complete arc at next path_corner while translating
			t = G_PickTarget(path->target);
			VectorSubtract(path->s.origin, t->s.origin, v);	// find vector from here to end
			traveltime = VectorLength(v)/self->moveinfo.speed;	// time to translate
			// set destdelta to the vector needed to move
			VectorSubtract (self->moveinfo.end_angles, self->s.angles, destdelta);
	
			// calculate length of vector
			len = VectorLength (destdelta);
	
			path->speed = len / traveltime;

			// Since the arc finishes at the next path_corner, there's no need to
			// set a separate think time to stop rotation. All rotation stops at the
			// next path_corner by default.

			// scale the destdelta vector by the time spent traveling to get velocity
			VectorScale (destdelta, 1.0 / traveltime, self->avelocity);
			break;
		}

		if (self->spawnflags & SPAWNFLAG_RT_TOUCH_PAIN)
			self->touch = rotating_touch;
	}

	// resume translation

	if (self->spawnflags & SPAWNFLAG_RT_TRANSLATING)
	{
		t = self->target_ent;
		VectorSubtract (t->s.origin, self->mins, dest);
		VectorSubtract (dest, self->s.origin, self->moveinfo.dir);
		self->moveinfo.remaining_distance = VectorNormalize (self->moveinfo.dir);
		if ((self->moveinfo.speed * FRAMETIME) >= self->moveinfo.remaining_distance)
		{
			rotrain_final (self);
			return;
		}
		VectorScale (self->moveinfo.dir, self->moveinfo.speed, self->velocity);
		frames = floor((self->moveinfo.remaining_distance / self->moveinfo.speed) / FRAMETIME);
		self->moveinfo.remaining_distance -= frames * self->moveinfo.speed * FRAMETIME;
		nextthink = level.time + (frames * FRAMETIME);	// when rotrain_final gets called to stop translation
		if (self->nextthink && (self->nextthink < nextthink))
			self->timestamp = nextthink; // save translation stop for later
		else if (self->nextthink)
		{
			self->timestamp = self->nextthink;
			self->nextthink = nextthink;
			self->think = rotrain_final;
		}
		else	// translation stop earlier than rotation stop?
		{
			self->nextthink = nextthink;
			self->think = rotrain_final;
		}
	}

	// resume waiting

	if (self->spawnflags & SPAWNFLAG_RT_WAITING)
	{
		self->nextthink = level.time + self->last_move_time;
		self->last_move_time = 0;
		self->think = rotrain_readpathcorner;
		return;	// return here because waiting implies no sound
	}
	
	if (!(self->flags & FL_TEAMSLAVE))
		self->s.sound = self->moveinfo.sound_middle;
}

void rotrain_use (edict_t *self, edict_t *other, edict_t *activator)
{
	int rotating,translating,rotation_paused,translation_paused,waiting,waiting_paused;

	rotating = ((self->spawnflags & SPAWNFLAG_RT_ROTATING) && (!VectorCompare(vec3_origin,self->avelocity))) ? 1 : 0;
	translating = ((self->spawnflags & SPAWNFLAG_RT_TRANSLATING) && (!VectorCompare(vec3_origin,self->velocity))) ? 1 : 0;
	rotation_paused = ((self->spawnflags & SPAWNFLAG_RT_ROTATING) && (VectorCompare(vec3_origin,self->avelocity))) ? 1 : 0;
	translation_paused = ((self->spawnflags & SPAWNFLAG_RT_TRANSLATING) && (VectorCompare(vec3_origin,self->velocity))) ? 1 : 0;
	waiting = ((self->spawnflags & SPAWNFLAG_RT_WAITING) && (self->last_move_time == 0)) ? 1 : 0;
	waiting_paused = ((self->spawnflags & SPAWNFLAG_RT_WAITING) && (self->last_move_time)) ? 1 : 0;

	if (!rotating && !translating && !rotation_paused && !translation_paused && !waiting && !waiting_paused)
	{
		self->activator = activator;
		rotrain_readpathcorner(self);	// get going
		return;
	}

	if ((rotating || translating || waiting) && (self->spawnflags & SPAWNFLAG_RT_TOGGLE))
	{
		if (rotating)
		{	// Note: The SPAWNFLAG_RT_ROTATING flag is left ON to indicate that we're paused.

			VectorClear (self->avelocity);
			self->touch = NULL;
		}

		if (translating)
		{	// Note: The SPAWNFLAG_RT_TRANSLATING flag is left ON to indicate that we're paused.
			VectorClear (self->velocity);
		}

		if (waiting)
		{	// Note: The SPAWNFLAG_RT_WAITING flag is left ON to indicate that we're paused.

		// If a func_rotrain gets triggered off in the middle of
		// a wait at a path_corner, we need know how much of the wait remained.
		// previously, a waiting func_rotrain triggered back on would re-wait the
		// entire wait time instead of only what remained. this change is needed
		// if we're to group func_trains and func_rotrains together into machinery
		// that can be triggered on and off.

		// we can save the remaining time in last_move_time, since that isn't used for anything
		// else by func_rotrains. if we're triggering off and NOT inside a wait, then
		// the normal translation-restoring code will ignore last_move_time.

			self->last_move_time = self->nextthink - level.time;	// remaining time to next think
		}

		self->nextthink = 0;
		self->s.sound = 0;		// kill the sound. everything's stopped.
		return;
	}

	if (rotation_paused || translation_paused || waiting_paused)
		rotrain_resume(self);
}


void func_rotrain_find (edict_t *self)
{
	edict_t *path;

	if (!self->target)
	{
		gi.dprintf ("rotrain_find: no target\n");
		return;
	}

	path = G_PickTarget (self->target);
	if (!path)
	{
		gi.dprintf ("rotrain_find: target %s not found\n", self->target);
		return;
	}

	self->target = path->target;
	self->curpath = path;	// so we know which path_corner we're sitting on

	// Place rotrain at first path_corner
	VectorSubtract (path->s.origin, self->mins, self->s.origin);
	gi.linkentity (self);

	self->nextthink = 0;	// initialize. no thoughts set up yet.

	// if not triggered, start immediately
	if (!self->targetname)
		self->spawnflags |= (SPAWNFLAG_RT_ROTATING|SPAWNFLAG_RT_TRANSLATING);

	if (self->spawnflags & (SPAWNFLAG_RT_ROTATING|SPAWNFLAG_RT_TRANSLATING))	// movement starts now?
	{
		self->activator = self;
		rotrain_readpathcorner(self);
	}
}


void SP_func_rotrain (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;

	VectorClear (self->s.angles);
	self->blocked = rotrain_blocked;
	if (self->spawnflags & SPAWNFLAG_RT_BLOCK_STOPS)
		self->dmg = 0;
	else
		if (!self->dmg)
			self->dmg = 100;

	self->solid = SOLID_BSP;
	gi.setmodel (self, self->model);

	if (st.noise)
		self->moveinfo.sound_middle = gi.soundindex(st.noise);

	if (!self->speed)
		self->speed = 100;

	self->moveinfo.speed = self->speed;
	self->moveinfo.accel = self->moveinfo.decel = self->moveinfo.speed;

	self->use = rotrain_use;

	// RSP 080499 - add animation flags
	if (self->spawnflags & SPAWNFLAG_RT_ANIMATED)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & SPAWNFLAG_RT_ANIMATED_FAST)
		self->s.effects |= EF_ANIM_ALLFAST;

	gi.linkentity (self);

	if (self->target)
	{
		// start trains on the second frame, to make sure their targets have had
		// a chance to spawn
		self->nextthink = level.time + FRAMETIME;
		self->think = func_rotrain_find;
	}
	else
	{
		gi.dprintf ("func_rotrain without a target at %s\n", vtos(self->absmin));
		G_FreeEdict(self);
	}
}

// RSP 101599 - new searchlight entity

/*QUAKED func_searchlight  (1 0 0) (-8 -8 -8) (8 8 8) START_ON REVERSE

This acts like a searchlight, circling a spot of light clockwise against the
surrounding walls.

"targetname" - if present, searchlight is triggerable so you can turn it on and off
               if not present, searchlight is always on

"speed" - angular speed, default is 10
"style"
  1 - yellow
  2 - green
  3 - blue
  4 - blinking red
  5 - white (default)
  6 - black

If the START_ON spawnflag is set, the searchlight starts on.
If the REVERSE spawnflag is set, circle counterclockwise.
 */

void searchlight_think(edict_t *self)
{
	vec3_t forward,end;
	trace_t tr;

	self->s.angles[YAW] = anglemod(self->s.angles[YAW] - self->speed);	// turn
	AngleVectors (self->s.angles, forward, NULL, NULL);	// forward direction
	VectorNormalize(forward);
	VectorMA(self->s.origin,8000,forward,end);			// look for something to shine on
	tr = gi.trace (self->s.origin,NULL,NULL,end,self,MASK_SOLID);
	VectorMA (tr.endpos, -8, forward, end); // Back up
	VectorCopy(end,self->target_ent->s.origin);
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self->target_ent);
}


void searchlight_use(edict_t *self, edict_t *other, edict_t *activator)
{
	self->flags ^= FL_ON;		// toggle the on/off flag
	if (self->flags & FL_ON)	// turned it on
	{
		spawn_beacon(self);
		self->target_ent->s.effects = self->style;
		self->nextthink = level.time + FRAMETIME;
		self->think = searchlight_think;
	}
	else
	{
		G_FreeEdict(self->target_ent);	// kill the beacon
		self->target_ent = NULL;
		self->nextthink = 0;			// turned it off
	}
}


void searchlight_init(edict_t *self)
{
	if (self->targetname)				// can be triggered on/off
	{
		self->use = searchlight_use;	// so set the routine that does that
		if (!(self->spawnflags & SPAWNFLAG_SL_START_ON))	// start now?
			return;
	}
	searchlight_use(self, NULL, NULL);	// turn it on
	self->think = searchlight_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_func_searchlight(edict_t *self)
{
	if (self->speed == 0)
		self->speed = 10;	// speed
	self->speed /= 10;		// one-tenth to match speed keyword on func_rotating
	if (self->spawnflags & SPAWNFLAG_SL_REVERSE)	// counterclockwise?
		self->speed *= -1;
	switch (self->style)
	{
		case 1:
			self->style = EF_HYPERBLASTER;
			break;
		case 2:
			self->style = EF_BFG;
			break;
		case 3:
			self->style = EF_BLUEHYPERBLASTER;
			break;
		case 4:
			self->style = EF_SPINNINGLIGHTS;
			break;
		default:
		case 5:
			self->style = EF_PLASMA;
			break;
		case 6:
			self->style = EF_TRACKER;
			break;
	}

	// Start searchlights on the second frame, so the world has a chance to
	// settle down
	self->think = searchlight_init;
	self->nextthink = level.time + FRAMETIME;

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	gi.linkentity (self);
}
