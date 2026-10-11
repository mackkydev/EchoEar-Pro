# EchoEar Pro Hardware Validation Checklist

This checklist covers only behavior that cannot be proven by simulator or firmware builds.
Record the board revision, firmware hash, date, and tester before starting. A build pass is
not a substitute for any result below.

## Test record

- Board / revision:
- Display / touch controller:
- Audio codec / amplifier:
- Firmware build:
- Tester / date:
- Power source:

Use `PASS`, `FAIL`, or `BLOCKED` for every item and attach the serial log for failures.

## 1. Boot and persistent settings

- [ ] Cold boot reaches the main UI without reset loops.
- [ ] Serial output contains `EchoEar settings ready` once during initialization.
- [ ] Change brightness, volume, animation speed, and Car Mode; press Save.
- [ ] Reboot from software and verify all four values are restored.
- [ ] Remove power for 30 seconds, boot again, and verify all four values are restored.
- [ ] Inject or preload out-of-range NVS values and verify the `EchoEarSettings` repair log.
- [ ] Reboot once more and verify the repair log does not repeat for the repaired record.

Pass criteria: values survive both reboot types, remain within supported ranges, and no NVS
or watchdog error appears.

## 2. Display, touch, and Settings UI

- [ ] Gear button opens Settings from idle, listening, speaking, and Car Mode screens.
- [ ] Every page and Back/Cancel/Save control is reachable without clipped touch targets.
- [ ] Brightness changes are visible and 0% does not make recovery impossible.
- [ ] Rapid slider movement for 30 seconds does not freeze LVGL or trigger a watchdog.
- [ ] Cancel restores the active values; Save applies and persists the draft values.
- [ ] Factory Reset requires the explicit second confirmation.

Pass criteria: no missed navigation path, stuck overlay, display corruption, or accidental reset.

## 3. Wi-Fi provisioning and reconnect

- [ ] First boot without credentials enters provisioning.
- [ ] Valid credentials connect and leave provisioning.
- [ ] Wrong password reports a recoverable error and allows retry.
- [ ] Power-cycle the access point during idle, listening, and speaking.
- [ ] DHCP loss, weak signal, and router restart recover without entering provisioning.
- [ ] Manual Wi-Fi setup remains available from Settings.
- [ ] Run at least 100 physical disconnect/reconnect cycles.

Pass criteria: automatic reconnect restores service, active voice state is cleaned up, and saved
credentials are not erased by ordinary runtime failures.

## 4. Audio output and playback lifecycle

- [ ] Play short, medium, and long TTS responses at 24 kHz Opus.
- [ ] First and final packets are audible without clipping or truncation.
- [ ] `playback_drained` arrives only after the speaker queue is physically empty.
- [ ] Run 200 sequential TTS responses without deadlock or stale audio.
- [ ] Verify volume 0, 1, 50, 99, and 100 and reboot at each boundary as practical.
- [ ] Confirm no decoder residue is audible after an interrupted response.

Pass criteria: speech completes exactly once, idle is reached after drain, and audio remains clean.

## 5. Microphone, interruption, and barge-in

- [ ] Wake word starts listening from idle.
- [ ] Interrupt TTS near its beginning, middle, and final frame.
- [ ] Speak immediately after interruption and verify the new turn is captured.
- [ ] Repeat interruption 100 times with varied timing.
- [ ] Disconnect Wi-Fi during barge-in and verify input/output are both released.

Pass criteria: old playback stops promptly, the decoder resets, and the next turn never inherits
the prior request or audio.

## 6. Factory reset and recovery

- [ ] Cancel at the confirmation screen and verify nothing is erased.
- [ ] Confirm reset and verify credentials plus EchoEar device settings are erased.
- [ ] Reboot completes and returns to provisioning with safe default UI values.
- [ ] Interrupt power once during reset, then confirm the next boot remains recoverable.

Pass criteria: reset is deliberate, complete, and power-loss tolerant; no boot loop occurs.

## 7. Car Mode / Zeekr scenarios

- [ ] Car Mode persists after reboot and power cycle.
- [ ] Voice listening/thinking/speaking temporarily overrides normal vehicle scenes.
- [ ] Critical low battery and door-open warnings override active voice visuals.
- [ ] Parked, charging, driving, stale-cloud, and offline-cloud states render correctly.
- [ ] Loss of vehicle data never blocks voice, Settings, or Wi-Fi recovery.

Pass criteria: critical priorities match policy and Car Mode remains a presentation layer rather
than a dependency for core voice operation.

