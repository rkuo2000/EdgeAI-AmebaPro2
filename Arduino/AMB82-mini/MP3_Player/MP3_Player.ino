/*

MP3 Player can play mp3 music with/without id3 info.
Please enter the mp3 file to sd card toor and keyin full filename to char filename[];

by Howard
*/

#include "MP3_Player.h"
#include "AmebaFatFS.h"
#include <section_config.h>

char filename[] = "song.mp3";	//your mp3 file name  

AmebaFatFS fs;

void setup() {
  // put your setup code here, to run once:
    char path[128];
    bool b_result;
    b_result = fs.begin();
  
    if (b_result)
    {
	    sprintf(path, "%s%s", fs.getRootPath(), filename);
	    File file = fs.open(path);	
	    int size = file.available();	
      mp3_data_len = size;
      file.seek(0);
      mp3_data = new unsigned char[mp3_data_len];
      file.read(mp3_data, mp3_data_len);
	    file.close();
	    fs.end();
	}
	else
  {
		printf("====   sd init failed   ====\r\n");
  }

  parseMP3();

  setOutputGain(0x80);  //The value must in [0x0~0xAF]

  audio_helix_mp3();
}

void loop() {
  // put your main code here, to run repeatedly:

}
