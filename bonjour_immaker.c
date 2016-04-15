#include <SPI.h>
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>
#include <Client.h>
#include <Temboo.h>
#include "TembooAccount.h" // Contains Temboo account information

#define ADAFRUIT_CC3000_IRQ 3
#define ADAFRUIT_CC3000_VBAT 5
#define ADAFRUIT_CC3000_CS 10

Adafruit_CC3000 cc3k = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT);

Adafruit_CC3000_Client client;

int numRuns = 1;   // Execution count, so this doesn't run forever
int maxRuns = 10;   // Maximum number of times the Choreo should be executed
boolean trigger = false;
void setup() {
  Serial.begin(9600);
  
  // For debugging, wait until the serial console is connected
  delay(4000);
  while(!Serial);

  status_t wifiStatus = STATUS_DISCONNECTED;
  while (wifiStatus != STATUS_CONNECTED) {
    Serial.print("WiFi:");
    if (cc3k.begin()) {
      if (cc3k.connectToAP(WIFI_SSID, WPA_PASSWORD, WLAN_SEC_WPA2)) {
        wifiStatus = cc3k.getStatus();
      }
    }
    if (wifiStatus == STATUS_CONNECTED) {
      Serial.println("OK");
    } else {
      Serial.println("FAIL");
    }
    delay(5000);
  }

  cc3k.checkDHCP();
  delay(1000);

  Serial.println("Setup complete.\n");
}

void loop() {
runGetStockQuote();
if (trigger==true) {
  if (numRuns <= maxRuns) {
    Serial.println("Running SendSMS - Run #" + String(numRuns++));

    TembooChoreo SendSMSChoreo(client);

    // Invoke the Temboo client
    SendSMSChoreo.begin();

    // Set Temboo account credentials
    SendSMSChoreo.setAccountName(TEMBOO_ACCOUNT);
    SendSMSChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    SendSMSChoreo.setAppKey(TEMBOO_APP_KEY);

    // Set profile to use for execution
    SendSMSChoreo.setProfile("[Your Twilio profile name goes here(set on Temboo)]");

    // Set Choreo inputs
    String FromValue = "[Your registered Twillio phone number goes here]";
    SendSMSChoreo.addInput("From", FromValue);

    // Identify the Choreo to run
    SendSMSChoreo.setChoreo("/Library/Twilio/SMSMessages/SendSMS");

    // Run the Choreo; when results are available, print them to serial
    SendSMSChoreo.run();

    trigger=false;

    while(SendSMSChoreo.available()) {
      char c = SendSMSChoreo.read();
      Serial.print(c);
    }
    SendSMSChoreo.close();
  }

  Serial.println("\nWaiting...\n");
  delay(30000); // wait 30 seconds between SendSMS calls
} 
}

void runGetStockQuote() {
  TembooChoreo GetStockQuoteChoreo(client);

  // Set Temboo account credentials
  GetStockQuoteChoreo.setAccountName(TEMBOO_ACCOUNT);
  GetStockQuoteChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
  GetStockQuoteChoreo.setAppKey(TEMBOO_APP_KEY);

  // Set Choreo inputs
  // We used Valeant Pharmaceuticals Inc as an example VRX
  String StockSymbolValue = "vrx";
  GetStockQuoteChoreo.addInput("StockSymbol", StockSymbolValue);

  // Identify the Choreo to run
  GetStockQuoteChoreo.setChoreo("/Library/Yahoo/Finance/GetStockQuote");

  // Run the Choreo
  unsigned int returnCode = GetStockQuoteChoreo.run();

  // A return code of zero means everything worked
  if (returnCode == 0) {
    while (GetStockQuoteChoreo.available()) {
      String name = GetStockQuoteChoreo.readStringUntil('\x1F');
      name.trim();

      String data = GetStockQuoteChoreo.readStringUntil('\x1E');
      data.trim();
      
      //Bid was our selection, but there is also ASK Value, Min Value, Max Value, Open Value. Please refer to the yahoo finance API
      if (name == "Bid") {
        if (data.toFloat() < 40) {
          trigger=true;
        }else{
           Serial.println("don't buy!");
          }
      }
    }
  }

  GetStockQuoteChoreo.close();
}