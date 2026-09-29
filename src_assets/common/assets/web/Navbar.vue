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

        <div class="dili-nav-label">More</div>
        <RouterLink class="dili-nav-item" to="/featured" active-class="active">
          <Star :size="20"></Star><span>{{ $t('navbar.featured') }}</span>
        </RouterLink>
        <RouterLink class="dili-nav-item" to="/config" active-class="active">
          <Settings :size="20"></Settings><span>{{ $t('navbar.configuration') }}</span>
        </RouterLink>
        <RouterLink class="dili-nav-item" to="/troubleshooting" active-class="active">
          <Info :size="20"></Info><span>{{ $t('navbar.troubleshoot') }}</span>
        </RouterLink>
      </div>

      <div class="dili-sidebar-bottom">
        <div class="dili-quick">
          <button v-if="runningApp" type="button" class="dili-quick-btn" :title="'Close ' + runningApp" @click="confirmOr('close', closeApp)">
            <CircleX :size="16"></CircleX>
            <span>{{ confirm === 'close' ? 'Tap again to close' : 'Close ' + runningApp }}</span>
          </button>
          <button type="button" class="dili-quick-btn" title="Restart Dili" :disabled="restarting" @click="confirmOr('restart', restartDili)">
            <RotateCw :size="16"></RotateCw>
            <span>{{ restarting ? 'Restarting…' : (confirm === 'restart' ? 'Tap again to restart' : 'Restart Dili') }}</span>
          </button>
        </div>
        <div class="dili-pc">
          <div class="dili-pc-name">{{ hostName || 'This PC' }}</div>
          <div class="dili-pc-state">
            <span class="dili-dot" :class="{ live: streaming }"></span>
            {{ streaming ? 'Streaming' : 'Ready' }}
          </div>
        </div>
        <DiliThemeSwitch></DiliThemeSwitch>
        <div class="dili-sidebar-tools">
          <div class="dropdown">
            <button class="dili-tool dropdown-toggle" type="button" id="navbarUserMenu"
                    data-bs-toggle="dropdown" aria-expanded="false" aria-label="User menu" title="User menu">
              <CircleUserRound :size="20"></CircleUserRound>
            </button>
            <ul class="dropdown-menu" aria-labelledby="navbarUserMenu">
              <li>
                <RouterLink class="dropdown-item d-flex align-items-center" to="/password">
                  <Shield :size="18" class="icon me-2"></Shield>
                  {{ $t('navbar.password') }}
                </RouterLink>
              </li>
              <li><hr class="dropdown-divider"></li>
              <li>
                <button type="button" class="dropdown-item d-flex align-items-center" @click="logout">
                  <LogOut :size="18" class="icon me-2"></LogOut>
                  {{ $t('navbar.logout') }}
                </button>
              </li>
            </ul>
          </div>
        </div>
      </div>
    </nav>
    <Notification></Notification>
  </div>
</template>

<script>
import { CircleUserRound, CircleX, Home, RotateCw, Info, Layers, LogOut, Monitor, MonitorSmartphone, Settings, Shield, Star } from '@lucide/vue'
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
    RotateCw
  },
  data() {
    return {
      hostName: '',
      streaming: false,
      runningApp: '',
      confirm: '',
      restarting: false,
      timer: null,
    }
  },
  mounted() {
    document.body.classList.add('dili-has-sidebar')
    this.refresh()
    this.timer = setInterval(this.refresh, 5000)
  },
  beforeUnmount() {
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
    // Buttons that interrupt a stream need a second tap
    confirmOr(action, run) {
      if (this.confirm === action) {
        this.confirm = ''
        run()
        return
      }
      this.confirm = action
      setTimeout(() => { if (this.confirm === action) this.confirm = '' }, 4000)
    },
    async closeApp() {
      await apiFetch('./api/apps/close', { method: 'POST', headers: { 'Content-Type': 'application/json' } })
      setTimeout(this.refresh, 1500)
    },
    restartDili() {
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

.dili-quick {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.dili-quick-btn {
  display: flex;
  align-items: center;
  gap: 10px;
  min-height: 38px;
  padding: 0 12px;
  border: none;
  border-radius: 10px;
  background: transparent;
  color: var(--color-text-base);
  font: inherit;
  font-size: 14px;
  font-weight: 500;
  text-align: left;
  cursor: pointer;
}

.dili-quick-btn:hover {
  background: var(--color-bg-muted);
}

.dili-quick-btn:disabled {
  opacity: 0.6;
  cursor: default;
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
  .dili-quick,
  .dili-pc {
    display: none;
  }

  .dili-sidebar-bottom {
    margin-top: 0;
    margin-left: auto;
  }
}
</style>
