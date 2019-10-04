/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

/*
V1    No Connect
V2    No Connect
V3    Clear Table
V11   Volume Paging
V12   Volume BGM
V20   FadeInterval
V21   FadeIncrement
V30   Terminal
V31   Table CurentDateTime
V100    UpTimeDisplay
V101    WiFi.RSSI(); WiFi Strength in dBm
V102    WiFi Quality in %
V103    Blynk Version
V104    ESP.getCoreVersion();
V105    WiFi.macAddress();
V106    WiFi.LocalIP().toString();
V107    sensorValue
*/

#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
//#include <BlynkSimpleEsp8266_SSL.h>

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "l6nfaf90oeFG5G4RZasQVygFocLk5Y3d"; // Token untuk ke Cloud
//char auth[] = "GBTvEYzGX1kLsoFC3_eojWZfeoXrBKq0"; // Token untuk ke Local Server ME

// Your WiFi credentials.
// Set password to "" for open networks.
char ssid[] = "DRM";
char pass[] = "3768326583";
//char blynksvr[] = "blynkapp.myddns.me"; // Local Server ME
char blynksvr[] = "blynk-cloud.com"; // Cloud Server
//int port = 8000; // Local Server ME
int port = 80; // Cloud Server

/*
// Mac address should be different for each device in your LAN
byte arduino_mac[] = { 0x01, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress device_ip  (192, 168,   0,  253);
IPAddress dns_ip     (  8,   8,   8,   8);
IPAddress gateway_ip (192, 168,   0,   254);
IPAddress subnet_mask(255, 255, 255,   0);
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <X9C.h>
#include <TimeLib.h>            // Used by WidgetRTC.h
#include <WidgetRTC.h>          // Blynk's RTC
//for LED status
#include <Ticker.h>
Ticker ticker;

WidgetRTC rtc;
int rowIndex = 0;

#define INC1 13   // D1 Mini D7 - pulled up in H/W (10k) ->  chip pin 1
#define UD1 15   // D1 Mini D8                          ->  chip pin 2
#define CS1 12  // D1 Mini D6 - pulled up in H/W (10k) ->  2nd chip pin 7

#define INC2 4   // D1 Mini D2 - pulled up in H/W (10k) ->  chip pin 1
#define UD2 5   // D1 Mini D1                          ->  chip pin 2
#define CS2 14  // D1 Mini D5 - pulled up in H/W (10k) ->  2nd chip pin 7

X9C pot1;                           // create a pot1 controller
X9C pot2;                           // create a pot2 controller

int volumeState1 =  LOW;
int volumeState2 =  LOW;

int btnState1 = HIGH;
int btnState2 = HIGH;

unsigned int i1 = 0;
unsigned int i2 = 0;

unsigned int volume1 = 100;
unsigned int volume2 = 100;

#define Seconds 1000UL
#define Minutes (5 * Seconds)

/* Sensor Audio on A0 */
int sensorPin = A0;    // select the input pin for the potentiometer
//int ledPin = 22;      // select the pin for the LED
int sensorValue = 0;
boolean pin_active = false ;
unsigned long pin_starttime ;
/* Sensor Audio on A0 */

/* LED Indicator */
const byte pwmLED = 2;
 
// define directions for LED fade
#define UP 0
#define DOWN 1
 
// constants for min and max PWM
const int minPWM = 0;
const int maxPWM = 1024; // ESP 32 : 255 | ESP 8266 : 1024
 
// State Variable for Fade Direction
byte fadeDirection = UP;
 
// Global Fade Value
// but be bigger than byte and signed, for rollover
int fadeValue = 0;
 
// How smooth to fade?
byte fadeIncrement = 5;
 
// millis() timing Variable, just for fading
unsigned long previousFadeMillis;
 
// How fast to increment?
int fadeInterval = 7;

/* LED Indicator */

/* UpTime */
int days = 0;
int seconds = 0;
int minutes = 0;
int hours = 0;
/* UpTime */

BlynkTimer timer;
WidgetTerminal terminal(V30);

