// Step 0 / (c): PptxGenJS で sample3.md を PPTX 化する試作。
// 固定座標のテキストボックスだけを使い、PowerPoint 側の自動縮小(autofit)は使わない。
// 本文は sample3.md から読み込む（スライドの内容はここでは持たない）。
'use strict';

const fs = require('fs');
const path = require('path');
const PptxGenJS = require('pptxgenjs');

const SRC = path.join(__dirname, 'sample3.md');
const OUT = path.join(__dirname, 'out', 'c_pptxgenjs.pptx');

// ---- レイアウト定数（インチ / pt）---------------------------------------
const SLIDE = { w: 13.333, h: 7.5 }; // 16:9
const PAGE = { mx: 0.7, top: 0.4, footerY: 7.05 };
const CONTENT_W = SLIDE.w - PAGE.mx * 2;

const FONT = { body: 'Noto Sans JP', mono: 'Roboto Mono' };
const COLOR = { text: '24292E', sub: '57606A', accent: '0366D6', codeBg: 'F6F8FA', codeLine: 'D0D7DE', footer: '8B949E' };
const SIZE = { h1: 24, h2: 15, body: 13, code: 10, footer: 9, titleH1: 40, titleH2: 22, titleP: 16 };
const LH = { body: 1.45, code: 1.3 }; // 行送り（フォントサイズ倍率）。pt の固定行送りとして書き出す
const GAP = 0.12;