## 8. Soak and acceptance

- [ ] Run powered for 8 hours with periodic TTS, listening, Settings saves, and reconnects.
- [ ] Capture `EchoEarHealth` once per minute and inspect every degraded/error transition.
- [ ] Verify free heap does not trend downward across the soak run.
- [ ] Confirm no watchdog reset, stack overflow, queue deadlock, or repeated NVS repair.

Final acceptance requires all sections to pass on the intended production board revision.

## Validation log

### 2026-08-21 — ESP-VoCat EchoEar UI first hardware image

- Board: ESP32-S3 QFN56 revision 0.2, 8 MB PSRAM, 16 MB flash
- Port / device MAC: COM9 / `30:ed:a0:1e:86:bc`
- Variant: `esp-vocat-echoear`, wake word `Hi Robo`
- App SHA-256: `4E8A3829AD20C0624F65E2F1A158B52682D253B3A9A7625757FFE23E07DC05CF`
- Assets SHA-256: `B93A6E89F09B7C2A2F59FAB87712091CFE211031975EAFCF93612D4D4ED2A52F`
- Build: PASS (`0x2f6520` app bytes, 25% app partition free)
- Flash app + assets: PASS; both write hashes verified, NVS left untouched
- Boot stability observation: PASS for 60 seconds; no reset loop, free SRAM approximately 69 KB,
  observed minimum approximately 62 KB
- Visual face rendering and touch navigation: pending tester confirmation
- Rollback: full 16 MB device backup and Emote fallback app retained

### 2026-08-26 — EchoEar Pro v0.1.0 XiaoZhi baseline

- Board: ESP32-S3 QFN56 revision 0.2, 8 MB PSRAM, 16 MB flash
- Port / device MAC: COM9 / `30:ed:a0:1e:86:bc`
- Product release: `EchoEar Pro v0.1.0 — XiaoZhi Baseline`
- Embedded XiaoZhi version: `2.4.2`
- App SHA-256: `2CB323CBF9D9E78D97F38BF9FF851280A84EDC9EF675CE80B1BD7D15CA82D093`
- Assets SHA-256: `C4E57A9367B1A57A5FF345EB225662290401301D392CDA4154F8A1F960F353A9`
- Full-device backup SHA-256: `3A43CE2822E157DA40CCB34DFB03DAE6354495710A29604C4BC1858C6D0131DD`
- Release archive SHA-256: `1A24ABA5B1CF8A7594CA978A9B3C154A07A1FE438EC734A3E5DDF503C856CA93`
- Build: PASS (`0x302300` app bytes, 24% app partition free)
- Hardware: PASS for boot, Wi-Fi, `Hi Robo`, Hands-free, TTS playback, subtitles,
  battery display, Settings navigation, persistence observed across app-only flashes, and
  Settings responsiveness after pausing hidden face animation
- Remaining before production-final: persistence power-cycle matrix, reconnect/factory-reset
  recovery, audio stress, Car Mode/Zeekr scenarios, and 8-hour soak
- Rollback: new private full 16 MB snapshot captured before Gemini integration; public release
  package contains only non-private flash partitions

### 2026-08-26 — Gemini Gateway software foundation

- Provider: Gemini Live, model default `gemini-3.1-flash-live-preview`
- Credential boundary: Gemini API key remains on the gateway and is never stored in firmware
- Audio contract: PCM16 mono, 16 kHz device input and 24 kHz Gemini output
- UI events: input/output transcripts, turn complete, and interruption are mapped to protocol v1
- Automated validation: PASS, 15 offline tests including a full simulated device/provider round trip
- SDK validation: PASS with `google-genai 1.75.0` and `websockets 16.1.1`
- Local server startup: PASS on `ws://0.0.0.0:8765`
- Pending: live Gemini cloud smoke test requires a locally configured API key; ESP32 transport is
  intentionally unchanged until cloud audio and Thai transcript quality pass

### 2026-08-27 — Gemini Live cloud smoke test

- Google Cloud project: `EchoEar Pro Gemini Gateway`
- Gemini API: enabled; API key stored only in ignored local `gateway/.env`
- Live connection and Thai output transcript: PASS
- Native audio response: PASS, PCM16 mono at 24 kHz saved as `gemini-thai.wav`
- Automated validation after live-path fixes: PASS, 17 offline tests
- Firmware remains unchanged; next step is the ESP32-to-Gateway transport adapter and hardware
  audio playback test
