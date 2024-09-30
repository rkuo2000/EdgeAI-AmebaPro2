## To run a server on PC: python AmebaPro2_Gemini_server.py
## PC needs ffmpeg.exe 

## To run a client: 
##    HTTP_Post_ImageText.ino
##    HTTP_Post_ImageText_TFTLCD.ino

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


GOOGLE_API_KEY="xxxxxxxxxxxxxxxxxxxx" ## https://aistudio.google.com/app/apikey
genai.configure(api_key=GOOGLE_API_KEY)

import nest_asyncio
nest_asyncio.apply()

class Base64TextImage(BaseModel):
    text_base64_string: str
    image_base64_string: str

app = FastAPI()

@app.get("/")
def home():
    return Response("hello")

@app.post("/imgtxt")
async def post_imgtxt(data: Base64TextImage):
    try:
        text = data.text_base64_string;
        print(text);

        # Save the decoded image data to a JPG file
        decoded_data = base64.b64decode(data.image_base64_string)
        with open("output.jpg", "wb") as f:
            f.write(decoded_data)

        # Gemini
        img = Image.open("output.jpg")
        inst = ", please answer briefly."
        prompt = text + inst
        model = genai.GenerativeModel("gemini-1.5-flash")
        result = model.generate_content( [prompt, img] )
        header1="User: "        
        header2="Gemini:"
        return Response(header1+text+"\n"+header2+result.text)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
        
if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=5000, log_level="info")
