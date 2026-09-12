/* Canvas 2D portrait rig: no WebGL, external libraries or pixel readback.
 * Coordinates are calibrated to the xiaorui.png (1024 × 1536), in a 320 × 480 coordinate space.
 * Text syllables and punctuation form a timeline, anchored by TTS boundaries.
 * Browser speech provides no PCM/phoneme timestamps, so timing is approximate.
 */
class PortraitRig {
  constructor(image) {
    this.image = image;
    this.canvas = document.createElement('canvas');
    this.canvas.className = 'portrait-mesh';
    this.canvas.setAttribute('aria-hidden', 'true');
    this.ctx = this.canvas.getContext('2d');
    if (!this.ctx) throw new Error('Canvas 2D is unavailable');
    this.canvas.width = 320;
    this.canvas.height = 480;
    this.ctx.imageSmoothingQuality = 'high';
    this.speaking = false;
    this.open = 0;
    this.level = 0;
    this.arms = [0,0];
    this.armUpdatedAt = null;
    this.started = 0;
    this.nextBlinkAt = Infinity;
    this.lastFrame = -Infinity;
    this.reduced = matchMedia('(prefers-reduced-motion: reduce)');
    // Keep the poster at its native resolution. Only these small regions are
    // tessellated; the face, hair, clothes, lettering and background stay intact.
    this.regions = [
      { x:128, y:88, w:37, h:38, step:1, kind:'eye', cx:145, cy:105 },
      { x:174, y:103, w:35, h:32, step:1, kind:'eye', cx:190, cy:120 },
      { x:139, y:130, w:39, h:28, step:1, kind:'mouth' },
      { x:0, y:180, w:139, h:188, step:3, kind:'arm', side:0 },
      { x:219, y:157, w:101, h:181, step:3, kind:'arm', side:1 }
    ];
    this.draw = this.draw.bind(this);
    // Draw before hiding the image, so a failed setup remains visible.
    this.render(0, 0);
    image.after(this.canvas);
    image.style.visibility = 'hidden';
  }
  start(text, rate=1) {
    this.text = text;
    this.rate = rate;
    const continuing = this.speaking;
    this.speaking = true;
    this.ending = false;
    if (!continuing) {
      this.started = performance.now();
      this.nextBlinkAt = this.started + 1200 + Math.random()*1800;
    }
    this.timeline = this.buildTimeline(text, rate);
    this.gestures = this.buildGestures(this.timeline);
    this.anchorAt = performance.now();
    this.anchorPosition = 0;
    this.timingScale = 1;
    this.previousBoundary = null;
    this.open = 0;
    cancelAnimationFrame(this.frame);
    this.lastFrame = -Infinity;
    this.frame = requestAnimationFrame(this.draw);
  }
  buildTimeline(text, rate) {
    const units = [];
    let position = 0;
    // A Chinese character is usually one syllable. Latin words use vowel groups
    // as a lightweight syllable estimate, instead of animating every letter.
    const tokens = text.matchAll(/[A-Za-z]+(?:'[A-Za-z]+)?|[0-9]+|[^\s]/gu);
    for (const token of tokens) {
      const word = token[0], index = token.index;
      const pause = /[，。！？、,.!?;；：:…—]/u.test(word);
      const syllables = /^[A-Za-z]/.test(word)
        ? Math.max(1, (word.replace(/e$/i, '').match(/[aeiouy]+/gi) || []).length)
        : /^[0-9]/.test(word) ? word.length : 1;
      const duration = (pause ? /[。！？.!?…]/u.test(word) ? 420 : 240 : 320) / rate;
      for (let i=0;i<syllables;i++) {
        units.push({ index, endIndex:index+word.length, start:position,
          duration, pause, amplitude: (.78 + (index % 3) * .05) * 1.4 });
        position += duration;
      }
    }
    return units;
  }
  boundary(event) {
    if (!this.speaking || !Number.isInteger(event.charIndex)) return;
    const unit = this.timeline.find(u => event.charIndex >= u.index && event.charIndex < u.endIndex);
    if (!unit) return;
    const now = performance.now();
    const previous = this.previousBoundary;
    // Learn the selected voice's actual pace from successive word boundaries.
    // Ignore duplicate sentence/word events at the same character index.
    if (previous && unit.start <= previous.position) return;
    if (previous) {
      const observed = (now-previous.at)/(unit.start-previous.position);
      if (observed > .35 && observed < 3) this.timingScale = .65*this.timingScale + .35*observed;
    }
    this.anchorAt = now;
    this.anchorPosition = unit.start;
    this.previousBoundary = { at:now, position:unit.start };
  }
  mouthTarget(now) {
    if (!this.speaking) return 0;
    const position = this.anchorPosition + (now-this.anchorAt)/this.timingScale;
    const unit = this.timeline.find(u => position >= u.start && position < u.start+u.duration);
    if (!unit || unit.pause) return 0;
    const phase = (position-unit.start)/unit.duration;
    const smooth = t => t*t*(3-2*t);
    // Use a larger opening during voiced syllables, with gentle vowel motion.
    const stretchedPhase = phase;
    const envelope = stretchedPhase < .28 ? smooth(stretchedPhase/.28)
      : stretchedPhase < .58 ? 1 : 1-smooth((stretchedPhase-.58)/.42);
    const minimum = .26 * 1.4;
    return minimum + (unit.amplitude - minimum) * envelope;
  }
  blinkTarget(now) {
    if (!this.speaking || this.reduced.matches) return 0;
    const elapsed = now-this.nextBlinkAt;
    if (elapsed < 0) return 0;
    if (elapsed >= 280) {
      this.nextBlinkAt = now + 2400 + Math.random()*3000;
      return 0;
    }
    const smooth = t => t*t*(3-2*t);
    // Close quickly, briefly hold, then reopen a little more slowly.
    return elapsed < 100 ? smooth(elapsed/100)
      : elapsed < 140 ? 1 : 1-smooth((elapsed-140)/140);
  }
  finish() {
    if (!this.speaking) return;
    this.speaking = false;
    this.ending = true;
    this.waveEndedAt = performance.now();
    this.lastFrame = -Infinity;
    cancelAnimationFrame(this.frame);
    this.frame = requestAnimationFrame(this.draw);
  }
  stop() {
    this.ending = false;
    this.speaking = false;
    cancelAnimationFrame(this.frame);
    this.open = this.level = 0;
    this.arms = [0,0];
    this.armUpdatedAt = null;
    this.nextBlinkAt = Infinity;
    this.render(0, 0);
  }
  buildGestures(timeline) {
    const gestures = [];
    let group = [];
    const flush = (emphasis=false) => {
      if (!group.length) return;
      const first=group[0], last=group[group.length-1];
      // Alternate explanatory gestures, with both hands on every third beat.
      const poses=[[1,.25],[.25,1],[.85,.85]];
      const pose=poses[gestures.length%poses.length];
      gestures.push({ start:first.start, duration:last.start+last.duration-first.start,
        pose:pose.map(value => value*(emphasis ? 1.15 : .9)) });
      group=[];
    };
    for (const unit of timeline) {
      if (unit.pause) {
        flush(/[！？!?]/u.test(this.text.slice(unit.index,unit.endIndex)));
      } else {
        if (group.length===5) flush();
        group.push(unit);
      }
    }
    flush();
    return gestures;
  }
  armPose(now) {
    if (this.reduced.matches) {
      this.arms=[0,0];
      this.armUpdatedAt=now;
      return this.arms;
    }
    let target=[0,0];
    if (this.speaking) {
      // Share the mouth's TTS boundary anchor, including learned voice speed.
      const position=this.anchorPosition+(now-this.anchorAt)/this.timingScale;
      const gesture=this.gestures.find(g => position>=g.start && position<g.start+g.duration);
      if (gesture) {
        const phase=(position-gesture.start)/gesture.duration;
        const smooth=t => t*t*(3-2*t);
        // Lift, briefly hold the explanatory pose, then relax before a pause.
        const envelope=phase<.3 ? smooth(phase/.3)
          : phase<.55 ? 1 : 1-smooth((phase-.55)/.45);
        target=gesture.pose.map(value => value*envelope);
      }
    }
    const delta=this.armUpdatedAt===null ? 40 : Math.max(0,Math.min(100,now-this.armUpdatedAt));
    this.armUpdatedAt=now;
    const blend=1-Math.exp(-delta/110);
    this.arms=this.arms.map((value,i) => value+(target[i]-value)*blend);
    return this.arms;
  }
  point(x, y, mouth, wave, blink=0, region=null, arms=[0,0]) {
    const smooth = (a,b,v) => { const t=Math.max(0,Math.min(1,(v-a)/(b-a))); return t*t*(3-2*t); };
    if (region?.kind === 'eye') {
      const c=Math.cos(.29), s=Math.sin(.29);
      const dx=x-region.cx, dy=y-region.cy;
      const u=c*dx+s*dy, v=-s*dx+c*dy;
      const edge=smooth(region.x,region.x+3,x)*(1-smooth(region.x+region.w-3,region.x+region.w,x))
        *smooth(region.y,region.y+3,y)*(1-smooth(region.y+region.h-3,region.y+region.h,y));
      const mask=(1-smooth(10,18,Math.abs(u)))*edge;
      // Compress the iris into the lash line while keeping the patch edges pinned.
      const lid=Math.abs(v)<=8 ? v*.96
        : Math.sign(v)*7.68*(1-smooth(8,17,Math.abs(v)));
      const closed=v-lid*blink*mask;
      return [region.cx+c*u-s*closed,region.cy+s*u+c*closed];
    }
    // Combine wrist waving and a visible forearm bend in one continuous patch.
    const sourceX=x, sourceY=y;
    const turn = (cx,cy,angle,weight) => {
      const dx=x-cx, dy=y-cy, c=Math.cos(angle), s=Math.sin(angle);
      x+=(cx+c*dx-s*dy-x)*weight;
      y+=(cy+s*dx+c*dy-y)*weight;
    };
    if (x>=0 && x<116 && y>262 && y<367) {
      const weight=smooth(0,12,x)*(1-smooth(101,116,x))*smooth(262,278,y)*(1-smooth(352,367,y));
      turn(90,287,wave*.13,weight);
    }
    if (x>252 && x<=320 && y>157 && y<257) {
      const weight=smooth(252,260,x)*smooth(157,170,y)*(1-smooth(238,257,y));
      // Bend around the wrist; fade at the image edge to keep the mesh sealed.
      turn(270,244,-wave*.20,weight*(1-smooth(313,320,x)));
    }
    if (region?.kind === 'arm') {
      const edge=smooth(region.x,region.x+12,sourceX)
        *(1-smooth(region.x+region.w-14,region.x+region.w,sourceX))
        *smooth(region.y,region.y+16,sourceY)
        *(1-smooth(region.y+region.h-16,region.y+region.h,sourceY));
      if (region.side===0) {
        // Pin the shoulder and torso, allowing sleeve, wrist and hand to follow.
        const weight=edge*smooth(196,270,sourceY)*(1-smooth(107,139,sourceX));
        turn(91,224,arms[0]*.105+wave*.035,weight);
      } else {
        const weight=edge*smooth(221,257,sourceX);
        turn(246,310,-arms[1]*.096-wave*.045,weight);
      }
      return [x,y];
    }
    if (x>139 && x<178 && y>130 && y<158) {
      const a=.29, c=Math.cos(a), s=Math.sin(a), dx=x-158, dy=y-142;
      const u=c*dx+s*dy; let v=-s*dx+c*dy;
      const edge=smooth(139,144,x)*(1-smooth(173,178,x))*smooth(130,134,y)*(1-smooth(154,158,y));
      const mask=(1-smooth(9,19,Math.abs(u)))*(1-smooth(4,14,Math.abs(v)))*edge;
      v*=1-(1-mouth)*.88*mask;
      x=158+c*u-s*v; y=142+s*u+c*v;
    }
    return [x,y];
  }
  triangle(source, dest) {
    const ctx=this.ctx;
    const [s0,s1,s2]=source, [d0,d1,d2]=dest;
    const ux=s1[0]-s0[0], uy=s1[1]-s0[1], vx=s2[0]-s0[0], vy=s2[1]-s0[1];
    const det=ux*vy-uy*vx;
    const dx1=d1[0]-d0[0], dy1=d1[1]-d0[1], dx2=d2[0]-d0[0], dy2=d2[1]-d0[1];
    const a=(dx1*vy-dx2*uy)/det, b=(dy1*vy-dy2*uy)/det;
    const c=(dx2*ux-dx1*vx)/det, d=(dy2*ux-dy1*vx)/det;
    ctx.save();
    // Overlap subpixel triangle edges so antialiasing cannot expose seams.
    const center=[(d0[0]+d1[0]+d2[0])/3,(d0[1]+d1[1]+d2[1])/3];
    const clip=dest.map(p => {
      const dx=p[0]-center[0], dy=p[1]-center[1], length=Math.hypot(dx,dy);
      return [p[0]+dx/length*.45,p[1]+dy/length*.45];
    });
    ctx.beginPath(); ctx.moveTo(...clip[0]); ctx.lineTo(...clip[1]); ctx.lineTo(...clip[2]); ctx.closePath(); ctx.clip();
    ctx.setTransform(a,b,c,d,d0[0]-a*s0[0]-c*s0[1],d0[1]-b*s0[0]-d*s0[1]);
    ctx.drawImage(this.image,0,0,320,480);
    ctx.restore();
  }
  render(mouth,wave,blink=0,arms=[0,0]) {
    const ctx=this.ctx;
    ctx.clearRect(0,0,320,480);
    ctx.drawImage(this.image,0,0,320,480);
    for (const region of this.regions) {
      if (region.kind==='eye' ? blink===0 : region.kind==='mouth' ? mouth===1 : wave===0 && arms[region.side]===0) continue;
      const {x,y,w,h,step}=region;
      ctx.save();
      ctx.beginPath(); ctx.rect(x,y,w,h); ctx.clip();
      ctx.clearRect(x,y,w,h);
      for(let sy=y;sy<y+h;sy+=step) for(let sx=x;sx<x+w;sx+=step) {
        const ex=Math.min(sx+step,x+w), ey=Math.min(sy+step,y+h);
        const source=[[sx,sy],[ex,sy],[sx,ey],[ex,ey]];
        const dest=source.map(([px,py])=>this.point(px,py,mouth,wave,blink,region,arms));
        this.triangle([source[0],source[1],source[2]],[dest[0],dest[1],dest[2]]);
        this.triangle([source[2],source[1],source[3]],[dest[2],dest[1],dest[3]]);
      }
      ctx.restore();
    }
  }
  draw(now) {
    if (!this.speaking && !this.ending) return;
    if (this.ending && now-this.waveEndedAt>=1600) { this.stop(); return; }
    if (now-this.lastFrame>=40) {
      const delta = Number.isFinite(this.lastFrame) ? Math.min(100,now-this.lastFrame) : 40;
      this.lastFrame=now;
      const target = this.mouthTarget(now);
      this.open += (target-this.open)*(1-Math.exp(-delta/150));
      if (!this.speaking) this.open = 0;
      const mouth = this.open;
      this.render(mouth,0,this.blinkTarget(now),this.armPose(now));
    }
    this.frame=requestAnimationFrame(this.draw);
  }
}
