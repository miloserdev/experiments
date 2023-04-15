#ESP8266 ESPNOW  
## ESP ESP-NOW Networking  
### A pure FreeRTOS-SDK version  

example:  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalRead": { "pin": 32 } }]` // Read pin state;  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 0 } }]` // Turn on;    

support double commands:  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 33, "value": 0 } }, { "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 0 } }]`  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 33, "value": 1 } }, { "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 1 } }]`  
