=== KinderMP3Player

Plays Audio files from SD card to be controlled by quick select buttons for Albums (= folders),
a volume encoder and 'next' and 'stop' buttons.

To be run on an Arduino Leonardo + Adafruit Music Shield
(other Arduinos and other Adafruit VS1053 variants might need small modifications)

Based on the adafruit simple_player example (and therefore including the original license text).

Needs a special version of the Adafruit VS1053 Library available here: https://github.com/Chaos99/Adafruit_VS1053_Library


Hardware:
10 large pushbuttons 
2 small pushbuttons
1 rotary encoder with push option
1 main power switch

Functional description:

Encoder: volume
Encoder push: pause/play
large buttons: quickselect album + start playing
same large button as currently playing: pause/play
small buttons: skip file, restart file, previous file
power switch: hard power off/on