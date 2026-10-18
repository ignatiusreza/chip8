// The web player. It runs the Rust core compiled to WebAssembly (chip8.wasm, from
// web/wasm), draws its display, reads the keyboard and plays the beeper; this file is
// only the browser glue. It can also share the game live through the server
// (web/server), and watch a shared game at /watch/<id>.

const FRAME_WIDTH = 128
const FRAME_HEIGHT = 64
const FRAME_BYTES = (FRAME_WIDTH * FRAME_HEIGHT) / 8
const FLAG_CHANGED = 1
const FLAG_SOUND = 2
const FLAG_HIRES = 4
// in frames sent to people watching
const SHARED_HIRES = 1
const SHARED_SOUND = 2

// the keyboard key, by position, for each keypad value, laid out like the COSMAC VIP
// keypad: 1 2 3 4 / Q W E R / A S D F / Z X C V are 1 2 3 C / 4 5 6 D / 7 8 9 E / A 0 B F
const KEY_CODES = [
  'KeyX', 'Digit1', 'Digit2', 'Digit3', 'KeyQ', 'KeyW', 'KeyE', 'KeyA',
  'KeyS', 'KeyD', 'KeyZ', 'KeyC', 'Digit4', 'KeyR', 'KeyF', 'KeyV',
]
const KEYPAD_ORDER = [1, 2, 3, 0xc, 4, 5, 6, 0xd, 7, 8, 9, 0xe, 0xa, 0, 0xb, 0xf]

const $ = (id) => document.getElementById(id)
const canvas = $('screen')
const context = canvas.getContext('2d')
const image = context.createImageData(FRAME_WIDTH, FRAME_HEIGHT)

function setStatus(text) {
  $('status').textContent = text
}

// draws a packed frame: a bit per pixel, row by row, the leftmost pixel highest
function draw(frame) {
  const style = getComputedStyle(document.documentElement)
  const on = colour(style.getPropertyValue('--screen-on'))
  const off = colour(style.getPropertyValue('--screen-off'))
  for (let i = 0; i < FRAME_WIDTH * FRAME_HEIGHT; i++) {
    const [r, g, b] = frame[i >> 3] & (0x80 >> (i & 7)) ? on : off
    image.data.set([r, g, b, 255], i * 4)
  }
  context.putImageData(image, 0, 0)
}

function colour(hex) {
  const value = parseInt(hex.trim().slice(1), 16)
  return [value >> 16, (value >> 8) & 0xff, value & 0xff]
}

// The beeper: a square wave, which browsers only allow after the page was interacted with.
const beeper = {
  context: null,
  gain: null,
  wanted: false,
  start() {
    if (this.context) return
    this.context = new AudioContext()
    const oscillator = this.context.createOscillator()
    oscillator.type = 'square'
    oscillator.frequency.value = 440
    this.gain = this.context.createGain()
    this.gain.gain.value = 0
    oscillator.connect(this.gain).connect(this.context.destination)
    oscillator.start()
    this.set(this.wanted)
  },
  set(playing) {
    this.wanted = playing
    if (this.gain) this.gain.gain.setTargetAtTime(playing ? 0.05 : 0, this.context.currentTime, 0.005)
  },
}

function panel(rom, quirks, speed) {
  const element = $('panel')
  element.replaceChildren()
  const add = (tag, text, className) => {
    const child = document.createElement(tag)
    child.textContent = text
    if (className) child.className = className
    return element.appendChild(child)
  }
  if (rom) {
    add('h1', rom.title)
    if (rom.author) add('p', `by ${rom.author}`, 'author')
    for (const line of rom.about) add('p', line)
    if (rom.controls.length) {
      add('h2', 'Controls')
      const list = add('ul', '')
      for (const line of rom.controls) {
        const item = document.createElement('li')
        item.textContent = line
        list.appendChild(item)
      }
    }
  }
  const running = add('div', '', 'running')
  running.append(`Quirks: ${quirks}`, document.createElement('br'), `Speed: ${speed} opcodes per frame`)
}

async function loadRoms() {
  const response = await fetch('/roms.json')
  return response.json()
}

// ---- playing