// Every time we connect to the cloud...
BLYNK_CONNECTED() {

  rtc.begin();
  // Request the latest state from the server
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
  Blynk.syncVirtual(V11);
  Blynk.syncVirtual(V12);
  Blynk.syncVirtual(V20);
  Blynk.syncVirtual(V21);
  Blynk.virtualWrite(V103, BLYNK_VERSION);
  Blynk.virtualWrite(V104, ESP.getCoreVersion());
  Blynk.virtualWrite(V105, WiFi.macAddress());
  Blynk.virtualWrite(V106, WiFi.localIP().toString());

  printOutput(String("Blynk v" BLYNK_VERSION ": Device started"));
  /******** SYNC **************/

  // Alternatively, you could override server state using:
  //Blynk.virtualWrite(V1, ledState1);
  //Blynk.virtualWrite(V2, ledState2);
  
}

const char* host = "esp8266-webupdate";
//const char* ssid = STASSID;
//const char* password = STAPSK;

ESP8266WebServer server(80);
//const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>ESP32 Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='admin')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
 
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

/*
// When App button is pushed - switch the state ON | OFF
BLYNK_WRITE(V1) {  
  // Turn the strip on
  volumeState1 = param.asInt();
  if (volumeState1 == 1) {
    //led_on1 = true;
    Serial.println(F("On 1"));
    i1 = 0;
    while (i1 < volume1) {
      //analogWrite(ledPin1, i1); //ESP 8266
      //ledcWrite(1, i1); // set the volume of the LED | ESP 32
      pot1.setPot(i1,true);
      Serial.println(i1);
      i1 += 1;
      delay(6);
    }
  }
  // Turn the strip off
  else if (volumeState1 == 0) {
    //led_on1 = false;
    Serial.println(F("Off 1"));
    while (i1 > 10) {
      //analogWrite(ledPin1, i1); //ESP 8266
      //ledcWrite(1, i1); // set the volume of the LED | ESP 32
      pot1.setPot(i1,true);
      Serial.println(i1);
      i1 -= 1;
      delay(6);
    }
    //analogWrite(ledPin1, 0); //ESP 8266
    //ledcWrite(1, 0); // set the volume of the LED | ESP 32
    pot1.setPot(0,true);
    i1 = 0;
    Serial.println(i1);
  }  
}

BLYNK_WRITE(V11) {  // volume 0-100 point
  unsigned int new_volume = param.asInt();
  //unsigned int new_volume = 255 * bright_perc / 100;
  if (new_volume > 100) new_volume = 100;
  //if (led_on1) {
    if (new_volume > i1) {
      while (i1 < new_volume) {
        //analogWrite(ledPin1, i1);
        //ledcWrite(1, i1); // set the volume of the LED | ESP 32
        pot1.setPot(i1,true);
        Serial.println(i1);
        i1 += 1;
        delay(6);
      }
      //analogWrite(ledPin1, new_volume);
      //ledcWrite(1, i1); // set the volume of the LED | ESP 32
      pot1.setPot(i1,true);
      Serial.println(i1);
      i1 = new_volume;
    }
    else {
      while (i1 > new_volume) {
        //analogWrite(ledPin1, i1); //ESP 8266
        //ledcWrite(1, i1); // set the volume of the LED | ESP 32
        pot1.setPot(i1,true);
        Serial.println(i1);
        i1 -= 1;
        delay(6);
      }
      //analogWrite(ledPin1, new_volume); //ESP 8266
      //ledcWrite(1, i1); // set the volume of the LED | ESP 32
      pot1.setPot(i1,true);
      Serial.println(i1);
      i1 = new_volume;
    }
  //}
  volume1 = new_volume;
}

// When App button is pushed - switch the state
BLYNK_WRITE(V2) {
  // Turn the strip on
  volumeState2 = param.asInt();
  if (volumeState2 == 1) {
    //led_on2 = true;
    Serial.println(F("On 2"));
    i2 = 0;
    while (i2 < volume2) {
      //analogWrite(ledPin2, i2); //ESP 8266
      //ledcWrite(2, i2); // set the volume of the LED | ESP 32
      pot2.setPot(i2,true);
      Serial.println(i2);
      i2 += 1;
      delay(6);
    }
  }
  // Turn the strip off
  else if (volumeState2 == 0) {
    //led_on2 = false;
    Serial.println(F("Off 2"));
    while (i2 > 10) {
      //analogWrite(ledPin2, i2); //ESP 8266
      //ledcWrite(2, i2); // set the volume of the LED | ESP 32
      pot2.setPot(i2,true);
      Serial.println(i2);
      i2 -= 1;
      delay(6);
    }
    //analogWrite(ledPin2, 0); //ESP 8266
    //ledcWrite(2, 0); // set the volume of the LED | ESP 32
    pot2.setPot(0,true);
    i2 = 0;
    Serial.println(i2);
  }
}

BLYNK_WRITE(V12) { // volume 0-100 point
  unsigned int new_volume = param.asInt();
  //unsigned int new_volume = 255 * bright_perc / 100;
  //pot2.setPot(new_volume,true);
  
  if (new_volume > 100) new_volume = 100;
  //if (led_on2) {
    if (new_volume > i2) {
      while (i2 < new_volume) {
        //analogWrite(ledPin2, i2);
        //ledcWrite(2, i2); // set the volume of the LED | ESP 32
        pot2.setPot(i2,false);
        //pot2.trimPot(i2,X9C_UP,false);
        Serial.println(i2);
        i2 += 1;
        //delay(6);
      }
      //analogWrite(ledPin2, new_volume);
      //ledcWrite(2, i2); // set the volume of the LED | ESP 32
      pot2.setPot(i2,false);
      //pot2.trimPot(i2,X9C_UP,false);
      Serial.println(i2);
      i2 = new_volume;
    }
    else {
      while (i2 > new_volume) {
        //analogWrite(ledPin2, i2); //ESP 8266
        //ledcWrite(2, i2); // set the volume of the LED | ESP 32
        pot2.setPot(i2,false);
        //pot2.trimPot(i2,X9C_DOWN,false);
        //pot2.trimPot(i2,X9C_UP,false);
        Serial.println(i2);
        i2 -= 1;
        //delay(6);
      }
      //analogWrite(ledPin2, new_volume); //ESP 8266
      //ledcWrite(2, i2); // set the volume of the LED | ESP 32
      pot2.setPot(i2,false);
      //pot2.trimPot(i2,X9C_DOWN,false);
      //pot2.trimPot(i2,X9C_UP,false);
      Serial.println(i2);
      i2 = new_volume;
    }
  //}
  volume2 = new_volume;
  
}
*/

