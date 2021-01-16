# teensy-midi-looper

A simple MIDI looper with external sync

## Features
- [x] Receive/send tempo
- [x] Receive/send midi notes
- [x] Start/stop recording
- [x] Clear recorded notes
- [ ] Change pattern length
- [ ] Show pattern length
- [ ] Record CC messages

## Parts required

- `Teensy 3.2`
- 2 Midi input circuits
  - 2 MIDI connectors
  - 2 `6N138` optocouplers
  - 2 diodes (`1N4148`)
  - 2 `220Ω` resistors
  - 2 `470Ω` resistors 
- 1 MIDI output circuit
  - 1 MIDI connector
  - 2 `47Ω` resistors
- Pushbutton for reset
- A LED for recording indicator
  - Suitable pull-up resistor (`220Ω?`)

 ## Schematics

 A good schematic on the MIDI input and output circuits can be found from [Deftaudio MIDI Teensy page](https://github.com/Deftaudio/Midi-boards/blob/master/MIDITeensy3.2/MIDITeensy3.2_sch.pdf)

A rough diagram of the complete circuit

![Simple schema](https://raw.githubusercontent.com/jjylik/teensy-midi-looper/main/simpleschema.png)