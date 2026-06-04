const int REC_BUTTON = 2;
const int GATE_IN = 3;
const int GATE_OUT = 4;
const int LED = 5;
const int TRIG_OUT = 6;

const uint8_t MODE_PLAY = 0;
const uint8_t MODE_REC_READY = 1;
const uint8_t MODE_RECORDING = 2;

uint8_t buffer[1024];
uint16_t length = 0;
uint8_t mode = MODE_PLAY;
bool looping = true;

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(REC_BUTTON, INPUT_PULLUP);
  pinMode(GATE_IN, INPUT_PULLUP);
  pinMode(GATE_OUT, OUTPUT);
  pinMode(LED, OUTPUT);
  pinMode(TRIG_OUT, OUTPUT);
}

unsigned long last_t = 0;
uint16_t loop_time = 0;

uint8_t setBuffer( int t, bool val ) {
  uint16_t buffer_index = loop_time >> 6;
  uint8_t buffer_bit = 1 << ((loop_time >> 3) & 0b111);
  if( val ) {
    buffer[buffer_index] |= buffer_bit;
  } else {
    buffer[buffer_index] &= ~buffer_bit;
  }
}

uint8_t getBuffer( int t ) {
  uint16_t buffer_index = loop_time >> 6;
  uint8_t buffer_bit = 1 << ((loop_time >> 3) & 0b111);
  return buffer[buffer_index] & buffer_bit;
}

void setGate( bool val ) {
  if( val ) {
    digitalWrite( GATE_OUT, HIGH );
    digitalWrite( LED, HIGH );
  } else {
    digitalWrite( GATE_OUT, LOW );
    digitalWrite( LED, LOW );
  }
}

void setLED( bool val ) {
  if( val ) {
    digitalWrite( LED, HIGH );
  } else {
    digitalWrite( LED, LOW );
  }
}

bool last_gate = 0;
bool last_rec = 0;

uint16_t last_gate_start = 0;

// the loop function runs over and over again forever
void loop() {

  // wait for the next tick
  unsigned long t = millis();
  if( t == last_t ) {
    return;
  }
  last_t = t;

  bool gate = !digitalRead(GATE_IN);
  bool gate_start = gate && !last_gate;
  last_gate = gate;

  bool rec = !digitalRead(REC_BUTTON);
  bool rec_start = rec && !last_rec;
  last_rec = rec;

  switch( mode ) {
    case MODE_PLAY:

      if( rec_start ) {
        mode = MODE_REC_READY;
        break;
      }

      if( gate_start ) {
        loop_time = 0;
      } else if( loop_time >= length ) {
        if( looping ) {
          loop_time = 0;
        } else {
          break;
        }
      }

      setGate( getBuffer( loop_time ) );
      loop_time = loop_time + 1;

      break;
    case MODE_REC_READY:

      setGate( gate );
      setLED( t & 0x0080 );

      if( gate_start || rec_start ) {
        mode = MODE_RECORDING;
        loop_time = 1;
        last_gate_start = 0;
        setBuffer( 0, gate );
      }

      break;
    case MODE_RECORDING:

      uint16_t dt = loop_time - last_gate_start;

      if( rec_start ) {
        if( gate ) {
          length = last_gate_start;
          if( dt < length ) {
            loop_time = dt;
          } else {
            loop_time = 0;
          }
        } else {
          length = loop_time;
          loop_time = 0;
        }
        looping = true;
        mode = MODE_PLAY;
        setGate( getBuffer( loop_time ) );
        break;
      }

      if( dt > 3000 ) {
        length = loop_time;
        looping = false;
        mode = MODE_PLAY;
        setGate( false );
        break;
      }

      if( gate_start ) {
        last_gate_start = loop_time;
      }

      setGate( gate );
      setLED( !gate );
      setBuffer( loop_time, gate );
      loop_time += 1;

      break;
  }

}
