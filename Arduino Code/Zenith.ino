/** Zenith

  The Zenith Voice Controlled Internet Radio. The idea is we will
  replace the innards of a vintage radio with an ESP32-S3 that plays
  Internet radio and is controlled by voice commands.

  Voice control is handled by the DFRobot SEN0539 Language Recognition
  Sensor. This sensor is able to learn one wake-word and 17 voice
  commands. It conects to our ESP32-S3 over I2C.

  IMPORTANT: The Zenith serves a webpage for configuring the WiFi and
  Internet Radio URLs.  When it is unable to connect to WiFi or WHEN
  THE VOLUME KNOB IS PRESSED IN AT STARTUP, Zenith creates an access
  point called "Zenith Setup" and serves the webpage at 192.168.1.4.
  You can always access this webpage when Zenith is on your home WiFi
  by going to http://zenith.local.
  
  The ESP32 has a stepper motor under its control that moves the dial
  pointer. The stepper driver is the TMC2209 and is controlled to the
  ESP32 by a few GPIO pins and configured using a hardward serial
  UART.  There is a small magnet on the pointer that is sensed by a
  reed switch mounted under the dial face. This is used to home the
  pointer at startup.

  Being an Internet radio, the ESP32-S3 sends audio over I2S (not a
  typo), to a MAX98357 audio amplifier.

  Finally, the operations of the volume and tuner knobs are handled by
  two rotary encoders. The volume knob controls volume and pressing it
  in toggles the mute state.  The Tuner know controls which Internet
  radio station is playing. Pressing the turn knob in only matters
  with the radio is tuned to a local stream (special channel). When
  this is the case, pressing the tuner in will play the next track in
  the stream.
  
  @author J. King
  @date 5 February 2025
  @copy All rights reserved.
  @brief discription
  
  $Revision: 1.52 $
  $Date: 2025/03/11 20:52:11 $

*/

// Include required libraries
#include <Arduino.h>
#include <esp_system.h>
#include <Preferences.h>

#include <WiFi.h>
#include <ESPmDNS.h>
#include "Audio.h"
#include <HardwareSerial.h>
#include <TMCStepper.h>
#include "DFRobot_DF2301Q.h"
#include "GetRequestParser.h"
#include "index.h"
#include "index2.h"

// debug
//#define DEBUG

#ifdef DEBUG
#define sp1(x)   Serial.println(x)
#define sp2(x,y) Serial.print(x);Serial.println(y)
#define sp2s(x,y) Serial.print(x);Serial.print(" ");Serial.println(y)
#else
#define sp1(x)
#define sp2(x,y)
#define sp2s(x,y)
#endif

// simplify serial print when debugging isn't desired
#define sp2sx(x,y) Serial.print(x);Serial.print(" ");Serial.println(y)
#define sp2x(x,y) Serial.print(x);Serial.println(y)
#define sp1x(x)   Serial.println(x)

// CONNECTION FOR YD ESP32-S3 DEVKITC-1 N8R2 (2 MB PSRAM, 8 MB FLASH)
// GPIO

const gpio_num_t mutePin = GPIO_NUM_4;      // the volume encoder push button
const gpio_num_t nextTrackPin = GPIO_NUM_14; // the favorite station push button

// Define I2S connections for audio output
#define I2S_DOUT  GPIO_NUM_21
#define I2S_BCLK  GPIO_NUM_2   // sck
#define I2S_LRC   GPIO_NUM_7  // ws

// STATION LIST
#define STATION_LIMIT       13  // 13 STATION LIMIT, one for each dial number
#define STATION_CHAR_LIMIT 256  // character length limit for station entries

// the station list set of keys
char *staIds[] = {"sta55", "sta60", "sta65", "sta70", "sta80", "sta90",
		  "sta100", "sta110", "sta120", "sta130", "sta140",
		  "sta150", "sta160"};

// normal loop or settings loop
bool gAPMode = false;

// we will populate this list from preferences and have web page for
// configuring new or additional stations up STATION_LIMIT (limited to
// 13), one for each dial number.

char *gStationList[STATION_LIMIT] = {0};
int gNumberOfStations = 0;
const int localStreamPos = 11;
const int localPodcastPos = 12;
bool gSkipRequest = false;

// COMMAND LIST -- populated from violet unless fails then prefs
// This list maps voice commands to tasks.

#define COMMAND_LIMIT      256
int8_t gCommands[COMMAND_LIMIT];  // sparsely populated lookup table
int gNumCommands = 0;

// Save the last station used, the volume level, wifi settings and the
// list of stations to preferences.

Preferences prefs;

// I2C for communicating with the DFRobot SENS0539-EN voice
// recognition module

DFRobot_DF2301Q_I2C zenith;

// Create audio object
Audio audio;

// Wifi Credentials, taken from prefs
char ssids[2][32];  
char password[2][80];
int gCurrentSSID = 0;
const int rssiMinimum = -65; // switch access points when signal gets
			     // marginal

// WiFi Access Point for setting passwords and station urls
const char *apSSID = "Zenith Setup";

IPAddress violet(10,0,0,9); // violet will host the voice to task mapping
WiFiClient commandListClient; // we connect to violet as a client

// we also run an http server for configuring the Zenith VC Radio
NetworkServer server(80);

// LEDs
const gpio_num_t grnLed = GPIO_NUM_40;
const gpio_num_t redLed = GPIO_NUM_41;

// reed switch for homing
const int homePin = GPIO_NUM_1;

// stepper motor definition
#define EN_PIN    GPIO_NUM_16   // Enable pin (wired to ground)
#define DIR_PIN   GPIO_NUM_6   // Direction pin
#define STEP_PIN  GPIO_NUM_5   // Step pin
#define UART_RX   GPIO_NUM_18   // UART RX (TMC2209 TX)
#define UART_TX   GPIO_NUM_17   // UART TX (TMC2209 RX)

