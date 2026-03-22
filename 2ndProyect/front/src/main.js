import { createApp } from 'vue'
import App from './App.vue'
import Primevue from 'primevue/config'
import Aura from "@primevue/themes/aura"
import "./main.css"

const app = createApp(App)

app.use(Primevue, {
  theme: {
    preset: Aura,
    options: {
      darkModeSelector: '.dark-mode'
    }
  },
})

app.mount('#app')
