#ESP32 Dev Board  
## ESP UDP 4 Port relay with RTC 
### A pure ESP-IDF version  

example:  
`[{ "digitalRead": { "pin": 32 } }]` // Read pin state;  
`[{ "digitalWrite": { "pin": 32, "value": 0 } }]` // Turn on;    
`[{ "schedule": { "time": 1000, "cmd": [{ "digitalWrite": { "pin": 32, "value": 1 } }] } }]` // Set scheduler to off;  
`[{ "schedule": "clear" }]` // Clear scheduler;  
`[{ "schedule": "state" }]` // Get scheduler state;  

support double commands:  
`[{ "digitalWrite": { "pin": 33, "value": 0 } }, { "digitalWrite": { "pin": 32, "value": 0 } }]`  
`[{ "digitalWrite": { "pin": 33, "value": 1 } }, { "digitalWrite": { "pin": 32, "value": 1 } }]`  
`[{ "schedule": { "time": 1000, "cmd": [{ "digitalWrite": { "pin": 33, "value": 0 } }, { "digitalWrite": { "pin": 32, "value": 0 } }] } }]`  
`[{ "schedule": { "time": 1000, "cmd": [{ "digitalWrite": { "pin": 33, "value": 1 } }, { "digitalWrite": { "pin": 32, "value": 1 } }] } }]`  
