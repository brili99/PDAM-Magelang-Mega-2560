#include <Arduino.h>
#include <Wire.h>
#include <RtcDS1307.h>
#include <SPI.h>
#include <SD.h>

#define sim Serial1
#define countof(a) (sizeof(a) / sizeof(a[0]))

void gsm_send_serial(String command);
void gsm_config_gprs();
void gsm_http_post(String postdata);
unsigned char h2int(char c);
String urlencode(String str);
void initSdCard();
void writeSdCardFile(char *fileName, String text);
String readSdCardFile(char *fileName);
void printDateTime(const RtcDateTime &dt);
bool wasError(const char *errorTopic = "");
void getRTC();
void initRTC();
bool deleteSdCardFile(char *fileName);

String apn = "3gprs";                                         // APN
String apn_u = "3gprs";                                       // APN-Username
String apn_p = "3gprs";                                       // APN-Password
String url = "http://pdam-magelang.pcbjogja.com/api/device/"; // URL of Server

String dataRetApi = "";
bool addDataRet = false;
String dataBuilder = "";
String dataReadAll = "";
RtcDS1307<TwoWire> Rtc(Wire);
RtcDateTime now;

void setup()
{
  Serial.begin(9600);
  Serial.println("SIM800 AT CMD Test");
  sim.begin(9600);
  initSdCard();
  delay(5000);
  while (sim.available())
  {
    Serial.write(sim.read());
  }
  delay(2000);
  gsm_config_gprs();
}

void loop()
{
  dataBuilder = "id=";
  dataBuilder += urlencode("D001");
  dataBuilder += "&t=";
  dataBuilder += urlencode("09:53:00");
  dataBuilder += "&p=";
  dataBuilder += urlencode((String)random(1, 9));
  dataBuilder += "&f=";
  dataBuilder += urlencode((String)random(1, 9));
  dataBuilder += "&vi=";
  dataBuilder += urlencode((String)random(1, 9));
  dataBuilder += "&vs=";
  dataBuilder += urlencode((String)random(1, 9));
  if (Serial.available())
  {
    switch (Serial.read())
    {
    case 'f':
      gsm_http_post(dataBuilder);
      break;
    case 's':
      gsm_send_serial("AT+SAPBR=1,1");
      gsm_send_serial("AT+SAPBR=2,1");
      gsm_send_serial("AT+HTTPINIT");
      gsm_send_serial("AT+HTTPPARA=CID,1");
      gsm_send_serial("AT+HTTPPARA=URL," + url);
      gsm_send_serial("AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded");
      break;
    case 'r':
      gsm_send_serial("AT+HTTPDATA=192,5000");
      gsm_send_serial(dataBuilder);
      gsm_send_serial("AT+HTTPACTION=1");
      gsm_send_serial("AT+HTTPREAD");
      break;
    case 't':
      gsm_send_serial("AT+HTTPTERM");
      gsm_send_serial("AT+SAPBR=0,1");
      break;
    default:
      break;
    }
  }
  if (sim.available())
  {
    Serial.write(sim.read());
  }
  

  // delay(10000);
}

void gsm_http_post(String postdata)
{
  Serial.println(" --- Start GPRS & HTTP --- ");
  gsm_send_serial("AT+SAPBR=1,1");
  gsm_send_serial("AT+SAPBR=2,1");
  gsm_send_serial("AT+HTTPINIT");
  gsm_send_serial("AT+HTTPPARA=CID,1");
  gsm_send_serial("AT+HTTPPARA=URL," + url);
  gsm_send_serial("AT+HTTPPARA=CONTENT,application/x-www-form-urlencoded");
  gsm_send_serial("AT+HTTPDATA=192,5000");
  gsm_send_serial(postdata);
  gsm_send_serial("AT+HTTPACTION=1");
  gsm_send_serial("AT+HTTPREAD");
  gsm_send_serial("AT+HTTPTERM");
  gsm_send_serial("AT+SAPBR=0,1");
}

void gsm_config_gprs()
{
  Serial.println(" --- CONFIG GPRS --- ");
  gsm_send_serial("AT+SAPBR=3,1,Contype,GPRS");
  gsm_send_serial("AT+SAPBR=3,1,APN," + apn);
  if (apn_u != "")
  {
    gsm_send_serial("AT+SAPBR=3,1,USER," + apn_u);
  }
  if (apn_p != "")
  {
    gsm_send_serial("AT+SAPBR=3,1,PWD," + apn_p);
  }
}

void gsm_send_serial(String command)
{
  Serial.println("Send ->: " + command);
  sim.println(command);
  long wtimer = millis();
  while (wtimer + 3000 > millis())
  {
    while (sim.available())
    {
      char d = sim.read();
      Serial.write(d);
      switch (d)
      {
      case '~':
        addDataRet = true;
        break;
      case '#':
        addDataRet = false;
        Serial.print("[HTTP RESP] ");
        Serial.println(dataRetApi);
        break;

      default:
        if (addDataRet)
        {
          dataRetApi += d;
          break;
        }
        dataRetApi = "";
        break;
      }

      // Serial.write(sim.read());
    }
  }
  // Serial.println();
}

