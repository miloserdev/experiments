#ESP8266 ESPNOW  
## ESP ESP-NOW Networking  
### A pure FreeRTOS-SDK version  

In this case we can setup peer-to-peer connecion throuth 2.4GHz and send packets with size less than 250 bytes  
ESP-NOW protocol can be used without wifi connection  
ESP-NOW Features  
    * Multicast (non-encrypted / encrypted data)  
    * Broadcast (non-encrypted data)  
    * Peer add / remove | up to 20 peers  

example:  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalRead": { "pin": 32 } }]` // Read pin state on board with MAC AB:CD:EF:A1:B2:C3;  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 0 } }]` // Turn on 32 pin on board with MAC AB:CD:EF:A1:B2:C3;    

support double commands:  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 33, "value": 0 } }, { "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 0 } }]`  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 33, "value": 1 } }, { "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 1 } }]`  
// only if JSON data length less than 200 bytes  