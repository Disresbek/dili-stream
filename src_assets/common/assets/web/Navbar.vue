<template>
  <div>
    <nav class="dili-sidebar" aria-label="Main">
      <RouterLink class="dili-brand" to="/" title="Dili">
        <DiliLogo></DiliLogo>
      </RouterLink>

      <div class="dili-nav">
        <RouterLink class="dili-nav-item" to="/" exact-active-class="active" active-class="">
          <Home :size="20"></Home><span>{{ $t('navbar.home') }}</span>
        </RouterLink>
        <RouterLink class="dili-nav-item" to="/apps" active-class="active">
          <Layers :size="20"></Layers><span>{{ $t('navbar.applications') }}</span>
        </RouterLink>
        <RouterLink class="dili-nav-item" to="/displays" active-class="active">
          <Monitor :size="20"></Monitor><span>{{ $t('navbar.displays') }}</span>
        </RouterLink>
        <RouterLink class="dili-nav-item" to="/devices" active-class="active">
          <MonitorSmartphone :size="20"></MonitorSmartphone><span>{{ $t('navbar.devices') }}</span>
        </RouterLink>
        <RouterLink class="dili-nav-item" to="/config" active-class="active">
          <Settings :size="20"></Settings><span>{{ $t('navbar.configuration') }}</span>
        </RouterLink>


      </div>

      <div class="dili-sidebar-bottom">
        <div class="dili-nav">
          <RouterLink class="dili-nav-item" to="/featured" active-class="active">
            <Star :size="20"></Star><span>{{ $t('navbar.featured') }}</span>
          </RouterLink>
        </div>
        <div class="dili-pc">
          <div class="dili-pc-name">{{ hostName || 'This PC' }}</div>
          <div class="dili-pc-state">
            <span class="dili-dot" :class="{ live: streaming }"></span>
            {{ streaming ? 'Streaming' : 'Ready' }}
          </div>
        </div>
        <DiliThemeSwitch></DiliThemeSwitch>

        <!-- System menu: opens a panel next to the sidebar -->
        <button type="button" class="dili-system-btn" :aria-expanded="systemOpen ? 'true' : 'false'" @click.stop="toggleSystem">
          <Power :size="16"></Power>
          <span>System</span>
          <ChevronRight :size="16" class="dili-system-chevron"></ChevronRight>
        </button>
        <div v-if="systemOpen" class="dili-system-panel" role="menu" aria-label="System" @click.stop>
          <button type="button" role="menuitem" class="dili-system-item" :disabled="restarting" @click="restartDili">
            <RotateCw :size="18"></RotateCw>
            <span>{{ restarting ? 'Restarting…' : 'Restart Dili' }}</span>
          </button>
          <button type="button" role="menuitem" class="dili-system-item" @click="quitDili">
            <Power :size="18"></Power>
            <span>Quit Dili</span>
          </button>
          <button type="button" role="menuitem" class="dili-system-item" :disabled="!runningApp" @click="closeApp">
            <CircleX :size="18"></CircleX>
            <span class="dili-nav-two-lines">
              Force Close App
              <small>{{ runningApp || 'No app is running' }}</small>
            </span>
          </button>
          <div class="dili-system-divider"></div>
          <RouterLink role="menuitem" class="dili-system-item" to="/password" @click="systemOpen = false">
            <Shield :size="18"></Shield>
            <span>{{ $t('navbar.password') }}</span>
          </RouterLink>
          <button type="button" role="menuitem" class="dili-system-item" @click="logout">
            <LogOut :size="18"></LogOut>
            <span>{{ $t('navbar.logout') }}</span>
          </button>
        </div>
      </div>
    </nav>
    <Notification></Notification>
  </div>
</template>

<script>
import { ChevronRight, CircleUserRound, CircleX, Home, Power, RotateCw, Info, Layers, LogOut, Monitor, MonitorSmartphone, Settings, Shield, Star } from '@lucide/vue'
import DiliThemeSwitch from './DiliThemeSwitch.vue'
import DiliLogo from './DiliLogo.vue'
import { apiFetch } from './fetch_utils'
import Notification from './Notification.vue'

