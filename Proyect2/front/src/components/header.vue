<script setup>
import { computed } from "vue"
import Button from "primevue/button"
import { Moon, Sun } from "lucide-vue-next"
import { useLayout } from "../composables/useLayout"

const props = defineProps({
  title: {
    type: String,
    default: "Telemetry Dashboard",
  },
  status: {
    type: String,
    default: "Disconnected",
  },
})

const { isDarkMode, toggleDarkMode } = useLayout()
const isLive = computed(() => props.status === "Live")
</script>

<template>
  <nav class="navbar">
    <div class="nav-container">
      <div class="nav-left"></div>

      <div class="nav-center">
        <h1 class="page-title">{{ title }}</h1>
      </div>

      <div class="nav-right">
        <span
          class="status-pill"
          :class="isLive ? 'status-live' : 'status-offline'"
        >
          {{ status }}
        </span>
        <div class="auth-theme-switch">
          <Button
            text
            @click="toggleDarkMode"
            aria-label="Toggle theme"
          >
            <Sun v-if="isDarkMode" :size="18" />
            <Moon v-else :size="18" />
          </Button>
        </div>
      </div>
    </div>
  </nav>
</template>

<style scoped>
.navbar {
  background: var(--p-surface-card);
  border-bottom: 1px solid var(--p-surface-border);
  position: sticky;
  top: 0;
  z-index: 1000;
}

.nav-container {
  max-width: 1200px;
  margin: 0 auto;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0.8rem 1rem;
  position: relative;
}

.nav-left {
  width: 2.5rem;
}

.nav-center {
  position: absolute;
  left: 50%;
  transform: translateX(-50%);
}

.page-title {
  margin: 0;
  font-size: 1.25rem;
  font-weight: 600;
  color: var(--p-text-color);
  text-align: center;
}

.nav-right {
  display: flex;
  align-items: center;
  gap: 0.5rem;
}

.status-pill {
  border-radius: 9999px;
  border: 1px solid;
  padding: 0.2rem 0.65rem;
  font-size: 0.75rem;
  font-weight: 600;
  letter-spacing: 0.02em;
}

.status-live {
  border-color: var(--p-green-200);
  background: var(--p-green-50);
  color: var(--p-green-700);
}

.status-offline {
  border-color: var(--p-surface-300);
  background: var(--p-surface-100);
  color: var(--p-text-muted-color);
}

@media (max-width: 768px) {
  .nav-container {
    padding: 0.75rem;
  }

  .page-title {
    font-size: 1.05rem;
  }
}
</style>
