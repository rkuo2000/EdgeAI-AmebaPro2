
## [AmebaNN (神經網路模型)](https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/ameba_pro2/amb82-mini/Example_Guides/Neural%20Network/index.html)

### Examples > AmebaNN > [RTSPImageClassification](https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/ameba_pro2/amb82-mini/Example_Guides/Neural%20Network/Image%20Classification.html)

[![](https://markdown-videos-api.jorgenkh.no/youtube/c3XGkc9ShwQ)](https://youtu.be/c3XGkc9ShwQ)

---
#### Garbage模型訓練與檔案轉換

#### 模型訓練： [kaggle.com/rkuo2000/garbage-cnn](https://www.kaggle.com/code/rkuo2000/garbage-cnn)
required in kaggle for AmebaPro2
1) pip install tensorflow==2.14.1
2) model.save('garbage_cnn.h5', include_optimizer=False)

#### 模型轉換：[AMB82 AI Model Conversion](https://www.amebaiot.com/en/amebapro2-ai-convert-model/)
1. Download garbage_cnn.h5 from [kaggle.com/rkuo2000/garbage-cnn](https://www.kaggle.com/code/rkuo2000/garbage-cnn) `Output`
2. Compress garbage_cnn.h5 to garbage_cnn.zip
3. Go to [Amebapro2 AI convert model](https://www.amebaiot.com/en/amebapro2-ai-convert-model/), fill up your E-mail
4. Upload garbage_cnn.zip
5. Upload one (.jpg) test picture (EX. glass100.jpg from [Garbage dataset](https://www.kaggle.com/datasets/asdasdasasdas/garbage-classification))
6. Email will be sent to you for the link of `network_binary.nb`

#### 程式範例：RTSP_GarbageClassification.ino
1. click the recieved Email link to download `network_binary.nb`
2. create NN_MDL folder in SDcard, save network_binary.nb under NN_MDL folder, and rename it to `imgclassification.nb`
3. plugin SDcard back to AMB82-MINI
4. modify Sketch RTSP_GarbageClassification.ino 
   1) modify SSID and PASSWD
   2) modify imgclass.modelSelect (change DEFAULT_IMGCLASS to CUSTOMIZED_IMGCLASS)
5. burn code into board AMB82-MINI, and run it with VLC player streaming
<p><img width="75%" height="75%" src="https://github.com/rkuo2000/EdgeAI-AmebaPro2/raw/main/assets/RTSP_GarbageClassification.png"><img width="25%" height="25%" src="https://github.com/rkuo2000/EdgeAI-AmebaPro2/raw/main/assets/RTSP_GarbageClassification_VLCplayer.jpeg"></p>

---
### 物件偵測 (Object Detection)
[Roboflow Datasets](https://universe.roboflow.com/)<br>

#### Examples > AmebaNN > [ObjectDetectionLoop](https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/ameba_pro2/amb82-mini/Example_Guides/Neural%20Network/Object%20Detection.html)
![](https://ameba-doc-arduino-sdk.readthedocs-hosted.com/en/latest/_images/image0687.png)

---
#### Kaggle 範例:
* [YOLOv7 FaceMask](https://www.kaggle.com/code/rkuo2000/yolov7-facemask)
* [YOLOv7 Pothole](https://www.kaggle.com/code/rkuo2000/yolov7-pothole)
* [YOLOv7 Sushi](https://www.kaggle.com/code/rkuo2000/yolov7-sushi)
* [YOLOv7 RefrigFood](https://www.kaggle.com/code/rkuo2000/yolov7-refrigfood)
* [YOLOv7 TrafficSign](https://www.kaggle.com/code/rkuo2000/yolov7-trafficsign)
* [YOLOv7 Tanks](https://www.kaggle.com/code/rkuo2000/yolov7-tanks)
* [YOLOv7 reparm](https://www.kaggle.com/code/rkuo2000/yolov7-reparam)

---
### Pothole模型訓練與檔案轉換

#### 模型訓練： [kaggle.com/rkuo2000/yolov7-pothole](https://www.kaggle.com/code/rkuo2000/yolov7-pothole)
1) repro [https://github.com/WongKinYiu/yolov7](https://github.com/WongKinYiu/yolov7)
2) create pothole.yaml
`%%writefile data/pothole.yaml`<br>
```
train: ./Datasets/pothole/train/images
val:  ./Datasets/pothole/valid/images
test: ./Datasets/pothole/test/images

# Classes
nc: 1  # number of classes
names: ['pothole']  # class names
```

3) YOLOv7-Tiny Fixed Resolution Training
```
!sed -i "s/nc: 80/nc: 1/" cfg/training/yolov7-tiny.yaml
!sed -i "s/IDetect/Detect/" cfg/training/yolov7-tiny.yaml
```

#### 模型轉換：[AMB82 AI Model Conversion](https://www.amebaiot.com/en/amebapro2-ai-convert-model/)
1. Download `best.pt` from [kaggle.com/rkuo2000/yolov7-pothole](https://www.kaggle.com/code/rkuo2000/yolov7-pothole)
2. Compress best.pt to `best.zip`
3. Go to [Amebapro2 AI convert model](https://www.amebaiot.com/en/amebapro2-ai-convert-model/), fill up your E-mail
4. Upload best.zip
5. Upload one (.jpg) test picture (EX. pothole_test.jpg from [Pothole dataset](https://www.kaggle.com/datasets/apoorvchaudhary/pothole))
6. Email will be sent to you for the link of `network_binary.nb`

#### 程式範例：RTSP_YOLOv7_Pothole_Detection.ino
1. click the recieved Email link to download `network_binary.nb`
2. create NN_MDL folder in SDcard, save network_binary.nb under NN_MDL folder, and rename it to `yolov7_tiny.nb`
3. plugin SDcard back to AMB82-MINI
4. modify Sketch RTSP_YOLOv7_Pothole_Detection.ino 
   1) modify SSID and PASSWD
   2) modify ObjDet.modelSelect(OBJECT_DETECTION, CUSTOMIZED_YOLOV7TINY, NA_MODEL, NA_MODEL);
5. burn code into board AMB82-MINI, and run it with VLC player streaming
<p><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-AmebaPro2/raw/main/assets/RTSP_YOLOv7_Pothole.png"><img width="50%" height="50%" src="https://github.com/rkuo2000/EdgeAI-AmebaPro2/raw/main/assets/RTSP_YOLOv7_Pothole_VCplayer.jpeg"></p>

---
#### AMB82 Mini - 物件偵測範例
[![](https://markdown-videos-api.jorgenkh.no/youtube/EvryVoQyqqk)](https://youtu.be/EvryVoQyqqk)

[RTSP_YOLOv7_Pothole](https://github.com/rkuo2000/EdgeAI-AMB82mini/blob/main/Arduino/AMB82-mini/RTSP_YOLOv7_Pothole)<br>
[RTSP_YOLOv7_Sushi](https://github.com/rkuo2000/EdgeAI-AMB82mini/blob/main/Arduino/AMB82-mini/RTSP_YOLOv7_Sushi)<br>

---
#### AMB82 Mini - SD卡加載模型範例
[RTPS_ObjectDetection_AudioClassification.ino](https://github.com/rkuo2000/Arduino/blob/master/examples/AMB82-MINI/RTSP_ObjectDetection_AudioClassification/)<br>
[![](https://markdown-videos-api.jorgenkh.no/youtube/cVvdnXiCAa4)](https://youtu.be/cVvdnXiCAa4)

---
#### AMB82 Mini - 綫上AI 模型轉換工具
[![](https://markdown-videos-api.jorgenkh.no/youtube/6cHC2cOKgQk)](https://youtu.be/6cHC2cOKgQk)

