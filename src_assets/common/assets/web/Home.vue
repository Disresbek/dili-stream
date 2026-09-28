<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Home</h1>
      <p v-if="status && status.sessions > 0">Someone is playing right now.</p>
      <p v-else>Everything is set up. Open Moonlight on any device and start playing.</p>
    </header>

    <!-- Startup errors -->
    <div class="alert alert-danger" v-if="fatalLogs.length">
      <div class="d-flex align-items-center mb-3">
        <alert-circle :size="28" class="me-3"></alert-circle>
        <div v-html="$t('index.startup_errors')"></div>
      </div>
      <ul class="mb-3">
        <li v-for="v in fatalLogs" :key="`${v.timestamp}-${v.value}`">{{ v.value }}</li>
      </ul>
      <RouterLink class="btn btn-danger" to="/troubleshooting#logs">View logs</RouterLink>
    </div>

    <!-- Status -->
    <section class="dili-status">
      <div class="dili-status-icon" :class="{ problem: fatalLogs.length }">
        <x v-if="fatalLogs.length" :size="28"></x>
        <check v-else :size="28"></check>
      </div>
      <div class="dili-status-text">
        <div class="dili-status-title">{{ fatalLogs.length ? 'Needs attention' : 'Ready to stream' }}</div>
        <div class="dili-muted">
          <template v-if="status">
            Your PC is visible to Moonlight as <strong>{{ status.host_name }}</strong>.
          </template>
          <template v-else>Checking…</template>
        </div>
      </div>
      <RouterLink class="dili-pill" to="/devices">Pair a device</RouterLink>
    </section>

    <!-- Streaming now -->
    <section class="dili-section">
      <h2>Streaming now</h2>
      <div v-if="status && status.sessions > 0" class="dili-panel dili-stream">
        <div class="dili-tile">
          <monitor :size="24"></monitor>
        </div>
        <div class="dili-stream-text">
          <div class="dili-stream-title">
            {{ status.app || 'A device is connected' }}
            <span v-if="status.sessions > 1" class="dili-muted"> · {{ status.sessions }} devices</span>
          </div>
          <div class="dili-muted" v-if="status.virtual_display && status.virtual_display.active">
            Virtual screen · {{ status.virtual_display.width }} × {{ status.virtual_display.height }} · {{ status.virtual_display.hz }} Hz
          </div>
          <div class="dili-muted" v-else>Streaming your monitor</div>
        </div>
        <button type="button" class="dili-pill dili-pill-outline" :disabled="ending" @click="endStream">
          {{ ending ? 'Ending…' : 'End stream' }}
        </button>
      </div>
      <div v-else class="dili-panel dili-empty">
        No one is streaming right now.
      </div>
    </section>

    <!-- Shortcuts -->
    <section class="dili-section">
      <h2>Quick settings</h2>
      <div class="dili-panel">
        <RouterLink class="dili-link-row" to="/displays">
          <div>
            <div class="dili-row-title">Displays</div>
            <div class="dili-muted dili-small">
              {{ status && status.virtual_enabled ? 'A virtual screen is created for each device' : 'Streaming one of your monitors' }}
            </div>
          </div>
          <chevron-right :size="20"></chevron-right>
        </RouterLink>
        <div class="dili-divider"></div>
        <RouterLink class="dili-link-row" to="/apps">
          <div>
            <div class="dili-row-title">Applications</div>
            <div class="dili-muted dili-small">What your devices can start from Moonlight</div>
          </div>
          <chevron-right :size="20"></chevron-right>
        </RouterLink>
      </div>
    </section>

    <footer class="dili-footer dili-muted dili-small">
      Dili <template v-if="version">{{ version }}</template> ·
      {{ $t('index.description') }} ·
      <a href="https://github.com/LizardByte/Sunshine" target="_blank" rel="noopener">Sunshine on GitHub</a> ·
      <a href="https://github.com/LizardByte/Sunshine/blob/master/LICENSE" target="_blank" rel="noopener">GPL-3.0</a>
    </footer>
  </div>
</template>

