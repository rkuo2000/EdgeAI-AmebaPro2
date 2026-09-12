/* Local animated portrait and browser speech controller. */
(() => {
  'use strict';
  const cfg = new URLSearchParams(location.search);
  const $ = id => document.getElementById(id);
  const character = $('character');
  let rig = null, animationTimer = null;
  function animateSpeech(text) {
    clearTimeout(animationTimer);
    if (!rig) { show('動畫尚未載入，請重新整理頁面後再試。'); return; }
    rig.start(text, rate);
    const last = rig.timeline?.[rig.timeline.length - 1];
    const duration = last ? last.start + last.duration : text.length * 320 / rate;
    animationTimer = setTimeout(() => rig?.finish(), Math.max(1000, duration));
  }
  let muted = false, rate = 1, recognition = null, speechId = 0;
  const post = type => window.parent.postMessage({ ns: 'avatar-widget', type }, '*');
  const show = text => {
    $('bubble').textContent = text;
    $('bubble').classList.add('show');
    clearTimeout(show.timer);
    show.timer = setTimeout(() => $('bubble').classList.remove('show'), 8000);
  };
  function stopSpeaking() {
    speechId++;
    clearTimeout(animationTimer);
    if ('speechSynthesis' in window) speechSynthesis.cancel();
    character.classList.remove('speaking');
    rig?.stop();
  }
  function speak(text) {
    stopSpeaking();
    show(text);
    if (muted || !('speechSynthesis' in window)) {
      if (rig) animateSpeech(text);
      return;
    }
    const id = speechId;
    const utterance = new SpeechSynthesisUtterance(text);
    utterance.lang = 'zh-TW';
    utterance.rate = rate;
    const voice = speechSynthesis.getVoices().find(v => v.name === cfg.get('voice'));
    if (voice) utterance.voice = voice;
    utterance.onstart = () => { if (id === speechId) { character.classList.add('speaking'); clearTimeout(animationTimer); rig?.start(text, rate); } };
    utterance.onboundary = event => { if (id === speechId) rig?.boundary(event); };
    utterance.onpause = () => { if (id === speechId) rig?.stop(); };
    utterance.onresume = () => { if (id === speechId) rig?.start(text, rate); };
    utterance.onend = () => { if (id === speechId) { clearTimeout(animationTimer); character.classList.remove('speaking'); rig?.finish(); } };
    utterance.onerror = () => { if (id === speechId) { clearTimeout(animationTimer); character.classList.remove('speaking'); rig?.stop(); } };
    speechSynthesis.speak(utterance);
  }
  async function initRig() {
    try {
      rig = new PortraitRig($('portrait'));
    } catch (error) {
      console.warn('Avatar animation unavailable:', error);
    }
    $('btn-preview').disabled = !rig;
  }
  async function respond(text) {
    text = String(text || '').trim().slice(0, 600);
    if (!text) return;
    show('你：' + text);
    const query = text.toLowerCase();
    const matches = (window.KB || []).map(entry => ({ entry, score:
      (query === entry.q.toLowerCase() ? 100 : 0) +
      [entry.q, ...(entry.kw || '').split(/\s+/)].filter(word => word && query.includes(word.toLowerCase())).reduce((n, word) => n + word.length, 0)
    })).sort((a, b) => b.score - a.score);
    let answer = matches[0]?.score ? matches[0].entry.a : '可以問我「學習目標」、「內容大綱」或「語音怎麼用」，我會幫你找到教材的重點。';
    if (window.LLM?.state === 'ready') {
      try {
        answer = await window.LLM.chat([
          { role: 'system', content: '你是小瑞，友善的繁體中文 AI 小助教。請簡短回答，參考資料：' + answer },
          { role: 'user', content: text }
        ]) || answer;
      } catch (error) { console.warn('Local AI unavailable:', error); }
    }
    speak(answer);
  }
  $('portrait').onload = () => {
    initRig().then(() => {
      post('ready'); show('嗨，我是小瑞！一起探索 AMB82-mini 吧！');
    });
  };
  $('portrait').onerror = () => show('角色圖片載入失敗，請檢查圖片路徑。');
  $('portrait').src = cfg.get('model') || '../assets/xiaorui.png';
  if (cfg.get('knowledge')) fetch(cfg.get('knowledge')).then(r => {
    if (!r.ok) throw new Error(r.status);
    return r.json();
  }).then(kb => { if (Array.isArray(kb)) window.KB = kb; }).catch(console.warn);
  for (const text of ['你是誰', '學習目標', '內容大綱']) {
    const button = document.createElement('button');
    button.className = 'sugg'; button.textContent = text;
    button.onclick = () => respond(text); $('suggestions').appendChild(button);
  }
  function submit() { const text = $('type-input').value.trim(); if (text) { $('type-input').value = ''; respond(text); } }
  $('btn-preview').onclick = () => {
    stopSpeaking();
    const text = '嗨，我是小瑞！很高興和你一起學習。';
    show(text);
    animateSpeech(text);
  };
  $('btn-send').onclick = submit;
  $('type-input').onkeydown = e => { if (e.key === 'Enter' && !e.isComposing) submit(); };
  $('btn-close').onclick = () => { stopSpeaking(); if (recognition) recognition.abort(); post('close'); };
  $('btn-mute').onclick = () => {
    muted = !muted; if (muted) stopSpeaking();
    $('btn-mute').textContent = muted ? '🔇' : '🔊';
    $('btn-mute').setAttribute('aria-pressed', String(muted));
    $('btn-mute').setAttribute('aria-label', muted ? '取消靜音' : '靜音');
  };
  $('btn-speed').onclick = () => { rate = rate === 1 ? 1.2 : rate === 1.2 ? .8 : 1; $('btn-speed').textContent = rate.toFixed(1) + '×'; };
  // The portrait has one assistant mode; no unsupported 3D/companion toggle.
  $('btn-mode').hidden = true;
  $('btn-mode').style.display = 'none';
  $('btn-mic').onclick = () => {
    if (recognition) { recognition.stop(); return; }
    const SR = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SR) { show('此瀏覽器不支援語音辨識，請使用文字輸入。'); return; }
    stopSpeaking();
    recognition = new SR(); recognition.lang = 'zh-TW'; recognition.interimResults = true;
    recognition.onstart = () => { $('btn-mic').classList.add('listening'); show('聆聽中…請說話'); };
    recognition.onresult = e => {
      const result = e.results[e.resultIndex];
      if (result.isFinal) respond(result[0].transcript); else show(result[0].transcript);
    };
    recognition.onerror = e => show('語音辨識：' + e.error + '。也可以用文字輸入。');
    recognition.onend = () => { recognition = null; $('btn-mic').classList.remove('listening'); };
    try { recognition.start(); } catch { recognition = null; show('無法開啟麥克風，請再試一次。'); }
  };
  $('btn-llm').onclick = async () => {
    if (!window.LLM?.supported) { show('此瀏覽器無法使用本機 AI，仍可查詢教材知識庫。'); return; }
    $('btn-llm').disabled = true;
    try {
      show('正在下載瀏覽器 AI 模型…');
      await window.LLM.load(p => show('AI 載入中：' + Math.round((p.progress || 0) * 100) + '%'));
      $('btn-llm').classList.add('llm-on'); $('btn-llm').setAttribute('aria-pressed', 'true'); show('AI 大腦已準備好！');
    } catch { show('AI 模型載入失敗，仍可查詢教材知識庫。'); }
    finally { $('btn-llm').disabled = false; }
  };
  window.addEventListener('message', e => {
    if (e.source !== window.parent || e.data?.ns !== 'avatar-widget-host') return;
    if (e.data.type === 'say') speak(String(e.data.text || '').slice(0, 600));
  });
  window.AvatarWidget = { say: speak, close: () => $('btn-close').click(), open: () => show('小瑞在這裡！') };
})();
