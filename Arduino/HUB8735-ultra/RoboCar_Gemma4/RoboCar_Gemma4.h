// RoboCar_Gemma4.h : helpers kept out of the .ino so the Arduino prototype generator
// does not parse them (C++ templates and the JavaScript inside the HTML page).

// ---- compile-time probe for Audio::micLevel() ----
// Some AmebaPro2 cores (realtek AMB82) provide Audio::micLevel(); the ideasHatch HUB8735
// core does not.
#pragma once
#include "AudioStream.h"
extern "C" {
#include "mmf2_module.h"
#include "module_audio.h"
}

// ---- Audio codec ownership (adapted from ILI9341_Giraffe) ----
// Audio::end() only stops DMA and never releases the codec, so a second Audio::begin()
// fails and tears down the mic thread and speaker buffers. On HUB8735 boards begin() also
// pinmuxes the DMIC pins (PD_14 / PD_18) every time, so a second begin() logs:
//   [MISC Err]Pin 3[14] is conflicted ... audio_dmic_pinmux : dmic clk pin is conflict.
// File::playMp3() inits/deinits the codec on its own. So: begin() once, then release the
// codec before MP3 playback and reset (re-init + restart DMA) it afterwards.
class RoboAudio : public Audio {
public:
    void releaseCodec()
    {
        mm_module_ctrl(_p_mmf_context, CMD_AUDIO_SET_TRX, 0);
        mm_module_ctrl(_p_mmf_context, CMD_AUDIO_FORCE_DEINIT, 0);
    }
    void resetCodec() { mm_module_ctrl(_p_mmf_context, CMD_AUDIO_SET_RESET, 0); }
};

struct MicProbe {
    // returns a.micLevel() if it exists, otherwise -1
    template <typename T>
    static auto level(T &a, int) -> decltype(a.micLevel(), int()) { return a.micLevel(); }
    template <typename T>
    static int level(T &, long) { return -1; }

    // true if T has micLevel()
    template <typename T>
    static auto has(T &a, int) -> decltype(a.micLevel(), bool()) { return true; }
    template <typename T>
    static bool has(T &, long) { return false; }
};

// ---- web page served at http://<robocar-ip>/ ----
const char INDEX_HTML[] = R"rawliteral(<!DOCTYPE html><html><head>
<meta name="viewport" content="width=device-width, initial-scale=1"><link rel="icon" href="data:,">
<title>RoboCar Gemma4</title>
<style>
body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:12px;max-width:520px}
h1{font-size:1.4em;margin:4px 0 10px}
.pad{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin:10px 0}
button{font-size:1.1em;padding:14px 0;border:0;border-radius:8px;background:#2d6cdf;color:#fff;cursor:pointer;
user-select:none;-webkit-tap-highlight-color:transparent;transition:transform .08s,filter .08s}
button:active{transform:scale(.93);filter:brightness(1.3)}
button.stop{background:#d33}
button.fn{background:#1e9e4a}
button.wide{grid-column:span 3}
button.hit{animation:hit .7s ease-out}
@keyframes hit{0%{box-shadow:0 0 0 0 rgba(255,255,255,.95);filter:brightness(1.6)}
100%{box-shadow:0 0 0 14px rgba(255,255,255,0);filter:none}}
#msg{min-height:1.4em;margin:2px 0 6px;color:#9ab}
#msg b{color:#fff}
input{width:100%;box-sizing:border-box;font-size:1.05em;padding:10px;border-radius:8px;border:0;margin:6px 0}
table{width:100%;border-collapse:collapse}td{padding:4px;border-bottom:1px solid #333;vertical-align:top}
td:first-child{color:#9ab;width:30%}img{width:100%;border-radius:8px;margin-top:8px;background:#222}
</style></head><body>
<h1>RoboCar &middot; Gemma-4</h1>
<div class="pad" id="pad">
<button class="fn" data-go="V">Voice</button><button data-go="F" data-name="Forward">&#9650;</button><button class="fn" data-cmd="status">Status</button>
<button data-go="L" data-name="Left">&#9664;</button><button class="stop" data-go="S">STOP</button><button data-go="R" data-name="Right">&#9654;</button>
<button class="fn" data-cmd="imu reset" data-name="IMU reset">IMU reset</button><button data-go="B" data-name="Backward">&#9660;</button><button class="fn" data-snap>Snapshot</button>
<button class="fn wide" data-cmd="scene" data-snap data-name="Scene">Scene (recognize with Gemma-4)</button>
</div>
<div id="msg">&nbsp;</div>
<input id="q" placeholder="go to kitchen / find the dog / turn left twice" onkeydown="if(event.key=='Enter'){cmd(this.value);this.value=''}">
<table id="st"></table>
<img id="img" alt="camera">
<script>
const msg=document.getElementById('msg');
let last=null,imuWait=null;   // last status.json; imuResets count when "IMU reset" was clicked
function esc(s){return String(s).replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]))}
// Show which button / command was sent and whether the RoboCar accepted it.
function note(name,p){
msg.innerHTML='<b>'+esc(name)+'</b> &hellip;';
if(!p){msg.innerHTML='<b>'+esc(name)+'</b> &#10003;';return}
p.then(r=>{msg.innerHTML='<b>'+esc(name)+'</b> '+(r.ok?'&#10003; sent':'&#10007; HTTP '+r.status)})
 .catch(()=>{msg.innerHTML='<b>'+esc(name)+'</b> &#10007; no connection'})}
function go(c){return fetch('/'+c)}
function cmd(t){if(t)note('Command: '+t,fetch('/cmd?q='+encodeURIComponent(t)))}
function snap(){document.getElementById('img').src='/snap.jpg?'+Date.now()}
document.getElementById('pad').addEventListener('click',e=>{
const b=e.target.closest('button');if(!b)return;
b.classList.remove('hit');void b.offsetWidth;b.classList.add('hit');   // restart the flash
let p=null;
if(b.dataset.go)p=go(b.dataset.go);
if(b.dataset.cmd)p=fetch('/cmd?q='+encodeURIComponent(b.dataset.cmd));
if(b.dataset.cmd=='imu reset'){imuWait=last?last.imuResets:0;setTimeout(upd,1500)}
if('snap' in b.dataset)snap();
note(b.dataset.name||b.textContent,p)});
function upd(){fetch('/status.json').then(r=>r.json()).then(j=>{
last=j;
let k=['task','motion','heading','imu','place','scene','heard','reply','voiceTrigger','ip'],h='';
for(const n of k)h+='<tr><td>'+n+'</td><td>'+esc(j[n])+'</td></tr>';
document.getElementById('st').innerHTML=h;
if(imuWait!==null){
 if(j.imuResets>imuWait){msg.innerHTML='<b>IMU reset</b> '+esc(j.imu)+', heading <b>'+(typeof j.heading=='number'?j.heading+'&deg;':esc(j.heading))+'</b>';imuWait=null}
 else msg.innerHTML='<b>IMU reset</b> &hellip; '+esc(j.imu)+' (keep the car still)'}
}).catch(e=>{})}
setInterval(upd,2000);upd();
</script></body></html>
)rawliteral";