async function play(roms) {
  const { instance } = await WebAssembly.instantiateStreaming(fetch('/chip8.wasm'), {})
  const wasm = instance.exports
  const frame = () => new Uint8Array(wasm.memory.buffer, wasm.frame(), FRAME_BYTES)
  let rom = null
  let flags = 0
  const share = sharing()

  async function start(name) {
    rom = roms.find((r) => r.name === name) ?? roms[0]
    const bytes = new Uint8Array(await (await fetch(`/roms/${rom.name}.ch8`)).arrayBuffer())
    const quirks = new TextEncoder().encode(rom.quirks)
    // asking for the buffer can grow the module's memory, which replaces memory.buffer, so
    // only look at memory.buffer afterwards
    const at = wasm.buffer(bytes.length + quirks.length)
    const input = new Uint8Array(wasm.memory.buffer, at, bytes.length + quirks.length)
    input.set(bytes)
    input.set(quirks, bytes.length)
    const seed = crypto.getRandomValues(new Uint32Array(1))[0]
    const error = wasm.load(bytes.length, quirks.length, seed)
    if (error) {
      setStatus(`Could not start ${rom.title} (error ${error})`)
      return
    }
    $('rom').value = rom.name
    history.replaceState(null, '', `/?rom=${rom.name}`)
    panel(rom, rom.quirks, rom.speed)
    setStatus('')
    share.hello(rom)
    canvas.focus()
  }

  for (const r of roms) $('rom').append(new Option(r.title, r.name))
  $('rom').addEventListener('change', () => start($('rom').value))

  // keyboard, by key position so the layout works on non-QWERTY keyboards too
  const held = new Set()
  function setKey(key, pressed) {
    if (pressed === held.has(key)) return
    pressed ? held.add(key) : held.delete(key)
    wasm.set_key(key, pressed ? 1 : 0)
    $('keypad').children[KEYPAD_ORDER.indexOf(key)].classList.toggle('down', pressed)
  }
  addEventListener('keydown', (event) => {
    beeper.start()
    const key = KEY_CODES.indexOf(event.code)
    if (key < 0 || event.target.closest?.('input, select') || event.ctrlKey || event.metaKey || event.altKey) return
    event.preventDefault()
    setKey(key, true)
  })
  addEventListener('keyup', (event) => {
    const key = KEY_CODES.indexOf(event.code)
    if (key >= 0) setKey(key, false)
  })
  addEventListener('blur', () => held.forEach((key) => setKey(key, false)))
  addEventListener('pointerdown', () => beeper.start())
  keypad(setKey)

  // one 1/60s frame of the game at a time, catching up a little after a slow one
  let last = performance.now()
  let behind = 0
  function tick(now) {
    behind = Math.min(behind + now - last, 100)
    last = now
    while (behind >= 1000 / 60) {
      behind -= 1000 / 60
      const previous = flags
      flags = wasm.run_frame(rom.speed)
      if (flags & FLAG_CHANGED) draw(frame())
      beeper.set(Boolean(flags & FLAG_SOUND))
      if (flags & FLAG_CHANGED || (flags ^ previous) & (FLAG_SOUND | FLAG_HIRES)) share.frame(flags, frame())
    }
    requestAnimationFrame(tick)
  }

  const wanted = new URLSearchParams(location.search).get('rom')
  await start(wanted)
  requestAnimationFrame(tick)
}

function keypad(setKey) {
  for (const key of KEYPAD_ORDER) {
    const button = document.createElement('button')
    button.type = 'button'
    button.append(key.toString(16).toUpperCase())
    const label = document.createElement('small')
    label.textContent = KEY_CODES[key].slice(-1)
    button.append(label)
    button.addEventListener('pointerdown', (event) => {
      event.preventDefault()
      button.setPointerCapture(event.pointerId)
      setKey(key, true)
    })
    for (const type of ['pointerup', 'pointercancel']) button.addEventListener(type, () => setKey(key, false))
    $('keypad').append(button)
  }
}