<script>
  import Navbar from './Navbar.vue'
  import { apiFetch } from './fetch_utils'
  import { AlertCircle, Check, ChevronRight, Monitor, X } from '@lucide/vue'

  export default {
    components: {
      Navbar,
      AlertCircle,
      Check,
      ChevronRight,
      Monitor,
      X,
    },
    data() {
      return {
        status: null,
        version: '',
        logs: '',
        ending: false,
        timer: null,
      };
    },
    computed: {
      fatalLogs() {
        if (!this.logs) return [];
        const regex = /(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3}]):\s/g;
        const raw = this.logs.split(regex).splice(1);
        const lines = [];
        for (let i = 0; i < raw.length; i += 2) {
          lines.push({ timestamp: raw[i], level: raw[i + 1].split(':')[0], value: raw[i + 1] });
        }
        return lines.filter((x) => x.level === 'Fatal');
      },
    },
    async created() {
      this.refresh();
      this.timer = setInterval(this.refresh, 3000);
      try {
        const config = await fetch('./api/config').then((r) => r.json());
        this.version = config.version || '';
      } catch (e) {
        console.error(e);
      }
      try {
        this.logs = await fetch('./api/logs').then((r) => r.text());
      } catch (e) {
        console.error(e);
      }
    },
    beforeUnmount() {
      clearInterval(this.timer);
    },
    methods: {
      async refresh() {
        try {
          this.status = await fetch('./api/status').then((r) => r.json());
        } catch (e) {
          console.error(e);
        }
      },
      async endStream() {
        this.ending = true;
        try {
          await apiFetch('./api/apps/close', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
          });
        } finally {
          setTimeout(() => {
            this.ending = false;
            this.refresh();
          }, 1500);
        }
      },
    },
  };
</script>

<style scoped>
  .dili-page {
    max-width: 1100px;
    padding-top: 36px;
    padding-bottom: 48px;
    display: flex;
    flex-direction: column;
    gap: 28px;
  }

  .dili-header h1 {
    margin: 0 0 6px 0;
    font-size: 32px;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .dili-header p,
  .dili-muted {
    margin: 0;
    color: var(--color-text-muted);
  }

  .dili-small {
    font-size: 13px;
  }

  .dili-status {
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
    padding: 28px;
    display: flex;
    align-items: center;
    gap: 24px;
    flex-wrap: wrap;
  }

  .dili-status-icon {
    width: 56px;
    height: 56px;
    border-radius: 28px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #ffffff;
    background: var(--color-success);
  }

  .dili-status-icon.problem {
    background: var(--color-danger);
  }

  .dili-status-text {
    flex-grow: 1;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .dili-status-title {
    font-size: 22px;
    font-weight: 700;
  }

  .dili-pill {
    flex-shrink: 0;
    display: inline-flex;
    align-items: center;
    height: 44px;
    padding: 0 20px;
    border-radius: 22px;
    border: none;
    background: var(--color-primary);
    color: var(--color-on-primary);
    text-decoration: none;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-pill:hover {
    background: var(--color-primary-hover);
    color: var(--color-on-primary);
  }

  .dili-pill-outline {
    background: transparent;
    color: var(--color-text-base);
    border: 1px solid var(--color-border-strong);
  }

  .dili-pill-outline:hover {
    background: var(--color-bg-subtle);
    color: var(--color-text-base);
  }

  .dili-section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .dili-section h2 {
    margin: 0;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .dili-panel {
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
  }

  .dili-stream {
    display: flex;
    align-items: center;
    gap: 20px;
    padding: 20px 24px;
  }

  .dili-tile {
    width: 48px;
    height: 48px;
    border-radius: 12px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: var(--color-bg-muted);
  }

  .dili-stream-text {
    flex-grow: 1;
    display: flex;
    flex-direction: column;
    gap: 3px;
  }

  .dili-stream-title {
    font-size: 17px;
    font-weight: 600;
  }

  .dili-empty {
    padding: 24px;
    color: var(--color-text-muted);
  }

  .dili-link-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
    padding: 16px 20px;
    color: var(--color-text-base);
    text-decoration: none;
  }

  .dili-link-row:hover {
    background: var(--color-bg-subtle);
    border-radius: 16px;
  }

  .dili-row-title {
    font-size: 15px;
    font-weight: 600;
  }

  .dili-divider {
    height: 1px;
    background: var(--color-border);
    margin: 0 20px;
  }

  .dili-footer a {
    color: var(--color-text-muted);
  }
</style>
