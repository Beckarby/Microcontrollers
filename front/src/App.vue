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

const loadError = ref("")


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
  // Connect to WebSocket 
})

onBeforeUnmount(() => {
  // Clean up WebSocket connection
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

      <pre class="mx-auto w-fit mt-12 max-w-full overflow-auto rounded-lg bg-slate-900 p-6 text-[10px] sm:text-xs leading-4 text-slate-300 shadow-inner">
:::'###::::'##::: ##::'######:::::'###::::'##::::'##:'##::: ##::::'########:'##::::'##:'########:::::'######::::'#######:::::'###::::'########:
::'## ##::: ###:: ##:'##... ##:::'## ##::: ##:::: ##: ###:: ##::::... ##..:: ##:::: ##: ##.....:::::'##... ##::'##.... ##:::'## ##:::... ##..::
:'##:. ##:: ####: ##: ##:::..:::'##:. ##:: ##:::: ##: ####: ##::::::: ##:::: ##:::: ##: ##:::::::::: ##:::..::: ##:::: ##::'##:. ##::::: ##::::
'##:::. ##: ## ## ##:. ######::'##:::. ##: ##:::: ##: ## ## ##::::::: ##:::: #########: ######:::::: ##::'####: ##:::: ##:'##:::. ##:::: ##::::
 #########: ##. ####::..... ##: #########: ##:::: ##: ##. ####::::::: ##:::: ##.... ##: ##...::::::: ##::: ##:: ##:::: ##: #########:::: ##::::
 ##.... ##: ##:. ###:'##::: ##: ##.... ##: ##:::: ##: ##:. ###::::::: ##:::: ##:::: ##: ##:::::::::: ##::: ##:: ##:::: ##: ##.... ##:::: ##::::
 ##:::: ##: ##::. ##:. ######:: ##:::: ##:. #######:: ##::. ##::::::: ##:::: ##:::: ##: ########::::. ######:::. #######:: ##:::: ##:::: ##::::
..:::::..::..::::..:::......:::..:::::..:::.......:::..::::..::::::::..:::::..:::::..::........::::::......:::::.......:::..:::::..:::::..:::::</pre>
</main>
  </div>
</template>