// ---- Markdown → スライドモデル ---------------------------------------------
function parseDeck(md) {
  const fm = md.match(/^---\n([\s\S]*?)\n---\n/);
  if (!fm) throw new Error('front matter not found');
  const meta = {};
  fm[1].split('\n').forEach((l) => {
    const i = l.indexOf(':');
    if (i > 0) meta[l.slice(0, i).trim()] = l.slice(i + 1).trim();
  });

  // `---` でスライド分割（コードフェンス内は無視）
  const slides = [[]];
  let inFence = false;
  md.slice(fm[0].length).split('\n').forEach((line) => {
    if (/^```/.test(line)) inFence = !inFence;
    if (!inFence && /^---\s*$/.test(line)) slides.push([]);
    else slides[slides.length - 1].push(line);
  });

  return { meta, slides: slides.map(parseSlide) };
}

function parseSlide(lines) {
  const blocks = [];
  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    if (/^```/.test(line)) {
      const code = [];
      for (i++; i < lines.length && !/^```/.test(lines[i]); i++) code.push(lines[i]);
      blocks.push({ type: 'code', lines: code });
    } else if (/^## /.test(line)) blocks.push({ type: 'h2', text: line.slice(3) });
    else if (/^# /.test(line)) blocks.push({ type: 'h1', text: line.slice(2) });
    else if (/^[*-] /.test(line)) blocks.push({ type: 'bullet', text: line.slice(2) });
    else if (/^\s+\S/.test(line)) throw new Error(`unsupported indented line: "${line}"`);
    else if (line.trim() !== '') blocks.push({ type: 'p', text: line });
  }
  return blocks;
}

// **bold** と `code` をランに分解
function inlineRuns(text, base) {
  return text.split(/(\*\*[^*]+\*\*|`[^`]+`)/).filter(Boolean).map((t) => {
    if (t.startsWith('**')) return { text: t.slice(2, -2), options: { ...base, bold: true } };
    if (t.startsWith('`')) return { text: t.slice(1, -1), options: { ...base, fontFace: FONT.mono, color: COLOR.accent } };
    return { text: t, options: { ...base } };
  });
}

// ---- 高さの見積もり（折り返し推定）---------------------------------------
const plain = (t) => t.replace(/\*\*/g, '').replace(/`/g, '');
const cells = (t) => [...plain(t)].reduce((n, ch) => n + (ch.codePointAt(0) > 0x2e7f ? 2 : 1), 0);
const textLines = (t, widthIn, sizePt) => Math.max(1, Math.ceil((cells(t) * 0.5 * sizePt) / (widthIn * 72)));
const lineHeightIn = (sizePt, mult) => (sizePt * mult) / 72;

// ---- 描画 ------------------------------------------------------------------
function addFooter(slide, text) {
  slide.addText(text, {
    x: PAGE.mx, y: PAGE.footerY, w: CONTENT_W, h: 0.3, margin: 0, valign: 'middle',
    fontFace: FONT.body, fontSize: SIZE.footer, color: COLOR.footer, fit: 'none',
  });
}

function renderTitleSlide(slide, blocks) {
  const h1 = blocks.find((b) => b.type === 'h1');
  const h2 = blocks.find((b) => b.type === 'h2');
  const p = blocks.find((b) => b.type === 'p');
  let y = 2.3;
  const put = (text, size, color, bold) => {
    const h = lineHeightIn(size, LH.body) * textLines(text, CONTENT_W, size);
    slide.addText(text, {
      x: PAGE.mx, y, w: CONTENT_W, h, margin: 0, valign: 'top', fit: 'none',
      fontFace: FONT.body, fontSize: size, color, bold, lineSpacing: size * LH.body,
    });
    y += h + 0.25;
  };
  if (h1) put(plain(h1.text), SIZE.titleH1, COLOR.text, true);
  if (h2) put(plain(h2.text), SIZE.titleH2, COLOR.sub, false);
  if (p) put(plain(p.text), SIZE.titleP, COLOR.sub, false);
  return y;
}

function renderContentSlide(slide, blocks) {
  let y = PAGE.top;
  const textBox = (runs, h, extra = {}) => {
    slide.addText(runs, {
      x: PAGE.mx, y, w: CONTENT_W, h, margin: 0, valign: 'top', fit: 'none', wrap: true,
      fontFace: FONT.body, ...extra,
    });
    y += h + GAP;
  };

  blocks.forEach((b, idx) => {
    switch (b.type) {
      case 'h1': {
        const h = lineHeightIn(SIZE.h1, LH.body) * textLines(b.text, CONTENT_W, SIZE.h1);
        textBox(plain(b.text), h, { fontSize: SIZE.h1, bold: true, color: COLOR.text, lineSpacing: SIZE.h1 * LH.body });
        break;
      }
      case 'h2': {
        const h = lineHeightIn(SIZE.h2, LH.body) * textLines(b.text, CONTENT_W, SIZE.h2);
        textBox(plain(b.text), h, { fontSize: SIZE.h2, color: COLOR.sub, lineSpacing: SIZE.h2 * LH.body });
        break;
      }
      case 'p': {
        const h = lineHeightIn(SIZE.body, LH.body) * textLines(b.text, CONTENT_W, SIZE.body);
        textBox(inlineRuns(b.text, { fontSize: SIZE.body, color: COLOR.text }), h, { lineSpacing: SIZE.body * LH.body });
        break;
      }
      case 'code': {
        const pad = 0.1; // inch
        const h = b.lines.length * lineHeightIn(SIZE.code, LH.code) + pad * 2;
        const runs = b.lines.map((l) => ({ text: l === '' ? ' ' : l, options: { breakLine: true } }));
        slide.addText(runs, {
          x: PAGE.mx, y, w: CONTENT_W, h, margin: pad * 72, valign: 'top', fit: 'none', wrap: true,
          fontFace: FONT.mono, fontSize: SIZE.code, color: COLOR.text, lineSpacing: SIZE.code * LH.code,
          fill: { color: COLOR.codeBg }, line: { color: COLOR.codeLine, width: 0.75 },
        });
        y += h + GAP;
        break;
      }
      case 'bullet': {
        // 連続する箇条書きを1つのテキストボックスにまとめる
        if (idx > 0 && blocks[idx - 1].type === 'bullet') return;
        const items = [];
        for (let j = idx; j < blocks.length && blocks[j].type === 'bullet'; j++) items.push(blocks[j]);
        const indentIn = 0.3;
        const lines = items.reduce((n, it) => n + textLines(it.text, CONTENT_W - indentIn, SIZE.body), 0);
        const spaceAfterPt = 4;
        const h = lines * lineHeightIn(SIZE.body, LH.body) + (items.length * spaceAfterPt) / 72;
        const runs = [];
        items.forEach((it) => {
          const rs = inlineRuns(it.text, { fontSize: SIZE.body, color: COLOR.text });
          rs[0].options = { ...rs[0].options, bullet: { indent: indentIn * 72 } };
          rs[rs.length - 1].options = { ...rs[rs.length - 1].options, breakLine: true };
          runs.push(...rs);
        });
        textBox(runs, h, { lineSpacing: SIZE.body * LH.body, paraSpaceAfter: spaceAfterPt });
        break;
      }
      default:
        throw new Error(`unknown block ${b.type}`);
    }
  });
  return y;
}

// ---- main ------------------------------------------------------------------
const deck = parseDeck(fs.readFileSync(SRC, 'utf8'));
const pptx = new PptxGenJS();
pptx.defineLayout({ name: 'W169', width: SLIDE.w, height: SLIDE.h });
pptx.layout = 'W169';
pptx.title = deck.meta.title;

deck.slides.forEach((blocks, i) => {
  const slide = pptx.addSlide();
  slide.background = { color: 'FFFFFF' };
  const isTitle = i === 0 && !blocks.some((b) => b.type === 'code' || b.type === 'bullet');
  const bottom = isTitle ? renderTitleSlide(slide, blocks) : renderContentSlide(slide, blocks);
  addFooter(slide, deck.meta.footer);
  const status = bottom - GAP > PAGE.footerY ? 'OVERFLOW (footer と重なる)' : 'ok';
  console.log(`slide ${i + 1}: content bottom = ${(bottom - GAP).toFixed(2)}in / footer top = ${PAGE.footerY}in -> ${status}`);
});

fs.mkdirSync(path.dirname(OUT), { recursive: true });
pptx.writeFile({ fileName: OUT }).then((f) => console.log(`written: ${f}`));