export default {
  components: {
    DiliLogo,
    DiliThemeSwitch,
    Notification,
    Home,
    Layers,
    Star,
    Settings,
    Shield,
    Info,
    CircleUserRound,
    LogOut,
    Monitor,
    MonitorSmartphone,
    CircleX,
    RotateCw,
    ChevronRight,
    Power
  },
  data() {
    return {
      hostName: '',
      streaming: false,
      runningApp: '',
      restarting: false,
      systemOpen: false,
      timer: null,
    }
  },
  mounted() {
    this.closeSystem = () => { this.systemOpen = false }
    this.onKey = (e) => { if (e.key === 'Escape') this.systemOpen = false }
    document.addEventListener('click', this.closeSystem)
    document.addEventListener('keydown', this.onKey)
    document.body.classList.add('dili-has-sidebar')
    this.refresh()
    this.timer = setInterval(this.refresh, 5000)
  },
  beforeUnmount() {
    document.removeEventListener('click', this.closeSystem)
    document.removeEventListener('keydown', this.onKey)
    document.body.classList.remove('dili-has-sidebar')
    clearInterval(this.timer)
  },
  methods: {
    async refresh() {
      try {
        const s = await fetch('./api/status').then((r) => r.json())
        this.hostName = s.host_name || ''
        this.streaming = (s.sessions || 0) > 0
        this.runningApp = s.app || ''
      } catch (e) {
        // The status is only a nice extra, ignore errors
      }
    },
    // Both actions interrupt a running stream, so ask first
    async closeApp() {
      if (!window.confirm(`Force close ${this.runningApp}? The stream on your device ends.`)) return
      await apiFetch('./api/apps/close', { method: 'POST', headers: { 'Content-Type': 'application/json' } })
      setTimeout(this.refresh, 1500)
    },
    toggleSystem() {
      const open = !this.systemOpen
      // Close other floating panels (like "More themes") first
      document.dispatchEvent(new MouseEvent('click'))
      this.systemOpen = open
    },
    quitDili() {
      if (!window.confirm('Quit Dili? Any running stream ends, and your devices cannot connect until Dili is started again. It starts by itself the next time you log in, if autostart is on.')) return
      apiFetch('./api/quit', { method: 'POST', headers: { 'Content-Type': 'application/json' } })
      this.systemOpen = false
    },
    restartDili() {
      if (!window.confirm('Restart Dili? Any running stream ends, and Dili is back after a few seconds.')) return
      this.restarting = true
      apiFetch('./api/restart', { method: 'POST', headers: { 'Content-Type': 'application/json' } })
      setTimeout(() => window.location.reload(), 8000)
    },
    logout() {
      const cacheBuster = Date.now().toString()
      const logoutPageUrl = new URL('/logout', globalThis.location.href)
      const request = new XMLHttpRequest()
      const finish = () => {
        globalThis.location.replace(logoutPageUrl.toString())
      }

      request.open('GET', '/', true, 'sunshine-logout', cacheBuster)
      request.setRequestHeader('Cache-Control', 'no-store')
      request.onload = finish
      request.onerror = finish
      request.ontimeout = finish
      request.timeout = 5000
      request.send()
    }
  }
}
</script>

<style>
/* Make room for the sidebar on every page that shows it */
body.dili-has-sidebar {
  padding-left: 272px;
}

.dili-sidebar {
  position: fixed;
  top: 0;
  left: 0;
  bottom: 0;
  width: 272px;
  box-sizing: border-box;
  padding: 24px 14px 16px 14px;
  display: flex;
  flex-direction: column;
  gap: 18px;
  background: var(--color-bg-subtle);
  border-right: 1px solid var(--color-border);
  overflow-y: auto;
  z-index: 1030;
}

.dili-brand {
  padding: 0 10px;
  text-decoration: none;
}

.dili-nav {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.dili-nav-label {
  margin: 14px 14px 4px 14px;
  font-size: 12px;
  font-weight: 600;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--color-text-muted);
}

.dili-nav-item {
  display: flex;
  align-items: center;
  gap: 12px;
  min-height: 44px;
  padding: 0 14px;
  border-radius: 10px;
  color: var(--color-text-base);
  text-decoration: none;
  font-size: 15px;
  font-weight: 500;
}

.dili-nav-item:hover {
  background: var(--color-bg-muted);
  color: var(--color-text-base);
}

.dili-nav-item.active,
.dili-nav-item.active:hover {
  background: var(--color-primary);
  color: var(--color-on-primary);
  font-weight: 600;
}

