bool clearUFO = false;
bool open = false;
bool close = false;
bool noFog = true;


// ****************** things of note *********************
// outer rings = 2x300 long
// vent lights = 4x18 long
// ramp lights = 68 long each, parrallel
// dome lights = 154 long  

// add parameters in to the functions
// address the gap at the back on the ring LEDs
// set LEDs by group - ring, dome, 4xjets, panels, ramp 

// ******* todo *********
// when there isnt fog - wait - repeat until fog ready 
// serial commands to stop running and clear the vents and idle
// functions/effects needed:
//    flying rings - + minor adjustment thrusts
//    landing rings - rapid 4 adjustment thrusts
//    take off rings - full on flamey thrust
// colors - gamma, remapping, and overall brightness control

// see the rings light up with fixed steer x & y
// sames for the dome
// same for the thrusters
// test perlin


// rough schedule
//    fly = 5 mins - until smoke ready
//    land = 30 seconds
//    park = 3 mins
//    take off = 30 seconds

// tue todo

// laser broken
// blue twinklies or rotating rings when strobing
  // mnake the blue colors to strobes & twinkles
// red twinklies rings when thusting
// put a println that prints alive every 5 seconds - it keeps the serial port active
// make sure ramp is up on power up 
// a new simple spinning ring sequence
// tune the effect of the landing seq
  // run the fans longer on landing to clear the smoke
  // ramp up front light to show the aliens face
  // light up the alien face for a green strobe take off
// ensure that fog is ready for the landing
  // make a 45 second busy banging period with the dome light and bay light and relays clacking
  // contingent on fog ready ideally
// check all end times in seqs
// make flying change colors

// done - finish flying thrusters
// done - get the fog production right for the thrusters
// done - find the active printlns
// done - set the 0 point for the dome lights to be at the back.
// done - bugs on flying seq
// done - fans not shutting off after ramp seq

#include <OctoWS2811.h>
#include <Wire.h>
#include <FastLED.h> // for the noise functions



enum relayPos {NC,NO};
relayPos relayState[16] = {NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC};
enum rampMode {OFF,DOWN,UP};

const int ledsPerStrip = 300;

