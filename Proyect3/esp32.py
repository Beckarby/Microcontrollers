import network
import ujson
import _thread
from dht import DHT11
from machine import Pin, PWM
from time import sleep, ticks_diff, ticks_ms
from umqtt.simple import MQTTClient

# -------------------------
# Network and MQTT settings
# -------------------------
WIFI_SSID = "CharlieKirk777"
WIFI_PASSWORD = "agartha67"

MQTT_CLIENT_ID = "esp32-scada-client"
MQTT_BROKER_HOST = "monorail.proxy.rlwy.net"
MQTT_BROKER_PORT = 47484
MQTT_KEEPALIVE = 30

TOPIC_SENSORS_ESP32 = b"sensors/esp32"
TOPIC_SENSORS_RASPERRY = b"sensors/rasperry"
TOPIC_SENSORS_RASPBERRY = b"sensors/raspberry"
TOPIC_ESP32_PLAY = b"esp32/play"
TOPIC_COMMANDS_ESP32_PLAY = b"commands/esp32/play"
TOPIC_PRESENCE_ESP32 = b"presence/esp32/status"
TOPIC_PRESENCE_ESP32_SLASH = b"/presence/esp32/status"
TOPIC_PRESENCE_RASPBERRY = b"presence/raspberry/status"
TOPIC_PRESENCE_RASPBERRY_SLASH = b"/presence/raspberry/status"

PRESENCE_ONLINE = b"online"
PRESENCE_OFFLINE = b"offline"

# Hardware
BUZZER_PIN = 12
DHT_PIN = 4
TEMP_THLD = 29

# Periodic publish interval for DHT11 telemetry
SENSOR_PUBLISH_INTERVAL_MS = 2600

dht_sensor = DHT11(Pin(DHT_PIN))
buzzer = PWM(Pin(BUZZER_PIN))
buzzer.duty(0)

# Notes map (Hz)
notes = {
    "F#2": 93, "A2": 110, "C#3": 139, "D3": 147, "E3": 165, "F#3": 185,
    "A3": 220, "B3": 247, "C#4": 277, "D4": 294, "E4": 330, "F#4": 370,
    "G#4": 415, "A4": 440, "B4": 494, "C#5": 554, "D5": 587, "E5": 659,
    "REST": 0,
}

# Sequencer (1 Tick = 1 Eighth Note)
melody2_bass = [
    "D3", "A3", "D3", "A3", "D3", "A3", "D3", "A3",
    "E3", "A3", "E3", "A3", "E3", "A3", "E3", "A3",
    "F#3", "A3", "F#3", "A3", "F#3", "A3", "F#3", "A3",
    "E3", "A3", "E3", "A3", "E3", "A3", "E3", "A3",
]

# Timing
tick_duration = 0.326
half_tick = tick_duration / 2
articulation_gap = 0.02

# Runtime state
note_index = 0
current_freq2 = 0

# Manual override state from esp32/play topics:
command_play_state = None

# Latest fallback value from sensors/raspberry
raspberry_play_state = False
raspberry_sensor_data = None


def connect_wifi():
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)

    if wlan.isconnected():
        print("WiFi already connected:", wlan.ifconfig())
        return wlan

    print("Connecting to WiFi...")
    wlan.connect(WIFI_SSID, WIFI_PASSWORD)

    timeout_ms = 15000
    start_ms = ticks_ms()
    while not wlan.isconnected():
        if ticks_diff(ticks_ms(), start_ms) > timeout_ms:
            raise RuntimeError("WiFi connection timeout")
        sleep(0.2)

    print("WiFi connected:", wlan.ifconfig())
    return wlan


def parse_play_value(raw_payload):
    payload = raw_payload.strip()
    if payload == "":
        return None

    lowered = payload.lower()
    if lowered in ("true", "1", "on", "play"):
        return True
    if lowered in ("false", "0", "off", "stop"):
        return False

    try:
        numeric = float(payload)
        return numeric > 0
    except:
        pass

    try:
        data = ujson.loads(payload)
        if isinstance(data, dict):
            if "play" in data:
                value = str(data["play"]).lower()
                if value in ("true", "1", "on", "play"):
                    return True
                if value in ("false", "0", "off", "stop"):
                    return False
            if "temperature" in data:
                return float(data["temperature"]) >= TEMP_THLD
            if "temp" in data:
                return float(data["temp"]) >= TEMP_THLD
        elif isinstance(data, (int, float)):
            return data > 0
    except:
        pass

    return False


def parse_sensor_payload(raw_payload):
    payload = raw_payload.strip()
    if payload == "":
        return None

    try:
        data = ujson.loads(payload)
        if isinstance(data, dict):
            return data
    except:
        pass

    return None


def sensor_data_to_play_state(sensor_data):
    if not isinstance(sensor_data, dict):
        return False

    if "over_threshold" in sensor_data:
        value = sensor_data["over_threshold"]
        if isinstance(value, bool):
            return value
        value_str = str(value).strip().lower()
        if value_str in ("true", "1", "on", "yes"):
            return True
        if value_str in ("false", "0", "off", "no"):
            return False

    threshold = TEMP_THLD
    if "threshold" in sensor_data:
        try:
            threshold = float(sensor_data["threshold"])
        except:
            threshold = TEMP_THLD

    if "temperature" in sensor_data:
        try:
            return float(sensor_data["temperature"]) >= threshold
        except:
            pass

    if "temp" in sensor_data:
        try:
            return float(sensor_data["temp"]) >= threshold
        except:
            pass

    return False