BLYNK_WRITE(V3){
  if (param.asInt()) {
  printOutput("Clear Table");
  /******** READY **************/
  Blynk.virtualWrite(V31, "clr"); //clear the table at start
  }
}

BLYNK_WRITE(V11)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  //pot.trimPot(pinValue,X9C_DOWN,false);  // pot will now be at about 7% (+10-3) max value
  Serial.println(pinValue);
  pot1.setPot(pinValue,true);
  // process received value
}

BLYNK_WRITE(V12)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  //pot.trimPot(pinValue,X9C_DOWN,false);  // pot will now be at about 7% (+10-3) max value
  Serial.println(pinValue);
  pot2.setPot(pinValue,true);
  // process received value
}

BLYNK_WRITE(V20)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  //pot.trimPot(pinValue,X9C_DOWN,false);  // pot will now be at about 7% (+10-3) max value
  fadeInterval = pinValue;
}

BLYNK_WRITE(V21)
{
  int pinValue = param.asInt(); // assigning incoming value from pin V1 to a variable
  //pot.trimPot(pinValue,X9C_DOWN,false);  // pot will now be at about 7% (+10-3) max value
  fadeIncrement = pinValue;
}

void UpTime(){
  int quality = 0;
  
  seconds = ++seconds;

  // Seconds logic
  switch(seconds){
    case 60:
      seconds = 0;
      minutes = ++minutes;
      break;
    default:
      break;
  }

  // Minutes logic
  switch(minutes){
    case 60:
      minutes = 0;
      hours = ++hours;
      break;
    default:
      break;
  }

  //Hours logic
  switch(hours){
    case 24:
      hours = 0;
      days = ++days;
      break;
    default:
      break;
  }
  //Serial.println(String(days) + "d:" + String(hours) + "h:" + String(minutes) + "m:" + String(seconds) + "s");
  String UpTimeDisplay = String(days) + "d:" + String(hours) + "h:" + String(minutes) + "m:" + String(seconds) + "s";
  Blynk.virtualWrite(V100, UpTimeDisplay); 
  Blynk.virtualWrite(V101, (WiFi.RSSI()));
  
  int dbstrength = WiFi.RSSI();
  
  if (dbstrength <= -100) {
    quality = 0;
  } else if (dbstrength >= -50) {
    quality = 100;
  } else {
    quality = 2 * (dbstrength + 100);
  }
  //Serial.println(quality);
  Blynk.virtualWrite(V102, String(quality));
}

