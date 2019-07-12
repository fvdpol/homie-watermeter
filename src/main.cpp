#include <Arduino.h>
#include <Homie.h>

// Water meter using TCRT5000 sensor
//
// count pulses
// compile-time configuration: #pulses/liter (depends on meter); now fixed 1 pulse per liter
// report on flowrate: l/map
// allow update/reset of totalizer from mqtt message
// totalizer; use persistence in mqtt server to survive reset if possible; otherwise reset using rule in OpenHAB
//
// 12 July 2019 issue with the counter value being reset to old value due to the watertotal/total/set being retained
//              solution is to decouple the reset from the counter, and have
//              - counter set as retained, not-settable
//              - reset   set as non-retained, settable


const int PIN_SENSOR = D1;  // D1 or gpio5

const int MEASURE_INTERVAL = 60;

unsigned long lastValueSent = 0;
unsigned long totalCount = 0;
unsigned long lastTotalCountSent = 0;

bool startupFlag = true;


Bounce debouncer = Bounce(); // Bounce is built into Homie, so you can use it without including it first
int lastSensorValue = -1;

HomieNode waterCounterNode("counter", "value","counter");
HomieNode waterCounterResetNode("counterreset", "value","reset");
HomieNode waterFlowNode("flow", "value","measurement");


void onHomieEvent(const HomieEvent& event) {
  switch (event.type) {
    case HomieEventType::STANDALONE_MODE:
      Homie.getLogger() << "Standalone mode started" << endl;
      break;
    case HomieEventType::CONFIGURATION_MODE:
      Homie.getLogger() << "Configuration mode started" << endl;
      break;
    case HomieEventType::NORMAL_MODE:
      Homie.getLogger() << "Normal mode started" << endl;
      break;
    case HomieEventType::OTA_STARTED:
      Homie.getLogger() << "OTA started" << endl;
      break;
//    case HomieEventType::OTA_PROGRESS:
//      Serial << "OTA progress, " << event.sizeDone << "/" << event.sizeTotal << endl;
//      break;
    case HomieEventType::OTA_FAILED:
      Homie.getLogger() << "OTA failed ----> force reset..." << endl;
      ESP.restart();
      break;
    case HomieEventType::OTA_SUCCESSFUL:
      Homie.getLogger() << "OTA successful" << endl;
      break;
    case HomieEventType::ABOUT_TO_RESET:
      Homie.getLogger() << "About to reset" << endl;
      break;
//    case HomieEventType::WIFI_CONNECTED:
//      Serial << "Wi-Fi connected, IP: " << event.ip << ", gateway: " << event.gateway << ", mask: " << event.mask << endl;
//      break;
//    case HomieEventType::WIFI_DISCONNECTED:
//      Serial << "Wi-Fi disconnected, reason: " << (int8_t)event.wifiReason << endl;
//      break;
//    case HomieEventType::MQTT_READY:
//      Serial << "MQTT connected" << endl;
//      break;
//    case HomieEventType::MQTT_DISCONNECTED:
//      Serial << "MQTT disconnected, reason: " << (int8_t)event.mqttReason << endl;
//      break;
//    case HomieEventType::MQTT_PACKET_ACKNOWLEDGED:
//      Serial << "MQTT packet acknowledged, packetId: " << event.packetId << endl;
//      break;
//    case HomieEventType::READY_TO_SLEEP:
//      Serial << "Ready to sleep" << endl;
//      break;
    default:    
      break;
  }
}


void setupHandler() {
  waterCounterNode.setProperty("unit").send("l");
  waterCounterResetNode.setProperty("unit").send("l");
  waterFlowNode.setProperty("unit").send("l/m");
}



void loopHandler() {
  if (millis() - lastValueSent >= MEASURE_INTERVAL * 1000UL || (millis() < lastValueSent || startupFlag)) {
    float duration_m = ((millis() - lastValueSent) / (60.0 * 1000.0));  // elapsed minutes
    float flow = ((totalCount - lastTotalCountSent) * 1.0) / duration_m;  // flow in liters/minute

    lastTotalCountSent = totalCount;
    lastValueSent = millis();

    Homie.getLogger() << "Flow: " << flow << " l/m" << endl;
    Homie.getLogger() << "Total: " << totalCount << " l" << endl;

    waterFlowNode.setProperty("value").send(String(flow));
    waterCounterNode.setProperty("value").send(String(totalCount));

    startupFlag = false;

  }


  int sensorValue = debouncer.read();
  if (sensorValue != lastSensorValue) {
     lastSensorValue = sensorValue;

     if (sensorValue != 0) {
       totalCount = totalCount + 1;
       Homie.getLogger() << "TotalCount is now " << totalCount << endl;
     }

  }
}

bool WaterCounterResetInputHandler(const HomieRange& range, const String& value) {
    Homie.getLogger() << "Received: " << value << endl;


    for (byte i = 0; i < value.length(); i++) {
      if (isDigit(value.charAt(i)) == false) return false;
    }

    const unsigned long numericValue = value.toInt();
    Homie.getLogger() << "Reset Total from " << totalCount << " to " << numericValue << " l" << endl;
    lastTotalCountSent = numericValue;
    totalCount = numericValue;
    lastValueSent = millis();

    // send value to provide immediate feedback
    waterCounterNode.setProperty("value").send(String(totalCount));

    return true;

}



void setup() {
    // put your setup code here, to run once:

  Serial.begin(115200);
  Serial << endl << endl;

  //pinMode(PIN_RELAY, OUTPUT);
  ///digitalWrite(PIN_RELAY, LOW);


  pinMode(PIN_SENSOR, INPUT);
  digitalWrite(PIN_SENSOR, LOW);  // no pull-up

  debouncer.attach(PIN_SENSOR);
  debouncer.interval(5); // was 50


  Homie_setFirmware("watermeter", "1.0.5");
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);

  waterCounterNode.advertise("unit");
  waterCounterNode.advertise("value");
      // old: (re)settable total count by sending mqtt message homie/node/watertotal/total/set 12345
      
  waterCounterResetNode.advertise("unit");
  waterCounterResetNode.advertise("value").setRetained(false).settable(WaterCounterResetInputHandler);
      // new: (re)settable total count by sending mqtt message homie/node/watertotalreset/reset/set 12345


  waterFlowNode.advertise("unit");
  waterFlowNode.advertise("value");


  Homie.onEvent(onHomieEvent);


  // Disable the LED blinking, to have the serial line working:
  Homie.disableLedFeedback();

  Homie.setup();

}

void loop() {
    // put your main code here, to run repeatedly:

  Homie.loop();
  debouncer.update();
}
