#include <MIDI.h>
#include <Bounce.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI1);
MIDI_CREATE_INSTANCE(HardwareSerial, Serial2, MIDI2);

const int RESET_BUTTON_PIN = 2;
const int RECORD_LED_PIN = 3;
Bounce pushbutton = Bounce(RESET_BUTTON_PIN, 10);  // 10 ms debounce

const int LOOP_BARS = 4;

int notesOnCount = 0;
int tempoMessageCount;
boolean looping = false;
boolean recording = false;
int currentBar = 0;
boolean onRecordingLoop = true;
boolean notifyClear = false;
int blinkCounter = 0;
byte ledStatus = HIGH;

byte noteOnOnTempo[386][16];
byte noteOffOnTempo[386][16];
int noteStartPosOffset[200];

void setup() {
  Serial.begin(115200);
  MIDI1.begin(MIDI_CHANNEL_OMNI);
  MIDI2.begin(MIDI_CHANNEL_OMNI);
  tempoMessageCount = 0;
  pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RECORD_LED_PIN, OUTPUT);

}

void handle_midi_start_stop(midi::MidiType mtype, byte data1, byte data2) {
    if (mtype == midi::Start) {
       Serial.println("Start");
       tempoMessageCount = 0;
       looping = true;
    } else if (mtype == midi::Continue) {
      Serial.println("Continue");
       looping = true;
    } else if (mtype == midi::Stop) {
       Serial.println("Stop");
       looping = false;
       clear_notes();
    }
}

void clear_notes() {
   // stop all messages
   MIDI1.send((midi::MidiType)0xB0, 0x78, 0, 2);
   MIDI1.send((midi::MidiType)0xB0, 0x7B, 0, 2);
   memset(noteStartPosOffset, 0, sizeof(noteStartPosOffset));
   for (int i = 0; i < 386; i++) {
      memset(noteOffOnTempo[i], 0, sizeof(noteOffOnTempo[i]));
      memset(noteOnOnTempo[i], 0, sizeof(noteOnOnTempo[i]));
   }
}

void handle_midi_note(midi::MidiType mtype, byte data1, byte data2) {
    if (!recording) {
      return;
    }
    if (mtype == midi::NoteOff) {
      int note = (int) data1;
      int quantizedOffset = noteStartPosOffset[note] + tempoMessageCount;
      append_to_list(noteOffOnTempo[quantizedOffset], data1);
      if (notesOnCount > 0) {
            notesOnCount--;
      }
    } else if (mtype == midi::NoteOn) {
       int quantized = quantize_note(tempoMessageCount);
       int note = (int) data1;
       noteStartPosOffset[note] = quantized - tempoMessageCount;
       append_to_list(noteOnOnTempo[quantized], data1);
       notesOnCount++;
    }
}

void append_to_list(byte list[], byte value) {
  for (int i = 0; i < 16; i++) {
    if (list[i] == 0) {
      list[i]= value;
      break;
    }
  }
}

int quantize_note(int n) {
  return (n/8 + (n%8>2)) * 8;
}

void loop() {
  read_record_button();

  // MIDI from sync port
  read_sync_midi();

  read_device_midi();
}

void read_device_midi() {
  // MIDI from the device
  if (MIDI1.read()) {
    byte type = MIDI1.getType();
    byte data1 = MIDI1.getData1();
    byte data2 = MIDI1.getData2();
    midi::MidiType mtype = (midi::MidiType)type;
    if (type != midi::Clock) {
      handle_midi_note(mtype, data1, data2);
    }
  }
}

void read_sync_midi() {
  if (MIDI2.read()) {
    byte type = MIDI2.getType();
    byte channel = MIDI2.getChannel();
    byte data1 = MIDI2.getData1();
    byte data2 = MIDI2.getData2();
    midi::MidiType mtype = (midi::MidiType)type;
    if (mtype != midi::SystemExclusive) {
      MIDI1.send(mtype, data1, data2, channel);
    } else {
      unsigned int SysExLength = data1 + data2 * 256;
      MIDI1.sendSysEx(SysExLength, MIDI2.getSysExArray(), true);
    }
    handle_midi_start_stop(mtype, data1, data2);
    if (mtype == midi::Clock) {
      blink_recording_led();
      if (looping) {
        tempoMessageCount++;
        // if note message on current sync message, write it to midi out
        if (notesOnCount == 0 && noteOnOnTempo[tempoMessageCount][0] != 0) {
          for (int i = 0; i < 16; i++) {
            byte toSend = noteOnOnTempo[tempoMessageCount][i];
            if (toSend == 0) {
              break;
            }
            MIDI1.send((midi::MidiType)0x90,toSend , 0x78, 2);
          }
        }
        if (notesOnCount == 0 && noteOffOnTempo[tempoMessageCount][0] != 0) {
          for (int i = 0; i < 16; i++) {
            byte toSend = noteOffOnTempo[tempoMessageCount][i];
            if (toSend == 0) {
              break;
            }
            MIDI1.send((midi::MidiType)0x80,toSend , 0x78, 2);
          }
        }
        // One bar == 96 sync messages
        if (tempoMessageCount >= 96*LOOP_BARS) {
           tempoMessageCount = 0;
        }
      }
    }
  }
}

void blink_recording_led() {
   if (blinkCounter > 0) {
        if (ledStatus == HIGH) {
          digitalWrite(RECORD_LED_PIN, LOW);
          ledStatus = LOW;
        } else {
          digitalWrite(RECORD_LED_PIN, HIGH);
          ledStatus = HIGH;
        }
        if (blinkCounter == 1 && recording) {
          ledStatus = HIGH;
          digitalWrite(RECORD_LED_PIN, HIGH);
        } else if (blinkCounter == 1 && !recording) {
          ledStatus = LOW;
          digitalWrite(RECORD_LED_PIN, LOW);
        }
        blinkCounter--;
      }
}


//
// TODO move to own file
//
unsigned int count = 0;
unsigned long countAt = 0;
unsigned int countPrinted = 0;

#define shortTime 500
#define longTime 900

unsigned long buttonPressStartTimeStamp;
unsigned long buttonPressDuration = 0;
boolean startTimeout = false;
boolean endTimeout = false;

void read_record_button()
{
  pushbutton.update();
  if (pushbutton.fallingEdge())
  {
    buttonPressStartTimeStamp = millis();
    startTimeout = true;
  }
  if (pushbutton.risingEdge())
  {
    buttonPressDuration = (millis() - buttonPressStartTimeStamp);
    startTimeout = false;
  }

  // start / stop recording on normal press
  if (buttonPressDuration > 0 && buttonPressDuration <= shortTime)
  {
    recording = !recording;
    if (recording) {
       digitalWrite(RECORD_LED_PIN, HIGH);
    } else {
       digitalWrite(RECORD_LED_PIN, LOW);
    }
    buttonPressDuration = 0;
  }

  // clear notes on long press and blink led
  if (startTimeout == true && (millis() - buttonPressStartTimeStamp) > longTime)
  {
    clear_notes();
    blinkCounter = 6;
    startTimeout = false;
    buttonPressDuration = 0;
  }

}
