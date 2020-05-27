#include <Streaming.h>
//#include <FSM.h>

// following:
// https://en.wikipedia.org/wiki/Elementary_cellular_automaton

byte Rule = 110;
//byte Rule = 54;
const int nCells = 40;
boolean cells[nCells] = {false};

void setup() {
  delay(300);
  Serial.begin(115200);
  Serial << endl << endl;
  
  randomStart();
}

void randomStart() {
  int i = random(0,nCells);
  cells[i] = 1;  
}

void loop() {
  // run a generation
  boolean nextGen[nCells] = {false};
  boolean allDead = true;
  for( int c=0; c<nCells; c++ ) {
    int l = c==0 ? (nCells-1) : (c-1)%nCells; // wrap
    int r = c==(nCells-1) ? 0 : (c+1)%nCells; // wrap

    byte currentPattern = 0;
    bitWrite(currentPattern, 2, cells[l]);
    bitWrite(currentPattern, 1, cells[c]);
    bitWrite(currentPattern, 0, cells[r]);

    boolean newState = bitRead(Rule, currentPattern);
    nextGen[c] = newState;
    if( newState ) allDead = false;
/*
    Serial << cells[l] << " " << cells[c] << " " << cells[r];
    Serial << " = " << String(currentPattern, BIN);
    Serial << " -> " << newState << endl;
*/
  }
  memcpy(cells, nextGen, nCells);
  if( allDead ) randomStart();
  
//  while(1) yield();
  
  // print
  for( int i=0; i<nCells; i++ ) Serial << cells[i] << " ";
  if( allDead ) Serial << " !";
  Serial << endl;

  delay(50);
}
