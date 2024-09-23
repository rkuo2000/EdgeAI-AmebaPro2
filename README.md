# EdgeAI AMB82-mini
```
教材網址：https://github.com/rkuo2000/EdgeAI-AMB82-mini
程式範例：https://github.com/rkuo2000/Arduino
```

自強基金會 WiFi 
```
SSID: TCFSTWIFI.ALL
Pass: 035623116
```

## 1. AI 介紹

### [AI 簡介](https://rkuo2000.github.io/AI-course/lecture/2024/08/01/AI-Brief.html)

### [AI 硬體介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/01/AI-Hardwares.html)

---
## 2. AMB82-mini 介紹
官方賣場：[ICshopping : Realtek AMB82-mini AI Camera (Ameba RTL8735B) $799 NTD](https://www.icshop.com.tw/products/368030501864)<br>
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini.png?raw=true)
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_kit.png?raw=true)

---
### [RTL8735B](https://www.amebaiot.com/en/amebapro2/):
* **[User Manual](https://www.amebaiot.com/?s2member_file_download=AMB82-Mini_Hardware_User_Guide_0V3_20230303.pdf)** <br>
* **[Datasheet](https://www.amebaiot.com/datasheet-download-amb82-mini/)** <br>

32-bit Arm v8M, up to 500MHz, 768KB ROM, 512KB RAM, 16MB Flash (MCM embedded DDR2/DDR3L up to 128MB)<br>
802.11 a/b/g/n WiFi 2.4GHz/5GHz, BLE 5.1, *NN Engine 0.4 TOPS*, Crypto Engine, Audio Codec, ...<br>

<p><img width="50%" height="50%" src="https://www.amebaiot.com/wp-content/uploads/2023/03/amb82_mini.png"></p>

---
### AMB82-mini Pinout Diagram
<p><img src="https://github.com/rkuo2000/EdgeAI-MCU/blob/main/assets/AMB82-MINI_pinout.png?raw=true"></p>

---
### [Ameba Arduino](https://www.amebaiot.com/en/ameba-arduino-summary/)

![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino.png?raw=true)

---
## 3. Arduino IDE使用介紹

### [Ameba AIoT 影片](https://www.youtube.com/@amebaiot7033)

[![](https://markdown-videos-api.jorgenkh.no/youtube/-jQDpDFX2ao)](https://youtu.be/-jQDpDFX2ao)

---
### 下載 [Arduino IDE 2.3.2](https://www.arduino.cc/en/software) ＆ 安裝
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduio_IDE_download.png?raw=true"></p>

---
### 執行Arduino IDE 與偏好設定
**Preferences**加上 `https://github.com/ambiot/ambpro2_arduino/raw/main/Arduino_package/package_realtek_amebapro2_index.json`<br>

![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino_IDE_preference.png?raw=true)

---
### 選定開發板 AMB82-MINI
Tools > Board Manager > AMB82 package > 4.0.7<br>
<p><img width="75%" height="75%" src="https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino_IDE_BoardManager.png?raw=true"></p>

---
### [Getting Started](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-getting-started/)
首先將AMB82-mini板子用MicroUSB線 連接至電腦的USB port<br>

<p><img width="50%" height="50%" src="https://www.amebaiot.com/wp-content/uploads/2022/12/amb82-mini/P03.png"></p>

---
### Arduino examples 範例練習
* 01.Basics> Blink
  
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino_examples_01.Basics_Blink.png?raw=true)
    
* 02.Digitial> GPIO> Button
  
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino_examples_02.Digital_Button.png?raw=true)

修改程式碼：<br>
```
const int buttonPin = 1;  // the number of the pushbutton pin
const int ledPin = LED_BUILTIN;    // the number of the LED pin
```

---
### [AMB82-Mini 程式範例](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI)
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino_Sketch_Download_ZIP.png?raw=true"></p>
1. 瀏覽器打開 https://github.com/rkuo2000/Arduino, 點[Code]並選 [Download ZIP]
2. 解壓縮.zip, 並extract examples
3. 放入 Documents/Arduino

---
## 4. 感測器範例練習
Serial-monitor baud rate = `115200`<br>

### 紅外線測距模組
**[VL53L0X v2](https://www.ruten.com.tw/item/show?22425810394279)** <br>
**[VL53L1X v2](https://www.ruten.com.tw/item/show?22425810394279)** <br>
<p><img width="50%" height="50%" src="https://esphome.io/_images/vl53l0x.png"></p>

**Datasheet**: [VL53L0X - Time-of-Flight ranging sensor](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/vl53l0x.pdf)<br>\
**Sketch:** [IR_VL53L0X](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IR_VL53L0X)<br>

---
### 慣性感測模組
**[MPU6050](https://www.ruten.com.tw/item/show?22428017261803)** <br>
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/MPU6050.png?raw=true)

**[慣性元件介紹](https://rkuo2000.github.io/EdgeAI-course/lecture/2024/05/24/IMU.html)** <br>

**Sketch:** [IMU MPU6050-DMP6v12](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IMU_MPU6050_DMP6v12)<br>

---
## 5. 影像串流範例練習

### 影像串流
[RTSP_VideoOnly](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_VideoOnly/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/OmAnWOmt6WQ)](https://youtu.be/OmAnWOmt6WQ)

* [Amebapro2 AMB82-mini Arduino Example Guides](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-peripherals-examples)

### 動作偵測 (motion detection)
[![](https://markdown-videos-api.jorgenkh.no/youtube/g_ZP023XCIw)](https://youtu.be/g_ZP023XCIw)

---
## 6. Streaming 範例練習

### 彩色TFTLCD
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_button_SPI_TFTLCD.jpg?raw=true)

**[SPI - LCD Screen ILI9341 TFT](https://www.amebaiot.com/en/amebapro2-arduino-spi-lcd/)** <br>


#### Interface signal names:
* MOSI: Standard SPI Pin
* MISO: Standard SPI Pin
* SLK: Standard SPI Pin
* CS: Standard SPI Pin
* RESET: Used to reboot LCD.
* D/C: Data/Command. When it is at LOW, the signal transmitted are commands, otherwise the data transmitted are data.
* LED (or BL): Adapt the screen backlight. Can be controlled by PWM or connected to VCC for 100% backlight.
* VCC: Connected to 3V or 5V, depends on its spec.
* GND: Connected to GND.

---
#### AMB82 MINI and QVGA TFT LCD Wiring Diagram:<br>
![](https://www.amebaiot.com/wp-content/uploads/2023/01/spi/lcdP01.png)

**Sketch:** [Camera_TFTLCD](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/Camera_TFTLCD/Camera_TFTLCD.ino)<br>

**Exmples/AmebaSPI:** <br>
* LCD_Screen_ILI9341_TFT : LCD Draw Tests
* Camera_2_Lcd : Camera output , then Jpeg Decoder to TFT-LCD
* Camera_2_Lcd_JPEGDEC : Camera output, saved to SDcard, then Jpeg Decoder to read to TFT-LCD
  
---
## 7. 人臉辨識與識別

### [人臉辨識與識別介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/07/Face-Recognition.html)

### 人臉檢測範例
[RTSP_FaceDetection](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceDetection/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/KD95JH6gVew)](https://youtu.be/KD95JH6gVew)

### 人臉識別範例
[RTSP_FaceRecognition](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceRecognition/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/GGOIQmMfeF8)](https://youtu.be/GGOIQmMfeF8)

---
## 8. 音頻

### 音頻分類範例
[AudioClassification](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/AudioClassification/)<br>
[YAMNet](https://codimd.mcl.math.ncu.edu.tw/s/hoOqEgBSf)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/oi8ML6aJcvI)](https://youtu.be/oi8ML6aJcvI)

---
## 9. 影像分類 (Image Classification)

### [卷積層神經網路介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/03/CNN.html)

### [影像分類介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/04/Image-Classification.html)

### 影像分類範例
[RTSP_ImageClassification.ino](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/RTSP_EmotionClassification/RTSP_EmotionClassification.ino)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/c3XGkc9ShwQ)](https://youtu.be/c3XGkc9ShwQ)

---
## 10. 物件偵測 (Object Detection)

### [物件檢測介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/05/Object-Detection.html)

### AMB82 Mini 物件偵測範例
[RTSP_ObjectDetectionLoop](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_ObjectDetectionLoop)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/EvryVoQyqqk)](https://youtu.be/EvryVoQyqqk)

### AMB82 Mini SD卡加載模型範例
[RTPS_ObjectDetection_AudioClassification.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_ObjectDetection_AudioClassification/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/cVvdnXiCAa4)](https://youtu.be/cVvdnXiCAa4)

### Online NN Conversion Tool (客製化模型轉換工具)
[![](https://markdown-videos-api.jorgenkh.no/youtube/6cHC2cOKgQk)](https://youtu.be/6cHC2cOKgQk)

---
## 11. 大型語言模型範例 (LLM)

### [大型語言模型範例介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/15/LLM.html) 

### AMB82 Mini - 語音辨識範例
* [AmebaPro2 Whisper server](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_server.py)
* [RecordMP4_HTTP_Post_Audio.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

### AMB82 Mini - 語音交談範例
* [AmebaPro2 Whisper LLM_server](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_LLM_server.py)
* [RecordMP4_HTTP_Post_Audio.ino](hhttps://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

---
## 12. 視覺語言模型 (VLM)

### [視覺語言模型介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/16/VLM.html) 

### AMB82 Mini - 影像+語音交談範例
* [AmebaPro2_whisper_llava_server.py](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/src/AmebaPro2_whisper_llava_server.py)<br>
* [RecordMP4_CaptureJPG_HTTP_Post_AudioImage](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_CaptureJPG_HTTP_Post_AudioImage)

---
## 13.AMB82-mini 專題實作

### [Portable ChatGPT](https://github.com/rkuo2000/portable-ChatGPT)
[![](https://markdown-videos-api.jorgenkh.no/youtube/7rfmXPqyLF0)](https://youtu.be/7rfmXPqyLF0)

### Vaccum Robot
[![](https://markdown-videos-api.jorgenkh.no/youtube/YHR3ZOmtcyU)](https://youtu.be/YHR3ZOmtcyU)

### RoboCar 
(voice-control, BLE remote-control, line-notify, object-detection)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/vr_V1QnVMts)](https://youtu.be/vr_V1QnVMts)

### AI Door-Bell
[![](https://markdown-videos-api.jorgenkh.no/youtube/IumI-uAtkRU)](https://youtu.be/IumI-uAtkRU)

### Portable LLM/VLM Demo
[![](https://markdown-videos-api.jorgenkh.no/youtube/3PnHSE_8rRM)](https://youtu.be/3PnHSE_8rRM)



