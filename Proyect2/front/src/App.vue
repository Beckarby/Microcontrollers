<script setup>
import { computed, onBeforeUnmount, onMounted, ref } from "vue"
import Card from "primevue/card"
import AppHeader from "./components/header.vue"

const emptyValue = "--"

const initialTelemetry = {
  temperature: null,
  humidity: null,
  pot: null,
  servoAngle: null,
  mode: null,
  timestamp: Date.now(),
}

const cardDefinitions = [
  { key: "temperature", label: "Temperature", suffix: "°C" },
  { key: "humidity", label: "Humidity", suffix: "%" },
  { key: "pot", label: "Pot", suffix: "" },
  { key: "servoAngle", label: "Servo Angle", suffix: "°" },
  { key: "mode", label: "Mode", suffix: "" },
  {
    key: "timestamp",
    label: "Timestamp",
    suffix: "",
    formatter: (value) => (value ? new Date(value).toLocaleString() : null),
  },
]

const telemetry = ref({ ...initialTelemetry })
const rotatingMessages = [
  `:::'###::::'##::: ##::'######:::::'###::::'##::::'##:'##::: ##::::'########:'##::::'##:'########:::::'######::::'#######:::::'###::::'########:
::'## ##::: ###:: ##:'##... ##:::'## ##::: ##:::: ##: ###:: ##::::... ##..:: ##:::: ##: ##.....:::::'##... ##::'##.... ##:::'## ##:::... ##..::
:'##:. ##:: ####: ##: ##:::..:::'##:. ##:: ##:::: ##: ####: ##::::::: ##:::: ##:::: ##: ##:::::::::: ##:::..::: ##:::: ##::'##:. ##::::: ##::::
'##:::. ##: ## ## ##:. ######::'##:::. ##: ##:::: ##: ## ## ##::::::: ##:::: #########: ######:::::: ##::'####: ##:::: ##:'##:::. ##:::: ##::::
 #########: ##. ####::..... ##: #########: ##:::: ##: ##. ####::::::: ##:::: ##.... ##: ##...::::::: ##::: ##:: ##:::: ##: #########:::: ##::::
 ##.... ##: ##:. ###:'##::: ##: ##.... ##: ##:::: ##: ##:. ###::::::: ##:::: ##:::: ##: ##:::::::::: ##::: ##:: ##:::: ##: ##.... ##:::: ##::::
 ##:::: ##: ##::. ##:. ######:: ##:::: ##:. #######:: ##::. ##::::::: ##:::: ##:::: ##: ########::::. ######:::. #######:: ##:::: ##:::: ##::::
..:::::..::..::::..:::......:::..:::::..:::.......:::..::::..::::::::..:::::..:::::..::........::::::......:::::.......:::..:::::..:::::..:::::`,
  `██╗   ██╗███╗   ██╗         ██╗ ██████╗      ██╗ ██████╗ ███████╗
██║   ██║████╗  ██║         ██║██╔═══██╗     ██║██╔═══██╗██╔════╝
██║   ██║██╔██╗ ██║         ██║██║   ██║     ██║██║   ██║███████╗
██║   ██║██║╚██╗██║    ██   ██║██║   ██║██   ██║██║   ██║╚════██║
╚██████╔╝██║ ╚████║    ╚█████╔╝╚██████╔╝╚█████╔╝╚██████╔╝███████║
 ╚═════╝ ╚═╝  ╚═══╝     ╚════╝  ╚═════╝  ╚════╝  ╚═════╝ ╚══════╝
                                                                 `,
  `Tip: keep this tab open to monitor live values\nThe dashboard reconnects automatically if connection drops.`,
]
const currentMessageIndex = ref(0)

const loadError = ref("")
const connectionState = ref("Disconnected")

let websocket = null
let reconnectTimeout = null
let messageRotateInterval = null

const reconnectDelayMs = 2000

function socketUrl() {
  const protocol = window.location.protocol === "https:" ? "wss" : "ws"
  return `${protocol}://${window.location.host}/ws`
}

function clearReconnectTimeout() {
  if (reconnectTimeout) {
    clearTimeout(reconnectTimeout)
    reconnectTimeout = null
  }
}

function scheduleReconnect() {
  clearReconnectTimeout()
  reconnectTimeout = setTimeout(() => {
    connectWebSocket()
  }, reconnectDelayMs)
}

function requestInitialValues() {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    websocket.send("GetValues")
  }
}

function toFixedOrNull(value) {
  if (value === null || value === undefined) {
    return null
  }

  const numericValue = Number(value)
  return Number.isFinite(numericValue) ? numericValue.toFixed(1) : null
}