.dili-sidebar-bottom {
  margin-top: auto;
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.dili-pc {
  padding: 12px 14px;
  border-radius: 12px;
  background: var(--color-surface);
  border: 1px solid var(--color-border);
  font-size: 13px;
  line-height: 1.4;
}

.dili-pc-name {
  font-weight: 600;
  color: var(--color-text-base);
}

.dili-pc-state {
  display: flex;
  align-items: center;
  gap: 6px;
  color: var(--color-text-muted);
}

.dili-dot {
  width: 8px;
  height: 8px;
  border-radius: 4px;
  background: var(--color-success);
}

.dili-dot.live {
  background: var(--color-primary);
}

.dili-sidebar-tools {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 6px;
}

.dili-sidebar-tools .nav-link,
.dili-tool {
  display: flex;
  align-items: center;
  gap: 6px;
  min-height: 40px;
  padding: 0 10px;
  border: none;
  border-radius: 10px;
  background: transparent;
  color: var(--color-text-base);
}

.dili-sidebar-tools .nav-link:hover,
.dili-tool:hover {
  background: var(--color-bg-muted);
}

.dili-tool span {
  font-size: 14px;
  font-weight: 500;
}

.dili-menu-hint {
  display: block;
  font-size: 12px;
  color: var(--color-text-muted);
}

button.dili-nav-item {
  width: 100%;
  border: none;
  background: transparent;
  font: inherit;
  text-align: left;
  cursor: pointer;
}

button.dili-nav-item:disabled {
  opacity: 0.55;
  cursor: default;
}

button.dili-nav-item:disabled:hover {
  background: transparent;
}

.dili-nav-two-lines {
  display: flex;
  flex-direction: column;
  line-height: 1.2;
}

.dili-nav-two-lines small {
  font-size: 12px;
  color: var(--color-text-muted);
}

.dili-nav-system .dili-nav-label {
  margin-top: 0;
}

.dili-system-btn {
  display: flex;
  align-items: center;
  gap: 8px;
  height: 36px;
  padding: 0 10px;
  border: none;
  border-radius: 10px;
  background: transparent;
  color: var(--color-text-base);
  font: inherit;
  font-size: 13px;
  font-weight: 600;
  cursor: pointer;
}

.dili-system-btn:hover,
.dili-system-btn[aria-expanded="true"] {
  background: var(--color-bg-muted);
}

.dili-system-chevron {
  margin-left: auto;
  color: var(--color-text-muted);
}

/* Fixed position, so the sidebar can never cut it off */
.dili-system-panel {
  position: fixed;
  left: 284px;
  bottom: 16px;
  z-index: 2000;
  width: 260px;
  padding: 8px;
  box-sizing: border-box;
  border-radius: 14px;
  border: 1px solid var(--color-border);
  background: var(--color-surface);
  box-shadow: 0 12px 40px rgba(0, 0, 0, 0.35);
}

.dili-system-item {
  display: flex;
  align-items: center;
  gap: 12px;
  width: 100%;
  min-height: 40px;
  padding: 6px 10px;
  border: none;
  border-radius: 8px;
  background: transparent;
  color: var(--color-text-base);
  font: inherit;
  font-size: 14px;
  text-align: left;
  text-decoration: none;
  cursor: pointer;
}

.dili-system-item:hover {
  background: var(--color-bg-subtle);
  color: var(--color-text-base);
}

.dili-system-item:disabled {
  opacity: 0.5;
  cursor: default;
}

.dili-system-item:disabled:hover {
  background: transparent;
}

.dili-system-divider {
  height: 1px;
  margin: 6px 4px;
  background: var(--color-border);
}

@media (max-width: 900px) {
  .dili-system-panel {
    left: 12px;
    right: 12px;
    width: auto;
  }
}

/* Small screens: the sidebar becomes a bar at the top */
@media (max-width: 900px) {
  body.dili-has-sidebar {
    padding-left: 0;
  }

  .dili-sidebar {
    position: static;
    width: 100%;
    flex-direction: row;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px;
    padding: 12px;
    border-right: none;
    border-bottom: 1px solid var(--color-border);
  }

  .dili-nav {
    flex-direction: row;
    flex-wrap: wrap;
  }

  .dili-nav-label,
  .dili-pc {
    display: none;
  }

  .dili-sidebar-bottom {
    margin-top: 0;
    margin-left: auto;
  }
}
</style>
