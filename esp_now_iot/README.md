# ESP8266 ESPNOW (Old version)  
## ESP ESP-NOW Networking  
### A pure FreeRTOS-SDK version  

## Features  
 * WiFi  
 * ESP-NOW  
 * OTA Updates  
 * JSON Command Parser  

In this case we can setup peer-to-peer connecion throuth 2.4GHz and send packets with size less than 250 bytes  
ESP-NOW protocol can be used without wifi connection  
ESP-NOW Features  
 * Multicast (non-encrypted / encrypted data)  
 * Broadcast (non-encrypted data)  
 * Peer add / remove | up to 20 peers  

## Example  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalRead": { "pin": 32 } }]` // Read pin state on board with MAC AB:CD:EF:A1:B2:C3;  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 0 } }]` // Turn on 32 pin on board with MAC AB:CD:EF:A1:B2:C3;    

### support double commands  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 33, "value": 0 } }, { "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 0 } }]`  
`[{ "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 33, "value": 1 } }, { "to": "AB:CD:EF:A1:B2:C3" "digitalWrite": { "pin": 32, "value": 1 } }]`  
// only if JSON data length less than 200 bytes  


### Errata  
    1. Print cJSON number variables causes Guru Meditation :D;  
        FIX: Need to disable "nano" formatting in menuconfig;  
        make menuconfig -> Component config -> Newlib -> "nano" formatting  

    2. UART data sending only in non separated format;  
        i will make a buffer concatenator (maybe later XD);  
        echo -en '\x12\x02[{"to":"34:94:54:62:9f:74","digitalWrite":{"pin":2,"value":2}}]' > /dev/ttyUSB1  

### WARNING  
    1. OTA Updates possible only if your partitions setup correctly  
        it need to contains `ota_0` and `ota_1` partitions  