function updateTelemetry(message) {
  const nextTelemetry = { ...telemetry.value }

  const temperatureValue = message.temperature
  if (temperatureValue !== undefined) {
    nextTelemetry.temperature = toFixedOrNull(temperatureValue)
  }
  const humidityValue = message.humidity
  if (humidityValue !== undefined) {
    nextTelemetry.humidity = toFixedOrNull(humidityValue)
  }

  const potValue = message.pot ?? message.pot_target_angle
  if (potValue !== undefined) {
    nextTelemetry.pot = potValue
  }

  const servoAngleValue = message.servoAngle ?? message.servo_angle
  if (servoAngleValue !== undefined) {
    nextTelemetry.servoAngle = servoAngleValue
  }

  const modeValue = message.mode ?? message.auto_mode
  if (modeValue !== undefined) {
    nextTelemetry.mode = typeof modeValue === "boolean" ? (modeValue ? "Auto" : "Manual") : modeValue
  }

  nextTelemetry.timestamp = message.timestamp ?? Date.now()
  telemetry.value = nextTelemetry
  console.log("Updated telemetry:", nextTelemetry)
}

function connectWebSocket() {
  if (websocket && (websocket.readyState === WebSocket.OPEN || websocket.readyState === WebSocket.CONNECTING)) {
    return
  }

  connectionState.value = "Connecting"
  loadError.value = ""

  websocket = new WebSocket(socketUrl())

  websocket.onopen = () => {
    connectionState.value = "Live"
    loadError.value = ""
    requestInitialValues()
  }

  websocket.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data)
      updateTelemetry(data)
    } catch {
      loadError.value = "Received a non-JSON WebSocket message."
    }
  }

  websocket.onerror = () => {
    loadError.value = "WebSocket connection error."
  }

  websocket.onclose = () => {
    connectionState.value = "Disconnected"
    scheduleReconnect()
  }
}

function closeWebSocket() {
  clearReconnectTimeout()

  if (websocket) {
    websocket.onopen = null
    websocket.onmessage = null
    websocket.onerror = null
    websocket.onclose = null
    websocket.close()
    websocket = null
  }

  connectionState.value = "Disconnected"
}

function startMessageRotation() {
  stopMessageRotation()

  messageRotateInterval = setInterval(() => {
    currentMessageIndex.value = (currentMessageIndex.value + 1) % rotatingMessages.length
  }, 6000)
}

function stopMessageRotation() {
  if (messageRotateInterval) {
    clearInterval(messageRotateInterval)
    messageRotateInterval = null
  }
}

function displayValue(value) {
  if (value === null || value === undefined || value === "") {
    return emptyValue
  }
  return value
}


const cards = computed(() => {
  return cardDefinitions.map((card) => {
    const rawValue = telemetry.value[card.key]
    const value = card.formatter ? card.formatter(rawValue) : rawValue

    return {
      ...card,
      displayVal: displayValue(value), // Expose the raw formatted value
    }
  })
})

onMounted(() => {
  connectWebSocket()
  startMessageRotation()
})

onBeforeUnmount(() => {
  closeWebSocket()
  stopMessageRotation()
})
</script>

<template>
  <div class="min-h-screen">
    <AppHeader title="Telemetry Dashboard" :status="connectionState" />

    <main class="p-6">
      <p v-if="loadError" class="mb-6 rounded-md border border-red-200 bg-red-50 p-3 text-sm text-red-700">
        {{ loadError }}
      </p>

      <section class="grid grid-cols-1 gap-5 sm:grid-cols-2 lg:grid-cols-3">
        <Card 
          v-for="card in cards" 
          :key="card.key"
          class="border border-slate-200 dark:border-slate-700 shadow-sm transition-all duration-200 hover:border-slate-300 dark:hover:border-slate-600 hover:shadow-md"
        >
          <template #title>
            <div class="text-xs font-semibold uppercase tracking-wider text-slate-500 dark:text-slate-400">
              {{ card.label }}
            </div>
          </template>
          <template #content>
            <div class="mt-1 flex items-baseline gap-1.5">
              <span class="text-3xl font-bold tracking-tight text-slate-800 dark:text-slate-100">
                {{ card.displayVal }}
              </span>
              <span 
                v-if="card.displayVal !== emptyValue && card.suffix" 
                class="text-sm font-medium text-slate-400 dark:text-slate-500"
              >
                {{ card.suffix }}
              </span>
            </div>
          </template>
        </Card>
      </section>

      <pre class="mx-auto w-fit mt-12 max-w-full overflow-auto rounded-lg bg-slate-900 p-6 text-[10px] sm:text-xs leading-4 text-slate-300 shadow-inner">{{ rotatingMessages[currentMessageIndex] }}</pre>
</main>
  </div>
</template>
