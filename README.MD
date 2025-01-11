# ESP32Framework
A basic framework for developing ESP32 based applications. The framework includes support for multiple instances, multi-target logging, remote firmware updates, MQTT messaging, Wifi, Display and Graphics, Audio, and many more useful tools.

## Product Details
### Requirements
#### Instance Management
1. Each instance shall support a way to uniquely identify itself
   - This may be an auto incrementing method or random number generation
#### MQTT messaging
1. Topic schema is {app_name}/Topic1/Topic2/.../Topic9 for outgoing (status/info) messages
   - ID shall be included in top level of payload with multiple parameters
   - ID shall be the last topic for single payload parameters unless the ID itself is the single parameter
2. Topic schema is {app_name}/{app_id}/Topic1/Topic2/.../Topic9 for incoming (command) messages
   - An special number/code will be reserved for the ALL {app_id} to which all instances shall respond
3. Payload shall be text for a single payload parameter or JSON for payloads with multiple parameters
4. Maximum topic string length shall be 25 characters
5. Common Commands:
   - Restart/Reboot - /reboot
   - Alive check, basic info - /hello?
   - Play Audio (with authorization token) - /play?
#### Connectivity
1. Support Wifi Connectivity - just configure SSID and password
2. Restart device if Wifi disconnects more than 5 times in 10 seconds
3. Connects to NTP Server
#### Firmware Updates
1. Instance will check a known server for updates
2. Firmware image will be secured
   - May be secured by preshared key (maybe not possible using SPIFFS)
   - May be secured by encrypted channel

### TODO


## Usage

## Libraries
	[bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)
	[bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson)
	[marvinroger/AsyncMqttClient](https://github.com/marvinroger/async-mqtt-client)
	[evotodi/TLogPlus](https://github.com/evotodi/tlogplus)
	[evotodi/TLogPlusTelnetStream](https://github.com/evotodi/tlogplus-telnet-stream)
	[evotodi/TLogPlusSyslogStream](https://github.com/evotodi/tlogplus-syslog-stream)
	[thijse/ArduinoLog](https://github.com/thijse/Arduino-Log)
   [takkaO/OpenFontRender](https://github.com/takkaO/OpenFontRender)
