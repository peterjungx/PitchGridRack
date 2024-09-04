# PitchGrid Modules for VCV Rack

## What is this? (TL;DR)

Okay, so let's assume you are the adventurous kind of musician who wants to know what lies beyond the standard tuning, maybe because you are excited by oriental music. Therefore, you want to go beyond equal temperament and want to try out, say, 1/4-comma Meantone (an important temperament in medeival times). And let's assume it was easy to do just that. Chances are you would not be too much satisfied with the result. While there might be multiple reasons as to why this might be so, one issue you definitely will stumble upon is that your instrument's timbre might not fit your new tuning. Tuning and timbre are commonly dealt with completely separately from each other, because it is easier to conceptualize and to work with, especially within your typical DAW. But in reality they are intimately connected. 

At this early stage, the PitchGrid modules aim at making the interaction between tuning and timbre explicitly experienceable. Select your tuning with the **Microtonal V/OCT Mapper** and change the timbre of the **Microtonal Hammond**, (a nine oscillator VCO module featuring the typical overtones of a Hammond organ, but with overtones matching a chosen tuning), and hear the interactions. 

## Intro

The tonal structure of Western music is two-dimensional, and is induced by the two different tone sizes as well as the desire to change keys. All notes are located on a 2-d lattice that I call the PitchGrid. On the PitchGrid, all equal temperaments (12-TET, 19-TET, 31-TET, etc.) and historically relevant regular temperaments (notably Pythagorean tuning and all Meantone temperaments) can be reproduced by a simple tilt of the axis of constant pitch. Non-conventional regular temperaments like the 5-limit tuning, which lift the assumption *octave equals 2/1 frequency ratio*, are also readily available. I am dedicated to the exploration of the implications of this realization. Visit the [PitchGrid Web App](https://pitchgrid.io) to get a taste. 

To make my theoretical explorations audible and playable for the practicing musician, I am here releasing some VCV Rack modules that implement some of the ideas. The goal is both educational and practical. I believe there is a whole unexplored universe lying beyond the twelve semitones per octave idea that we Westerners are raised with. I can't possibly explore that universe on my own. I want to build tools that make it easy for you, the musician, to transgress the boundaries, and to join me on my journey. The first step is to make you see (or rather, hear) that there indeed is the unknown hidden in plain sight. This is what this VCV Rack modules are about today. They eventually will evolve into practical tools, with your valuable feedback. 

## VCV Rack Modules

Tuning and timbre are intimately connected. The PitchGrid Modules for VCV Rack allow you to explore this intimate connection in a (virtual) modular setting. 

### Microtonal V/OCT Mapper

By convention, modular VCO's frequency are controlled by a 1V/octave signal, with a strictly exponential mapping of voltage to frequency. The **Microtonal V/OCT Mapper** will adjust this V/OCT signal in such a way that makes it easy to match the pitches of a selected temperament. The output will be a mapped V/OCT signal, or **MV/OCT**, for short.

In any tuning besides 12-TET, F# and Gb have different pitches. Thus, there is an ambiguity in mapping notes to the black keys on a piano keyboard: Shall the black key between F and G be assigned an F# or a Gb? When coming from a MIDI controller with a piano keyboard, a press of that black key above middle C will send the MIDI note number 66, and your MIDI to CV interface will translate it to a voltage of +0.5V (with 0V being the middle C, note number 60) on V/OCT. The **Microtonal V/OCT Mapper** will map that voltage to a slightly different one, depending on the chosen tuning, such that a standard VCO receiving the module's **MV/OCT** output will play the tuned pitch corresponding to either F# or Gb, depending on the chosen keyboard mapping for the black keys.

#### Usage

- Select one of six black key mappings from the context menu (right-click on the module):
  - **All flat (Gb)** Black keys are Db Eb Gb Ab Bb
  - **F#** Black keys are Db Eb F# Ab Bb (default)
  - **C#** Black keys are C# Eb F# Ab Bb
  - **G#** Black keys are C# Eb F# G# Bb
  - **D#** Black keys are C# D# F# G# Bb
  - **All sharp (A#)** Black keys are C# D# F# G# A#
- Select a tuning from the context menu (right-click on the module):
  - **Original Hammond (12-TET)** The original tuning of the Hammond organ. What we all are used to.
  - **19-TET** In 19-TET, a half step gets assigned 2/19-th of an octave and the whole step 3/19-th. Close to 1/3-comma meantone.
  - **31-TET** A half step gets assigned 3/31-th of an octave and the whole step 5/31-th. Close to 1/4-comma meantone.
  - **Pythagorean** The octave gets assigned the just frequency ratio 2/1 and the perfect fifth the just frequency ratio 3/2. The origin of all Western music. Good tuning to keep the perfect 5th stable.
  - **1/4-comma meantone** The octave gets assigned the frequency ratio 2/1 and the major third 5/4. Good for pieces in major. Historically relevant in medeival music.
  - **1/3-comma meantone** The octave gets assigned the frequency ratio 2/1 and the minor third 6/5. Good for pieces in minor.
  - **5-limit (1/2-comma Cleantone)** The perfect fifth is 3/2 and the major third is 5/4. Consequently the minor third becomes just, too, at 6/5. All (non-inverted) triads and 7th chords are just(!). Octave is ~10ct wider than just 2/1. Mathematically proven to be the *best sounding temperament for Western music* (according to some measures) in the [Musical Tonality Paper](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=4452394) by Hans-Peter Deutsch.
  - **7-limit (P5=3/2 m3=7/6)** Experimental tuning in which all triads are just with 7th-harmonic based just intervals (minor 3rd =7/6 and Major 3rd =9/7). The octave becomes narrower than just 2/1 by about 20ct. Has distinctly Kafkaesque qualities.


### Microtonal Hammond

Both the pitches and the timbre of Hammond organs are (to a good approximation) tuned to 12-TET. It is commonly assumed that the magic of the Hammond organ derives from the slight deviations from the precise equal temperament, that are dictated by the gear ratios driving the tone wheels (besides of course from the characteristic clickiness of note onsets and the rotary speakers, which we are not concerned with here). It is however my hypothesis that a lot of the magic derives from the fact that the partials (controlled by the drawbar) are tuned to the scale played. (In fact, in the Hammond Organ, they derive from exactly the same sound source, the rotating tone wheels.) 

If this hypothesis is true, the original Hammond timbre can only sound good when played in 12-TET. If you want to play other temperaments and want the Hammond sound to still do its magic, you necessarily have to adjust the timbre, as well. The **Microtonal Hammond** module does just that.

#### Usage

- Adjust the eight amplitude trimpots that correspond to eight of the nine drawbars on the Hammond (I have spared the one for the fundamental, that I quietly assume you always want to hear at full power). These represent:
  - **1/2** Sub-harmonic, -1 octave from the fundamental
  - **3/2** ~3rd harmonic of the sub-harmonic octave, +perfect fifth from the fundamental
  - **2** 2nd harmonic of the fundamental, +1 octave from the fundamental
  - **3** ~3rd harmonic of the fundamental, +perfect fifth from 1st octave
  - **4** 4th harmonic of the fundamental, +2 octave from the fundamental
  - **5** ~5th harmonic of the fundamental, +major third from 2nd octave
  - **6** ~6th harmonic of the fundamental, +perfect fifth from 2nd octave
  - **8** 8th harmonic of the fundamental, +3 octave from the fundamental
- Select a tuning from the context menu (right-click on the module):
  - **Original Hammond (12-TET)** The original tuning of the Hammond organ. What we all are used to.
  - **19-TET** In 19-TET, a half step gets assigned 2/19-th of an octave and the whole step 3/19-th. Close to 1/3-comma meantone.
  - **31-TET** A half step gets assigned 3/31-th of an octave and the whole step 5/31-th. Close to 1/4-comma meantone.
  - **Pythagorean** The octave gets assigned the just frequency ratio 2/1 and the perfect fifth the just frequency ratio 3/2. The origin of all Western music. Good tuning to keep the perfect 5th stable.
  - **1/4-comma meantone** The octave gets assigned the frequency ratio 2/1 and the major third 5/4. Good for pieces in major. Historically relevant in medeival music.
  - **1/3-comma meantone** The octave gets assigned the frequency ratio 2/1 and the minor third 6/5. Good for pieces in minor.
  - **5-limit (1/2-comma Cleantone)** The perfect fifth is 3/2 and the major third is 5/4. Consequently the minor third becomes just, too, at 6/5. All (non-inverted) triads and 7th chords are just(!). Octave is ~10ct wider than just 2/1. Mathematically proven to be the *best sounding temperament for Western music* (according to some measures) in the [Musical Tonality Paper](https://papers.ssrn.com/sol3/papers.cfm?abstract_id=4452394) by Hans-Peter Deutsch.
  - **7-limit (P5=3/2 m3=7/6)** Experimental tuning in which all triads are just with 7th-harmonic based just intervals (minor 3rd =7/6 and Major 3rd =9/7). The octave becomes narrower than just 2/1 by about 20ct. Has distinctly Kafkaesque qualities.

## Building

Follow the build instructions for the [VCV Rack plugins](https://vcvrack.com/manual/Building#Building-Rack-plugins).
