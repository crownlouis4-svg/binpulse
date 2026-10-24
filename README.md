# 🗑️ BinPulse — Blynk-style IoT app for a Segregation Dustbin (ESP32)

A self-contained Blynk alternative: a **web dashboard** + **ESP32 firmware** that shows
live fill levels of every bin and **sends notifications when a bin gets full**.

- No backend, no account, no monthly limits — it talks **directly over MQTT**
  (free public broker) between your ESP32 and the dashboard.
- Works on **phone and PC**. Notifications pop up while the dashboard page is open.

```
ESP32 + HC-SR04 sensors ──MQTT──▶ broker.emqx.io ──MQTT-over-WebSocket──▶ BinPulse dashboard (browser)
```

## 📁 Files

| File | What it is |
|---|---|
| `index.html` | The whole app (dashboard + notifications). One file — host it anywhere or even double-click it. |
| `esp32_dustbin/esp32_dustbin.ino` | ESP32 Arduino sketch: reads sensors, publishes fill %. |

---

## 🚀 Quick start (5 minutes)

### 1. The app
- **Easiest:** double-click `index.html` — it works straight from disk.
- **Better (real URL for your phone):** drag the `binpulse` folder onto
  [Netlify Drop](https://app.netlify.com/drop) → you get a free `https://...` link in seconds.
  (Also works: GitHub Pages, Vercel, Cloudflare Pages.)
- On your phone open the link, tap **🔔 Enable notifications**, then **Add to Home Screen**.

Press **▶ Demo** any time to see the app with simulated bins (great for testing/demo day).
If no ESP32 is online when you open the app, it automatically switches to clearly-labeled
**simulated bins** after a few seconds — and switches to **live data automatically** the moment
your ESP32 publishes (`?demo=1` forces simulated mode and keeps it).
Alerts fire when a bin crosses the threshold (default **80%**) and re-arm only after it drops 10%.

### 2. The ESP32
1. Arduino IDE → Library Manager → install **PubSubClient** (Nick O'Leary).
2. Open `esp32_dustbin/esp32_dustbin.ino` and edit the **USER CONFIG** block:
   your WiFi name/password and — if you changed it — broker + `DEVICE_ID`.
3. Wire one HC-SR04 per bin (sensor at the **top, looking down**):

   | Bin | TRIG | ECHO | VCC | GND |
   |---|---|---|---|---|
   | Wet | GPIO 5 | GPIO 18 | 5V (VIN) | GND |
   | Dry | GPIO 19 | GPIO 21 | 5V | GND |
   | Metal | GPIO 22 | GPIO 23 | 5V | GND |

   > Tip: HC-SR04 ECHO outputs 5V — a 1kΩ/2kΩ voltage divider on each ECHO pin
   > keeps the ESP32 GPIO safe (many boards work without it, but be safe).
4. Set each bin's real **height in cm** in the `bins[]` table (measure from sensor to empty bin floor).
5. Flash it. Open Serial Monitor (115200) — you'll see `Published: {"wet":42,...}` every 5 s.

### 3. Connect them
In the dashboard open **⚙ Settings** — broker `wss://broker.emqx.io:8084/mqtt`
and Device ID `bin01` are already the defaults. Cards appear as soon as the ESP32 publishes.
**↻ Ping device** forces an immediate update.

## 📡 MQTT topics

| Topic | Direction | Payload |
|---|---|---|
| `dustbin/<id>/state` | ESP32 → dashboard (retained) | `{"wet":78,"dry":45,"metal":12}` (fill %, `-1` = sensor error) |
| `dustbin/<id>/status` | ESP32 → dashboard (retained, LWT) | `online` / `offline` |
| `dustbin/<id>/cmd` | dashboard → ESP32 | `ping` → immediate re-publish |

## 🔔 How notifications work
- Browser **push notifications** + in-app toast + **alert sound** + alert history log.
- They arrive while the dashboard tab is open (like the Blynk app running).
  For a hosted https:// URL you can also "Add to Home Screen" on Android for app-like behavior.
- Want an offline channel too? Add Telegram alerts easily — ask and it can be added.

## 🔧 Tuning
- **Threshold / re-arm drop / sound / broker / device ID** → dashboard ⚙ Settings (saved in your browser).
- **Publish rate, sensor gap, more bins, buzzer pin** → `USER CONFIG` block in the sketch.
- Fill % = `(height − distance) / (height − 4cm)`. If a bin shows wrong %, adjust its `heightCm`.

## 🧪 Test without hardware
Open the dashboard → **▶ Demo** (or open `index.html?demo=1`): bins slowly fill, hit 80%,
and fire a real notification — perfect for checking everything before the demo.
