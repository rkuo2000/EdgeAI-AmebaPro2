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

from transformers import AutoProcessor, AutoModelForCausalLM
import torch

model_id = "microsoft/Phi-3-vision-128k-instruct"

model = AutoModelForCausalLM.from_pretrained(model_id, device_map="cuda", trust_remote_code=True, torch_dtype="auto")

processor = AutoProcessor.from_pretrained(model_id, trust_remote_code=True)

generation_args = {
    "max_new_tokens": 500,
    "temperature": 0.0,
    "do_sample": False,
}

import nest_asyncio
nest_asyncio.apply()

class Base64audio(BaseModel):
    base64_string: str

class Base64AudioImage(BaseModel):
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
    print("/asr")
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
    print("/audio")
    try:
        # Save the decoded audio data to a MP4 file
        decoded_data = base64.b64decode(data.base64_string)
        with open("output.mp4", "wb") as f:
            f.write(decoded_data)

        # Whisper transcribe
        result = ASR.transcribe("output.mp4",fp16=False)
        header1 = "ASR:"
        print(header1+result["text"])

        # Phi3 (VLM)
        text = result["text"]
        img = Image.open("output.jpg")
        messages = [
            {"role": "user", "content": "<|image_1|>\n"+text},
        ]
        prompt = processor.tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
        inputs = processor(prompt, [img], return_tensors="pt").to("cuda:0")
        generate_ids = model.generate(**inputs, eos_token_id=processor.tokenizer.eos_token_id, **generation_args) #max_new_tokens=200

        # remove input tokens
        outputs = generate_ids[:, inputs['input_ids'].shape[1]:]
        result = processor.batch_decode(outputs, skip_special_tokens=True, clean_up_tokenization_spaces=False)[0]
        print(result)
        header2="Phi3:"
        return Response(header1+text+"\n"+header2+result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
        
@app.post("/imgau")
async def post_imgau(data: Base64AudioImage):
    print("/imgau")
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

        # Phi3 (VLM)
        text = result["text"] 
        img = Image.open("output.jpg")
        messages = [
            {"role": "user", "content": "<|image_1|>\n"+text}, 
        ]
        prompt = processor.tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
        inputs = processor(prompt, [img], return_tensors="pt").to("cuda")
        generate_ids = model.generate(**inputs, eos_token_id=processor.tokenizer.eos_token_id, **generation_args) #max_new_tokens=200

        # remove input tokens
        outputs = generate_ids[:, inputs['input_ids'].shape[1]:]
        result = processor.batch_decode(outputs, skip_special_tokens=True, clean_up_tokenization_spaces=False)[0]
        print(result)
        header2="Phi3:"
        return Response(header1+text+"\n"+header2+result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/imgtxt")
async def post_imgtxt(data: Base64TextImage):
    print("/imgtxt")
    try:
        text = data.text_base64_string;
        print(text);

        # Save the decoded image data to a JPG file
        decoded_data = base64.b64decode(data.image_base64_string)
        with open("output.jpg", "wb") as f:
            f.write(decoded_data)

        # Phi3 (VLM)
        img = Image.open("output.jpg")
        messages = [
            {"role": "user", "content": "<|image_1|>\n"+text+", please give answer in a short sentence."},
        ]
        prompt = processor.tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
        inputs = processor(prompt, [img], return_tensors="pt").to("cuda")
        generate_ids = model.generate(**inputs, eos_token_id=processor.tokenizer.eos_token_id, **generation_args) #max_new_tokens=200
        outputs = generate_ids[:, inputs['input_ids'].shape[1]:]
        result = processor.batch_decode(outputs, skip_special_tokens=True, clean_up_tokenization_spaces=False)[0]
        print(result)
        header1="User: "
        header2="Phi3:"
        return Response(header1+text+"\n"+header2+result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5000, log_level="info")
