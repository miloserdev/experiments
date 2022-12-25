#ESP32 Dev Board  
## ESP UDP 4 Port relay with RTC 

libs:	`Arduino JSON`, `RTCDS1307`, `AsyncUDP`, `WiFi`  

example:  
{ "digitalRead": { "pin": 15 }  
{ "digitalWrite": { "pin": 15, "value": 1 }  
{ "schedule": { "time": 1000, "cmd": { "digitalWrite": { "pin": 15, "value": 1 } } } }  
{ "schedule": "state" }  
{ "schedule": "clear" }  
