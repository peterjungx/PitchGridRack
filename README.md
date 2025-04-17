# PitchGrid Modules for VCV Rack

The diversity in the area of tunings is rapidly shrinking: Everything is converging toward the 12-TET (mediated by the piano keyboard). PitchGrid is an answer to this cultural decay. We need tools that make it easy to play in arbitrary tunings.

The [PitchGrid](https://pitchgrid.io/scalemapper) generalizes the structure within Western tonality, including diatonic scales, harmonies, key changes, notation, to a much larger class of scales. The PitchGrid Modules for VCV Rack allow you to explore the resulting structures in an intuitive and powerful way.

If you have an [Exquis](https://dualo.com/exquis/?utm_source=VCVplugin) by Intuitive Instruments at hand, you are lucky. As an MPE controller with an isomorphic keyboard layout, it is a perfect fit for exploring a universe of scales. Together with the **Microtonal Exquis** module, selecting scales, re-tuning and tempering them, as well as configuring the note layout -- all via the Exquis' interface -- is a breeze. It also allows you to interactively retune, transpose and scramble melodies and harmonies in a structurally sensible way. 

To make arbitrary scales sound pleasing, use **reverse tuning**. Normal tuning is finding pitches that fit a given overtone spectrum, as in the Pythagorean tuning procedure. **Reverse tuning** starts with a tuned scale, and adjusts the spectrum of the sound to fit the chosen scale, leading to consonance. This results in unusual timbres. This is a departure from the usual way, where we think of tuning and timbre separately. The PitchGrid modules make the intimate connection between tuning and timbre directly experienceable. 

The **Microtonal Hammond** module is an organ-like VCO featuring nine tunable sine waves. Tuning is achieved either by patching with the **Microtonal Exquis** module, which forwards scale and tuning information via the _TDAT_ connector, or via selecting one of several presets for the Western scale (see below). At this stage, if you don't have the [Exquis](https://dualo.com/exquis/?utm_source=VCVplugin), you still can experience some preconfigured tunings for the Western scale and see how tuning and timbre interact. Use the **Microtonal V/OCT Mapper** to map MIDI notes to a tempered **V/OCT** signal.

More VCV Modules featuring microtonality and reverse tuning are planned. Stay tuned.

## About the PitchGrid Concept

The tonal structure of Western music is two-dimensional, and is induced by the two different tone sizes as well as the desire to change keys. All notes are located on a 2-d lattice, the PitchGrid. On the PitchGrid, all equal temperaments (12-TET, 19-TET, 31-TET, etc.) and historically relevant regular temperaments (notably Pythagorean tuning and all Meantone temperaments) can be reproduced by a simple tilt of the axis of constant pitch. (see the [PitchGrid Web App](https://pitchgrid.io/)) Non-conventional regular temperaments like the 1/2-comma Cleantone temperament, which lift the assumption *octave equals 2/1 frequency ratio*, are also readily available. 

But most importantly, you now can leave the Western tonal universe altogether and explore what lies beyond it. The [PitchGrid ScaleMapper Web App](https://pitchgrid.io/scalemapper) showcases not only a unified framework for thinking about regular scales, but also devises a new and unique way of mapping between any of those scales.

To make my theoretical explorations audible and playable for the practicing musician, I am here releasing some VCV Rack modules that implement some of the ideas. The goal is both educational and practical. I believe there is a whole unexplored universe lying beyond the twelve semitones per octave idea that we Westerners are raised with. I can't possibly explore that universe on my own. I want to build tools that make it easy for you, the musician, to transgress the boundaries, and to join me on my journey. The first step is to make you see (or rather, hear) that there indeed is the unknown hidden in plain sight. This is what this VCV Rack modules are about today. They eventually will evolve into practical tools, with your valuable feedback. 

## VCV Rack Modules

### Microtonal Exquis

The PitchGrid Microtonal Exquis (or MicroExquis for short) is a module in the VCV Rack PitchGrid plugin. Its purpose is to provide a means to explore different tunings, scales and hexagonal keyboard layouts via the [Exquis MPE controller by Intuitive Instruments](https://dualo.com/exquis/?utm_source=VCVplugin).


- Supported FW Version: 2.1.3
- Acts as MTS-ESP Master
- Scale and scale mode selection (all MOSes and more)
- Consistent Tuning of Scales
- Scale layout on Hex-Isomorphic Exquis Keyboard

See the [Documentation for the MicroExquis](docs/PitchGrid%20MicroExquis.pdf) for details.


### Microtonal Hammond

A Hammond Orgcodan is a perfect model system for a tunable timbre. The overtones are, to a very good approximation, tuned to a 12-TET temperament, the same as the available pitches. In fact, the sound source of the overtones and the fundamentals are the same by construction. Arguably this is what makes the sound of the Hammond Organ special: Its timbre better fits the 12 tone equal temperament than an harmonic spectrum. (In a harmonic spectrum, the 3rd, 5th and 6th overtones are just ratios. On the Hammond, they are derived from the Perfect 5th and the Major 3rd from the above octaves of the 12-TET the organ is tuned to.) In this sense, the Hammond organ already employs reverse tuning to the 12-TET.

The **Microtonal Hammond** allows to reverse tune its overtones (controlled by the "drawbar" knobs) to an arbitrary scale, via the _TDAT_ connector.  

#### Usage

- Adjust the eight amplitude trimpots that correspond to eight of the nine drawbars on the Hammond (I have spared the one for the fundamental, that I quietly assume should be at full power). These represent:
  - **1/2'** Sub-harmonic, -1 octave from the fundamental
  - **3/2'** ~3rd harmonic of the sub-harmonic octave, +perfect fifth from the fundamental
  - **2'** 2nd harmonic of the fundamental, +1 octave from the fundamental
  - **3'** ~3rd harmonic of the fundamental, +perfect fifth from 1st octave
  - **4'** 4th harmonic of the fundamental, +2 octave from the fundamental
  - **5'** ~5th harmonic of the fundamental, +major third from 2nd octave
  - **6'** ~6th harmonic of the fundamental, +perfect fifth from 2nd octave
  - **8'** 8th harmonic of the fundamental, +3 octave from the fundamental
- Connect the _TDAT_ connector from the _MicroExquis_ or select a tuning from the context menu (right-click on the module).
  
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
- Select one of the available tunings from the context menu (right-click on the module).


## Building

Follow the build instructions for the [VCV Rack plugins](https://vcvrack.com/manual/Building#Building-Rack-plugins).