#define R_SENSE 0.11f    // Sense resistor value
#define DRIVER_ADDRESS 0 // Default address for TMC2209

#define DIRECTION_CW  LOW
#define DIRECTION_CCW   HIGH
#define SPEED_FAST   10  // might be too fast for most tasks
#define SPEED_MEDIUM 5
#define SPEED_SLOW   0

#define STEPPER_CURRENT 80 // around 56 mA is as low as it will go
#define STEPPER_SPEED   SPEED_SLOW
#define MICRO_STEPS     16

// This are emperically determined stepper positions for each dial
// number.
// Zero is when the needle is against the left stop.
const int numPos = 14;
const int dialPosition[] = {0, 9, 25, 39, 50, 67, 80, 92, 105,
			  //  55  60  65  70  80  90 100  110
			    116, 126, 137, 146, 157};
                         // 120  130  140  150  160

int gCurrentIndex = 0;

// this configures the stepper motor driver with a serial connect
TMC2209Stepper driver(&Serial1, R_SENSE, DRIVER_ADDRESS);

// Tuner and Volume encoder definitions
#define TUNER_APIN GPIO_NUM_13
#define TUNER_BPIN GPIO_NUM_10
#define VOLUME_APIN GPIO_NUM_12
#define VOLUME_BPIN GPIO_NUM_11
#define MAX_VOLUME 100

// timing constants
const unsigned long encDebounceMS = 10;
const unsigned long encWindow = 500; // consistent results or forget

// encoder knob position
volatile int gTunEncoderPos = 0;
volatile int gVolEncoderPos = 22;
volatile unsigned long gPlayStart = 0;
volatile unsigned long gPlayTime = 0;

///////////////  INTERRUPTS ///////////////////////////////////////

/** tunerEncoderISR
    This interrupt updates the value stored in gTunEncoderPos.
    This is triggered on an A pulse, the state of B tells us the
    direction. Because encoders are mechanical switches, we need to
    debounce them.
*/

// Tuner (T) ISR function for reading A pulses
unsigned long encTLastPulse = 0;
int lastTResult = 0; // require pause before changing direction

static void IRAM_ATTR tunerEncoderISR(void *arg) {
  int8_t bStateSum = 0;
  int8_t result;
  int rtn = gTunEncoderPos;
  
  for (int i=0; i<4; i++) {
    bStateSum += digitalRead(TUNER_BPIN) == HIGH ? 1 : 0; // try to debounce B
    delayMicroseconds(100);
  }

  // only concensus readings are accepted
  if ( bStateSum == 0 || bStateSum == 4 ) {
    result = bStateSum < 2 ? 1 : -1;

    // debounce and delay turning around to get smooth forward or
    // reverse changes
    if ( millis() - encTLastPulse > encDebounceMS ) { // debounce A
      if ( result == lastTResult || millis() - encTLastPulse > encWindow ) {
	rtn = gTunEncoderPos + result;
	lastTResult = result;
	encTLastPulse = millis();
      }
    }
  }
  // clip to our station list
  if ( rtn >= gNumberOfStations )
    gTunEncoderPos = gNumberOfStations - 1;
  else if ( rtn < 0 )
    gTunEncoderPos = 0;
  else
    gTunEncoderPos = rtn;
}

/** volumeEncoderISR
    This interrupt updates the value stored in gVolEncoderPos.
    This is triggered on an A pulse, the state of B tells us the
    direction. Because encoders are mechanical switches, we need to
    debounce them.
 */

// Volume (V) ISR function for reading A pulses
unsigned long encVLastPulse = 0;
int lastVResult = 0; // require pause before changing direction

static void IRAM_ATTR volumeEncoderISR(void *arg) {
  int8_t bStateSum = 0;
  int8_t result;
  int rtn = gVolEncoderPos;
  
  for (int i=0; i<4; i++) {
    bStateSum += digitalRead(VOLUME_BPIN) == HIGH ? 1 : 0; // try to debounce B
    delayMicroseconds(100);
  }

  // accept consensus values only
  if ( bStateSum == 0 || bStateSum == 4 ) {
    result = bStateSum < 2 ? 1 : -1;

    // smooth transiition
    if ( millis() - encVLastPulse > encDebounceMS ) { // debounce A
      if ( result == lastVResult || millis() - encVLastPulse > encWindow ) {
	rtn = gVolEncoderPos + result;
	lastVResult = result;
	encVLastPulse = millis();
      }
    }
  }
  // clip volume
  if ( rtn >= MAX_VOLUME )
    gVolEncoderPos = MAX_VOLUME;
  else if ( rtn < 0 )
    gVolEncoderPos = 0;
  else
    gVolEncoderPos = rtn;
}

/** muteEncoderISR

    This ISR sets the state of gPlay.
    Interrupt routine to handle the button press of the volume
    encoder. It toggles the pause/resume state of Internet Radio.
*/

volatile bool gPlay = true;

unsigned long btnDebounceMS = 500;
unsigned long muteLastPress = 0;
unsigned long muteBtnDebounce = btnDebounceMS * 2;

static void IRAM_ATTR muteEncoderISR(void *arg) {
  if ( millis() - muteLastPress > muteBtnDebounce ) { // debounce
    if ( gPlay )
      gPlay = false;
    else
      gPlay = true;
    muteLastPress = millis();
  }
}

/** nextTrackEncoderISR

    This ISR sets the state of gNextTrack which advances the playback
    of local music one track.
    Interrupt routine to handle the button press of the tuner
    encoder. This button is only active when the station is set to
    STREAM LOCAL.
*/

volatile bool gNextTrack = false;
unsigned long nextTrackPress = 0;

static void IRAM_ATTR nextTrackEncoderISR(void *arg) {
  if ( millis() - nextTrackPress > btnDebounceMS ) { // debounce
    gNextTrack = true;
    nextTrackPress = millis();
  }
}

