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
## 2. 開發板介紹

### RTL8735B晶片簡介
32-bit Arm v8M, up to 500MHz, 768KB ROM, 512KB RAM, 16MB Flash (MCM embedded DDR2/DDR3L up to 128MB)<br>
802.11 a/b/g/n WiFi 2.4GHz/5GHz, BLE 5.1, *NN Engine 0.4 TOPS*, Crypto Engine, Audio Codec, ...<br>

---
### [HUB 8735 Ultra](https://www.ruten.com.tw/item/show?22434411139868)
#### [https://github.com/ideashatch/HUB-8735](https://github.com/ideashatch/HUB-8735)

![](http://winstouch.com.tw/Util/GetPicture.aspx?PicturePath=/UploadFile/IMG_17188933653479.jpg&QueryWidth=400)
![](http://winstouch.com.tw/Util/GetPicture.aspx?PicturePath=/UploadFile/IMG_172407964226684.jpg&QueryWidth=800)

[![](https://markdown-videos-api.jorgenkh.no/youtube/-_NMUnY0kK4)](https://youtu.be/-_NMUnY0kK4)

---
### [AMB82-mini](https://www.icshop.com.tw/products/368030501864)
#### [Ameba Arduino](https://www.amebaiot.com/en/ameba-arduino-summary/)

![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini.png?raw=true)
![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_kit.png?raw=true)
<p><img src="https://github.com/rkuo2000/EdgeAI-MCU/blob/main/assets/AMB82-MINI_pinout.png?raw=true"></p>

---
## 3. Arduino IDE使用介紹

### [Arduino IDE 2.3.2](https://www.arduino.cc/en/software) 下載 & 安裝
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduio_IDE_download.png?raw=true"></p>

---
### 偏好設定 (Preferences)
**Hub8735 ultra**<br>
`https://raw.githubusercontent.com/ideashatch/HUB-8735/main/amebapro2_arduino/Arduino_package/ideasHatch.json`<br>

**AMB832-mini**<br>
`https://github.com/ambiot/ambpro2_arduino/raw/main/Arduino_package/package_realtek_amebapro2_index.json`<br>

![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_Arduino_IDE_preference.png?raw=true)

### [Ameba AIoT 影片](https://www.youtube.com/@amebaiot7033)
[![](https://markdown-videos-api.jorgenkh.no/youtube/-jQDpDFX2ao)](https://youtu.be/-jQDpDFX2ao)
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
## 5.彩色顯示器 TFTLCD

### AMB82-mini + ILI9341 TFT-LCD
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
## 6. 影像串流範例練習

### 影像串流
**Sketch**: [RTSP_VideoOnly](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_VideoOnly/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/OmAnWOmt6WQ)](https://youtu.be/OmAnWOmt6WQ)

* [Amebapro2 AMB82-mini Arduino Example Guides](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-peripherals-examples)

---
### [動作偵測 (motion detection)](https://www.amebaiot.com/en/amebapro2-arduino-video-motion/)
**Examples:** AmebaMultimedia / MotionDetection / LoopPostProcessing.ino<br>
* 修改ssid, passwd, 後燒錄到AMB82-mini,
* 按reset後程式即開始運行, 用serial-monitor 查看顯示串流網址
* 啟動手機或電腦上之VLC player, 設定RTSP串流網址
<p><img width="50%" height="50%" src="https://www.amebaiot.com/wp-content/uploads/2023/01/video/motionP06.png"></p>

---
### [Motion Detection Google Line Notify](https://www.amebaiot.com/en/amebapro2-arduino-motion-notify/)
**Examples:** AmebaMultimedia / MotionDetection / MotionDetectGoogleLineNotify<br>

[![](https://markdown-videos-api.jorgenkh.no/youtube/g_ZP023XCIw)](https://youtu.be/g_ZP023XCIw)

---
## 7. 音頻應用 
Audio & Mic<br>

### 音頻環回測試
**Examples:** AmebaMultimedia / Audio /LoopbackTest<br>

![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_button_audiojack.jpeg?raw=true)

---
### 音頻串流範例
**Examples:** AmebaMultimedia / Audio / RTSPAudioStream<br>

[RTSP Audio Stream](https://www.amebaiot.com/en/amebapro2-arduino-audio-rtsp/)<br>

---
### MP4錄音範例
**Examples:** AmebaMultimedia / RecordMP4 / AudioOnly<br>

[Multimedia - MP4 Recording](https://www.amebaiot.com/en/amebapro2-arduino-video-mp4/)<br>

---
### 音頻分類範例
**Examples:** [AmebaNN / AudioClassification](https://www.amebaiot.com/en/amebapro2-arduino-neuralnework-audio-classification/)<br>

[YAMNet](https://codimd.mcl.math.ncu.edu.tw/s/hoOqEgBSf)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/oi8ML6aJcvI)](https://youtu.be/oi8ML6aJcvI)

![](https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/AMB82-mini_TFTLCD_button_audiojack.jpeg?raw=true)

---
## 8. 人臉辨識與識別

### [人臉辨識與識別介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/07/Face-Recognition.html)

### [人臉檢測範例](https://www.amebaiot.com/en/amebapro2-arduino-neuralnework-face-detection/)
**Examples:** AmebaNN / RTSPFaceDetection<br>

[![](https://markdown-videos-api.jorgenkh.no/youtube/KD95JH6gVew)](https://youtu.be/KD95JH6gVew)

### [人臉識別範例](https://www.amebaiot.com/en/amebapro2-arduino-neuralnework-face-recognition/)
**Examples:** AmebaNN / RTSPFaceRecognition<br>

[![](https://markdown-videos-api.jorgenkh.no/youtube/GGOIQmMfeF8)](https://youtu.be/GGOIQmMfeF8)

Serial_monitor: `REG=RKUO`<br>
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-AMB82-mini/blob/main/assets/FaceRecognition_REG_RKUO.jpeg?raw=true"></p>

* Enter the command **REG=Name** to give the targeted face a name.
* Enter the command **DEL=Name** to delete a certain registered face. For example, `DEL=SAM`
* Enter the command **BACKUP** to save a copy of registered faces to flash.
* If a backup exists, enter the command **RESTORE** to load registered faces from flash.
* Enter the command **RESET** to forget all previously registered faces. 

---
## 9. 影像分類 (Image Classification)
**[Kaggle平台使用介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/02/Kaggle-Intro.html)** <br>

### [卷積層神經網路介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/03/CNN.html)

### [影像分類介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/04/Image-Classification.html)

### 影像分類範例
[RTSP_ImageClassification.ino](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/RTSP_EmotionClassification/RTSP_EmotionClassification.ino)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/c3XGkc9ShwQ)](https://youtu.be/c3XGkc9ShwQ)

---
### 模型檔案轉換
**垃圾回收辨識：** [kaggle.com/rkuo2000/garbage-cnn](https://www.kaggle.com/code/rkuo2000/garbage-cnn)<br>

1. 修改成 model.save('garbage_cnn.h5', include_optimizer=False) # for AMB82-mini
2. 重新訓練模型 Run.all
3. 下載garbage_cnn.h5
4. [Amebapro2 AI convert model](https://www.amebaiot.com/en/amebapro2-ai-convert-model/)
5. zip garbage_cnn.h5, and upload
6. provide one test picture (.jpg), and upload

---
## 10. 物件偵測 (Object Detection)

### Dataset
**[Roboflow](https://universe.roboflow.com/)** <br>

### [物件檢測介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/05/Object-Detection.html)
**Kaggle:** <br>
* [YOLOv7 Facemask detection](https://www.kaggle.com/code/rkuo2000/yolov7-facemask-detection)
* [YOLOv7 Pothole detection](https://www.kaggle.com/code/rkuo2000/yolov7-pothole)
* [YOLOv7 Sushi detection](https://www.kaggle.com/code/rkuo2000/yolov7-sushi-detection)
* [YOLOv7 refrigeratoryfood](https://www.kaggle.com/code/rkuo2000/yolov7-refrigeratoryfood)<br>
* [YOLOv7 reparm](https://www.kaggle.com/code/rkuo2000/yolov7-reparam)<br>

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

### [大型語言模型介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/15/LLM.html) 

### AMB82 Mini - 語音辨識範例
#### ffmpeg.exe is needed for Windows to run Whisper!
* [AmebaPro2 Whisper server](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_server.py)
  
* [RecordMP4_HTTP_Post_Audio.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

---
### AMB82 Mini - 語音交談範例
#### ffmpeg.exe is needed for Windows to run Whisper!
Download [ffmpeg-master-latest-win64-gpl.zip](https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-win64-gpl.zip), extract & put ffmpeg.exe into  where you run Whisper server.<br>

#### [AmebaPro2 Whisper LLM_server](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_LLM_server.py)

#### [AmebaPro2_Whisper_Gemini_server](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_Gemini_server.py) 
* 到 https://aistudio.google.com/app/apikey 取得API_Key 填入GOOGLE_API_KEY, 再執行AmebaPro2_Whisper_Gemini_server.py

#### [RecordMP4_HTTP_Post_Audio.ino](hhttps://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)
* 修改server IP位址 in RecordMP4_HTTP_Post_Audio.ino server IP位址, then 燒錄到 AMB82-MINI
* reset AMB82-MINI 來啟動, 按鍵兩秒後即可錄音詢問 LLM/Gemini
  
---
## 12. 視覺語言模型 (VLM)

### [視覺語言模型介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/16/VLM.html) 

### AMB82 Mini - 影像+語音交談範例
* [AmebaPro2_whisper_llava_server.py](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/src/AmebaPro2_whisper_llava_server.py)
  
* [RecordMP4_CaptureJPG_HTTP_Post_AudioImage](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_CaptureJPG_HTTP_Post_AudioImage)

---
## 13.AMB82-mini 應用專題實作

### Vaccum Robot
(trash classification, IR collision detection)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/YHR3ZOmtcyU)](https://youtu.be/YHR3ZOmtcyU)

### RoboCar 
(voice-control, BLE remote-control, line-notify, object-detection)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/vr_V1QnVMts)](https://youtu.be/vr_V1QnVMts)

### AI Door-Bell
[![](https://markdown-videos-api.jorgenkh.no/youtube/IumI-uAtkRU)](https://youtu.be/IumI-uAtkRU)

### Portable LLM/VLM Demo
[![](https://markdown-videos-api.jorgenkh.no/youtube/3PnHSE_8rRM)](https://youtu.be/3PnHSE_8rRM)

### Portable ChatGPT
[![](https://markdown-videos-api.jorgenkh.no/youtube/7rfmXPqyLF0)](https://youtu.be/7rfmXPqyLF0)

### Portable Gemini
