ENEE452 Final Project
Tyler Wong

## Phase 1

1. Flash the firmware with `MYDACQ_PHASE2` commented out in `freertos.c`
2. Open PuTTY: **Serial, 115200 baud, 8N1, no flow control**, correct COM port
3. Press reset button
4. Sensor readings appear every second

**Commands** (type in PuTTY, press Enter):

| Key | Action |
|-----|--------|
| `s` | Toggle sampling on/off |
| `r` | Report current config |
| `+` | Slower (double interval) |
| `-` | Faster (halve interval) |

---

## Phase 2

### One-time Windows setup (PowerShell Administrator)
```powershell
netsh interface portproxy add v4tov4 listenport=1883 listenaddress=192.168.137.1 connectport=1883 connectaddress=127.0.0.1
netsh advfirewall firewall add rule name="MQTT 1883" dir=in action=allow protocol=TCP localport=1883
```

### Each session (via WSL)
**Terminal 1 — broker:**
```bash
./mqtt_broker 1883
```

**Terminal 2 — display:**
```bash
./mydacq_display 127.0.0.1 1883
```

### Board
1. Enable hotspot on PC at 2.4 GHz
2. Flash firmware with `#define MYDACQ_PHASE2` uncommented in `freertos.c`
3. Start the display client first, then press reset button
4. Wait ~15 seconds for WiFi to connect
5. Readings appear in the display every 3 seconds:
```
[14:23:01] M24SR addr=0x0000  val=0x6A (106)  pack(12 bytes): 82A46164647200A376616C6A
```
