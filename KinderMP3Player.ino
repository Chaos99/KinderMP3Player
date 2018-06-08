

/*************************************************** 
  This is an example for the Adafruit VS1053 Codec breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout 
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// include SPI, MP3 and SD libraries
#include <SD.h>
#include <SPI.h>
#include "Adafruit_VS1053_mod.hpp"
#include <Adafruit_MCP23017.h>

#include <Encoder.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1     // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

#define DEBUG true
#define VOLUMEBASE 20

const int buttonPin = 10;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin for testing

//status variables
bool button; // button pressed?
bool start; // still in startup?
File currentTrack;
File lastTrack;
File currentAlbumDir;
typedef enum{PRESTART, PLAYING, STOPPED, PAUSED} status_type;
status_type status;
long encoderposition;
int currentvolume;
char currentAlbumChar;

//managing objects
Adafruit_MCP23017 mcp; // io extender
Adafruit_VS1053_FilePlayer musicPlayer =   // music shield
Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);
Encoder rotEnc(3,4); // pins for rotary encoder


void handle_console_input(char c)
{
  // console command for physical buttons

  // album select via 'keypad' buttons
  if (strchr("0123456789", c))
  {
    Serial.print("Album #");
    Serial.println(c);
    // if same album is re-pressed ... 
    if (currentAlbumChar == c){
      if (status == PLAYING)
        pause();
      else if (status == PAUSED)
        unpause();
    } else {
      // start new album play unconditionally
      currentAlbumChar = c;
      playAlbum(c);
    }
  }
  // 'next' or 'previous' button pressed
  else if (c == 'n' or c == 'b') {
    // active only if playing
    if (status == PLAYING) {
      if (c == 'n')
        play_next();
      else // c == b
      { 
        restart_title();
        // todo: go to previous if re-pressed in 3 sec
      }
    }
  }
  // rotary_encoder pressed
  else if (c == 'p'){
    if (status == PLAYING)
      pause();
    else if (status == PAUSED)
      unpause();
    // else: ignore
  }

  // if we get an 's' on the serial console, stop!
  if (c == 's' ) {
    if(not musicPlayer.stopped()){
      musicPlayer.stopPlaying();      
      Serial.println("Done playing music, switching off.");
    }
    else {
      Serial.println("Welcome. Player is ready. s(start/stop), p(pause/unpause), ,.(volume), 1-9(play album))");  
    }
  }

// volume up and down (like rotary encoder)
  if (c == ',') 
  {
    currentvolume ++;
    update_volume();
    Serial.print("Volume: ");
    Serial.println(currentvolume);   
  }
  if (c == '.')
  {
    currentvolume --;
    update_volume();
    Serial.print("Volume: ");
    Serial.println(currentvolume);
  }

// non-button codes for debugging

  // list albums/files
  if (c=='l') // lowercase L
  {
    File root;
    root = SD.open("/");
    printDirectory(root, 0);
  }

  // give status of all buttons
  if (strchr("d", c))
  {
   int a0 = mcp.digitalRead(0);
   int a1 = mcp.digitalRead(1);
   int a2 = mcp.digitalRead(2);
   int a3 = mcp.digitalRead(3);
   int a4 = mcp.digitalRead(4);
   int a5 = mcp.digitalRead(5);
   int a6 = mcp.digitalRead(6);
   int a7 = mcp.digitalRead(7);

   int b8 = mcp.digitalRead(8);
   int b9 = mcp.digitalRead(9);
   int b10 = mcp.digitalRead(10);
   int b11 = mcp.digitalRead(11);
   int b12 = mcp.digitalRead(12);
   int b13 = mcp.digitalRead(13);
   int b14 = mcp.digitalRead(14);
   int b15 = mcp.digitalRead(15);
   
   Serial.println(a0);
   Serial.println(a1);
   Serial.println(a2);
   Serial.println(a3);
   Serial.println(a4);
   Serial.println(a5);
   Serial.println(a6);
   Serial.println(a7);

   Serial.println(b8);
   Serial.println(b9);
   Serial.println(b10);
   Serial.println(b11);
   Serial.println(b12);
   Serial.println(b13);
   Serial.println(b14);
   Serial.println(b15);    

 }

   // stop playback/reset (normaly done via power off)
 if (c == 's') {
  musicPlayer.stopPlaying();
}

}

// update volume
void update_volume()
{
  long newValue = rotEnc.read();
  if (newValue != encoderposition){
    long change = newValue - encoderposition;
    encoderposition = newValue;
    currentvolume = currentvolume+change;   
    if (DEBUG) {
      Serial.print("Volume (knob): ");
      Serial.println(currentvolume);
    }
    musicPlayer.setVolume(currentvolume, currentvolume);
  }
}


////old code
//play or resume a file
void play_track(File track)
{
  // if new track, save old for future reference
  if (track != currentTrack) {
    lastTrack = currentTrack; // save old track
    currentTrack = track;
  }
  // if not just paused, start playing
  if (status != PAUSED) {
    musicPlayer.startPlayingFile(track); // start playing
    Serial.print("Playing...");
  } else {
    // TODO: yhis should check if track is unchanged
    // if just paused, resume
    musicPlayer.pausePlaying(false); //resume  
    Serial.print("Resuming...");    
  }
  // update log and status
  Serial.println(track.name());
  status = PLAYING;
  return;
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry =  dir.openNextFile();
    if (! entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}


void pause()
{
//TODO: implement me
}

void unpause()
{
//TODO: implement me
}

// start playing an album
void playAlbum(char c)
{
  if (musicPlayer.paused() or musicPlayer.stopped() or c != currentAlbum)
  {
    if (musicPlayer.paused() and c == currentAlbum)
      musicPlayer.pausePlaying(false); // resume
    else
    {     
      //start paying album
      Serial.println("Playing ...");
      currentAlbum = c;
      char albumname[7]="Album ";
      albumname[5] = c;
      albumname[6] = '\0';
      File albumdir;
      if (SD.exists(albumname)){        
        albumdir = SD.open(albumname); // open dir readonly
        currentAlbumDir = albumdir;
      } else
      return;
      File track = albumdir.openNextFile();
      
      if (!track)
        return; // no tracks
      else
      {
        play_track(track);
      }
    }
  }
  else {// current Album is playing
    musicPlayer.pausePlaying(true);
    status = PAUSED;
  }
}

void play_next(){
  File track = currentAlbumDir.openNextFile();
  play_track(track);
}


///////////////////////////////////////// default callbacks //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  //pre-start initialization of variables
  button = false;
  start = true;

  pinMode(buttonPin, INPUT_PULLUP);
  status = PRESTART;

  //initialize I2C for io extender
  mcp.begin();      // use default i2c address 0
  for(int i = 0; i<16; i++)
  {
    mcp.pinMode(i, INPUT);  // make pin 0 an input pin
    mcp.pullUp(i, HIGH);  // turn on a 100K pullup internally (activate = short to ground)
  }

  // initialize serial console
  Serial.begin(9600);
  while (!Serial) 
    if (millis() > 5000)
     break;
   Serial.println("Adafruit VS1053 Simple Test");

  if (! musicPlayer.begin()) { // initialise the music player
   Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
   while (1);
 }
 Serial.println(F("VS1053 found"));

  SD.begin(CARDCS);    // initialise the SD card

  // Timer interrupts are not suggested, better to use DREQ interrupt!
  //musicPlayer.useInterrupt(VS1053_FILEPLAYER_TIMER0_INT); // timer int

  // If DREQ is on an interrupt pin (on uno, #2 or #3) we can do background
  // audio playing
  musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT);  // DREQ int
  
  // Play one file, don't return until complete
 // Serial.println(F("Playing track 001"));
 // musicPlayer.startPlayingFile("track001.mp3");
  // Play another file in the background, REQUIRES interrupts!

}


void loop() {
  // File is playing in the background
  if (Serial.available()) {
    if (start){
      Serial.println("Welcome. Player is ready. s(start/stop), p(pause/unpause), ,.(volume), 1-9(play album))");
      start = false;
    }
    
    char c = Serial.read();
    
    handle_console_input(c);
    
 } // if serial.available

 update_volume();
 delay(100);
}