DMAMEM int displayMemory[ledsPerStrip*6];
int drawingMemory[ledsPerStrip*6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

int Black = leds.color(0, 0, 0); 

int Red = leds.color(128, 0, 0); 
int Yellow = leds.color(255, 255, 0);  
int Blue = leds.color(0,0,255);
int White = leds.color(255,255,255);
int Green = leds.color(0,255,0); 
int Orange = leds.color(255,96,0); 
int Purple = leds.color(192,0,255); 

int hRed = leds.color(128, 0, 0); 
int hYellow = leds.color(128, 128, 0);  
int hBlue = leds.color(0,0,128);
int hWhite = leds.color(128,128,128);
int hGreen = leds.color(0,128,0);
int hOrange = leds.color(128,48,0); 
int hPurple = leds.color(96,0,128); 

int fogPin = 23;
int fanLocs[8] = {7,6,5,4,3,2,1,0};
int fogLoc = 8;
int laserLoc = 11;
int rampLoc[2] = {14,15};

int ledCH0 = ledsPerStrip * 2;  // ring swapped
int ledCH1 = ledsPerStrip * 1;  // ring
int ledCH2 = ledsPerStrip * 0;  // ring swapped
int ledCH3 = ledsPerStrip * 3;  // ring
int ledCH4 = ledsPerStrip * 4;  // dome
int ledCH5 = ledsPerStrip * 5;  // all 4 ducts/fans + a 64bay ligth leds
int ledCH6 = ledsPerStrip * 6;  // ramp
int ledCH7 = ledsPerStrip * 7;  // ramp

struct Event {
  unsigned long startTime;
  unsigned long endTime;
  void (*startFunction)();
  void (*endFunction)();
  bool continuous = false;   
  bool startHasRun = false; 
  bool endHasRun = false; 
};
const bool CONT = true;

struct Sequence {
  Event* events;
  int numEvents;
  unsigned long duration;
  int repeatCount;         
  int currentCount;       
};

unsigned long seqStart = 0;
int currentSequenceIndex = 0;

// **************************************** All of the functions below are schedulable effects **************************************** 
void fogOn() {
  blowFog(true);
}

void fogOff() {
  blowFog(false);
}

int rndDuctNum = 0;
boolean rndDuctNumSet = false;
void randomDuctFanOn() {
  if (!rndDuctNumSet) {
    rndDuctNum = random(4);
    rndDuctNumSet = true;
  }
  runFan(rndDuctNum, true);  
}

void randomDuctFanOff() {
  rndDuctNumSet = false;
  runFan(rndDuctNum, false);  
}

unsigned long rdlMS = 0;
void randomDuctLightOn() {
  if (millis() - rdlMS > 30) {
    int r = random(128);
    for (int j=0; j<18; j++) {
      leds.setPixel(ledCH5+j+(rndDuctNum*18),0,r,255);
    } 
    rdlMS = millis(); 
  }

}

void randomDuctLightOff() {
  for (int j=0; j<18; j++) {
    leds.setPixel(ledCH5+j+(rndDuctNum *18),0,0,0);
  }
}

void allDuctFansOff() {
  runFan(0, false);
  runFan(1, false); 
  runFan(2, false); 
  runFan(3, false);   
}

void allDuctFansOn() {
  runFan(0, true);
  runFan(1, true); 
  runFan(2, true); 
  runFan(3, true);   
}

void allDuctLightsOn() {
  for (int j=0; j<72; j++) {
    leds.setPixel(ledCH5+j,255,255,255);
  }
}

void rampBayLightsRandom() {
  for (int j=0; j<64; j++) {
    int c = leds.color(0,0,0);
    if (random(10)==0)
      c = leds.color(0,random(128),0);
    leds.setPixel(ledCH5+j+72,c);
  }
}

void rampBayLightsOff() {
  for (int j=0; j<64; j++) {
    leds.setPixel(ledCH5+72+j,0,0,0);
  }
}

unsigned long adlsMS = 0;
boolean adlsStrobe = false;
void allDuctLightsStrobe() {
  if (millis()-adlsMS > 50) {
    adlsStrobe = !adlsStrobe;
    adlsMS = millis();
  }  
  if (adlsStrobe) {
    for (int i=0; i<72; i++) {
      leds.setPixel(i+ledCH5,random(200)+56,random(64),0);
    }
  }
  else {
    for (int i=0; i<72; i++) {
      leds.setPixel(i+ledCH5,0,0,0);
    }  
  }
}

unsigned long rtMS = 0;
void ringTwinkle() {
  if (millis()-rtMS > 20) {
    for (int i=0; i<600; i++) {
      int c = Black;
      if (random(30)==0) 
        c = leds.Color(random(96), random(8), random(4));    
      leds.setPixel(topRingIdx(i),c);
      c = Black;
      if (random(30)==0)
        c = leds.Color(random(96), random(8), random(4));    
      leds.setPixel(botRingIdx(i),c);
    }
    rtMS = millis();
  }
}

unsigned long dtMS = 0;
void domeTwinkle() {
  if (millis()-dtMS > 20) {
    for (int i=0; i<154; i++) {
      int c = Black;
      if (random(30)==0) 
        c = leds.Color(random(200)+56, random(64),0);    
      leds.setPixel(ledCH4+i,c);
    }
    dtMS = millis();
  }
}


void allDuctLightsOff() {
  for (int j=0; j<72; j++) {
    leds.setPixel(ledCH5+j,0,0,0);
  }
}

void domeFanOn() {
  runFan(5, true);  
}

void domeFanOff() {
  runFan(5, false);  
}

void rampFanOn() {
  runFan(4, true);  
}

void rampFanOff() {
  runFan(4, false);  
}

void laserOn() {
  shineLaser(true);  
}

void laserOff() {
  shineLaser(false);  
}

void rampDown() {
  moveRamp(DOWN);  
}

void rampOff() {
  moveRamp(OFF); 
}

void rampUp() {
  moveRamp(UP); 
}

void domeLightsOn() {
  for (int i=0; i<154; i++) {
    leds.setPixel(i+ledCH4,0,32,0);
  }
}

void domeLightsOff() {
  for (int i=0; i<154; i++) {
    leds.setPixel(i+ledCH4,0,0,0);
  }
}

void rampLightsOn() {
  for (int i=0; i<68; i++) {
    leds.setPixel(i+ledCH6,0,1,0);
    leds.setPixel(i+ledCH7,0,1,0);
  }
}

void rampLightsOff() {
  for (int i=0; i<68; i++) {
    leds.setPixel(i+ledCH6,0,0,0);
    leds.setPixel(i+ledCH7,0,0,0);
  }
}

unsigned long rlmMS = 0;
int rlmP = 0;
void rampLightsMoving() {
  if (millis() - rlmMS > 15) {
    for (int i=0; i<68; i++) {
      leds.setPixel(i+ledCH6,0,0,0);
      leds.setPixel(i+ledCH7,0,0,0);
    }
    for (int j=0; j<10; j++) {
      int pp = rlmP-j;
      if (pp<0) pp+=68;
      leds.setPixel(pp+ledCH6,8,8,0);
      leds.setPixel(pp+ledCH7,8,8,0);  
    } 
    rlmP += 1;
    if (rlmP>=68) rlmP-=68;
    rlmMS = millis();

  }
}

unsigned long rrbMS;
int topRing[600];
int botRing[600];
boolean swap = false;
void randomRingBlips() {

  if (millis() - rrbMS > 30) {
    int idx = random(600);
    topRing[idx] = leds.Color(random(64), random(255), random(64)); 
    idx = random(600);
    botRing[idx] = leds.Color(random(64), random(255), random(64)); 

    for (int i = 0; i<600; i++) {
      int j=i;
      if (swap)
        j = 599-i;
      int p1 = (j+1)%600;
      int m1 = (j+600-1)%600;
      int leftColor = topRing[p1];
      int rightColor = topRing[m1];
      int blendedColor = blendColors(leftColor, rightColor);
      topRing[j] = blendedColor;
      leftColor = botRing[p1];
      rightColor = botRing[m1];
      blendedColor = blendColors(leftColor, rightColor);
      botRing[j] = blendedColor;
    }
    swap = !swap;

    for (int i = 0; i < 600; i++) {
      leds.setPixel(topRingIdx(i), topRing[i]);
      leds.setPixel(botRingIdx(i), botRing[i]);
    }
    rrbMS = millis();  
  }

}

void ringsClear() {
  for (int i = 0; i < 600; i++) {
    leds.setPixel(topRingIdx(i), 0,0,0);
    leds.setPixel(botRingIdx(i), 0,0,0);
  }  
}

int df = 0;
int db = 0;
unsigned long dcMS=0;
void domeCoast() {
  if (millis()-dcMS > 15) {
    for (int i=0; i<154; i++) {
      leds.setPixel(i+ledCH4,0,0,0);
    }
    for (int j=0; j<20; j++) {
      int ppf = (df+154-j)%154;
      int ppb = (db+j)%154;
      
      int lf = ppf+ledCH4;
      int lb = ppb+ledCH4;
      leds.setPixel(lf,0,0,255);
      leds.setPixel(lb,0,128,128);   
    } 
    df = (df+3)%154;
    db = (db+154-2)%154;
    dcMS = millis();
  } 
}

int rf = 0;
int rb = 0;
unsigned long rcMS=0;
void ringCoast() {
  if (millis()-rcMS > 3) {
    for (int i=0; i<600; i++) {
      int c = Black;
      if (random(100)==0) 
        c = leds.Color(0, random(8), random(32)+8);    
      leds.setPixel(topRingIdx(i),c);
      c = Black;
      if (random(100)==0)
        c = leds.Color(0, 0, random(40));    
      leds.setPixel(botRingIdx(i),c);
    }
    for (int j=0; j<100; j++) {
      int ppf = (rf+600-j)%600;
      int ppb = (rb+j)%600;
      float m = 1.0-((float)j/99.0);
      m = pow(m,4);
      m *= 128;
      
      leds.setPixel(topRingIdx(ppf),0,0,m);
      leds.setPixel(botRingIdx(ppb),0,m,m);   
    } 
    rf = (rf+5)%600;
    rb = (rb+600-4)%600;
    rcMS = millis();
  } 
}



void domeClear() {
  for (int i = 0; i <154; i++) {
    leds.setPixel(i+ledCH4, 0,0,0);
  }  
}

uint32_t steerRate = 1500;
uint32_t t1=0;
uint32_t t2=0;
float steerX;
float steerY;
unsigned long ssMS=0;
void steerShip() {

  if (millis()-ssMS > 20) {
    t1=t1+steerRate;
    t2=t2+steerRate;

    float n  = (float)inoise16(t1);
    steerX = (n/32768)-1;
    n  = inoise16(t2);
    steerY = (n/32768)-1;

    ssMS = millis();
  }
}

void centerShip() {
  steerX = 0;
  steerY = 0; 
  t1=random(1000000000);
  t2=random(1000000000);
}

float wave1Offset = 0;
float wave2Offset = 0;
float wave3Offset = 0;
float wave4Offset = 0;
unsigned long prMS=0;
void plasmaRings() { 

  if (millis()-prMS > 20) {

    float rotationSpeed1 = mapFloat(steerX, -1, 1, -0.6, 0.6); 
    float rotationSpeed2 = mapFloat(steerY, -1, 1, -0.6, 0.6);   
    wave1Offset += rotationSpeed1;
    wave2Offset += rotationSpeed1*0.8;
    wave3Offset += rotationSpeed2;
    wave4Offset += rotationSpeed2*0.8; 
    for (int i = 0; i < 600; i++) {
      float brightness = plasmaWave(i, wave1Offset,wave2Offset);
      int c = interpolateThreeColors(Black,hOrange,hPurple,0.3,brightness);
      leds.setPixel(topRingIdx(i), c);
      brightness = plasmaWave(i,wave3Offset,wave4Offset);
      c = interpolateThreeColors(Black,hOrange,hPurple,0.5,brightness);
      leds.setPixel(botRingIdx(i), c);
    }
    prMS = millis();
  }
}

float plasmaWave(int position, float offset1, float offset2) {
  int freq1 = 10;  // Adjust for wave frequency
  int freq2 = 2;  // Adjust for a different interference frequency
  float wave1 = sin(position * (TWO_PI * freq1 / 600) + offset1);
  float wave2 = sin(position * (TWO_PI * freq2 / 600) + offset2);
  float combinedWave = (wave1 + wave2) / 2; // Combining two waves
  float nonLinear = pow(combinedWave, 2.1);
  return nonLinear;
}



unsigned long dfMS=0;
const float Pi = 3.14159265359;
void domeFly() {

  if (millis()-dfMS > 20) {
  
    for(int i=0; i<154; i++) {
      float theta = (2 * PI * i) / 154;
      float xp = cos(theta);
      float yp = sin(theta);    
      float distance = sqrt(pow(steerX - xp, 2) + pow(steerY - yp, 2));
      float nonLinear = 0;
      if (distance < 0.9) {
        distance = (0.9 - distance);
        distance = mapFloat(distance,0,0.9,0,1);
        distance *=1.3;
        if (distance > 1)
          distance = 1;
        nonLinear = pow(distance, 1.0);
      }
      
      int c = interpolateThreeColors(Black,Orange,Purple, 0.5, nonLinear);
      leds.setPixel(mapDomeLED(i),c);
    }
  
    dfMS=millis();
  }

}

float maxX[] = {1,-1,-1,1};
float maxY[] = {1,1,-1,-1};
int tmap[] = {2,3,0,1};
unsigned long ftMS=0;
unsigned long fogMS=0;
int thrCnt = 0;
void flyThrusters() {

    if (millis()-ftMS > 20) {
      
      for (int i = 0; i < 4; i++) {
        //float xp = cos(thrusterLocDeg[i] * PI / 180.0);
        //float yp = sin(thrusterLocDeg[i] * PI / 180.0); 
        float xp = maxX[i];
        float yp = maxY[i];
        float distance = sqrt(pow(steerX - xp, 2) + pow(steerY - yp, 2));
        
        int c = Black;
        if (distance <= 1.1)
          if (random(2)==0)
            c = Purple;
          else
            c = Orange;

        for (int j=0; j<18; j++) {
          leds.setPixel(ledCH5+j+(tmap[i]*18),c);
        }

        if (distance <= 1.1) {
          runFan(tmap[i], true);
          thrCnt++;
        } else {
            if (thrCnt < 350)
              runFan(tmap[i], false); 
        }
      }

      if (thrCnt >= 350) { 
        blowFog(true);
        runFan(tmap[0], true); 
        runFan(tmap[1], true); 
        runFan(tmap[2], true); 
        runFan(tmap[3], true); 
      } else {
        blowFog(false);
      }

      if (thrCnt >= 400)
        thrCnt = 0;
       
      ftMS=millis();
    }

    

}

void thrustersOff() {
  allDuctLightsOff();
  allDuctFansOff();
  blowFog(false);
}

// Effects **************************************** All of the functions above are schedulable effects **************************************** 

// Schedules **************************************** All of the definitions below are the the schedules **************************************** 
Event seqFlying[] = {
  {0, 180000, steerShip, centerShip, CONT},
  {0, 180000, plasmaRings, ringsClear, CONT},
  {0, 180000, domeFly, domeLightsOff, CONT},
  {0, 180000, flyThrusters, thrustersOff,CONT}
};




Event seqRampLanding[] = {
  {0, 15000, fogOn, fogOff},
  {3000,50000, domeFanOn, domeFanOff},
  {3000,20000, domeLightsOn, domeLightsOff},
  {10000,60000, randomRingBlips, ringsClear, CONT}, 
  {18000,74500, rampLightsMoving, rampLightsOff, CONT},
  {19000,74500, laserOn, laserOff},
  {19000,74500, rampBayLightsRandom, rampBayLightsOff,CONT},
  {20000,40000, rampDown, rampOff},
  {20000,75000, rampFanOn, rampFanOff},
  {60000,75000, rampUp, rampOff}
};

Event testRamp[] = {
  {0,15000, rampDown, rampOff},
  {17000,32000, rampUp, rampOff},
};

Event seqBlueCoast[] = {
  {0, 500, fogOn, fogOff},
  {0, 15000, domeCoast, domeClear, CONT},
  {0, 15000, ringCoast, ringsClear, CONT},
  {500, 12500, randomDuctFanOn, randomDuctFanOff},
  {2500, 12500, randomDuctLightOn, randomDuctLightOff, CONT}
};

Event seqRedStrobe[] = {
  {2000, 20000, allDuctFansOn, allDuctFansOff},
  {0, 2000, fogOn, fogOff},
  {4000, 18000, allDuctLightsStrobe, allDuctLightsOff, CONT},
  {0,20000, ringTwinkle, ringsClear, CONT},
  {0,20000, domeTwinkle, domeClear, CONT}
};

Sequence sequences[] = {
  {seqFlying, sizeof(seqFlying) / sizeof(Event), 180000,1,0},
  //{seqBlueCoast, sizeof(seqBlueCoast) / sizeof(Event), 15000,4,0},
  //{seqRedStrobe, sizeof(seqRedStrobe) / sizeof(Event), 20000,1,0},
  //{seqRampLanding, sizeof(seqRampLanding) / sizeof(Event), 75000,1,0},
  //{seqRedStrobe, sizeof(seqRedStrobe) / sizeof(Event), 20000,1,0},
  //{seqBlueCoast, sizeof(seqBlueCoast) / sizeof(Event), 15000,4,0}
};

// Schedules **************************************** All of the definitions above are the the schedules **************************************** 


// Runners **************************************** All of the code below is the UFO runner and scheduler **************************************** 

void setup() {
  
  Serial.begin(38400);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(fogPin, INPUT);

  leds.begin();
  leds.show();

  Wire.begin();
  Wire.beginTransmission(0x27);
  Wire.write(0xFF); 
  Wire.endTransmission();
  Wire.beginTransmission(0x20);
  Wire.write(0xFF); 
  Wire.endTransmission();

  seqStart = millis();

  t1=random(1000000000);
  t2=random(1000000000);



  if (open)
    rampDown();
  if (close)
    rampUp();
  if (clearUFO)
    clearUFOwait();


}





void loop() {

  unsigned long currTime = millis() - seqStart;

  Sequence& currentSequence = sequences[currentSequenceIndex];
  Event* currentEvents = currentSequence.events;
  
  for (int i = 0; i < currentSequence.numEvents; i++) {
    if (currTime >= currentEvents[i].startTime && currTime < currentEvents[i].endTime) {
      if (currentEvents[i].continuous) {
        currentEvents[i].startFunction();
      } else if (!currentEvents[i].startHasRun) {
        currentEvents[i].startFunction();
        currentEvents[i].startHasRun = true;
      }
    } else if (currTime >= currentEvents[i].endTime) {
        if (!currentEvents[i].endHasRun) {
          currentEvents[i].endFunction();
          currentEvents[i].endHasRun = true;
        }
    }
  }

  if (currTime >= currentSequence.duration) {
    currentSequence.currentCount++;
    for (int i = 0; i < currentSequence.numEvents; i++) {
      currentEvents[i].startHasRun = false;
      currentEvents[i].endHasRun = false; 
    }
    if (currentSequence.currentCount >= currentSequence.repeatCount) {
      currentSequence.currentCount = 0;
      currentSequenceIndex = (currentSequenceIndex + 1) % (sizeof(sequences) / sizeof(Sequence));  // Cycle to the next sequence
    }
    seqStart = millis();
  }

  isFogReady();
  leds.show();

}
// Runners **************************************** All of the code above is the UFO runner and scheduler **************************************** 




// Worker functions **************************************** All of the code below is the low level UFO component's wagglers *******************

void clearUFOwait() {
  runFan(0,true);
  runFan(1,true);
  runFan(2,true);
  runFan(3,true);
  delay(120000);
  while(true) {}
}


void moveRamp(rampMode rm) {
  if (rm == OFF) {
    setRelay(rampLoc[0],NC); 
    setRelay(rampLoc[1],NC); 
  } else if (rm == UP) {
    setRelay(rampLoc[0],NC); 
    setRelay(rampLoc[1],NO); 
  } else if (rm == DOWN) {
    setRelay(rampLoc[0],NO); 
    setRelay(rampLoc[1],NC); 
  }
}

void shineLaser(boolean b) {
  if (b) {
    setRelay(laserLoc,NO);  
  } else {
    setRelay(laserLoc,NC);
  }
}

void blowFog(boolean b) {
  if (b && fogReady()) {
    setRelay(fogLoc,NO);  
  } else {
    setRelay(fogLoc,NC);
  }
}

void runFan(int num, boolean b) {
  if (num<8) {
    relayPos rp = NC;
    if (b)
      rp = NO;
    setRelay(fanLocs[num],rp);  
  }
}

void setRelay(int num, relayPos rc) {
  boolean set = false;
  if (num >= 0 && num <= 15) {
    if (rc != relayState[num]) {
      relayState[num] = rc;
      set = true;  
    }     
  } else {
    Serial.println("relay num out of range");  
  }  
  if (set) {
    byte b = 0;
    int add = 0x27;
    int offs = 0;
    if (num > 7) {
      add = 0x20;
      offs = 8;
    } 
    for (int i=0; i<8; i++) {
      if (relayState[i+offs] == NC)
        b += pow(2,i);  
    }
    Wire.beginTransmission(add);
    Wire.write(b); 
    Wire.endTransmission(); 
  }
}

boolean fogReady() {
  int fogV = analogRead(fogPin);  // read the input pin
  if (fogV > 750 && !noFog)
    return true;
  else
    return false;  
}

void isFogReady() {

  if (!fogReady())
    setRelay(fogLoc,NC);
}

int topRingIdx(int i) {
  int idx = i;
  int ch = ledCH0;
  if (idx>=300) {
    idx -= 300;
    idx = 299-idx;
    ch = ledCH1;
  }
  return ch+idx;
}

int botRingIdx(int i) {
  int idx = i;
  int ch = ledCH2;
  if (idx>=300) {
    idx -= 300;
    idx = 299-idx;
    ch = ledCH3;
  }
  return ch+idx;
}

int blendColors(int color1, int color2) {
  int r1, g1, b1, r2, g2, b2;
  r1 = (color1 >> 16) & 0xFF;
  g1 = (color1 >> 8) & 0xFF;
  b1 = color1 & 0xFF;
  
  r2 = (color2 >> 16) & 0xFF;
  g2 = (color2 >> 8) & 0xFF;
  b2 = color2 & 0xFF;

  // Blend colors by averaging
  int rBlend = (r1 + r2) / 2;
  int gBlend = (g1 + g2) / 2;
  int bBlend = (b1 + b2) / 2;

  return leds.Color(rBlend, gBlend, bBlend);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int interpolateThreeColors(int col1, int col2, int col3, float changepoint, float mix) {
  if (mix <= changepoint) {
    // Normalize mix for the first segment (from 0 to changepoint)
    float progress = mix / changepoint;
    return interpolateColor(col1, col2, progress);
  } else {
    // Normalize mix for the second segment (from changepoint to 1)
    float progress = (mix - changepoint) / (1.0 - changepoint);
    return interpolateColor(col2, col3, progress);
  }
}

int interpolateColor(int col1, int col2, float progress) {
  int r1 = (col1 >> 16) & 0xFF;
  int g1 = (col1 >> 8) & 0xFF;
  int b1 = col1 & 0xFF;

  int r2 = (col2 >> 16) & 0xFF;
  int g2 = (col2 >> 8) & 0xFF;
  int b2 = col2 & 0xFF;

  int r = (int) (r1 + (r2 - r1) * progress);
  int g = (int) (g1 + (g2 - g1) * progress);
  int b = (int) (b1 + (b2 - b1) * progress);

  return (int)leds.color(r, g, b);
}

int mapDomeLED(int idx) {
  return ledCH4+((idx+58)%154);
}

// Worker functions **************************************** All of the code above is the low level UFO component's wagglers *******************






