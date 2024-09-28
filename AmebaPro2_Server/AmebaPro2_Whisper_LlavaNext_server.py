## To run server: python AmebaPro2_whisper_llava_server.py
## To run client: RecordMP4_CaptureJPG_HTTP_Post_WhisperLLaVA.ino

# pip install git+https://github.com/openai/whisper.git
# pip install bitsandbytes
# pip install fastapi uvicorn
# pip install nest-asyncio
# pip install accelerate

from PIL import Image
from fastapi import FastAPI, HTTPException
from fastapi.responses import Response
from pydantic import BaseModel
import uvicorn
import base64

import whisper
ASR = whisper.load_model("base").to("cpu")

from transformers import LlavaNextProcessor, LlavaNextForConditionalGeneration
import torch

model_id = "llava-hf/llava-v1.6-vicuna-7b-hf"

processor = LlavaNextProcessor.from_pretrained(model_id)

VLM = LlavaNextForConditionalGeneration.from_pretrained(model_id, torch_dtype=torch.float16, low_cpu_mem_usage=True, load_in_4bit=True)

import nest_asyncio
nest_asyncio.apply()


class Base64audio(BaseModel):
    base64_string: str

class Base64Data(BaseModel):
    audio_base64_string: str
    image_base64_string: str

class Base64TextImage(BaseModel):
    text_base64_string: str
    image_base64_string: str


app = FastAPI()

@app.get("/")
def home():
    return Response("Hello World")

@app.post("/asr")
async def post_asr(data: Base64audio):
    try:
        # Save the decoded audio data to a MP4 file
        decoded_data = base64.b64decode(data.base64_string)
        with open("speech.mp4", "wb") as f:
            f.write(decoded_data)

        # Whisper transcribe
        result = ASR.transcribe("speech.mp4",fp16=False)
        header1 = "ASR:"
        print(header1+result["text"])
        return Response(header1+result["text"])
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/audio")
async def post_audio(data: Base64audio):
    try:
        # Save the decoded audio data to a MP4 file
        decoded_data = base64.b64decode(data.base64_string)
        with open("output.mp4", "wb") as f:
            f.write(decoded_data)

        # Whisper transcribe
        result = ASR.transcribe("output.mp4",fp16=False)
        header1 = "ASR:"
        print(header1+result["text"])

        # LLaVA (VLM)
        text = result["text"]
        prompt = "USER: \n"+text+inst+"\nASSISTANT:"
        inputs = processor(text=prompt, return_tensors="pt").to("cuda")
        output = VLM.generate(**inputs, max_new_tokens=200)
        generated_text = processor.decode(output[0], skip_special_tokens=True)
        result = generated_text.split("ASSISTANT:")[-1]
        print(result)
        header2="VLM:"
        return Response(header1+text+"\n"+header2+result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
        
@app.post("/imgau")
async def post_imgau(data: Base64Data):
    inst = ", give answer in a simple sentence."
    try:
        # Save the decoded audio data to a MP4 file
        decoded_data = base64.b64decode(data.audio_base64_string)
        with open("output.mp4", "wb") as f:
            f.write(decoded_data)

        # Save the decoded image data to a JPG file
        decoded_data = base64.b64decode(data.image_base64_string)
        with open("output.jpg", "wb") as f:
            f.write(decoded_data)

        # Whisper transcribe
        result = ASR.transcribe("output.mp4",fp16=False)
        header1 = "ASR:"
        print(header1+result["text"])

        # LLaVA (VLM)
        text = result["text"]
        img = Image.open("output.jpg")
        prompt = "USER: <image>\n"+text+inst+"\nASSISTANT:"
        inputs = processor(text=prompt, images=img, return_tensors="pt").to("cuda")
        output = VLM.generate(**inputs, max_new_tokens=200)
        generated_text = processor.decode(output[0], skip_special_tokens=True)
        result = generated_text.split("ASSISTANT:")[-1]
        print(result)
        header2="VLM:"
        return Response(header1+text+"\n"+header2+result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/imgtxt")
async def post_imgtxt(data: Base64TextImage):
#    inst = ", give answer in a few words."
    inst = ""
    try:
        text = data.text_base64_string;
        print(text);

        # Save the decoded image data to a JPG file
        decoded_data = base64.b64decode(data.image_base64_string)
        with open("output.jpg", "wb") as f:
            f.write(decoded_data)

        # LLaVA (VLM)
        img = Image.open("output.jpg")
        prompt = "USER: <image>\n"+text+inst+"\nASSISTANT:"
        inputs = processor(text=prompt, images=img, return_tensors="pt").to("cuda")
        output = VLM.generate(**inputs, max_new_tokens=200)
        generated_text = processor.decode(output[0], skip_special_tokens=True)
        result = generated_text.split("ASSISTANT:")[-1]
        print(result)
        header1="User: "
        header2="VLM:"
        return Response(header1+text+"\n"+header2+result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5000, log_level="info")
