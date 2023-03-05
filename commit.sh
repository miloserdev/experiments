#cp ~/Desktop/sketch_relay_new/sketch_relay_new.ino ./esp32_relay/esp32_relay_http.ino

#cp -r ~/Documents/PlatformIO/Projects/esp_dev/src/* ./esp_dev_relay/src/
#cp -r ~/Documents/PlatformIO/Projects/esp_dev/include/* ./esp_dev_relay/include/
#cp -r ~/Documents/PlatformIO/Projects/esp_dev/lib/* ./esp_dev_relay/lib/
#cp -r ~/Documents/PlatformIO/Projects/esp_dev/test/* ./esp_dev_relay/test

git add . && git commit -am "null" --no-edit && git push origin master
