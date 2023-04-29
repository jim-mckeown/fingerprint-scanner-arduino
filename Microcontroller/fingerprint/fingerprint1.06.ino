/******************************************************************
*  Send password associated with each finger to keyboard
*  Send "enroll" to serial port to enter enroll mode
*  Send "RESET" to serial port to clear fingerprints and passwords
*
*  Based on Adafruit examples
*  
******************************************************************/

#include <Adafruit_Fingerprint.h>
#include "Keyboard.h"
#include <EEPROM.h>

String rev = "1.06";
String releaseDate = "29 April 2023";
String author = "Jim McKeown";
int eeStrOffset = 64; // size of passwords in bytes, including null terminator
int pwLength = EEPROM.length() / eeStrOffset; // maximum number of passwords

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);
int mode = 0;   // mode 0: read finger (default)
                // mode 1: enroll finger and password to eeprom (enroll)
                // mode 2: clear fingerprints and passwords (RESET)
                // mode 3: repeat USB to and from UART (passthru)
                // mode 4: edit passwords no fingerprint enrollment (editPW) 
                // mode 5: clear single fingers (unenroll)

uint8_t id; // enrolled fingerprint and password index

void setup()
{
  Keyboard.begin(); // setup usb keyboard
  Serial.begin(57600); // setup usb serial port
  delay(1000);
  finger.begin(57600); // setup fingerprint reader on hardware serial port
  delay(5);
  if (! finger.verifyPassword()) // verify fingerprint reader communication 
  {
    Keyboard.println("Did not find fingerprint sensor...stopping.");
    while (1) { delay(1); } // stop
  }
}

void loop()
{
  if(mode == 0) // normal fingerprint to password mode
  {
    getFingerprintID(); // check for an enrolled fingerprint
    delay(50);          // don't need to run this at full speed
    if(Serial.available() != 0) // check for serial input
    {
      String strCmd = Serial.readString();
      if(strCmd.startsWith("enroll"))
      {
        mode = 1;
      }
      if(strCmd.startsWith("RESET"))
      {
        mode = 2;
      }
      if(strCmd.startsWith("passthru"))
      {
        mode = 3;
      }
      if(strCmd.startsWith("editPW"))
      {
        mode = 4;
      }
      if(strCmd.startsWith("unenroll"))
      {
        mode = 5;
      }      
      if(strCmd.startsWith("?"))
      {
        about();
      }
    }
  }
  if(mode == 1) // enroll fingerprint, password
  {
    Serial.println("Ready to enroll a fingerprint");
    Serial.print("Please type in the ID # (from 0 to ");
    Serial.print(pwLength - 1);
    Serial.println(") you want to save this finger as...");
    id = readnumber();
    if (id >= 0 && id < pwLength) 
    {
      Serial.print("Enrolling ID #");
      Serial.println(id);
      while (! getFingerprintEnroll() );
    }
  }
  if(mode == 2)
  {
    Serial.println("\n\nWARNING: Deleting all fingerprint templates!");
    Serial.println("Press 'Y' key to continue");

    while (1) 
    {
      if (Serial.available() && (Serial.read() == 'Y')) 
      {
        finger.emptyDatabase();
        for (int i = 0 ; i < EEPROM.length() ; i++) 
        {
          EEPROM.write(i, 0);
        }
        break;
      }
      else if(Serial.available() && (Serial.read() != 'Y'))
      {
        mode = 0;
        about();
        break;
      }
    }
  }
  if(mode == 3)
  {
    while(1) // stay in passthru mode
    {
      // read from port 1, send to port 0:
      if (Serial1.available()) {
        int inByte = Serial1.read();
        Serial.write(inByte);
      }
    
      // read from port 0, send to port 1:
      if (Serial.available()) {
        int inByte = Serial.read();
        Serial1.write(inByte);
      }
    }      
  }
  if(mode == 4) // edit passwords, do not disturb enrolled fingerprints
  {
    Serial.println("Ready to edit password");
    Serial.print("Please type in the ID # (from 0 to ");
    Serial.print(pwLength - 1);
    Serial.println(" you want to save this password as...");
    id = readnumber();
    if (id >= 0 && id < pwLength) 
    {
      Serial.print("Editing password for ID #");
      Serial.println(id);
      Serial.print("Enter password for ID "); 
      Serial.print(id); 
      Serial.print(" max "); 
      Serial.print(eeStrOffset - 1); 
      Serial.println(" characters:");
      String pw = readStr();
      Serial.print("Password = "); Serial.println(pw);
      int eeAddress = id * eeStrOffset;
      eeWriteString(eeAddress, pw);
    }
  }
  if(mode == 5) // unenroll individual fingers
  {
    Serial.println("Ready to delete fingerprint");
    Serial.print("Please type in the ID # (from 0 to ");
    Serial.print(pwLength - 1);
    Serial.println(" of the fingerprint model you want to detete...");
    id = readnumber();
    if (id >= 0 && id < pwLength) 
    {
      Serial.print("Deleting fingerprint model ID #");
      Serial.println(id);
      uint8_t p = -1;
      p = finger.deleteModel(id);
      if (p == FINGERPRINT_OK) 
      {
        Serial.println("Deleted");
      } 
      else if (p == FINGERPRINT_PACKETRECIEVEERR) 
      {
        Serial.println("Communication error");
      }
      else if (p == FINGERPRINT_BADLOCATION) 
      {
        Serial.println("Could not delete in that location");
      }
      else if (p == FINGERPRINT_FLASHERR) 
      {
        Serial.println("Error writing to flash");
      }
      else 
      {
        Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
      }
    }
  }
}

