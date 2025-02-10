# Source
[Link](https://github.com/tesa-klebeband/RTL8720dn-Deauther)
## DISCLAIMER

This tool has been made for **educational** and **testing** purposes only. Any misuse or illegal activities conducted with the tool are strictly prohibited.  
I am **not** responsible for any consequences arising from the use of this tool, which is done at your own risk.

---

## Firmware Upload Guide

### Setup
1. Open **Arduino IDE**.
2. Click **File** > **Preferences**.
3. Add the following Additional Boards Manager URL:  
   `https://github.com/ambiot/ambd_arduino/raw/master/Arduino_package/package_realtek_amebad_index.json`
4. Go to **Boards Manager** and search for **BW16**.
5. Download the **Realtek Amebaa Boards** (version 3.1.5) manager.

### How to Upload Firmware
1. Open the `.ino` file in Arduino IDE.
2. Click on **Tools**.
3. Enable **Auto Upload Mode** and click **Enable**.
4. Enable **Standard Lib** and select **Arduino_STD_PRINTF**.
5. Click **Upload** to upload the firmware.

---

## Requirements

- **SSD1306 Display**
- **4 Buttons**
- **BW16 Board**

---

## Connections

### Buttons
- **Up Button**: `PA27`
- **Down Button**: `PA12`
- **Select Button**: `PA13`
- **Back Button**: `PB2`

### SSD1306 Display
- **SDA**: `PA26`
- **SCL**: `PA25`

---
