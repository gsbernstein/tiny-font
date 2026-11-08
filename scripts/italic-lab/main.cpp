#include <Font.hpp>
#include <IBMFFonts/SolSans_75.h>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace ibmf_defs;
using Pixels = std::set<std::pair<int, int>>;

constexpr int GRID_W = 24;
constexpr int GRID_H = 24;
constexpr int ORIGIN = 6;
constexpr int BASE = 18;

auto band(int h) -> int { return static_cast<int>(std::floor(h / 3.0)); }

struct Shape {
    Pixels upright;
    Pixels italic;
};

auto pixelJSON(const Pixels &pixels) -> std::string {
    std::string result = "[";
    for (const auto &[x, y] : pixels) {
        if (result.size() > 1) {
            result += ",";
        }
        result += "[" + std::to_string(x) + "," + std::to_string(y) + "]";
    }
    return result + "]";
}

auto main(int argc, char **argv) -> int {
    try {
        if (argc != 2 && argc != 3) {
            throw std::runtime_error("Usage: italic-lab OUTPUT_DIRECTORY [GLYPH_JSON]");
        }
        std::string saved = "null";
        {
            const std::string source = argc == 3 ? argv[2] : DEFAULT_GLYPH_JSON;
            std::ifstream input(source);
            if (!input) {
                throw std::runtime_error("Cannot open saved glyph JSON: " + source);
            }
            std::filesystem::create_directories(argv[1]);
            saved.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
            if (saved.find('<') != std::string::npos) {
                throw std::runtime_error("Saved glyph JSON contains unsafe HTML characters");
            }
        }
        FontData data(SOLSANS_75_IBMF, SOLSANS_75_IBMF_LEN);
        Font font(data, 1);
        if (!font.isInitialized()) {
            throw std::runtime_error("Medium font not initialized");
        }
        auto face = data.getFace(1);
        std::map<char, Shape> shapes;
        for (int c = 32; c <= 126; ++c) {
            const auto code = data.translate(c);
            Glyph metrics{};
            if (!face->getGlyph(code, metrics, false)) {
                throw std::runtime_error("Missing ASCII metrics");
            }
            Glyph glyph{};
            if (!face->getGlyph(code, glyph, true, true, Pos(0, metrics.metrics.yoff), false)) {
                throw std::runtime_error("Glyph decoding failed");
            }
            std::unique_ptr<uint8_t[]> decoded(glyph.bitmap.pixels);
            Shape shape;
            if (decoded) {
                for (int row = 0; row < glyph.bitmap.dim.height; ++row) {
                    for (int col = 0; col < glyph.bitmap.dim.width; ++col) {
                        if (((decoded[row * glyph.bitmap.pitch + col / 8] >> (7 - col % 8)) & 1) !=
                            BLACK_ONE_BIT) {
                            continue;
                        }
                        const int h = metrics.metrics.yoff - row;
                        const int x = ORIGIN - metrics.metrics.xoff + col;
                        const int y = BASE - h;
                        if (x < 0 || x + band(h) >= GRID_W || y < 0 || y >= GRID_H) {
                            throw std::runtime_error("Glyph outside editor grid");
                        }
                        shape.upright.emplace(x, y);
                        shape.italic.emplace(x + band(h), y);
                    }
                }
            }
            shapes.emplace(static_cast<char>(c), std::move(shape));
        }
        std::ofstream html(std::string(argv[1]) + "/italic-glyph-editor.html");
        html.exceptions(std::ios::failbit | std::ios::badbit);
        html << R"HTML(<!doctype html><html lang="en"><meta charset="utf-8">
<title>Medium hybrid italic glyph lab</title><style>
body{font:15px system-ui;background:#eee;margin:24px auto;max-width:950px;padding:0 16px}
section{background:white;border:1px solid #ccc;padding:18px;margin:16px 0}
button,select{font:inherit;padding:6px;margin:3px}h1{font-size:24px}
textarea{width:100%;height:180px;font:12px monospace}
.tools{display:flex;flex-wrap:wrap;gap:16px}.glyph{display:inline-block}
.glyph svg{width:192px;height:192px}.editor{display:grid;grid-template-columns:repeat(24,16px);width:384px}
.editor button{width:16px;height:16px;border:1px solid #ddd;margin:0;padding:0;background:white}
.editor button.ink{background:#111}.editor button.base{border-top:2px solid #28a}
svg{display:block;shape-rendering:crispEdges;background:white}
.sample{margin:8px 0;overflow:auto}.sample svg{width:600px;height:28px}
.zoom svg{width:720px;height:112px}p{line-height:1.5}.status{color:#694400}
@media print{body{background:white}button,.editor{display:none}section{break-inside:avoid}}
</style><h1>Medium hybrid italic glyph lab</h1>
<p>Keep the three-row synthetic italic for ordinary letters; try custom pixels for capitals,
digits, lowercase v and punctuation. Glyph origins, advances, existing pair kerning and italic end correction are
unchanged. A replacement can still collide with its neighbours: the context previews
report overlapping ink. This is an experiment, not a firmware change.</p>
<section><label>Glyph <select id="choice"></select></label>
<button id="synthetic">Reset selected to synthetic</button>
<button id="upright">Try upright selected glyph</button>
<button id="gentle">Try gentler diagonal placement</button>
<button id="trial">Load trial A / V / W</button>
<button id="saved">Restore your saved edits</button>
<button id="all-reset">Reset all</button><button id="export">Export custom glyphs</button>
<p>Your exported edits are loaded when available. Newly added glyphs start synthetic.
Restore your saved edits replaces the entire custom set; reset all returns to synthetic.
The gentler preset is a starting point, not a hand-designed font:
it keeps the first four rows above the baseline aligned. Click pixels to refine it.
Blue line marks the baseline. Edits stay in this page until reload; export to keep them.</p>
<div class="tools"><div><p>Editable hybrid, 16x</p><div class="editor" id="editor"></div></div>
<div><p>Original synthetic / upright, 8x</p><div class="glyph" id="original"></div>
<div class="glyph" id="reference"></div><p id="metrics"></p></div></div>
<p role="status" id="status" class="status"></p>
<div id="export-panel" hidden><p>Copy this JSON into a file to keep your glyphs.
Export again after further edits to update it.</p>
<textarea id="export-data" readonly aria-label="Custom glyph JSON"></textarea></div></section>
<section><h2>Same layout: synthetic vs hybrid</h2><div id="samples"></div></section>
<script>
const W=24,H=24,origin=6,baseline=18;
const shapes={
)HTML";
        for (auto it = shapes.begin(); it != shapes.end(); ++it) {
            if (it != shapes.begin()) {
                html << ",";
            }
            html << '"' << static_cast<int>(it->first)
                 << "\":{upright:" << pixelJSON(it->second.upright)
                 << ",italic:" << pixelJSON(it->second.italic) << "}";
        }
        html << "};\nconst saved=" << saved << ";\nconst runs=[";
        const std::array<std::string, 10> samples = {
            "AV VA AW WA VW WV AVATAR WAVE VIVID",  "A VERY WARM VIEW: V W A 0123456789",
            "2026 11:45 100% 1,234.56 78/90",       "some prose with a V and W in the same line.",
            "Vast waves move over a quiet valley.", "A0 V1 W2 3V 4W 5A 6789 (V), W.",
            "R Y v % ( ) , :   RY YR RV VR YV VY",  "River, very vivid: (YES), 25% (75%).",
            "R: 1,234.56 (Y), 99%: v, v: (v)",      "vivid valleys, very warm: (over, under)."};
        for (size_t sample = 0; sample < samples.size(); ++sample) {
            const auto &text = samples[sample];
            if (sample) {
                html << ",";
            }
            const int pitch = (600 + 7) / 8;
            std::vector<uint8_t> bitmapPixels(static_cast<size_t>(pitch) * 28,
                                              WHITE_ONE_BIT ? 255 : 0);
            Bitmap bitmap{};
            bitmap.dim = Dim(600, 28);
            bitmap.pitch = pitch;
            bitmap.pixels = bitmapPixels.data();
            font.setItalicFont(true);
            const int nativeEnd = font.drawSingleLineOfText(bitmap, Pos(12, 2), text, false);
            const int up = (font.lineHeight() - font.getTextSize("T").height - 1) / 2;
            const int textBase = 2 + font.lineHeight() - up;
            Pixels expected;
            int atX = 12;
            int correction = 0;
            bool first = true;
            html << "{text:\"" << text << "\",base:" << textBase << ",placements:[";
            for (size_t i = 0; i < text.size(); ++i) {
                const char c = text[i];
                const auto code = data.translate(c);
                auto next = i + 1 == text.size() ? NO_GLYPH_CODE : data.translate(text[i + 1]);
                FIX16 kern = 0;
                if (face->ligKern(code, &next, &kern)) {
                    throw std::runtime_error("Experiment sample requires a ligature");
                }
                const bool last = next == SPACE_CODE || next == NO_GLYPH_CODE;
                atX += first ? face->getGlyphHOffset(code) : 0;
                if (i) {
                    html << ",";
                }
                html << "[" << static_cast<int>(c) << "," << atX << "]";
                for (const auto &[x, y] : shapes.at(c).italic) {
                    expected.emplace(atX + x - ORIGIN, textBase + y - BASE);
                }
                Glyph metrics{};
                if (!face->getGlyph(code, metrics, false)) {
                    throw std::runtime_error("Sample glyph metrics unavailable");
                }
                if (code == SPACE_CODE) {
                    atX += metrics.metrics.advance >> 6;
                } else {
                    atX += last ? face->getGlyphWidth(code) - kern / 64 - metrics.metrics.xoff
                                : (metrics.metrics.advance + kern) >> 6;
                    correction = band(metrics.metrics.yoff);
                }
                first = c == ' ';
            }
            if (atX + correction != nativeEnd) {
                throw std::runtime_error("Sample advance differs from native renderer");
            }
            for (int y = 0; y < 28; ++y) {
                for (int x = 0; x < 600; ++x) {
                    const bool nativeBlack =
                        ((bitmapPixels[y * pitch + x / 8] >> (7 - x % 8)) & 1) == BLACK_ONE_BIT;
                    if (nativeBlack != (expected.count({x, y}) != 0)) {
                        throw std::runtime_error("Sample pixels differ from native renderer");
                    }
                }
            }
            html << "],end:" << nativeEnd << "}";
        }
        html << R"HTML(];
const editable='AVWRYv%(),:0123456789',custom={},choice=document.getElementById('choice');
const key=([x,y])=>`${x},${y}`;
function glyph(c){return custom[c]||shapes[c].italic;}
function svg(pixels,w,h){
    const element=document.createElementNS('http://www.w3.org/2000/svg','svg');
    element.setAttribute('viewBox',`0 0 ${w} ${h}`);
    element.setAttribute('width',w);element.setAttribute('height',h);
    for(const [x,y] of pixels){
        const rect=document.createElementNS(element.namespaceURI,'rect');
        rect.setAttribute('x',x);rect.setAttribute('y',y);
        rect.setAttribute('width',1);rect.setAttribute('height',1);
        element.append(rect);
    }
    return element;
}
function layout(run,hybrid){
    const ink=new Set();let overlaps=0,clipped=0;
    for(const [c,at] of run.placements){
        for(const [x,y] of hybrid?glyph(c):shapes[c].italic){
            const dx=at+x-origin,dy=run.base+y-baseline;
            if(dx<0||dx>=600||dy<0||dy>=28)clipped++;
            const k=key([dx,dy]);if(ink.has(k))overlaps++;ink.add(k);
        }
    }
    return {pixels:[...ink].map(k=>k.split(',').map(Number)),overlaps,clipped};
}
function refresh(){
    const c=choice.value,current=new Set(glyph(c).map(key));
    for(const button of document.querySelectorAll('#editor button')){
        const x=Number(button.dataset.x),y=Number(button.dataset.y);
        button.classList.toggle('ink',current.has(key([x,y])));
        button.setAttribute('aria-pressed',current.has(key([x,y])));
    }
    document.getElementById('original').replaceChildren(svg(shapes[c].italic,W,H));
    document.getElementById('reference').replaceChildren(svg(shapes[c].upright,W,H));
    document.getElementById('metrics').textContent=
        `Selected ${String.fromCharCode(Number(c))}: ${glyph(c).length} ink pixels; layout unchanged.`;
    const container=document.getElementById('samples');container.replaceChildren();
    let added=0,clipped=0;
    for(const run of runs){
        const original=layout(run,false),hybrid=layout(run,true);
        added+=Math.max(0,hybrid.overlaps-original.overlaps);clipped+=hybrid.clipped;
        const label=document.createElement('p');
        label.textContent=run.text+` — overlapping pixels: synthetic ${original.overlaps}, hybrid ${hybrid.overlaps}`;
        container.append(label);
        for(const [name,result] of [['Synthetic',original],['Hybrid',hybrid]]){
            const caption=document.createElement('small');caption.textContent=name;
            const div=document.createElement('div');div.className='sample';
            div.append(svg(result.pixels,600,28));container.append(caption,div);
        }
        const zoom=document.createElement('div');zoom.className='sample zoom';
        zoom.append(svg(hybrid.pixels.filter(([x])=>x<180),180,28));container.append(zoom);
    }
    document.getElementById('status').textContent=
        `${Object.keys(custom).length} custom glyphs. `+
        (added?`Warning: ${added} extra overlapping pixels in these samples. `:'No added overlaps in these samples. ')+
        (clipped?`Warning: ${clipped} pixels outside sample bounds.`:'No clipped sample pixels.')+
        ' These checks do not guarantee spacing for every possible pair.';
}
for(const c of editable){
    const option=document.createElement('option');option.value=c.charCodeAt(0);option.textContent=c;
    choice.append(option);
}
choice.value=String('V'.charCodeAt(0));choice.addEventListener('change',refresh);
for(let y=0;y<H;y++)for(let x=0;x<W;x++){
    const button=document.createElement('button');button.dataset.x=x;button.dataset.y=y;
    button.setAttribute('aria-label',`Pixel x ${x-origin}, height ${baseline-y}`);
    if(y===baseline)button.classList.add('base');
    button.addEventListener('click',()=>{
        const c=choice.value,ink=new Set(glyph(c).map(key)),k=key([x,y]);
        if(ink.has(k))ink.delete(k);else ink.add(k);
        custom[c]=[...ink].map(k=>k.split(',').map(Number));refresh();
    });
    document.getElementById('editor').append(button);
}
document.getElementById('synthetic').onclick=()=>{delete custom[choice.value];refresh();};
document.getElementById('upright').onclick=()=>{
    custom[choice.value]=shapes[choice.value].upright.map(p=>[...p]);refresh();
};
function gentle(c){
    return shapes[c].upright.map(([x,y])=>[x+Math.floor((baseline-y)/4),y]);
}
document.getElementById('gentle').onclick=()=>{custom[choice.value]=gentle(choice.value);refresh();};
function trial(){
    const v=[];
    const right=[15,14,14,13,12,11,11,10,9];
    for(let y=9;y<=17;y++){
        v.push([9,y]);
        if(right[y-9]!==9)v.push([right[y-9],y]);
    }
    custom[86]=v;
    const a=[],left=[12,12,11,10,10,9,8,7,6];
    for(let y=9;y<=17;y++){
        a.push([left[y-9],y]);
        if(left[y-9]!==12)a.push([12,y]);
    }
    for(let x=9;x<12;x++)a.push([x,15]);
    custom[65]=a;
    const offsets={9:3,10:3,11:2,12:2,13:2,14:1,15:1,16:0,17:0};
    custom[87]=shapes[87].upright.map(([x,y])=>[x+offsets[y],y]);
    refresh();
}
document.getElementById('trial').onclick=trial;
function restoreSaved(){
    if(!saved)throw Error('No saved glyph export was supplied.');
    for(const [field,value] of Object.entries({font:'SolSans_75',face:1,origin,baseline,width:W,height:H})){
        if(saved[field]!==value)throw Error('Saved glyph metadata mismatch: '+field);
    }
    if(!saved.glyphs||typeof saved.glyphs!=='object'||Array.isArray(saved.glyphs)){
        throw Error('Saved glyphs must be a glyph-to-pixels object.');
    }
    const replacement={};
    for(const [c,pixels] of Object.entries(saved.glyphs)){
        if(c.length!==1||!editable.includes(c)||!Array.isArray(pixels)){
            throw Error('Invalid saved glyph: '+c);
        }
        const occupied=new Set();
        for(const p of pixels){
            if(!Array.isArray(p)||p.length!==2||!p.every(Number.isInteger)||
               p[0]<0||p[0]>=W||p[1]<0||p[1]>=H||occupied.has(key(p))){
                throw Error('Invalid or duplicate saved pixel for '+c);
            }
            occupied.add(key(p));
        }
        replacement[c.charCodeAt(0)]=pixels.map(p=>[...p]);
    }
    for(const c of Object.keys(custom))delete custom[c];
    Object.assign(custom,replacement);refresh();
}
document.getElementById('saved').disabled=!saved;
document.getElementById('saved').onclick=restoreSaved;
document.getElementById('all-reset').onclick=()=>{
    for(const c of Object.keys(custom))delete custom[c];refresh();
};
document.getElementById('export').onclick=()=>{
    const glyphs=Object.fromEntries(Object.entries(custom).map(([c,pixels])=>
        [String.fromCharCode(Number(c)),pixels]));
    const output=document.getElementById('export-data');
    output.value=JSON.stringify({font:'SolSans_75',face:1,origin,baseline,
        width:W,height:H,glyphs},null,2);
    document.getElementById('export-panel').hidden=false;
    output.focus();output.select();
};
if(saved)restoreSaved();else trial();
</script></html>)HTML";
        html.close();
        std::cout << "Verified " << samples.size()
                  << " sample layouts and pixels against native medium italics.\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
