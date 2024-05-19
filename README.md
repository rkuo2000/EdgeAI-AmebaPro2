# EdgeAI-MCU
* **[AI-lecture](https://rkuo2000.github.io/AI-course/)**
* **[EdgeAI-lecture](https://rkuo2000.github.io/EdgeAI-course/)**
  
* **[AMB82-Mini 程式範例](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI)**
* **[生成式AI 程式範例](https://github.com/rkuo2000/GenAI/)**

---
## 1. Introduction

### [AI 簡介](https://rkuo2000.github.io/AI-course/lecture/2023/12/01/AI-Brief.html)

* [Kaggle 使用介紹](https://rkuo2000.github.io/EdgeAI-course/lecture/2024/03/02/Kaggle-Tutorial.html)
* [Colab 使用介紹](https://steam.oxxostudio.tw/category/python/info/online-editor.html)

---
## EdgeAI MCU 介紹

### Realtek AMB82-mini
<p><img width="50%" height="50%" src="https://www.amebaiot.com/wp-content/uploads/2023/03/amb82_mini.png"></p>

#### [RTL8735B](https://www.amebaiot.com/en/amebapro2/):
32-bit Arm v8M, up to 500MHz, 768KB ROM, 512KB RAM, 16MB Flash (MCM embedded DDR2/DDR3L up to 128MB)<br>
802.11 a/b/g/n WiFi 2.4GHz/5GHz, BLE 5.1, *NN Engine 0.4 TOPS*, Crypto Engine, Audo Codec, ...<br>

* [Ameba Arduino](https://www.amebaiot.com/en/ameba-arduino-summary/)
* [Amebapro2 AMB82-mini Arduino Example Guides](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-peripherals-examples)
* [Amebapro2 AMB82-mini Arduino getting started](https://www.amebaiot.com/en/amebapro2-amb82-mini-arduino-getting-started/)
* [Ameba AIoT 影片](https://www.youtube.com/@amebaiot7033)
* Preferences = https://raw.githubusercontent.com/ambiot/ambpro2_arduino/dev/Arduino_package/package_realtek_amebapro2_early_index.json<br>

<iframe width="1024" height="576" src="https://www.youtube.com/embed/-jQDpDFX2ao" title="AMB82 Mini - 入門教學" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

---
## 2. AMB82 Mini 範例練習

### 影像串流範例
[RTSP_VideoOnly]([https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_VideoOnly/)<br>
<iframe width="1024" height="576" src="https://www.youtube.com/embed/OmAnWOmt6WQ" title="AMB82 Mini - RTSP 影片串流範例" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

### 人臉檢測範例
[RTSP_FaceDetection](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceDetection/)
<iframe width="1024" height="576" src="https://www.youtube.com/embed/KD95JH6gVew" title="AMB82 Mini - 人臉檢測範例" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

### 人臉識別範例
[RTSP_FaceRecognition](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/RTSP_FaceRecognition/)
<iframe width="1024" height="576" src="https://www.youtube.com/embed/GGOIQmMfeF8" title="AMB82 Mini - 人臉識別範例" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

### 音頻分類範例
[AudioClassification](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/AudioClassification/)<br>
[YAMNet](https://codimd.mcl.math.ncu.edu.tw/s/hoOqEgBSf)<br>
<iframe width="1024" height="576" src="https://www.youtube.com/embed/oi8ML6aJcvI" title="AMB82 Mini - 音頻分類" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

### [音頻Loopback測試](https://www.amebaiot.com/en/amebapro2-arduino-audio-basic/)
[Audio_LoopbackTest](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/Audio_LoopbackTest/)<br>

### 慣性感測範例
[IMU MPU6050-DMP6v12](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IMU_MPU6050_DMP6v12)<br>

### 紅外線測距
[IR_VL53L0X](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/IR_VL53L0X)<br>

### 彩色TFTLCD
[TFTLCD_ILI9341](https://github.com/rkuo2000/Arduino/tree/master/examples/AMB82-MINI/TFTLCD_ILI9341)

---
## 3. EdgeAI Image Classification

### [Convolutional Neural Networks](https://rkuo2000.github.io/AI-course/lecture/2023/12/03/CNN.html) (卷積層神經網路介紹)

### [Image Classification](https://rkuo2000.github.io/AI-course/lecture/2023/12/04/Image-Classification.html) (影像分類介紹)

### AMB82 Mini 影像分類範例
[RTSP_ImageClassification.ino](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/Arduino/AMB82-mini/RTSP_EmotionClassification/RTSP_EmotionClassification.ino)<br>

---
## 4. EdgeAI Object Detection

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
<iframe width="1024" height="576" src="https://www.youtube.com/embed/cVvdnXiCAa4" title="AMB82 Mini - 使用SD 卡加載神經網絡模型" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

### Online NN Conversion Tool (客製化模型轉換工具)
<iframe width="1024" height="576" src="https://www.youtube.com/embed/6cHC2cOKgQk" title="AMB82 Mini - 綫上AI 模型轉換工具" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" referrerpolicy="strict-origin-when-cross-origin" allowfullscreen></iframe>

---
## 5. EdgeAI with LLM
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
## 6. EdgeAI with VLM

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
![](https://github.com/rkuo2000/EdgeAI-MCU/blob/main/assets/AMB82_Mini_PortableChatGPT.jpg?raw=true)


