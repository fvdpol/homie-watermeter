# homie-watermeter
esp8266/mqtt watermeter using homie platform

# Objective

Enable read-out of the water meter to get measurement of this meter in OpenHAB using mqtt messages.

# Tools used
- ESP8266 dev board (Wemos D1 mini)
- Homie-esp8266 framework / platformIO
- TCRT5000 reflection sensor
- OpenHAB script for persistence



## OpenHAB Setup

### items
```xtend
// receive count from mqtt on intermediate tag; OpenHAB Rule will perform filter/log to update to total
// OR reset the instrument to previous value if it was restarted

Number WaterMeter_TotalReceived  "Water Total Received[%d l]" <water> () {mqtt="<[mosquitto:homie/600194100ff2/watertotal/total:state:default]"}
Number WaterMeter_Total  "Water Total [%d l]"    <water>  (Utilities, Water, F0_MeterCupboard)
Number WaterMeter_Flow   "Water Flow [%.1f l/m]" <water>  (Utilities, Water, F0_MeterCupboard) {mqtt="<[mosquitto:homie/600194100ff2/waterflow/flow:state:default]"}

Number WaterMeter_Uptime "Water Uptime [%d s]"   <water>  (Utilities, Water, F0_MeterCupboard) {mqtt="<[mosquitto:homie/600194100ff2/$stats/uptime:state:default]"}
```

### rules
```xtend
/* rules to handle the home water counter */

rule "Handle water meter update"
when
        Item WaterMeter_TotalReceived received update
then
        if (WaterMeter_TotalReceived.state < 1000) {
                // set last value to the water meter if it was reset to zero after powerup
                // -- allow for max 1m3 of pulses during startup
                publish("mosquitto","homie/600194100ff2/watertotal/total/set", WaterMeter_Total.state.toString)
        } else {
                postUpdate(WaterMeter_Total, WaterMeter_TotalReceived.state)
        }
end
```

# Tips

Adjust/reset counter by updating the counter device:
 `$mosquitto_pub -t homie/600194100ff2/watertotal/total/set -m 1234567`



... script



## Schematic

![Schematic](https://github.com/fvdpol/homie-watermeter/blob/master/images/schematic.PNG?raw=true)




## Pictures

Water Meter without the sensor:
![Water Meter](https://github.com/fvdpol/homie-watermeter/blob/master/images/meter_plain.jpg?raw=true)


Top view of the sensor board:
![Board top side](https://github.com/fvdpol/homie-watermeter/blob/master/images/board_top.jpg?raw=true)


Bottom view of the sensor board:
![Board back side](https://github.com/fvdpol/homie-watermeter/blob/master/images/board_back.jpg?raw=true)


Water Meter with the sensor installed:
![Water Meter with sensor](https://github.com/fvdpol/homie-watermeter/blob/master/images/meter_with_sensor.jpg?raw=true)
