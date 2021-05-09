Installation :
--------------
I apologize for the extra step and this is only needed to check out
the .bsp I included. I don't know if the loops I used to test it are
in the PAK, Biz sent me them but I don't know if they are finished or
not... copy the bizloops directory into your luc game directory under
sound (so, if your luc directory is tnt_luc and it is located under
c:\quake2\tnt_luc, you want the .wav files to be under :
c:\quake2\tnt_luc\sound\bizloops\ ) The reason I didn't use sounds
in the PAK was I'm about to go to bed and I don't think I can stay 
awake long enough to time some sounds in the pak and update the
sequencer and speakers... Sorry :-)

Then, open up spkseq_ents.c and copy that into you ents.c file if you
want to play around with your own speaker sequencers. One note,
The ent description is set up to work in regular mode (the sequencer
triggers a list of speakers at specified time intervals) AND has
the options for SSM (Single Speaker Mode) in which 1 speaker
plays the specified sounds at specified times, with their own
specified volumes and attenuations. SSM MODE IS NOT IMPLEMENTED,
DON'T TRY IT, IT MAY DO NOTHING OR BOMB!

Here's how it works :
---------------------
SpawnFlags :
 1 :	Start On - starts the sequencer on and doesn't need to be
	triggered
 2 :	SSM - Single Speaker Mode
 4 :  RELIABLE - Whether the sound is reliable or not, Only used in
	SSM. All sounds in sequence will be reliable or not, no
	individual per sound setting...

Key / Value Pairs :
	"target"		List of speakers to activate 
				In Single Speaker Mode the speaker to use
	"pathtarget"	Time to wait before triggering the next
				speaker.
				In Single Speaker Mode Time to wait before 				triggering the next sound
	"message"		wav files to play if in SINGLE_SPEAKER_MODE
	"combattarget"	Attenuation if in SINGLE_SPEAKER_MODE
					-1 = none, send to whole level
					1 = normal fighting sounds
					2 = idle sound level
					3 = ambient sound level
	"killtarget"	Volume 0.0 to 1.0 if in SINGLE_SPEAKER_MODE

To Use:
-------
(1) Make various speakers and set them up with NO LOOPING!!!!! Well, you
can use looping but then the sequencer will turn them on and off each
time it triggers a speaker so your speaker will turn on and start
looping the first time the sequencer fires and will not turn off 
until it starts the sequence over an triggers that speaker again. If
"Looped On" mode is on the speaker, the speaker will start on and be
triggered off the 1st time the sequencer fires and then back on the
next time... Pretty useless but feel free to exploit it if 
possible...
(2) Add a func_speaker_sequencer
(3) Set the "target" key to a comma seperated list of speaker targetnames
(ex. if you have speakers with target names of "one", "two", "three" and "four",
your target key value would be : "one,two,three,four". Put the speaker names in
the order you want them triggered.
(4) Set the "pathtarget" key to a comma seperated list of times to wait to trigger
the next speaker. These should be the lengths in seconds of the .wav but they don't
have to be. If your speakers have sounds with the following lengths : 2 seconds, 
3 seconds, 1 second, 6 seconds. Your pathtarget value would be : "2,3,1,6"
(5) Compile the map and test away!

Please report any problems or questions to jminadeo@nforce.com or the LUC Proj 
mailing list :-)

--
John Minadeo
jminadeo@nforce.com