// Composes the banner at the top of the fjord-base README (docs/screenshots/banner-light.webp / banner-dark.webp):
// the three plugins built on fjord-base – Torden, Fjord, Huldra – in perspective over a northern-lights fjord.
//
//   node banner.mjs [plugins folder]      (default: the folder that holds fjord-base, fjord, torden and huldra)
//
// Needs the page shots of the three plugins (run tools/screenshots/shoot.sh in each of them first),
// Playwright (npm install in this folder) and cwebp.

import { chromium } from "playwright";
import { execFileSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";

const BASE = resolve(dirname(fileURLToPath(import.meta.url)), "../..");
const PLUGINS = resolve(process.argv[2] ?? join(BASE, ".."));
const OUT_DIR = join(BASE, "docs/screenshots");
const WORK = join(BASE, "build/banner");
const FONTS = join(BASE, "resources/fonts");

// Which page of which plugin is in the picture
const SHOTS = {
  torden: "kick",
  fjord: "synth",
  huldra: "engine",
};
const raw = (plugin, theme) => join(PLUGINS, plugin, "build/screenshots/raw", `${SHOTS[plugin]}-${theme}.png`);

// Each plugin brings its own accent; the banner mixes the three of them in the sky
const PALETTE = {
  light: { bg1: "#eef4f6", bg2: "#d3e1e8", text: "#15253a", dim: "#5f7383", edge: "rgba(0,0,0,0.08)", shadow: "rgba(15,40,60,0.30)",
           ridge: ["#d0dde4", "#bccdd7", "#a8bdc9"], aurora: ["#1f9a7c", "#e8a04a", "#d98a8f"], auroraAlpha: 0.30, accent: "#0e7566" },
  dark:  { bg1: "#0f1c33", bg2: "#060a12", text: "#e2edf0", dim: "#8a9bb0", edge: "rgba(255,255,255,0.08)", shadow: "rgba(0,0,0,0.65)",
           ridge: ["#0e1828", "#0a1220", "#070d18"], aurora: ["#58d1a4", "#f2b544", "#f29c9c"], auroraAlpha: 0.30, accent: "#58d1a4" },
};

function ridges(c, height) {
  const poly = (pts, fill) => `<polygon points="${pts}" fill="${fill}"/>`;
  return `<svg class="ridges" viewBox="0 0 1800 ${height}" preserveAspectRatio="none" style="height:${height}px">
    ${poly(`0,${height * 0.45} 220,${height * 0.2} 420,${height * 0.5} 650,${height * 0.08} 920,${height * 0.46} 1180,${height * 0.2} 1440,${height * 0.52} 1650,${height * 0.16} 1800,${height * 0.4} 1800,${height} 0,${height}`, c.ridge[0])}
    ${poly(`0,${height * 0.68} 260,${height * 0.46} 560,${height * 0.74} 820,${height * 0.48} 1100,${height * 0.78} 1380,${height * 0.5} 1620,${height * 0.74} 1800,${height * 0.58} 1800,${height} 0,${height}`, c.ridge[1])}
    ${poly(`0,${height * 0.88} 340,${height * 0.74} 700,${height * 0.92} 1000,${height * 0.78} 1400,${height * 0.94} 1800,${height * 0.8} 1800,${height} 0,${height}`, c.ridge[2])}
  </svg>`;
}

function page(theme) {
  const c = PALETTE[theme];
  const font = (weight, file) => `@font-face { font-family: Manrope; font-weight: ${weight}; src: url(${pathToFileURL(join(FONTS, file)).href}); }`;
  const img = (plugin) => pathToFileURL(raw(plugin, theme)).href;
  return `<!doctype html><meta charset="utf-8"><style>
  ${font(400, "Manrope-Regular.ttf")} ${font(500, "Manrope-Medium.ttf")} ${font(600, "Manrope-SemiBold.ttf")}
  * { box-sizing: border-box; margin: 0; padding: 0; }
  html, body { width: 100%; height: 100%; }
  body { font-family: Manrope, sans-serif; color: ${c.text}; overflow: hidden; position: relative;
         background: linear-gradient(172deg, ${c.bg1}, ${c.bg2}); }
  /* the northern lights: one ribbon per plugin accent */
  .sky { position: absolute; inset: 0; filter: blur(46px); opacity: ${theme === "dark" ? 1 : 0.9}; }
  .sky i { position: absolute; border-radius: 50%; }
  .sky .a { left: 240px;  top: -150px; width: 760px; height: 360px; background: ${c.aurora[0]}; opacity: ${c.auroraAlpha + 0.04}; transform: rotate(-8deg); }
  .sky .b { left: 760px;  top: -190px; width: 820px; height: 330px; background: ${c.aurora[1]}; opacity: ${c.auroraAlpha}; transform: rotate(5deg); }
  .sky .c { left: 1220px; top: -110px; width: 700px; height: 380px; background: ${c.aurora[2]}; opacity: ${c.auroraAlpha}; transform: rotate(-4deg); }
  .stars { position: absolute; inset: 0; opacity: ${theme === "dark" ? 0.8 : 0}; }
  .ridges { position: absolute; left: 0; bottom: 0; width: 100%; }
  .copy { position: absolute; left: 96px; top: 190px; width: 520px; z-index: 3; }
  .logo { font-size: 112px; font-weight: 500; letter-spacing: -3.5px; line-height: 1; white-space: nowrap; }
  .logo span { color: ${c.accent}; }
  .tag { margin-top: 24px; font-size: 27px; line-height: 1.35; font-weight: 500; }
  .facts { margin-top: 28px; font-size: 14.5px; letter-spacing: 2.2px; text-transform: uppercase; color: ${c.dim}; line-height: 2.1; font-weight: 600; }
  .facts b { color: ${c.accent}; font-weight: 600; }
  .scene { position: absolute; left: 0; top: 0; width: 1800px; height: 760px; perspective: 2600px; z-index: 2; }
  .win { position: absolute; display: block; border-radius: 14px; width: 860px; transform-origin: left center;
         box-shadow: 0 0 0 1px ${c.edge}, 0 44px 80px -28px ${c.shadow}, 0 16px 34px -16px ${c.shadow}; }
  .huldra { left: 880px; top: 40px;  transform: rotateY(-17deg) rotateX(3deg) scale(0.9); filter: drop-shadow(0 0 40px ${c.aurora[2]}55); }
  .torden { left: 760px; top: 150px; transform: rotateY(-17deg) rotateX(3deg) scale(0.96); filter: drop-shadow(0 0 44px ${c.aurora[1]}55); }
  .fjord  { left: 640px; top: 278px; transform: rotateY(-17deg) rotateX(3deg) scale(1.0);  filter: drop-shadow(0 0 48px ${c.aurora[0]}66); }
  </style>
  <div class="sky"><i class="a"></i><i class="b"></i><i class="c"></i></div>
  <svg class="stars" viewBox="0 0 1800 760">${Array.from({ length: 70 }, (_, i) => {
    const x = (i * 211) % 1800, y = (i * 97) % 330, r = 0.7 + ((i * 7) % 5) * 0.28;
    return `<circle cx="${x}" cy="${y}" r="${r}" fill="#fff" opacity="${0.25 + ((i * 13) % 6) * 0.1}"/>`;
  }).join("")}</svg>
  ${ridges(c, 250)}
  <div class="copy">
    <div class="logo">fjord-base<span>.</span></div>
    <div class="tag">One foundation for every Fjord plugin.<br>Interface, effects, presets, build.</div>
    <div class="facts">
      <b>16</b> effects · <b>35</b> chains<br>
      <b>700+</b> effect presets<br>
      <b>VST3</b> · <b>AU</b> · <b>LV2</b> · standalone
    </div>
  </div>
  <div class="scene">
    <img class="win huldra" src="${img("huldra")}">
    <img class="win torden" src="${img("torden")}">
    <img class="win fjord" src="${img("fjord")}">
  </div>`;
}

for (const p of Object.keys(SHOTS))
  for (const theme of ["light", "dark"])
    if (!existsSync(raw(p, theme))) { console.error(`Missing ${raw(p, theme)} – run tools/screenshots/shoot.sh in ${p} first`); process.exit(1); }

mkdirSync(WORK, { recursive: true });
mkdirSync(OUT_DIR, { recursive: true });
const browser = await chromium.launch();
const tab = await browser.newPage({ deviceScaleFactor: 1.5, viewport: { width: 1800, height: 760 } });
try {
  for (const theme of ["light", "dark"]) {
    const html = join(WORK, `banner-${theme}.html`);
    writeFileSync(html, page(theme));
    await tab.goto(pathToFileURL(html).href);
    await tab.evaluate(() => document.fonts.ready);
    await tab.waitForFunction(() => [...document.images].every((i) => i.complete && i.naturalWidth > 0));
    const png = join(WORK, `banner-${theme}.png`);
    await tab.screenshot({ path: png });
    const out = join(OUT_DIR, `banner-${theme}.webp`);
    execFileSync("cwebp", ["-quiet", "-q", "88", "-m", "6", "-resize", "2200", "0", png, "-o", out]);
    console.log(`banner ${out.replace(BASE + "/", "")}`);
  }
} finally {
  await browser.close();
}