void CheckSensor(){
   
  sensorValue = analogRead(sensorPin);
  Blynk.virtualWrite(V107, sensorValue);
  
  if (pin_active)   // pin active, only check for timeout
  {
    if (millis () - pin_starttime >= Minutes)  // wraparound-safe timeout test
    {
      pin_active = false ;   // change state
      //Blynk.virtualWrite(V12, 100);
      /* Log di Terminal */
      terminal.print("Sensor Value : ");
      terminal.println(sensorValue);
      printOutput("Do'a / Paging End");
      terminal.flush();  //agar semua data dikirimkan.
      /* Log di Terminal */
      
      /* Table CurrentDateTime */
      Blynk.virtualWrite(V31, "add", rowIndex, getCurrentDate() + String("  ") + getCurrentTime(), "UnMute");
      //highlighting latest added row in table
      Blynk.virtualWrite(V31, "pick", rowIndex); //highlight any item in a table by using it’s id in a table:
      rowIndex++;
      /* Table CurrentDateTime */
      
      //Serial.println("V2 = HIGH");
      pot2.setPot(100,true);
      //digitalWrite (ledPin, LOW) ;
      //Serial.println("LOW");
    }
  }
  
  if (sensorValue > 479 || sensorValue < 449 ) // just check for analog input pin 32 ESP32
    {
       pin_active = true ;  // change state to active
       //Blynk.virtualWrite(V12, 0);
       pot2.setPot(0,true);
       /* Log di Terminal */
       terminal.print("Sensor Value : ");
       terminal.println(sensorValue);
       printOutput("Do'a / Paging Start");
       terminal.flush();
       /* Log di Terminal */

       /* Table CurrentDateTime */
       Blynk.virtualWrite(V31, "add", rowIndex, getCurrentDate() + String("  ") + getCurrentTime(), "Mute");
       //highlighting latest added row in table
       Blynk.virtualWrite(V31, "pick", rowIndex); //highlight any item in a table by using it’s id in a table:
       rowIndex++;
       /* Table CurrentDateTime */
       
       //Serial.println("V2 = LOW");
       //digitalWrite (ledPin, HIGH) ;
       pin_starttime = millis () ;   // setup for timeout testing
       
    }  
}

/*
  getCurrentTime() - returns the current time as String 11:59:59
*/
String getCurrentTime() {
  String extraZeroH, extraZeroM, extraZeroS;
  if (hour() < 10) extraZeroH = '0';
  if (minute() < 10) extraZeroM = '0';
  if (second() < 10) extraZeroS = '0';
  return String(extraZeroH + hour()) + ':' + extraZeroM + minute() + ':' + extraZeroS + second();
}
/*
  getCurrentDate() - returns the current date as String DD-MM-YYYY
*/
String getCurrentDate() {
  return String(day()) + '-' + monthShortStr(month()) + '-' + year();
}
/*
  printTimeDate() - prints the current date and time to terminal with line break
*/
void printTimeDate() {
  terminal.println("-----------------------------");
  terminal.println( getCurrentDate() + String(" | ") + getCurrentTime() );
}
/*
  printOutput() - easy print to terminal with date function
*/
void printOutput(String a) {
  printTimeDate();
  terminal.println(a);
  terminal.flush();
}
/*
  formatTime() - Format millis in to MM:SS
*/
String formatTime(long milliSeconds) {
  long hours = 0, mins = 0, secs = 0;
  String returned, secs_o, mins_o;
  secs = milliSeconds / 1000;
  mins = secs / 60; 
  hours = mins / 60; 
  secs = secs - (mins * 60); 
  mins = mins - (hours * 60); 
  if (secs < 10 && mins) secs_o = "0";
  if (mins) returned += mins + String("m ");
  returned += secs_o + secs + String("s");
  return returned;
}

