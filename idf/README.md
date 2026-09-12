# Bruce Waveshare AMOLED 1.64 ESP-IDF probe

This is an isolated ESP-IDF hardware probe. It does not replace the existing
PlatformIO/Arduino Bruce build yet.

```bash
cd idf
source "$HOME/esp/esp-idf/export.sh"
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

The probe uses SPI2 for the SH8601 QSPI display and SPI3 for the SD card.
FT3168 is checked at I2C address `0x38`. The SH8601 command sequence is the
generic Arduino_GFX sequence and must be verified against hardware revision V1
before using it for the full Bruce display HAL.
