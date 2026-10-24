# ♻️ EcoPulse — Smart Waste Segregation IoT (ESP32)

A self-contained Blynk alternative: a **web dashboard** + **ESP32 firmware** that shows
live fill levels of every section and **sends notifications when one gets full**.

- No backend, no account, no monthly limits — it talks **directly over MQTT**
  (free public broker) between your ESP32 and the dashboard.
- Works on **phone and PC**. Notifications pop up while the dashboard page is open.

```
ESP32 + HC-SR04 sensors ──MQTT──▶ broker.emqx.io ──MQTT-over-WebSocket──▶ EcoPulse dashboard (browser)
```

## 📁 Files

| File | What it is |
|---|---|
| `index.html` | The whole app (dashboard + notifications). One file — host it anywhere or even double-click it. |
| `esp32_ecopulse/esp32_ecopulse.ino` | ESP32 Arduino sketch: reads sensors, publishes fill %. |

---

## 🚀 Quick start (5 minutes)

### 1. The app
- **Easiest:** double-click `index.html` — it works straight from disk.
- **Better (real URL for your phone):** drag the `binpulse` folder onto
  [Netlify Drop](https://app.netlify.com/drop) → you get a free `https://...` link in seconds.
  (Also works: GitHub Pages, Vercel, Cloudflare Pages.)
- On your phone open the link, tap **🔔 Enable notifications**, then **Add to Home Screen** —
  it installs like a real app (own icon, fullscreen) and notifications work from there.

Press **▶ Demo** any time to see the app with simulated waste levels (great for testing/demo day).
If no ESP32 is online when you open the app, it automatically switches to clearly-labeled
**simulated waste levels** after a few seconds — and switches to **live data automatically** the moment
your ESP32 publishes (`?demo=1` forces simulated mode and keeps it).
Alerts fire when a section crosses the threshold (default **80%**) and re-arm only after it drops 10%.

### 2. The ESP32
1. Arduino IDE → Library Manager → install **PubSubClient** (Nick O'Leary).
2. Open `esp32_ecopulse/esp32_ecopulse.ino` and edit the **USER CONFIG** block:
   your WiFi name/password and — if you changed it — broker + `DEVICE_ID`.
3. Wire one HC-SR04 per waste section (sensor at the **top, looking down**):

| Waste | TRIG | ECHO | VCC | GND |
|---|---|---|---|---|
| Wet | GPIO 5 | GPIO 18 | 5V (VIN) | GND |
| Dry | GPIO 19 | GPIO 21 | 5V | GND |

   > Tip: HC-SR04 ECHO outputs 5V — a 1kΩ/2kΩ voltage divider on each ECHO pin
   > keeps the ESP32 GPIO safe (many boards work without it, but be safe).
4. Set each section's real **height in cm** in the `bins[]` table (measure from sensor to empty section floor).
5. Flash it. Open Serial Monitor (115200) — you'll see `Published: {"wet":42,...}` every 5 s.

### 3. Connect them
In the dashboard open **⚙ Settings** — broker `wss://broker.emqx.io:8084/mqtt`
and Device ID `eco01` are already the defaults. Cards appear as soon as the ESP32 publishes.
**↻ Ping device** forces an immediate update.

## 📡 MQTT topics

| Topic | Direction | Payload |
|---|---|---|
| `ecopulse/<id>/state` | ESP32 → dashboard (retained) | `{"wet":78,"dry":45}` (fill %, `-1` = sensor error) |
| `ecopulse/<id>/status` | ESP32 → dashboard (retained, LWT) | `online` / `offline` |
| `ecopulse/<id>/cmd` | dashboard → ESP32 | `ping` → immediate re-publish |

## 🔔 How notifications work
- Browser **push notifications** + in-app toast + **alert sound** + alert history log.
- They arrive while the dashboard tab is open (like the Blynk app running).
  For a hosted https:// URL you can also "Add to Home Screen" on Android for app-like behavior.
- Want an offline channel too? Add Telegram alerts easily — ask and it can be added.

## 🔧 Tuning
- **Threshold / re-arm drop / sound / broker / device ID** → dashboard ⚙ Settings (saved in your browser).
- **Publish rate, sensor gap, more sections, buzzer pin** → `USER CONFIG` block in the sketch.
- Fill % = `(height − distance) / (height − 4cm)`. If a section shows wrong %, adjust its `heightCm`.

## 🧪 Test without hardware
Open the dashboard → **▶ Demo** (or open `index.html?demo=1`): levels slowly fill, hit 80%,
and fire a real notification — perfect for checking everything before the demo.
