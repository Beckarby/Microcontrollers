import { ref } from "vue"

const storageKey = "layout-dark-mode"
const isDarkMode = ref(false)

function applyDarkClass(enabled) {
  if (typeof document === "undefined") {
    return
  }

  document.documentElement.classList.toggle("dark-mode", enabled)
}

function loadInitialMode() {
  if (typeof window === "undefined") {
    return false
  }

  const savedMode = window.localStorage.getItem(storageKey)
  return savedMode === "true"
}

isDarkMode.value = loadInitialMode()
applyDarkClass(isDarkMode.value)

function toggleDarkMode() {
  isDarkMode.value = !isDarkMode.value
  applyDarkClass(isDarkMode.value)

  if (typeof window !== "undefined") {
    window.localStorage.setItem(storageKey, String(isDarkMode.value))
  }
}

export function useLayout() {
  return {
    isDarkMode,
    toggleDarkMode,
  }
}
