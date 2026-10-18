// Plays, shares and watches a game in Chromium against a running server:
//
//   npm install --no-save playwright && npx playwright install chromium
//   node tests/browser.mjs http://localhost:8080 [screenshot directory]
//
// It takes about 40 seconds, most of it waiting for a stopped session to end.

import { chromium } from 'playwright'

const base = process.argv[2]
const shots = process.argv[3] ?? '.'
const errors = []
const browser = await chromium.launch()
const check = (name, ok, detail = '') => { console.log(`${ok ? 'PASS' : 'FAIL'} ${name} ${detail}`); if (!ok) process.exitCode = 1 }

async function page(context) {
  const p = await context.newPage()
  p.on('pageerror', (e) => errors.push(`${e}`))
  p.on('console', (m) => m.type() === 'error' && errors.push(m.text()))
  return p
}
// the screen's lit pixels, and where the paddle (bottom rows) is
const screen = (p) => p.evaluate(() => {
  const c = document.getElementById('screen')
  const d = c.getContext('2d').getImageData(0, 0, 128, 64).data
  let lit = 0, sum = 0, n = 0, hash = 0
  for (let i = 0; i < 128 * 64; i++) {
    const on = d[i * 4] > 128
    if (on) { lit++; hash = (hash * 31 + i) | 0 }
    if (on && i >= 128 * 60) { sum += i % 128; n++ }
  }
  return { lit, paddle: n ? sum / n : null, hash }
})

const player = await page(await browser.newContext({ viewport: { width: 1200, height: 800 } }))
await player.goto(`${base}/?rom=br8kout`)
await player.waitForFunction(() => document.getElementById('panel').textContent.includes('Br8kout'))
// Br8kout draws its bricks before it reads the keys
await player.waitForTimeout(2000)
let s = await screen(player)
check('the game draws', s.lit > 100, `lit=${s.lit}`)
check('the panel shows how to play and settings', (await player.textContent('#panel')).includes('Speed: 7 opcodes per frame'))
const before = s.paddle
await player.focus('#screen')
await player.keyboard.down('a'); await player.waitForTimeout(300); await player.keyboard.up('a')
s = await screen(player)
check('holding A moves the paddle left', s.paddle < before, `${before} -> ${s.paddle}`)
await player.screenshot({ path: `${shots}/player.png` })

await player.click('#share')
await player.waitForFunction(() => document.getElementById('url').value.includes('/watch/'))
const url = await player.inputValue('#url')
check('sharing gives a watch link', /\/watch\/[A-Za-z0-9]{8}$/.test(url), url)

const viewerContext = await browser.newContext({ viewport: { width: 1200, height: 800 } })
const viewer = await page(viewerContext)
await viewer.goto(url)
await viewer.waitForFunction(() => document.getElementById('status').textContent.includes('Watching Br8kout live'))
check('the viewer sees the game', true)
await player.waitForFunction(() => document.getElementById('viewers').textContent === '1 watching')
check('the player sees 1 watching', true)
await player.waitForTimeout(300)
const [a, b] = await Promise.all([screen(player), screen(viewer)])
check('the viewer shows the same picture', Math.abs(a.lit - b.lit) <= 4, `player lit=${a.lit}, viewer lit=${b.lit}`)
check('the viewer panel shows the game', (await viewer.textContent('#panel')).includes('Br8kout'))
check('the viewer has no game controls', !(await viewer.isVisible('#rom')) && !(await viewer.isVisible('#share')))

await player.selectOption('#rom', 'octogon')
await viewer.waitForFunction(() => document.getElementById('panel').textContent.includes('Super Octogon'))
check('switching games reaches the viewer', (await viewer.textContent('#panel')).includes('Quirks: schip'))
await player.waitForTimeout(1500)
await viewer.screenshot({ path: `${shots}/viewer.png` })

// a reload keeps sharing the same session
await player.reload()
await player.waitForFunction(() => document.getElementById('url').value.includes('/watch/'))
check('a reload keeps the same session', (await player.inputValue('#url')) === url)
await player.waitForFunction(() => document.getElementById('viewers').textContent === '1 watching')

await player.click('#share')
const t0 = Date.now()
await viewer.waitForFunction(() => document.getElementById('status').textContent.includes('no longer being shared'), null, { timeout: 45000 })
check('the session ends after the player stops', true, `after ${Math.round((Date.now() - t0) / 1000)}s`)

check('no page errors', errors.length === 0, errors.join(' | '))
await browser.close()
