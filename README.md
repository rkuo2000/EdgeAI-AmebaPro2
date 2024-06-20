# EdgeAI-MCU
* **[AI-lecture](https://rkuo2000.github.io/AI-course/)**
* **[EdgeAI-lecture](https://rkuo2000.github.io/EdgeAI-course/)**
  
* **[AMB82-Mini 程式範例](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI)**
* **[生成式AI 程式範例](https://github.com/rkuo2000/GenAI/)**

---
## 1. Introduction

### [AI 簡介](https://rkuo2000.github.io/AI-course/lecture/2023/12/01/AI-Brief.html)

### [EdgeAI MCU 簡介](https://rkuo2000.github.io/EdgeAI-course/lecture/2024/03/01/Edge-AI-MCU-Intro.html)

---
## 2. AMB82-mini Introduction

### [RTL8735B](https://www.amebaiot.com/en/amebapro2/):
32-bit Arm v8M, up to 500MHz, 768KB ROM, 512KB RAM, 16MB Flash (MCM embedded DDR2/DDR3L up to 128MB)<br>
802.11 a/b/g/n WiFi 2.4GHz/5GHz, BLE 5.1, *NN Engine 0.4 TOPS*, Crypto Engine, Audo Codec, ...<br>

<p><img width="50%" height="50%" src="https://www.amebaiot.com/wp-content/uploads/2023/03/amb82_mini.png"></p>

### AMB82-mini Pinout Diagram
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-MCU/blob/main/assets/AMB82-MINI_pinout.png?raw=true"></p>

* [Ameba Arduino](https://www.amebaiot.com/en/ameba-arduino-summary/)

---
## 3. [Arduino IDE使用介紹](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-getting-started/)
* Preferences : 
  
* [Ameba AIoT 影片](https://www.youtube.com/@amebaiot7033)
  
[![](https://markdown-videos-api.jorgenkh.no/youtube/-jQDpDFX2ao)](https://youtu.be/-jQDpDFX2ao)

---
## 4. AMB82 Mini 範例練習
* [Amebapro2 AMB82-mini Arduino Example Guides](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-peripherals-examples)
  
### 影像串流範例
[RTSP_VideoOnly]([https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_VideoOnly/)<br>

[![](https://markdown-videos-api.jorgenkh.no/youtube/OmAnWOmt6WQ)](https://youtu.be/OmAnWOmt6WQ)

### 人臉檢測範例
[RTSP_FaceDetection](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceDetection/)

[![](https://markdown-videos-api.jorgenkh.no/youtube/KD95JH6gVew)](https://youtu.be/KD95JH6gVew)

### 人臉識別範例
[RTSP_FaceRecognition](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceRecognition/)
[![](https://markdown-videos-api.jorgenkh.no/youtube/GGOIQmMfeF8)](https://youtu.be/GGOIQmMfeF8)

### 音頻分類範例
[AudioClassification](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/AudioClassification/)<br>
[YAMNet](https://codimd.mcl.math.ncu.edu.tw/s/hoOqEgBSf)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/oi8ML6aJcvI)](https://youtu.be/oi8ML6aJcvI)

### 慣性感測範例
[IMU MPU6050-DMP6v12](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IMU_MPU6050_DMP6v12)<br>

### 紅外線測距
[IR_VL53L0X](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IR_VL53L0X)<br>

### 彩色TFTLCD
[TFTLCD_ILI9341](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/TFTLCD_ILI9341)

---
## 5. EdgeAI Image Classification

### [Convolutional Neural Networks](https://rkuo2000.github.io/AI-course/lecture/2023/12/03/CNN.html) (卷積層神經網路介紹)

### [Image Classification](https://rkuo2000.github.io/AI-course/lecture/2023/12/04/Image-Classification.html) (影像分類介紹)

### AMB82 Mini 影像分類範例
[RTSP_ImageClassification.ino](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/RTSP_EmotionClassification/RTSP_EmotionClassification.ino)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/7YxshRJLCSk)](https://youtu.be/7YxshRJLCSk)

---
## 6. EdgeAI Object Detection

### [Object Detection](https://rkuo2000.github.io/AI-course/lecture/2023/12/05/Object-Detection.html) (物件檢測介紹)
### [Object Detection Exercises](https://rkuo2000.github.io/AI-course/lecture/2023/12/05/Object-Detection-Exercises.html) (物件檢測範例練習)

### [YOLOv7](https://github.com/WongKinYiu/yolov7)
[YOLOv7: Trainable bag-of-freebies sets new state-of-the-art for real-time object detectors](https://arxiv.org/abs/2207.02696)<br>
**Kaggle:** [https://www.kaggle.com/code/rkuo2000/yolov7](https://www.kaggle.com/code/rkuo2000/yolov7)<br>

### [YOLOv9](https://github.com/WongKinYiu/yolov9)
[YOLOv9: Learning What You Want to Learn Using Programmable Gradient Information](https://arxiv.org/abs/2402.13616)<br>
![](https://learnopencv.com/wp-content/uploads/2024/02/feature-1.gif)
**Kaggle:** [https://www.kaggle.com/code/rkuo2000/yolov9](https://www.kaggle.com/code/rkuo2000/yolov9)<br>

### AMB82 Mini 物件偵測範例
[RTSP_ObjectDetectionLoop](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_ObjectDetectionLoop)<br>
<iframe width="1024" height="576" src="https://www.youtube.com/embed/EvryVoQyqqk" title="AMB82 Mini -  物件偵測範例" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

### AMB82 Mini SD卡加載模型範例
[RTPS_ObjectDetection_AudioClassification.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_ObjectDetection_AudioClassification/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/cVvdnXiCAa4)](https://youtu.be/cVvdnXiCAa4)

### Online NN Conversion Tool (客製化模型轉換工具)
[![](https://markdown-videos-api.jorgenkh.no/youtube/6cHC2cOKgQk)](https://youtu.be/6cHC2cOKgQk)

---
## 7. EdgeAI with LLM
* [生成式AI 程式範例](https://github.com/rkuo2000/GenAI)<br>

### [Large Language Model](https://rkuo2000.github.io/AI-course/lecture/2024/03/21/LLM.html) (大型語言模型介紹)

### Text-to-Text (LLM 程式範例)
* [llm_server.py](https://github.com/rkuo2000/GenAI/blob/main/Text-to-Text/llm_server.py)
* [llm_client.py](https://github.com/rkuo2000/GenAI/blob/main/Text-to-Text/llm_client.py)

### AMB82 Mini - 語音辨識範例
* [AmebaPro2 whisper server](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/src/AmebaPro2_whisper_server.py)
* [RecordMP4_HTTP_Post_Audio.ino](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

### AMB82 Mini - 語音交談範例
* [AmebaPro2 whisper llm_server](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/src/AmebaPro2_whisper_llm_server.py)
* [RecordMP4_HTTP_Post_Audio.ino](hhttps://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_HTTP_Post_Audio/)

---
## 8. EdgeAI with VLM

### [Vision Language Model](https://rkuo2000.github.io/AI-course/lecture/2024/03/27/VLM.html) (視覺語言模型介紹)

### Image+Text
* [llava_server.py](https://github.com/rkuo2000/GenAI/blob/main/Image-to-Text/llava_server.py)
* [post_imgtxt.py](https://github.com/rkuo2000/GenAI/blob/main/Image-to-Text/post_imgtxt.py)

### Image+Audio
* [whisper_llava_server.py](https://github.com/rkuo2000/GenAI/blob/main/Image-to-Text/whisper_llava_server.py)
* [post_imgau.py](https://github.com/rkuo2000/GenAI/blob/main/Image-to-Text/post_imgau.py) 

### AMB82 Mini - 影像+語音交談範例
* [AmebaPro2_whisper_llava_server.py](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/src/AmebaPro2_whisper_llava_server.py)<br>
* [RecordMP4_CaptureJPG_HTTP_Post_AudioImage](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RecordMP4_CaptureJPG_HTTP_Post_AudioImage)

---
## EdgeAI Projects

### [Portable ChatGPT](https://github.com/rkuo2000/portable-ChatGPT)
[![](https://markdown-videos-api.jorgenkh.no/youtube/7rfmXPqyLF0)](https://youtu.be/7rfmXPqyLF0)