def on_mqtt_message(topic, payload):
    global command_play_state, raspberry_play_state, raspberry_sensor_data
    global note_index, current_freq2

    try:
        topic_str = topic.decode()
    except:
        topic_str = str(topic)

    try:
        payload_str = payload.decode().strip()
    except:
        payload_str = str(payload).strip()

    print("MQTT <- topic={}, payload={}".format(topic_str, payload_str))

    if topic in (TOPIC_ESP32_PLAY, TOPIC_COMMANDS_ESP32_PLAY):
        command_play_state = parse_play_value(payload_str)
    elif topic in (TOPIC_SENSORS_RASPERRY, TOPIC_SENSORS_RASPBERRY):
        raspberry_sensor_data = parse_sensor_payload(payload_str)
        raspberry_play_state = sensor_data_to_play_state(raspberry_sensor_data)
    elif topic in (TOPIC_PRESENCE_RASPBERRY, TOPIC_PRESENCE_RASPBERRY_SLASH):
        lowered = payload_str.lower()
        if lowered == "online":
            note_index = 0
            current_freq2 = 0
            print("Presence raspberry=online -> melody reset for sync")


def connect_mqtt():
    client = MQTTClient(
        client_id=MQTT_CLIENT_ID,
        server=MQTT_BROKER_HOST,
        port=MQTT_BROKER_PORT,
        keepalive=MQTT_KEEPALIVE,
    )
    client.set_last_will(TOPIC_PRESENCE_ESP32, PRESENCE_OFFLINE, retain=True)
    client.set_callback(on_mqtt_message)
    client.connect()

    client.subscribe(TOPIC_ESP32_PLAY)
    client.subscribe(TOPIC_COMMANDS_ESP32_PLAY)
    client.subscribe(TOPIC_SENSORS_RASPERRY)
    client.subscribe(TOPIC_SENSORS_RASPBERRY)
    client.subscribe(TOPIC_PRESENCE_RASPBERRY)
    client.subscribe(TOPIC_PRESENCE_RASPBERRY_SLASH)

    client.publish(TOPIC_PRESENCE_ESP32, PRESENCE_ONLINE, retain=True)
    client.publish(TOPIC_PRESENCE_ESP32_SLASH, PRESENCE_ONLINE, retain=True)

    print("MQTT connected to {}:{}".format(MQTT_BROKER_HOST, MQTT_BROKER_PORT))
    return client


def get_buzzer_state():
    if command_play_state is None:
        return raspberry_play_state
    return command_play_state


def publish_dht_state(client):
    try:
        dht_sensor.measure()
        temp = dht_sensor.temperature()
        hum = dht_sensor.humidity()

        payload = {
            "temperature": temp,
            "humidity": hum,
            "threshold": TEMP_THLD,
            "over_threshold": temp >= TEMP_THLD,
            "unit": "C",
        }
        raw = ujson.dumps(payload)
        client.publish(TOPIC_SENSORS_ESP32, raw)
        print("MQTT -> sensors/esp32 {}".format(raw))
    except Exception as e:
        print("DHT read/publish error:", e)


def reconnect_mqtt_loop():
    while True:
        try:
            return connect_mqtt()
        except Exception as e:
            print("MQTT reconnect failed:", e)
            sleep(2)


# ==========================================
# THREAD 2: Dedicated Audio Sequencer Loop
# ==========================================
def melody_task():
    global note_index, current_freq2
    print("Melody thread started.")
    
    while True:
        try:
            should_play_now = get_buzzer_state()

            note2 = melody2_bass[note_index]
            if note2 != "-":
                current_freq2 = notes.get(note2, 0)

            is_new_2 = note2 not in ("-", "REST")

            if is_new_2:
                buzzer.duty(0)
            sleep(articulation_gap)

            if current_freq2 > 0 and should_play_now:
                buzzer.freq(current_freq2)
                buzzer.duty(512)
            else:
                buzzer.duty(0)

            sleep(half_tick - articulation_gap)
            buzzer.duty(0)
            sleep(half_tick)

            note_index = (note_index + 1) % len(melody2_bass)
            
        except Exception as e:
            print("Melody thread error:", e)
            sleep(0.5)


# ==========================================
# THREAD 1 (MAIN): Network & Sensor Loop
# ==========================================
def main():
    connect_wifi()
    client = reconnect_mqtt_loop()
    next_sensor_publish_ms = ticks_ms()

    # Launch the sequencer in a separate thread
    _thread.start_new_thread(melody_task, ())

    print("Starting ESP32 MQTT loop...")

    while True:
        try:
            # Check for new MQTT messages without blocking indefinitely
            client.check_msg()

            now = ticks_ms()
            if ticks_diff(now, next_sensor_publish_ms) >= 0:
                publish_dht_state(client)
                next_sensor_publish_ms = ticks_ms() + SENSOR_PUBLISH_INTERVAL_MS

            # Yield briefly to ensure the melody thread gets scheduled evenly
            sleep(0.05)

        except OSError as e:
            print("MQTT OSError, reconnecting:", e)
            buzzer.duty(0)
            try:
                client.disconnect()
            except:
                pass
            client = reconnect_mqtt_loop()
        except Exception as e:
            print("Loop error:", e)
            sleep(0.5)


try:
    main()
finally:
    try:
        offline_client = MQTTClient(
            client_id=MQTT_CLIENT_ID + "-offline",
            server=MQTT_BROKER_HOST,
            port=MQTT_BROKER_PORT,
            keepalive=MQTT_KEEPALIVE,
        )
        offline_client.connect()
        offline_client.publish(TOPIC_PRESENCE_ESP32, PRESENCE_OFFLINE, retain=True)
        offline_client.publish(TOPIC_PRESENCE_ESP32_SLASH, PRESENCE_OFFLINE, retain=True)
        offline_client.disconnect()
    except:
        pass
    buzzer.duty(0)
    buzzer.deinit()