/** playStationTask

    This task just keeps the radio playing.  The station is selected
    elsewhere.  It is paused by setting gPlay to false.
*/

TaskHandle_t PlayStationHandle;
int gPlayParam = 0;

void playStationTask(void *pvParams) {
  bool state = gPlay;

  while ( true ) {
    if ( gPlay ) {
      audio.loop();
    }
      
    vTaskDelay(1/portTICK_PERIOD_MS);
  }
}

/** zenithVCTask

    This task handles the voice commands.  When you say, "Hello,
    Robot" and the DFRobot SEN0539 recognizes this the wake-word, the
    radio will be paused.  It is then ready to recieve a command.
    BTW: by pressing the volume knob to pause the radio before saying
    "Hello, robot", is adviced.

    There are primary control commands and commands for specific
    Internet Radio Stations.  You can reprogram the DFRobot without
    writing code but the code here would need to change to add and
    delete tasks.  If you reprogram the DFRobot Voice Commands, these
    can be mapped to the tasks coded using a text file served by a
    local webserver.  Changing the webserver IP, however, requires
    modification of this program.
    
    FYI: the purpose of pause/resume radio is to allow programming of
    the DFRobot or for verifying the voice command ids.

*/
    
TaskHandle_t ZenithVCHandle;
int gTaskVar = 0;
bool gRadioOff = false; // use this to train the sen0539 language
			// recognition sensor

