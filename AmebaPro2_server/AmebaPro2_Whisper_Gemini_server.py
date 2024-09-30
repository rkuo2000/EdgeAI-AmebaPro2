## To run a server on PC: python AmebaPro2_Whisper_Gemini_server.py
## PC needs ffmpeg.exe 

## To run a client: HTTP_Post_ImageText.ino
### HTTP_Post_Image.ino
### HTTP_Post_ImageAudio.ino
### HTTP_Post_ImageText.ino
### HTTP_Post_ImageText_TFTLCD.ino

# pip install git+https://github.com/openai/whisper.git
# pip install fastapi uvicorn
# pip install nest-asyncio
# pip install google.generativeai

from PIL import Image
import google.generativeai as genai
from fastapi import FastAPI, HTTPException
from fastapi.responses import Response
from pydantic import BaseModel
import uvicorn
import base64

import whisper
ASR = whisper.load_model("base").to("cpu")

GOOGLE_API_KEY="xxxxxxxxxxxxxxxxxxxxxxxx" ## https://aistudio.google.com/app/apikey
genai.configure(api_key=GOOGLE_API_KEY)

import nest_asyncio
nest_asyncio.apply()

app = FastAPI()

class Base64Data(BaseModel):
    base64_string: str

class Base64Data(BaseModel):
    audio_base64_string: str
    image_base64_string: str

class Base64TextImage(BaseModel):
    text_base64_string: str
    image_base64_string: str

@app.get("/")
def home():
    return Response("hello")

@app.post("/audio")
async def post_audio(data: Base64Data):
    try:
        #Decode the base64 string
        decoded_data = base64.b64decode(data.base64_string)
        
        # print(decoded_data)
        #Save the decoded data to an MP4 file
        with open("output.mp4", "wb") as f:
            f.write(decoded_data)
      
        # Whisper transcribe
        result = ASR.transcribe("output.mp4",fp16=False)
        header = "ASR: "
        print(header+result["text"])

        # Gemini 
        prompt = result["text"]
        model = genai.GenerativeModel("gemini-1.5-flash")
        result = model.generate_content( [prompt] )
        header2="Gemini: "    
        print(header2+result.text)
        return Response(header2+result.text)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
        
@app.post("/imgau")
async def post_imgau(data: Base64Data):
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

        # Gemini
        img = Image.open("output.jpg")
        prompt = result["text"]
        model = genai.GenerativeModel("gemini-1.5-flash")
        result = model.generate_content( [prompt, img] )
        print(result.text)
        header2="Gemini:"
        return Response(header1+text+"\n"+header2+result.text)
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

        # Gemini
        img = Image.open("output.jpg")
        prompt = text
        model = genai.GenerativeModel("gemini-1.5-flash")
        result = model.generate_content( [prompt, img] )
        header1="User: "        
        header2="Gemini:"
        return Response(header1+text+"\n"+header2+result.text)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
        
if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5000, log_level="info")