/****************************************************************
 * Checks for a recognized fingerprint from the currently
 * enrolled set.
 * 
 * If a fingerprint is recognized, the associated text (password)
 * is transmitted on the usb keyboard device.
 ***************************************************************/

void getFingerprintID() 
{
  uint8_t p = finger.getImage(); // scan for fingerprint
  if(p == FINGERPRINT_OK) // image taken
  {
    p = finger.getImage(); // scan for fingerprint a second time
    if(p == FINGERPRINT_OK) // image taken a second time  
    {
      p = finger.getImage(); // scan for fingerprint a thirs time
      if(p == FINGERPRINT_OK) // image taken a third time  
      {
        p = finger.image2Tz(); // convert image
        if(p == FINGERPRINT_OK) // image converted
        { 
          p = finger.fingerSearch(); // search for enrolled match
          if (p == FINGERPRINT_OK) // found a print match
          {
            Keyboard.print(eeReadString(finger.fingerID * eeStrOffset)); // send password to keyboard 
          }      
        }
      }
    }
  }
  return;
}

/**************************************************************
 * Prompt for index number of fingerprint/password
 * 
 * Wait for 3 consecutive valid fingerprint scans
 * 
 * Save associated password in eeprom
 *************************************************************/

uint8_t getFingerprintEnroll() 
{
  int p = -1;
  int i = 0;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) 
  {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      if(i > 2)
      {
        Serial.println();
        Serial.println("Image taken");
      }
      else
      {
        i++;
        p =  -1;
      }
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      i = 0;
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      i = 0;
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      i = 0;
      break;
    default:
      Serial.println("Unknown error");
      i = 0;
      break;
    }
  }

  p = finger.image2Tz(1);
  switch (p) 
  {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) 
  {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) 
  {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println();
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  p = finger.image2Tz(2);
  switch (p) 
  {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) 
  {
    Serial.println("Prints matched!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR) 
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_ENROLLMISMATCH) 
  {
    Serial.println("Fingerprints did not match");
    return p;
  }
  else 
  {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID ");
  Serial.println(id);
  
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) 
  {
    Serial.println("Stored!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("Enter password for ID "); 
  Serial.print(id); 
  Serial.print(" max "); 
  Serial.print(eeStrOffset - 1); 
  Serial.println(" characters:");
  String pw = readStr();
  Serial.print("Password = "); Serial.println(pw);
  int eeAddress = id * eeStrOffset;
  eeWriteString(eeAddress, pw);
  return true;
}

uint8_t readnumber(void) {
  uint8_t num = -1;
  while (! Serial.available());
  num = Serial.parseInt();
  Serial.readString(); // clear serial buffer
  return num;
}

String readStr(void) 
{
  while (! Serial.available());
  {
    delay(1);
  }
  String strRead = Serial.readString();
  return strRead;
}

void eeWriteString(int eeAddr, String strWrite)
{
  int strLen = strWrite.length() + 1;
  char charArray[strLen];
  strWrite.toCharArray(charArray, strLen);
  for(int i = 0;i < strLen;i++)
  {
    EEPROM.write(eeAddr + i, charArray[i]);
  }
}

String eeReadString(int eeAddr)
{
  char charArray[eeStrOffset];
  for(int i = 0;i < eeStrOffset;i++)
  {
    charArray[i] = EEPROM.read(eeAddr + i);
    if(charArray[i] == 0)
    {
      break;
    }
  }
  return String(charArray);
}
/***************************************************************
 * Send information on firmware, fingerprint sensor, eeprom
 * to serial port.
 * 
 **************************************************************/
void about()
{
  Serial.println("Fingerprint Password Manager");
  Serial.print("Version "); Serial.println(rev); 
  Serial.print("Release date: "); Serial.println(releaseDate); 
  Serial.print("Author: "); Serial.println(author);    
  if (finger.verifyPassword()) // verify fingerprint reader communication 
  {
    Serial.println("Found fingerprint sensor");
  } else 
  {
    Serial.println("Did not find fingerprint sensor...stopping.");
    while (1) { delay(1); } // stop
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x")); Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x")); Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: ")); Serial.println(finger.capacity);
  Serial.print(F("Security level: ")); Serial.println(finger.security_level);
  Serial.print(F("Device address: ")); Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: ")); Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: ")); Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0) 
  {
    Serial.print("Sensor doesn't contain any fingerprint data. Please enter 'enroll' mode.");
  }
  else 
  {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  }
  Serial.print("Maximum password length = "); Serial.println(eeStrOffset - 1);
  Serial.print("Maximum number of passwords = "); Serial.println(pwLength);  
  if(mode == 0)
  {
    Serial.println("Enter 'enroll' to switch to enroll mode.");
    Serial.println("Enter 'RESET' to clear all fingerprints and passwords.");
    Serial.println("Enter 'passthru' to switch to enter passthru mode (USB <--> UART.");
    Serial.println("Enter 'editPW' to edit passwords without disturbing fingerprint models.");
    Serial.println("Enter 'unenroll' to delete individual fingerprint models.");    
    Serial.println("Enter '? to see this information with no mode change.");
  }
}