void zenithVCTask(void *pvParams) {
  while ( true ) {
    bool speakerOn = false;
    uint8_t zenithCmd = 0;

    // don't take commands away from the radio off loop
    if ( gRadioOff ) {
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    else {
      zenithCmd = zenith.getCMDID();
      speakerOn = audio.isRunning();
    }
    
    if ( (zenithCmd == 1 || zenithCmd == 2) && speakerOn ) {
      // ideally both wake-words would silence the dfrobot
      playPause();
    }
    
    // the programmable wake-word won't silence the speaker
    
    if ( zenithCmd > 2 ) { // command 0 is nothing,
                           // 1 = programmable wake-work
			   // 2 = hello robot (wake word)
      int changeVolume = 0;
      int changeStation = 0;
      int taskId = 0;

      taskId = gCommands[zenithCmd];
      sp2("*** CMDID: ", zenithCmd);
      sp2("*** TaskID: ", taskId);
      
      switch ( taskId ) {
      case 5:                 // volume up
	changeVolume = gVolEncoderPos + 3;
	break;
      case 6:                 // volume down
	changeVolume = gVolEncoderPos - 3;
	break;
      case 7:                 // change volume to maximum
	gVolEncoderPos = 30;
	break;
      case 8:                // change volume to minimum
	gVolEncoderPos = 5;
	break;
      case 9:                // change volume to medium
	gVolEncoderPos = 10;
	break;
      case 10:                  // station up
	changeStation = gTunEncoderPos + 1;
	break;
      case 11:                  // station down
	changeStation = gTunEncoderPos - 1;
	break;
      case 12:                  // Wisconsin Public Radio
	gTunEncoderPos = 5;
	break;
      case 13:                  // N P R
	gTunEncoderPos = 4;
	break;
      case 14:                  // CBC Music, slot 8
	gTunEncoderPos = 7;
	break;
      case 15:                 // CBC Talk, slot 7
	gTunEncoderPos = 6;
	break;
      case 16:                 // prairie public radio, slot #4
	gTunEncoderPos = 3;
	break;
      case 17:                 // Latin Radio
	gTunEncoderPos = 1;
	break;
      case 18:                 // Northern State Radio (Chico CA)
	gTunEncoderPos = 0;
	break;
      case 19:
	gTunEncoderPos = 2;    // For Classical
	break;
      case 20:
	gTunEncoderPos = 7;    // For Jazz
	break;
      case 21:
	gTunEncoderPos = localStreamPos; // Stream Local
	break;
      case 22:                 // pause radio
	gPlay = false;
	break;
      case 23:                 // resume radio
	gPlay = true;
	break;
      case 24:
	gRadioOff = true;    // turn off radio (for training sen0539)
	break;
      case 25:
	gRadioOff = false;   // trun on radio
	break;
      case 26:               // how's marvin
	audio.connecttohost("http://10.0.0.30:8000/marvin");
	gPlay = true;
	speakerOn = true;
	break;
	
      default:
	gPlay = true;
	sp2("Unrecognized command: ", zenithCmd);
      }

      // clip station if changed
      if ( changeStation != 0 ) {
	if ( changeStation >= gNumberOfStations )
	  gTunEncoderPos = gNumberOfStations - 1;
	else if ( changeStation < 1 )
	  gTunEncoderPos = 1;
	else
	  gTunEncoderPos = changeStation;
      }
      
      // clip volume if changed
      if ( changeVolume != 0 ) {
	if ( changeVolume >= MAX_VOLUME )
	  gVolEncoderPos = MAX_VOLUME;
	else if ( changeVolume < 0 )
	  gVolEncoderPos = 0;
	else
	  gVolEncoderPos = changeVolume;
      }
      if ( speakerOn )
	playResume();
      
    } // end if zenithCmd is not 0
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// GLOBALS
int gLastVolume = gVolEncoderPos;
int gLastTunEncoderPos = 0;
unsigned long gLastStationChange = 0;
bool gPlayLast = gPlay;

/** setup

    So much to do here!!!
*/

void setup() {
  // Start Serial Monitor
  Serial.begin(115200);
  delay(500);

  // get preferences
  prefs.begin("Zenith");
  if ( false ) { // first time settings
    prefs.putInt("Volume", gVolEncoderPos);
    prefs.putInt("Station", gTunEncoderPos);
  }
  gVolEncoderPos = prefs.getInt("Volume");
  gTunEncoderPos = prefs.getInt("Station");

  // leds
  pinMode(redLed, OUTPUT);
  pinMode(grnLed, OUTPUT);
  
  // Stepper pins
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);  // keep the driver off until we want to move

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);

  // this is a reed switch for homing the stepper motor
  pinMode(homePin, INPUT_PULLUP);

  // STATION DIAL
  pinMode(TUNER_APIN, INPUT_PULLUP);
  pinMode(TUNER_BPIN, INPUT_PULLUP);
  pinMode(nextTrackPin, INPUT_PULLUP);
  
  // VOLUME DIAL
  pinMode(VOLUME_APIN, INPUT_PULLUP);
  pinMode(VOLUME_BPIN, INPUT_PULLUP);
  pinMode(mutePin, INPUT_PULLUP);

  // Two ways to get to settings, no wifi ssids or by pressing in the
  // volume button when powering on.

  String ssid1Pref = prefs.getString("ssid1");
  String ssid2Pref = prefs.getString("ssid2");
  String pw1Pref = prefs.getString("pw1");
  String pw2Pref = prefs.getString("pw2");
  
  strcpy(ssids[0], ssid1Pref.c_str());
  strcpy(ssids[1], ssid2Pref.c_str());
  strcpy(password[0], pw1Pref.c_str());
  strcpy(password[1], pw2Pref.c_str());

  // populate station array
  size_t maxLen = STATION_CHAR_LIMIT;
  gNumberOfStations = 0;
  for (int i=0; i<STATION_LIMIT; i++) {
    String str = prefs.getString(staIds[i]);
    if ( str.length() >= maxLen ) maxLen = str.length() + 1; 
    gStationList[i] = (char *)malloc(maxLen); // max length
    strcpy(gStationList[i], str.c_str());
    gNumberOfStations++;
    sp2s(i, gStationList[i]);
    // we can read in station urls that are too long but we can't keep them
    if ( strlen(gStationList[i]) >= STATION_CHAR_LIMIT ) {
      gStationList[i][STATION_CHAR_LIMIT-1] = '\0';
    }
  }
  
  // GENERAL SETTINGS (only used when moved to a new network)
  vTaskDelay(300 / portTICK_PERIOD_MS); // needed this for mutePin
  if ( strlen(ssids[0]) == 0 || digitalRead(mutePin) == LOW ) {
    WiFi.disconnect();
    sp1("Calling General Settings...");
    generalSettings();
    gAPMode = true;
    return;
  }
  
  // Setup WiFi in Station mode
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);

  sp2("ssid1 ", prefs.getString("ssid1"));
  sp2("ssid2 ", prefs.getString("ssid2"));  
  sp2("password1 ", prefs.getString("pw1"));
  sp2("password2 ", prefs.getString("pw2"));
  
  // connects to flowers4 unless it's slow to respond then flowers3
  for (gCurrentSSID=0; gCurrentSSID<2; gCurrentSSID++) {
    WiFi.begin(ssids[gCurrentSSID], password[gCurrentSSID]);
    sp2("Connecting to ", ssids[gCurrentSSID]);
    for (int i=0; i<20; i++) {  // giving it about 10 seconds
      if ( WiFi.status() == WL_CONNECTED ) break;
      Serial.print(".");
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
    if ( WiFi.status() == WL_CONNECTED ) break;
  }

  // both were slow to connect, try ssids[0] again
  if ( WiFi.status() != WL_CONNECTED ) {
    gCurrentSSID = 0;
    WiFi.begin(ssids[gCurrentSSID], password[gCurrentSSID]);
    sp2("Trying ", ssids[gCurrentSSID]);
  }
  for (int i=0; i<40; i++) { // now 20 seconds
    if ( WiFi.status() == WL_CONNECTED ) break;
    vTaskDelay(500/portTICK_PERIOD_MS);
    Serial.print(".");
  }
  // all failed so go into setup AP mode
  if ( WiFi.status() != WL_CONNECTED ) {
    sp1("WiFi failed, entering setup mode.");
    sp1("Connect to AP zenith and 192.168.4.1.");
    WiFi.disconnect();
    generalSettings();
    gAPMode = true;
    return;
  }

  // WiFi Connected, print IP to serial monitor
  sp1x("\nWiFi connected.");
  sp2x("IP address: ", WiFi.localIP());
  sp2x("AP ", WiFi.SSID());
  sp2x("RSSI ", WiFi.RSSI());
  sp1x("");
  //WiFi.setTxPower(WIFI_POWER_19_5dBm); // high power (78) (doesn't help)

  // use mDNS so that others can find us
  if ( !MDNS.begin("zenith") ) { // zenith.local
    sp1x("Error starting mDNS!");
  }
  else {
    sp1x("mDNS started, visit: http://zenith.local");
  }
  MDNS.addService("http", "tcp", 80);
  
  // Connect MAX98357 I2S Amplifier Module
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Set thevolume (0-100)
  audio.setVolume(gVolEncoderPos);

  // let's see if we can get our list of stations and commands from violet
  if ( commandListClient.connect(violet, 80) ) {
    char line[STATION_CHAR_LIMIT];
    int nLines = 0;
    int chars = 0;
    
    // get the commands
    while ( gNumCommands == 0 ) {
      char *savePtr;
      commandListClient.println("GET /voice_commands.txt HTTP/1.0");
      commandListClient.println();

      sp1("Wait for commands...");
      while ( ! commandListClient.available() ) {} // wait for response
      sp1("Reading commands...");
      while ( (chars = clientReadLine(&commandListClient,
				      line, STATION_CHAR_LIMIT)) != -1 ) {
	if ( chars > 0 && line[0] == '>' ) {
	  char *cmd, *tsk;
	  char cmdStr[16];
	  int tskId;
	  cmd = strtok_r(line+1, ",", &savePtr);
	  tsk = strtok_r(NULL, ",", &savePtr);
	  if ( cmd != NULL && tsk != NULL ) {
	    tskId = atoi(tsk);
	    gCommands[atoi(cmd)] = tskId;
	    gNumCommands++;
	    strcpy(cmdStr, "cmd");
	    strcat(cmdStr, cmd);
	    sp2s(cmdStr, tskId);
	    prefs.putInt(cmdStr, tskId); // save for when we can't connect
	  }
	}
      }
      sp2("Commands read: ", gNumCommands);
    }      
    commandListClient.stop();
  }
  else {
    // get from preferences, iterate commands in prefs
    for (int i=0; i<COMMAND_LIMIT; i++) {
      char cmdStr[16];
      sprintf(cmdStr, "cmd%d", i);
      if ( prefs.isKey(cmdStr) ) {
	gCommands[i] = prefs.getInt(cmdStr);
	gNumCommands++;
      }
      else {
	gCommands[i] = 0;
      }
      sp2sx(i, gCommands[i]);
    }
  }
  
  // get the stepper motor ready before ISRs
  sp1("Starting the stepper motor...");

  Serial1.begin(115200, SERIAL_8N1, UART_RX, UART_TX);
  vTaskDelay(1000);  // Wait for driver to power up

  driver.begin();          // Initialize TMC2209
  driver.toff(5);          // Enable driver
  driver.rms_current(STEPPER_CURRENT); // was 600 mA but got hot
  driver.microsteps(MICRO_STEPS); // I don't think we need microstepping (was
			   // 1/16). Although microstepping might use
			   // less current so it should run cooler
  sp2("MICRO STEPS : ", driver.microsteps());
  sp2("CURRENT : ", driver.rms_current());  
  driver.pwm_autoscale(true);    // Enable autoscaling of PWM

  // HOME
  // use the stop to position the dial (the entire dial is 100 steps)
  digitalWrite(EN_PIN, LOW);  // energizes the motor
  
  // calibration ---- Here's how we find exactly where the reed switch
  // gets tripped by the magnet on the needle -- this is off for
  // normal information
  bool calibrate = false;
  bool calForward = false;
  bool calReverse = false;
  int cnt = 0;

  if ( calibrate ) sp1("Calibration Mode Entered");

  while ( calibrate ) {
    if ( Serial.available() > 0 ) {
      char ch = (char )Serial.read();
      if ( ch == '\n' ) continue;
      
      if ( calForward || calReverse ) {
	sp2("Number of steps = ", cnt);
	calForward = false;
	calReverse = false;
	cnt = 0;
      }
      if ( ch == 'a' ) { // advance
	stepMotor(1, DIRECTION_CW, SPEED_MEDIUM);
      }
      if ( ch == 'd' ) { // down (regress)
	stepMotor(1, DIRECTION_CCW, SPEED_MEDIUM);
      }
      if ( ch == 'f' ) calForward = true;
      if ( ch == 'r' ) calReverse = true;
      sp2("Reed switch: ", digitalRead(homePin));
      if ( calForward || calReverse ) {
	while ( Serial.available() ) { ch = (char)Serial.read(); }
      }
    }
    if ( calForward ) {
      while ( !Serial.available() && digitalRead(homePin) == HIGH ) {
	stepMotor(1, DIRECTION_CW, SPEED_MEDIUM);
	cnt++;
      }
    }
    if ( calReverse ) {
      while ( !Serial.available() && digitalRead(homePin) == HIGH ) {
	stepMotor(1, DIRECTION_CCW, SPEED_MEDIUM);
	cnt++;
      }
    }
  }

  // HOME THE DIAL NEEDLE
  // ------------------------------------------------------------
  sp2("Finding home... ", digitalRead(homePin));

  int stepWidth = 200; // the dial is about 180 steps from end to end
  int homeSpeed = SPEED_SLOW;
  bool inZone = false;
  
  // if we are in the zone, good, do nothing for now
  if ( digitalRead(homePin) == LOW ) inZone = true;

  if ( ! inZone ) {
    // assume we are all the way left
    for (int i=0; i < 20 && digitalRead(homePin) == HIGH; i++) {
      stepMotor(1, DIRECTION_CW, homeSpeed);
    }
    if ( digitalRead(homePin) == LOW ) inZone = true;
  }
  if ( ! inZone ) {
    // find from the right
    for (int i=0; i < stepWidth && digitalRead(homePin) == HIGH; i++) {
      stepMotor(1, DIRECTION_CCW, homeSpeed);
    }
    if ( digitalRead(homePin) == LOW ) inZone = true;
  }
  
  // if in the home zone move to the lower edge
  sp2("Finding lower edge: ", digitalRead(homePin));
  for (int i=0; i < stepWidth && digitalRead(homePin) == LOW; i++) {
    stepMotor(1, DIRECTION_CCW, homeSpeed);
  }
  if ( digitalRead(homePin) == HIGH ) {
    sp2("Home found: ", digitalRead(homePin));
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  else {
    sp1("Home not found. One last try...");
    // find from the left  
    // the final hunt for home
    for (int i=0; digitalRead(homePin) == HIGH; i++) {
      stepMotor(1, DIRECTION_CCW, homeSpeed);
      if ( i > stepWidth ) {
	sp1x("Failed to find home!!!");
	// We seem to be lost.  Display the status of the homing signal
	// so we can adjust the position of the reed switch.  We also
	// disengage the motor so it doesn't get hot and we can easily
	// move the needle by hand.  To do this the dial has to be removed.
	digitalWrite(EN_PIN, HIGH);
	while ( true ) {             // HERE FOREVER
	  digitalWrite(redLed, HIGH);
	  vTaskDelay(300 / portTICK_PERIOD_MS);
	  sp2("HOME pin: ", digitalRead(homePin));
	  digitalWrite(redLed, LOW);
	}
      }
    }
  }
  // move to zero position (needs to be set by trial and error)
  stepMotor(4, DIRECTION_CCW, STEPPER_SPEED);
  sp1("Found home.");
  // move to our favorite station number (zero is off the scale to the left)
  moveToDialPos(gTunEncoderPos+1);
  gLastTunEncoderPos = gTunEncoderPos;
  
  // play the last played station
  audio.connecttohost(gStationList[gTunEncoderPos]);
  gPlayTime = 0;
  gPlayStart = millis();

  // install the  ISR service
  if ( gpio_install_isr_service(0) != ESP_OK ) {
    sp1x("Failed to install interrupt service!!!");
    while ( true ) { vTaskDelay(100); } // we be done
  }

  // Install the Tuner Encoder Interrupt
  if ( gpio_isr_handler_add(TUNER_APIN, tunerEncoderISR, (void *)gTunEncoderPos)
       != ESP_OK ) {
    sp1x("Failed to install tuner ISR!!!");
    while ( true ) { vTaskDelay(100); } // we be done
  }
  if ( gpio_set_intr_type(TUNER_APIN, GPIO_INTR_NEGEDGE) != ESP_OK ) {
    sp1x("Failed to set interrupt the tuner!!!");
    while ( true ) { vTaskDelay(100); } // we be done
  }    
  
  // Install the Volume Encoder Interrupt
  if ( gpio_isr_handler_add(VOLUME_APIN,
			    volumeEncoderISR, (void *)gVolEncoderPos)
       != ESP_OK ) {
    sp1x("Failed to install the volume ISR!!!");
    while ( true ) { vTaskDelay(100); } // we be done
  }
  if ( gpio_set_intr_type(VOLUME_APIN, GPIO_INTR_NEGEDGE) != ESP_OK ) {
    sp1x("Failed to set interrupt type for volume!!!");
    while ( true ) { vTaskDelay(100); } // we be done
  }    

  // Install the mute button interrupt
  if ( gpio_isr_handler_add(mutePin, muteEncoderISR, (void *)gVolEncoderPos)
       != ESP_OK ) {
    sp1x("Failed to install mute button ISR!!!");
    while ( true ) { vTaskDelay(100); }
  }
  if ( gpio_set_intr_type(mutePin, GPIO_INTR_NEGEDGE) != ESP_OK ) {
    sp1x("Failed to set interrupt type for mute button!!!");
    while ( true ) { vTaskDelay(100); }
  }
  
  // Install the nextTrack button interrupt
  if ( gpio_isr_handler_add(nextTrackPin,
			    nextTrackEncoderISR,
			    (void *)gNextTrack) != ESP_OK ) {
    sp1x("Failed to install next track button ISR!!!");
    while ( true ) { vTaskDelay(100); }
  }
  if ( gpio_set_intr_type(nextTrackPin, GPIO_INTR_NEGEDGE) != ESP_OK ) {
    sp1x("Failed to set interrupt type for mute button!!!");
    while ( true ) { vTaskDelay(100); }
  }

  sp1("Interrupts installed");
  
  // the play loop needs it's own task
  xTaskCreate(playStationTask, "PlayIRadio", 10000,
	      (void *)&gPlayParam, 1, NULL);

  // finally start the voice command module
  while ( !(zenith.begin()) ) {
    sp1x("Communication with Zenith failed. Check the connect to the\n"
	 " DFRoot voice recognition module.");
    delay(3000);
  }
  // DFRobot SEN0539 settings
  zenith.setVolume(7);   // volume (0-7) for the voice control responses
  zenith.setMuteMode(0);
  zenith.setWakeTime(20); // after wake-word, allow 20 seconds for commands
  zenith.playByCMDID(0); //  PLAY THE WAKE WORD

  // start the voice control loop here
  xTaskCreate(zenithVCTask, "ZenithVoiceControl", 50000,
	      (void *)&gTaskVar, 1, NULL);

  digitalWrite(grnLed, HIGH);
  server.begin();
}

unsigned long playPause() {
  gPlay = false;
  gPlayTime += (millis() - gPlayStart);
  if ( audio.isRunning() ) audio.pauseResume();
  return gPlayTime;
}

unsigned long playResume() {
  gPlay = true;
  gPlayStart = millis();
  if ( !audio.isRunning() ) audio.pauseResume();
  return gPlayTime;
}

unsigned long loopTimer = 0;

void loop() {
  if ( !gAPMode ) mainLoop();
  setupLoop();  
}

GetRequestParser reqParser;

void setupLoop() {
  WiFiClient client = server.accept();

  // stannn=url is 7 characters, 4*32 for password settings
  // nb: special characters can extend data beyond the prediction
  // (e.g. & becomes %26) but user input is restricted by characters typed
  //const size_t maxDataSize = 4*32 + STATION_LIMIT*STATION_CHAR_LIMIT;
  const size_t maxLineSize = STATION_LIMIT*7 + STATION_LIMIT*STATION_CHAR_LIMIT;
  const size_t maxValueSize = STATION_CHAR_LIMIT;
  
  if ( client ) {
    char line[maxLineSize];
    int idx = 0;
    bool espRestart = false;
    bool havePost = false;
    
    sp1("client connected");
    
    while ( client.connected() ) {
      if ( client.available() ) {
	char c = client.read();
	Serial.write(c);
	
	if ( c == '\n' ) {
	  if ( strstr(line, "POST ") ) {
	    havePost = true;
	  }
	  if ( strstr(line, "GET") ) {
	    char name[maxValueSize], value[maxValueSize];
	    int err;
	    
	    sp2("LINE: ", line);
	    sp2("Line size: ", strlen(line));
	    
	    if ( (err = reqParser.parse(line, false)) == REQPARSE_OKAY ) {
	      sp1("PARSED---------");
	      while ( reqParser.nextRequest(name, value, maxValueSize) ) {
		sp2sx(name, value);
		if ( strcmp(name, "Restart") == 0 ) espRestart = true;
	      }
	      if ( espRestart ) {
		// back to normal operation
		client.clear();
		client.stop();
		ESP.restart();
	      }
	    }
	  }
	  
	  if ( idx == 0 ) {
	    if ( havePost ) {
	      // POST content here
	      sp1("********* POST EVENTS HERE ********");
	      char *p = line;
	      *p = '\0';
	      while ( client.available() ) {
		char c = client.read();
		Serial.write(c);
		*p = c;
		p++;
		*p = '\0';
	      }
	      sp2("L:", line);
	      char name[maxValueSize], value[maxValueSize];
	      int err;
	      
	      sp2("LINE: ", line);
	      sp2("Line size: ", strlen(line));
	      
	      if ( (err = reqParser.parse(line, true)) == REQPARSE_OKAY ) {
		while ( reqParser.nextRequest(name, value, maxValueSize) ) {
		  sp2sx(name, value);
		  prefs.putString(name, value);// this updates all the settings
		  if ( strcmp(name, "ssid1") == 0 ) strcpy(ssids[0], value);
		  if ( strcmp(name, "ssid2") == 0 ) strcpy(ssids[1], value);
		  if ( strcmp(name, "pw1") == 0 ) strcpy(password[0], value);
		  if ( strcmp(name, "pw2") == 0 ) strcpy(password[1], value);
		  for (int i=0; i<STATION_LIMIT; i++) {
		    if ( strcmp(name, staIds[i]) == 0 ) {
		      strcpy(gStationList[i], value);
		    }
		  }
		}
	      }
	      else {
		if ( err == REQPARSE_NO_QUERY_ERR ) {
		  sp2("No query error ", err);
		}
		else if ( err == REQPARSE_MALLOC_ERR ) {
		  sp2("Memory allocation error ", err);
		}
		else {
		  sp2("Unspecified error ", err);
		}
	      }
	      Serial.println("********* END POST EVENTS *************");
	    }
	    
	    // start a reply
	    client.println("HTTP/1.1 200 OK");
	    client.println("Content-type:text/html");
	    client.println();

	    client.print(index_html);
	    
	    // Let's try to calculate the predicted data size to determine
	    // allocated size.
	    // plenty of room for wifi
	    sp2("size of index2 = ", sizeof(index2_html));
	    size_t predictedSize = sizeof(index2_html) + 4 * 80;
					   // settings (also headroom)
	    for (int i=0; i<STATION_LIMIT; i++) {
	      predictedSize += (strlen(gStationList[i])+1);
	    }
	    
	    // send back the settings page
	    char *buf1 = (char *)malloc(predictedSize);
	    if ( buf1 == NULL ) {
	      sp1x("Failed to allocate memory!!!");
	      vTaskDelay(100000/portTICK_PERIOD_MS);
	    }

	    snprintf(buf1, predictedSize, index2_html,
		    ssids[0], password[0], ssids[1], password[1], //  4
		    gStationList[0], gStationList[1],
		    gStationList[2], gStationList[3],
		    gStationList[4], gStationList[5],            // 10
		    gStationList[6], gStationList[7],
		    gStationList[8], gStationList[9],
		    gStationList[10], gStationList[11],           // 16
		    gStationList[12]
		    );

	    // did we exceed the length!?! (should be impossible)
	    sp2("allocated buf1: ", predictedSize);
	    sp2("buf1 size: ", strlen(buf1));
	    if ( strlen(buf1) >= predictedSize ) {
	      sp1x("\n************ WEB PAGE MEMORY EXCEEDED **************");
	      sp1x("restarting...");
	      ESP.restart();
	    }
	    sp1("Sending to client...");
	    sp2("Heap ", ESP.getFreeHeap());
	    client.print(buf1);
	    client.println();
	    free(buf1);
	    
	    break;
	  }
	  else {
	    idx = 0;
	    line[idx] = '\0';
	  }
	}
	else if ( c != '\r' ) {
	  line[idx] = c;
	  idx++;
	  line[idx] = '\0';
	  if ( idx > maxLineSize ) {
	    sp2("Line size exceeded!!! Size=", idx);
	  }
	}
      }
    } // end while client connected

    // clear the channel and stop the client
    idx = 0;
    client.clear();
    client.stop();
    sp1("Client disconnected.\n");
  } // end if client
} // end setupLoop

void mainLoop() {
  
  // we turn off the radio with gRadioOff so that we can update the
  // voice commands that are understood by the DFRobot SEN0359
  
  if ( gRadioOff ) {
    if ( audio.isRunning() ) playPause();
    while ( gRadioOff ) {
      uint8_t cmdId = zenith.getCMDID();
      if ( cmdId != 0 ) {
	sp2("CmdId = ", cmdId);
      }
      if ( cmdId == 20 ) { // turn on radio
	gRadioOff = false;
	gPlay = true;
      }
      vTaskDelay(500/portTICK_PERIOD_MS);
    }
  }
  
  // Skip to the next track served by the local stream
  if ( gNextTrack ) {
    gNextTrack = false;  // set to true by interrupt, restored here
    if ( gTunEncoderPos == localStreamPos ) {
      // for the local stream we go to the next track
      playPause();
      audio.connecttohost(gStationList[localStreamPos]);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      gPlayTime = 0;
      gPlayStart = millis();
      playResume();
      gNextTrack = false;
    }
    else if ( gTunEncoderPos == localPodcastPos ) {
      // for podcasts we advance playback by a minute
      char url[STATION_CHAR_LIMIT+14];
      playPause();
      int skipSeconds = 60 + (gPlayTime / 1000);
      sp2("Skipping seconds to ", skipSeconds);
      sprintf(url, "%s?start=%d", gStationList[localPodcastPos], skipSeconds);
      audio.connecttohost(url);    
      playResume();
      gPlayTime += 60000;
      gNextTrack = false;
    }
  }
  
  // play / resume
  
  if ( gPlay != gPlayLast ) {
    sp2("Changing mute status to ", gPlay);
    if ( gPlay && !audio.isRunning() ) playResume();
    if ( !gPlay && audio.isRunning() ) playPause();
    gPlayLast = gPlay;
  }

  // volume encoder changes
  
  if ( gVolEncoderPos != gLastVolume ) {
    gLastVolume = gVolEncoderPos;
    audio.setVolume(gLastVolume);
    sp2("Volume: ", gLastVolume);
    prefs.putInt("Volume", gLastVolume);
  }

  // Tuner encoder to select stations
  
  if ( gTunEncoderPos != gLastTunEncoderPos
       && millis() - gLastStationChange > 2000 ) {
    int pos = gTunEncoderPos;
    sp2("************* Encoder pos ", gTunEncoderPos);
    gPlay = false;
    moveToDialPos(pos+1);
    // it takes time to move, in that time the dial may have moved so
    // don't change stations until next time around
    if ( pos == gTunEncoderPos ) {
      audio.connecttohost(gStationList[pos]);
      gPlayTime = 0;
      gPlayStart = millis();
      gPlay = true;
      gLastTunEncoderPos = pos;
      gLastStationChange = millis();
      prefs.putInt("Station", pos);
    }
  }
  
  // set to true to monitor rssi and switch to best access point
  
  if ( false && (millis() - loopTimer > 10000) ) {
    // print the rssi every two seconds
    int rssi = WiFi.RSSI();
    sp2("AP ", WiFi.SSID());
    sp2("RSSI ", rssi);
    loopTimer = millis();
    if ( rssi < rssiMinimum ) {
      // just switch
      if ( gCurrentSSID == 0 )
	gCurrentSSID = 1;
      else
	gCurrentSSID = 0;
      WiFi.disconnect(true, true);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      WiFi.begin(ssids[gCurrentSSID], password[gCurrentSSID]);
      sp2("Switching to ", ssids[gCurrentSSID]);
      while ( WiFi.status() != WL_CONNECTED ) {
	sp1(".");
	vTaskDelay(500 / portTICK_PERIOD_MS);
      }
      sp2("Connected to ", ssids[gCurrentSSID]);
      sp2("RSSI ", WiFi.RSSI());
      // do a reconnect (might not be needed)
      audio.connecttohost(gStationList[gTunEncoderPos]);
    }
  }
}

// Audio status functions
//  we can rewrite these so they respect the debug setting

void audio_info(const char *info) {
  sp2("info        ", info);
  //Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info) { //id3 metadata
  Serial.print("id3data     "); Serial.println(info);
}
void audio_eof_mp3(const char *info) { //end of file
  Serial.print("eof_mp3     "); Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     "); Serial.println(info);
}
void audio_showstreaminfo(const char *info) {
  Serial.print("streaminfo  "); Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle "); Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     "); Serial.println(info);
}
void audio_commercial(const char *info) { //duration in sec
  Serial.print("commercial  "); Serial.println(info);
}
void audio_icyurl(const char *info) { //homepage
  Serial.print("icyurl      "); Serial.println(info);
}
void audio_lasthost(const char *info) { //stream URL played
  Serial.print("lasthost    "); Serial.println(info);
}
void audio_eof_speech(const char *info) {
  Serial.print("eof_speech  "); Serial.println(info);
}

/** clientReadLine
    @brief Read a line from the WiFi client. The returned line is null
    terminated. The end of lines are defined by \n characters on the
    input and the newline character is not returned. If size is
    reached before the newline character is encounted or there are no
    more characters to read, the line is returned. When this
    functional is called and there are no characters to read, it
    returns -1;
    
    @param lineBuf (char *) The line read is returned in this string.
    @param size (int) The size of the lineBuf.
    @return The number of characters returned in lineBuf or -1 if
    there is nothing more to read.
*/

int clientReadLine(WiFiClient *client, char *lineBuf, int size) {
  int n = 0;
  char c;

  if ( ! client->available() ) return -1;
  
  while ( client->available() && n < size ) {
    c = client->read();
    if ( c == '\n' ) break;
    lineBuf[n] = c;
    n++;
  }
  
  if ( n < size )
    lineBuf[n] = '\0';
  else if ( n > 0 )
    lineBuf[n-1] = '\0';
  return n;
}

void stepMotor(int steps, bool direction, int speed) {
  speed = constrain(speed, 0, 10);  // Ensure speed is within range
  int stepDelay = 20000 - (1000 * speed); // was 20000 (120,000 okay)
  digitalWrite(EN_PIN, LOW);   // enable
  digitalWrite(redLed, HIGH);  // motor active indication
  
  digitalWrite(DIR_PIN, direction); // Set direction
  
  for (int i = 0; i < abs(steps); i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(stepDelay); //stepDelay);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(stepDelay); // stepDelay);
  }
  digitalWrite(redLed, LOW);      // done moving
}

void moveToDialPos(int dialIndex) {
  int positionDiff = dialPosition[gCurrentIndex] - dialPosition[dialIndex];
  int steps = abs(positionDiff);
  int direction = positionDiff < 0 ? DIRECTION_CW : DIRECTION_CCW;
  stepMotor(steps, direction, STEPPER_SPEED);
  gCurrentIndex = dialIndex;
}

//////////////////////////////////////////////////////////////////////////

// GENERAL SETTINGS

void generalSettings() {

  WiFi.softAP(apSSID, NULL);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  sp2x("IP=", WiFi.softAPIP());
  
  server.begin();
}
