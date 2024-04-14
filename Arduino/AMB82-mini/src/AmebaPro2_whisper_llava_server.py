## To run server: python AmebaPro2_whisper_llava_server.py
## To run client: RecordMP4_CaptureJPG_HTTP_Post_WhisperLLaVA.ino

# pip install git+https://github.com/openai/whisper.git
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
ASR = whisper.load_model("base")

from transformers import AutoProcessor, LlavaForConditionalGeneration
from transformers import BitsAndBytesConfig
import torch

model_name = "llava-hf/llava-1.5-7b-hf"

quantization_config = BitsAndBytesConfig( load_in_4bit=True, bnb_4bit_compute_dtype=torch.float16 )

VLM = LlavaForConditionalGeneration.from_pretrained(model_name, quantization_config=quantization_config, device_map="auto")
processor = AutoProcessor.from_pretrained(model_name)

import nest_asyncio
nest_asyncio.apply()

class Base64Data(BaseModel):
    audio_base64_string: str
    image_base64_string: str

app = FastAPI()

@app.get("/")
def home():
    return Response("Hello World")

@app.post("/audio")
async def post_data(data: Base64Data):
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
        result = ASR.transcribe("output.mp4")
        print("ASR: "+result["text"])

        # LLaVA (VLM)
        prompt = result["text"]
        img = Image.open("output.jpg")
        text = "USER: <image>\n"+prompt+"\nASSISTANT:"
        print(text)
        inputs = processor(text=text, images=img, return_tensors="pt")
        output = VLM.generate(**inputs, max_new_tokens=200)
        generated_text = processor.batch_decode(output, skip_special_tokens=True)[0]
        result = generated_text.split("ASSISTANT:")[-1]
        print(result)
        return Response(result)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5000, log_level="info")
