# EdgeAI AMB82-mini

## 1. AI 介紹

### [AI 簡介](https://rkuo2000.github.io/AI-course/lecture/2023/12/01/AI-Brief.html)

### [AI 硬體介紹](https://rkuo2000.github.io/AI-course/lecture/2024/08/01/AI-Hardwares.html)

---
## 2. AMB82-mini 介紹

### [RTL8735B](https://www.amebaiot.com/en/amebapro2/):
32-bit Arm v8M, up to 500MHz, 768KB ROM, 512KB RAM, 16MB Flash (MCM embedded DDR2/DDR3L up to 128MB)<br>
802.11 a/b/g/n WiFi 2.4GHz/5GHz, BLE 5.1, *NN Engine 0.4 TOPS*, Crypto Engine, Audo Codec, ...<br>

<p><img width="50%" height="50%" src="https://www.amebaiot.com/wp-content/uploads/2023/03/amb82_mini.png"></p>

### AMB82-mini Pinout Diagram
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-MCU/blob/main/assets/AMB82-MINI_pinout.png?raw=true"></p>

* [Ameba Arduino](https://www.amebaiot.com/en/ameba-arduino-summary/)
* [AMB82-Mini 程式範例](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI)
  
---
## 3. [Arduino IDE使用介紹](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-getting-started/)
* Preferences : `https://raw.githubusercontent.com/ambiot/ambpro2_arduino/dev/Arduino_package/package_realtek_amebapro2_early_index.json`
  
* [Ameba AIoT 影片](https://www.youtube.com/@amebaiot7033)
  
[![](https://markdown-videos-api.jorgenkh.no/youtube/-jQDpDFX2ao)](https://youtu.be/-jQDpDFX2ao)

---
### Arduino 範例練習
* Blinky
* GPIO button

---
## 4. 感測器範例練習

### 紅外線測距
**Datasheet**: [VL53L0X - Time-of-Flight ranging sensor]()<br>
[IR_VL53L0X](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IR_VL53L0X)<br>

---
### 慣性感測範例
**[慣性元件介紹](https://rkuo2000.github.io/EdgeAI-course/lecture/2024/05/24/IMU.html)**<br>

[IMU MPU6050-DMP6v12](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IMU_MPU6050_DMP6v12)<br>

---
## 5. Motion Detection 範例練習
* [Amebapro2 AMB82-mini Arduino Example Guides](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-peripherals-examples)

### 動作偵測 
[![](https://markdown-videos-api.jorgenkh.no/youtube/g_ZP023XCIw)](https://youtu.be/g_ZP023XCIw)

---
## 5. Streaming 範例練習

### 影像串流範例
[RTSP_VideoOnly](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_VideoOnly/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/OmAnWOmt6WQ)](https://youtu.be/OmAnWOmt6WQ)

### 
---
## 5. Detection
### 人臉檢測範例
[RTSP_FaceDetection](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceDetection/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/KD95JH6gVew)](https://youtu.be/KD95JH6gVew)

### 人臉識別範例
[RTSP_FaceRecognition](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceRecognition/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/GGOIQmMfeF8)](https://youtu.be/GGOIQmMfeF8)

### 音頻分類範例
[AudioClassification](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/AudioClassification/)<br>
[YAMNet](https://codimd.mcl.math.ncu.edu.tw/s/hoOqEgBSf)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/oi8ML6aJcvI)](https://youtu.be/oi8ML6aJcvI)




### 彩色TFTLCD
[Camera_TFTLCD](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/Camera_TFTLCD/Camera_TFTLCD.ino)

---
## 5. EdgeAI Image Classification

### [卷積層神經網路介紹](https://rkuo2000.github.io/AI-course/lecture/2023/12/03/CNN.html)

### [影像分類介紹](https://rkuo2000.github.io/AI-course/lecture/2023/12/04/Image-Classification.html)

### AMB82 Mini 影像分類範例
[RTSP_ImageClassification.ino](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/RTSP_EmotionClassification/RTSP_EmotionClassification.ino)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/c3XGkc9ShwQ)](https://youtu.be/c3XGkc9ShwQ)

---
## 6. EdgeAI Object Detection

### [物件檢測介紹](https://rkuo2000.github.io/AI-course/lecture/2023/12/05/Object-Detection.html)

### AMB82 Mini 物件偵測範例
[RTSP_ObjectDetectionLoop](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_ObjectDetectionLoop)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/EvryVoQyqqk)](https://youtu.be/EvryVoQyqqk)

### AMB82 Mini SD卡加載模型範例
[RTPS_ObjectDetection_AudioClassification.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_ObjectDetection_AudioClassification/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/cVvdnXiCAa4)](https://youtu.be/cVvdnXiCAa4)

### Online NN Conversion Tool (客製化模型轉換工具)
[![](https://markdown-videos-api.jorgenkh.no/youtube/6cHC2cOKgQk)](https://youtu.be/6cHC2cOKgQk)

---
## 7. EdgeAI with LLM

### [Large Language Model](https://rkuo2000.github.io/AI-course/lecture/2024/03/21/LLM.html) (大型語言模型介紹)

### AMB82 Mini - 語音辨識範例
* [AmebaPro2 Whisper server](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_server.py)
* [RecordMP4_HTTP_Post_Audio.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

### AMB82 Mini - 語音交談範例
* [AmebaPro2 Whisper LLM_server](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/src/AmebaPro2_Whisper_LLM_server.py)
* [RecordMP4_HTTP_Post_Audio.ino](hhttps://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

---
## 8. EdgeAI with VLM

### [Vision Language Model](https://rkuo2000.github.io/AI-course/lecture/2024/03/27/VLM.html) (視覺語言模型介紹) 

### AMB82 Mini - 影像+語音交談範例
* [AmebaPro2_whisper_llava_server.py](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/src/AmebaPro2_whisper_llava_server.py)<br>
* [RecordMP4_CaptureJPG_HTTP_Post_AudioImage](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_CaptureJPG_HTTP_Post_AudioImage)

---
## 9. RAG (檢索增強生成）

### [Retrieval-Augmented Generation](https://rkuo2000.github.io/AI-course/lecture/2024/05/04/RAG.html)

* [AmebaPro2_Whisper_LlamaIndex_RAG_server.py](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AmebaPro2_Whisper_LlamaIndex_RAG_server.py)<br>
* [RecordMP4_HTTP_Post_Audio](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio)

---
## EdgeAI Projects

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