// Sharing the game: frames go to the server, which forwards them to everyone watching.
// The session survives reloads and dropped connections for a while, using the id and
// token kept for this tab.
function sharing() {
  let socket = null
  let rom = null
  let lastFrame = null
  let wanted = false

  function connect() {
    const saved = JSON.parse(sessionStorage.getItem('chip8-session') ?? 'null')
    const query = saved ? `?id=${encodeURIComponent(saved.id)}&token=${encodeURIComponent(saved.token)}` : ''
    socket = new WebSocket(`${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws/play${query}`)
    socket.binaryType = 'arraybuffer'
    socket.addEventListener('message', (event) => {
      const message = JSON.parse(event.data)
      if (message.type === 'session') {
        sessionStorage.setItem('chip8-session', JSON.stringify({ id: message.id, token: message.token }))
        $('url').value = `${location.origin}/watch/${message.id}`
        $('link').hidden = false
        $('share').textContent = 'Stop sharing'
        setStatus('Sharing: anyone with the link can watch')
        if (rom) send(JSON.stringify({ type: 'hello', rom: rom.name, quirks: rom.quirks, speed: rom.speed }))
        if (lastFrame) send(lastFrame)
      } else if (message.type === 'viewers') {
        $('viewers').textContent = message.count === 1 ? '1 watching' : `${message.count} watching`
      } else if (message.type === 'full') {
        setStatus('The server is too busy to share right now')
        stop()
      }
    })
    socket.addEventListener('close', () => {
      if (wanted) setTimeout(() => wanted && connect(), 1000)
    })
  }

  function send(data) {
    if (socket?.readyState === WebSocket.OPEN) socket.send(data)
  }

  function stop() {
    wanted = false
    socket?.close()
    socket = null
    sessionStorage.removeItem('chip8-session')
    $('link').hidden = true
    $('viewers').textContent = ''
    $('share').textContent = 'Share live'
  }

  $('share').addEventListener('click', () => {
    if (wanted) {
      stop()
      setStatus('Stopped sharing')
    } else {
      wanted = true
      connect()
    }
  })
  $('copy').addEventListener('click', async () => {
    $('url').select()
    try {
      await navigator.clipboard.writeText($('url').value)
      setStatus('Link copied')
    } catch {
      setStatus('Press Ctrl+C (or Cmd+C) to copy the link')
    }
  })
  // carry on sharing after a reload
  if (sessionStorage.getItem('chip8-session')) {
    wanted = true
    connect()
  }

  return {
    hello(next) {
      rom = next
      send(JSON.stringify({ type: 'hello', rom: rom.name, quirks: rom.quirks, speed: rom.speed }))
    },
    frame(flags, frame) {
      const message = new Uint8Array(1 + FRAME_BYTES)
      message[0] = (flags & FLAG_HIRES ? SHARED_HIRES : 0) | (flags & FLAG_SOUND ? SHARED_SOUND : 0)
      message.set(frame, 1)
      lastFrame = message
      send(message)
    },
  }
}

// ---- watching

function watch(roms, id) {
  document.body.classList.add('watching')
  setStatus('Connecting…')
  let rom = null
  const socket = new WebSocket(`${location.protocol === 'https:' ? 'wss' : 'ws'}://${location.host}/ws/watch/${encodeURIComponent(id)}`)
  socket.binaryType = 'arraybuffer'
  socket.addEventListener('open', () => setStatus('Waiting for the player…'))
  socket.addEventListener('message', (event) => {
    if (event.data instanceof ArrayBuffer) {
      const message = new Uint8Array(event.data)
      draw(message.subarray(1))
      beeper.set(Boolean(message[0] & SHARED_SOUND))
      setStatus(`Watching${rom ? ` ${rom.title}` : ''} live`)
      return
    }
    const message = JSON.parse(event.data)
    if (message.type === 'hello') {
      rom = roms.find((r) => r.name === message.rom) ?? null
      panel(rom, message.quirks, message.speed)
      setStatus(`Watching${rom ? ` ${rom.title}` : ''} live`)
    } else if (message.type === 'ended') {
      ended(rom)
    } else if (message.type === 'full') {
      setStatus('Too many people are watching this game right now')
    }
  })
  socket.addEventListener('close', () => {
    if (!$('status').textContent.startsWith('This game')) ended(rom)
  })

  // browsers only play sound after a click
  $('sound').hidden = false
  $('sound').addEventListener('click', () => {
    beeper.start()
    $('sound').hidden = true
  })
}

function ended(rom) {
  const status = $('status')
  status.textContent = 'This game is no longer being shared. '
  const link = document.createElement('a')
  link.href = rom ? `/?rom=${rom.name}` : '/'
  link.textContent = rom ? `Play ${rom.title} yourself` : 'Play a game yourself'
  status.append(link)
}

// Embedded in another page, such as the blog, tell it how tall the page is, so it can
// size the frame to fit instead of scrolling inside it.
if (window.parent !== window) {
  const main = document.querySelector('main')
  new ResizeObserver(() => {
    window.parent.postMessage({ type: 'chip8-height', height: Math.ceil(main.getBoundingClientRect().height) }, '*')
  }).observe(main)
}

const roms = await loadRoms()
const watching = location.pathname.match(/^\/watch\/([A-Za-z0-9]+)$/)
if (watching) watch(roms, watching[1])
else play(roms).catch((error) => setStatus(`Could not start the emulator: ${error}`))
