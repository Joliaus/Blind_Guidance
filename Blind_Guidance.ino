/**
 * @file      Blind_Guidance.ino
 * @authors   Timothe PETITJEAN
 * @copyright ESEO
 *
 * @brief  Robotic file  in relation  ship  with Arduino  hardware and  software
 * platform in order to guide a blinding human.
 */

// Import libraries
#include <Arduino.h>

#include <SD.h>
#include <SPI.h>        // SDcard SPI services
#include "HCSR04.h"     // Radar
#include <TMRpcm.h>     // Audio player

// Constantes

// Card        pin
// Mega2560    53
// Uno         10
#define SD_ChipSelectPin       53

// Global variables

// Audio variables
char* audioFile = "atnobs.wav";
const int speakerPin = 46;
// Radar variables
const uint8_t ECHO_IN_PIN = 4;
const uint8_t TRIGGER_OUT_PIN = 5;

// Create objects (Memory instance)

Sd2Card card;
SdVolume volume;
SdFile root;
TMRpcm tmrpcm;

// Program initialization

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  setup_console();
  setup_sdcard();
  setup_audio();
  setup_radar();
}

/******************
 * Main loop entry
 ******************/
void loop()
{
  /* --- Radar --- */
  int lengthCentimeter = getUSDistanceAsCentiMeterWithCentimeterTimeout(300);
  int lengthCentimeterAlert = 200;  // cm
  int intervalMessage = getPeriod(lengthCentimeter);

  // Debug :: Send data to the Serial Port
  Serial.print("info: Period = ");
  Serial.println(intervalMessage);

  /* --- Audio player --- */
  sendMessage(lengthCentimeter, lengthCentimeterAlert, intervalMessage);
}

/*****************/
/* Local methods */
/*****************/
void setup_console(void)
{
  // Open serial communications and wait for port to open:
  Serial.begin(115200); // Serial console bit rate (115200 bauds)
#if defined(__AVR_ATmega32U4__)                 \
  || defined(SERIAL_USB)                        \
  || defined(SERIAL_PORT_USBVIRTUAL)            \
  || defined(ARDUINO_attiny3217)
  // To be able  to connect Serial monitor  after reset or power  up and before
  // first print out. Do not wait for an attached Serial Monitor!
  while (!Serial) {}
#endif
}

void setup_sdcard(void)
{
  // Show message at the begining of SD card initialization
  Serial.println("");
  Serial.println("Initializing SD card ... ");
  // Setup digital port SD_ChipSelectPin in output mode
  pinMode(SD_ChipSelectPin, OUTPUT);
  // Call init service from Sd2Card library
  if (!card.init(SPI_HALF_SPEED, SD_ChipSelectPin))
    {
      // Exception: Initialization failed
      Serial.println("Initialization failed. Things to check:");
      Serial.println("* is card inserted ?");
      Serial.println("* Is your wiring correct ?");
      Serial.println("* did you change the chipSelect pin to match your shield or module ?");
      return;
    }
  // Print this message whether SD card initialization is successfully
  Serial.println("Wiring is correct and a card is present.");

  if (!SD.begin(SD_ChipSelectPin))
    {
      Serial.println("SD fail");
      return;
    }

  // Print the type of detected SD card
  Serial.print("SD card type: ");
  switch(card.type()) {
  case SD_CARD_TYPE_SD1:
    Serial.println("SD1");
    break;
  case SD_CARD_TYPE_SD2:
    Serial.println("SD2");
    break;
  case SD_CARD_TYPE_SDHC:
    Serial.println("SDHC");
    break;
  default:
    Serial.println("Unknown");
  }

  // Try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }

  // Print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  volumesize = volume.blocksPerCluster();    // Clusters are collections of blocks
  volumesize *= volume.clusterCount();       // We'll have a lot of clusters
  volumesize *= 512;                         // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);

  // List all files in the card with date and size
  root.ls(LS_R | LS_DATE | LS_SIZE);
  delay(2000);
}

void setup_radar(void)
{
  /*********/
  /* Radar */
  /*********/
  initUSDistancePins(TRIGGER_OUT_PIN, ECHO_IN_PIN); // Distance measurement

#if defined(TEENSYDUINO) // Add-on Teensyduino for Arduino's software (SDK)
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH); // Enable Amplified PROP shield
#endif
  delay(1000);
}

void setup_audio(void)
{
  tmrpcm.speakerPin = speakerPin; // 5,6,11 or 46 on Mega, 9 on Uno, Nano, etc
  // Complimentary Output or Dual Speakers:
  // pinMode(10,OUTPUT); Pin pairs: 9,10 Mega: 5-2,6-7,11-12,46-45
}

void sendSound(void)
{
  // Send warning message.
  tmrpcm.play(audioFile);
  while (tmrpcm.isPlaying()) {} // Wait until file is played
  tmrpcm.stopPlayback(); // Wait 1sec and stop
}

int getPeriod(int lengthCentimeter)
{
  if (lengthCentimeter >= 150)
    return 4000;
  else if (lengthCentimeter >= 100)
    return 2000;
  else if (lengthCentimeter >= 80)
    return 1000;
  else
    return 500;
}

void sendMessage(int lengthCentimeter, int lengthCentimeterAlert, int intervalMessage)
{
  if (lengthCentimeter >= lengthCentimeterAlert)
    {
      // Use Case :: Exception
      //    info :: Out of range :: Long range
      Serial.print("info: Longue distance, ");
      Serial.print(lengthCentimeter);
      Serial.println("cm");

    }
  else
    {
      // Use Case :: Nominal
      //     Print message, for instance: cm=32
      Serial.print("info: Cas nominal, ");
      Serial.print("cm=");
      Serial.println(lengthCentimeter);
      sendSound();
    }
  delay(intervalMessage);
}