void doTheFade(unsigned long thisMillis) {
  // is it time to update yet?
  // if not, nothing happens
  if (thisMillis - previousFadeMillis >= fadeInterval) {
    // yup, it's time!
    if (fadeDirection == UP) {
      fadeValue = fadeValue + fadeIncrement;  
      if (fadeValue >= maxPWM) {
        // At max, limit and change direction
        fadeValue = maxPWM;
        fadeDirection = DOWN;
      }
    } else {
      //if we aren't going up, we're going down
      fadeValue = fadeValue - fadeIncrement;
      if (fadeValue <= minPWM) {
        // At min, limit and change direction
        fadeValue = minPWM;
        fadeDirection = UP;
      }
    }
    // Only need to update when it changes
    analogWrite(pwmLED, fadeValue);  // set the brightness of the LED | arduino
    //Serial.println(fadeValue);
    //ledcWrite(0, fadeValue); // set the brightness of the LED | ESP 32
 
    // reset millis for the next iteration (fade timer only)
    previousFadeMillis = thisMillis;
  }
}

void setup(void) {
  Serial.begin(115200);
  ticker.attach(0.2, tick);
  // initialize the LED pin as an output:
  pinMode(pwmLED, OUTPUT);
  pot1.begin(CS1,INC1,UD1);
  pot1.setPot(5,true);           // true=save, pot1 (Paging) 5% max volume saat start sehingga tidak ada hentakan 
  pot2.begin(CS2,INC2,UD2);
  pot2.setPot(5,true);           // true=save, pot2 (BGM) 5% max volume saat start sehingga tidak ada hentakan
  // pot.setpotMin(false);           // don't save set to 0% max - actually 200R-300R on my samples 
  // pot1.trimPot(10,X9C_UP,false);   // temporarily about 10% max value
  //pot1.trimPot(50,X9C_DOWN,false);  // pot1 will now be at about 7% (+10-3) max value
  //
  // because none of the above "tweaks" saved their value, you should find pot set at about 44% after shutdown
  // and of course, next time you run the sketch.
  //  
  Serial.println();
  Serial.println("Booting Sketch... Berhasil yey");
  //WiFi.mode(WIFI_AP_STA);
  //WiFi.begin(ssid, password);

  /* Blynk Connection Start */
  //WiFi.config(device_ip, gateway_ip, subnet_mask);
  WiFi.begin(ssid, pass); 
  Blynk.config(auth, blynksvr, port);
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.connect(2000);
  }
  //Blynk.begin(auth, ssid, pass, "blynkapp.myddns.me", 8000); //Local Server
  //Blynk.begin(auth, ssid, pass, IPAddress(10,57,57,88), 8080); //Local Server  
  //Blynk.begin(auth, ssid, pass);
  
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    MDNS.begin(host);
    server.on("/", HTTP_GET, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", serverIndex);
    });
    server.on("/update", HTTP_POST, []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(maxSketchSpace)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
    server.begin();
    MDNS.addService("http", "tcp", 80);

    Serial.printf("Ready! Open http://%s.local in your browser\n", host);
  } else {
    Serial.println("WiFi Failed");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  terminal.clear();
  timer.setInterval(1000L, UpTime); //UpTime 
  timer.setInterval(100L, CheckSensor); //CheckSensor  
  ticker.detach();
  
}

void loop(void) {
  /*  Running Blynk hanya jika WiFi ada koneksi, jika tidak ada maka tidak dijalankan  */
  if (WiFi.status() == WL_CONNECTED) {
    if (Blynk.connected() == true) {
      Blynk.run();
    }
    else {
      Blynk.connect(3000);
    }
  }
  //Blynk.run();
  server.handleClient();
  timer.run();
  MDNS.update();
   // get the current time, for this time around loop
  // all millis() timer checks will use this time stamp
  unsigned long currentMillis = millis(); 
  doTheFade(currentMillis); //call function doTheFade 
}