String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++)
  {
    c = str.charAt(i);
    if (c == ' ')
    {
      encodedString += '+';
    }
    else if (isalnum(c))
    {
      encodedString += c;
    }
    else
    {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9)
      {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9)
      {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      // encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}

unsigned char h2int(char c)
{
  if (c >= '0' && c <= '9')
  {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f')
  {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F')
  {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

//////////////////
// RTC

void initRTC()
{
  Rtc.Begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(3000 /* us */, true /* reset_on_timeout */);
#endif

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();

  if (!Rtc.IsDateTimeValid())
  {
    if (!wasError("setup IsDateTimeValid"))
    {
      // Common Causes:
      //    1) first time you ran and the device wasn't running yet
      //    2) the battery on the device is low or even missing

      Serial.println("RTC lost confidence in the DateTime!");
      // following line sets the RTC to the date & time this sketch was compiled
      // it will also reset the valid flag internally unless the Rtc device is
      // having an issue

      Rtc.SetDateTime(compiled);
    }
  }

  if (!Rtc.GetIsRunning())
  {
    if (!wasError("setup GetIsRunning"))
    {
      Serial.println("RTC was not actively running, starting now");
      Rtc.SetIsRunning(true);
    }
  }

  now = Rtc.GetDateTime();
  if (!wasError("setup GetDateTime"))
  {
    if (now < compiled)
    {
      Serial.println("RTC is older than compile time, updating DateTime");
      Rtc.SetDateTime(compiled);
    }
    else if (now > compiled)
    {
      Serial.println("RTC is newer than compile time, this is expected");
    }
    else if (now == compiled)
    {
      Serial.println("RTC is the same as compile time, while not expected all is still fine");
    }
  }

  // never assume the Rtc was last configured by you, so
  // just clear them to your needed state
  Rtc.SetSquareWavePin(DS1307SquareWaveOut_Low);
  wasError("setup SetSquareWavePin");
}

void getRTC()
{
  if (!Rtc.IsDateTimeValid())
  {
    if (!wasError("loop IsDateTimeValid"))
    {
      // Common Causes:
      //    1) the battery on the device is low or even missing and the power line was disconnected
      Serial.println("RTC lost confidence in the DateTime!");
    }
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (!wasError("loop GetDateTime"))
  {
    printDateTime(now);
    Serial.println();
  }
}

bool wasError(const char *errorTopic = "")
{
  uint8_t error = Rtc.LastError();
  if (error != 0)
  {
    // we have a communications error
    // see https://www.arduino.cc/reference/en/language/functions/communication/wire/endtransmission/
    // for what the number means
    Serial.print("[");
    Serial.print(errorTopic);
    Serial.print("] WIRE communications error (");
    Serial.print(error);
    Serial.print(") : ");

    switch (error)
    {
    case Rtc_Wire_Error_None:
      Serial.println("(none?!)");
      break;
    case Rtc_Wire_Error_TxBufferOverflow:
      Serial.println("transmit buffer overflow");
      break;
    case Rtc_Wire_Error_NoAddressableDevice:
      Serial.println("no device responded");
      break;
    case Rtc_Wire_Error_UnsupportedRequest:
      Serial.println("device doesn't support request");
      break;
    case Rtc_Wire_Error_Unspecific:
      Serial.println("unspecified error");
      break;
    case Rtc_Wire_Error_CommunicationTimeout:
      Serial.println("communications timed out");
      break;
    }
    return true;
  }
  return false;
}

void printDateTime(const RtcDateTime &dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(datestring);
}

//////////////////

//////////////////
// SD Card
void initSdCard()
{
  Serial.print("Initializing SD card...");

  if (!SD.begin())
  {
    Serial.println("failed!");
    return;
  }
  Serial.println("done.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
}

void writeSdCardFile(char *fileName, String text)
{
  File myFile = SD.open(fileName, FILE_WRITE);
  // if the file opened okay, write to it:
  if (!myFile)
  {
    // if the file didn't open, print an error:
    Serial.print("error opening ");
    Serial.println(fileName);
    return;
  }
  Serial.print("Writing to ");
  Serial.print(fileName);
  myFile.println(text);
  // close the file:
  myFile.close();
  Serial.println(" done.");
}

String readSdCardFile(char *fileName)
{
  String ret = "";
  // re-open the file for reading:
  File myFile = SD.open(fileName);
  if (!myFile)
  {
    // if the file didn't open, print an error:
    Serial.print("error opening ");
    Serial.println(fileName);
    return ret;
  }
  // read from the file until there's nothing else in it:
  while (myFile.available())
  {
    ret += (char)myFile.read();
  }
  // close the file:
  myFile.close();
  return ret;
}

bool deleteSdCardFile(char *fileName)
{
  Serial.print("Removing ");
  Serial.print(fileName);
  Serial.print("...");
  SD.remove(fileName);

  if (SD.exists(fileName))
  {
    Serial.println("failed!");
    return false;
  }
  Serial.println("done!");
  return true;
}

//////////////////