/*************************************************** 
  This is an example for the Adafruit VS1053 Codec Breakout

  Designed specifically to work with the Adafruit VS1053 Codec Breakout 
  ----> https://www.adafruit.com/products/1381

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// These are the pins used for the music maker shield
#define SHIELD_RESET  -1      // VS1053 reset pin (unused!)
#define SHIELD_CS     7      // VS1053 chip select pin (output)
#define SHIELD_DCS    6      // VS1053 Data/command select pin (output)

// These are common pins between breakout and shield
#define CARDCS 4     // Card chip select pin
// DREQ should be an Int pin, see http://arduino.cc/en/Reference/attachInterrupt
#define DREQ 3       // VS1053 Data request, ideally an Interrupt pin

Adafruit_VS1053_FilePlayer musicPlayer = 
  // create shield-example object!
  Adafruit_VS1053_FilePlayer(SHIELD_RESET, SHIELD_CS, SHIELD_DCS, DREQ, CARDCS);

int volume = 20;

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

void setup() {
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
   
  // Set volume for left, right channels. lower numbers == louder volume!
  musicPlayer.setVolume(volume,volume);

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

char currentAlbum = '0';
//File lastPlayedByAlbum[9] = {null, null, null, null, null, null, null, null, null};
File currentTrack;

void playAlbum(char c)
{
  if (musicPlayer.paused() or musicPlayer.stopped() or c != currentAlbum)
  {
    if (musicPlayer.paused() and c == currentAlbum)
      musicPlayer.pausePlaying(false); // resume
    else
    {     
      //start paying album
      currentAlbum = c;
      char albumname[7]="Album ";
      albumname[5] = c;
      albumname[6] = '\0';
      File albumdir;
      if (SD.exists(albumname))        
        albumdir = SD.open(albumname); // open dir readonly
      else
        return;
      File track = albumdir.openNextFile();
      if (!track)
        return; // no tracks
      else
      {
        musicPlayer.startPlayingFile(track);
        currentTrack = track;
        Serial.println(track.name());
      }
    }
  }
  else // current Album is playing
    musicPlayer.pausePlaying(true);
}

void loop() {
  // File is playing in the background
  if (musicPlayer.stopped()) {
     Serial.println(F("Playing next track"));
     File next = currentTrack.openNextFile();
     if (next)
       musicPlayer.startPlayingFile(next);
  }
  if (Serial.available()) {
    char c = Serial.read();
    
    // if we get an 's' on the serial console, stop!
    if (c == 's') {
      musicPlayer.stopPlaying();
      Serial.println("Done playing music");
      while (1);
    }
    
    // if we get an 'p' on the serial console, pause/unpause!
    if (c == 'p') {
      if (! musicPlayer.paused()) {
        Serial.println("Paused");
        musicPlayer.pausePlaying(true);
      } else { 
        Serial.println("Resumed");
        musicPlayer.pausePlaying(false);
      }
    }

    if (c == ',') 
    {
      volume ++;
      musicPlayer.setVolume(volume,volume);
      Serial.print("Volume: ");
      Serial.println(volume);    }
    if (c == '.')
    {
      volume --;
      musicPlayer.setVolume(volume,volume);
      Serial.print("Volume: ");
      Serial.println(volume);
   }

   if (c=='l')
   {
      File root;
      root = SD.open("/");
      printDirectory(root, 0);
   }

   if (strchr("123456789", c))
   {
      Serial.print("Album");
      Serial.println(c);
      playAlbum(c);
   } 
 } // if serial.available

  delay(100);
}
