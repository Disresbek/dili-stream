<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Displays</h1>
      <p>Choose what your devices see when they stream from this PC.</p>
    </header>

    <div v-if="loading" class="dili-muted">Looking for screens…</div>

    <template v-else>
      <section class="dili-grid" aria-label="Screen to stream">
        <button
          v-if="virtualSupported"
          type="button"
          class="dili-card"
          :class="{ selected: isVirtual }"
          :aria-pressed="isVirtual ? 'true' : 'false'"
          @click="select('virtual')"
        >
          <div class="dili-card-top">
            <svg width="56" height="40" viewBox="0 0 56 40" fill="none" aria-hidden="true">
              <rect x="2" y="2" width="52" height="30" rx="4" stroke="currentColor" stroke-width="2.2" stroke-dasharray="5 4"></rect>
              <path d="M20 38h16" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"></path>
            </svg>
            <CircleCheck v-if="isVirtual" :size="22" class="dili-check"></CircleCheck>
            <CircleIcon v-else :size="22" class="dili-uncheck"></CircleIcon>
          </div>
          <div class="dili-card-title">
            Virtual screen
            <span class="dili-badge">Recommended</span>
          </div>
          <div class="dili-card-text">
            Created fresh for every stream and sized to your device. Your monitors stay untouched.
          </div>
        </button>

        <button
          v-for="d in physicalDisplays"
          :key="d.name"
          type="button"
          class="dili-card"
          :class="{ selected: config.output_name === d.name }"
          :aria-pressed="config.output_name === d.name ? 'true' : 'false'"
          @click="select(d.name)"
        >
          <div class="dili-card-top">
            <svg v-if="d.portrait" width="56" height="40" viewBox="0 0 56 40" fill="none" aria-hidden="true">
              <rect x="17" y="1" width="22" height="34" rx="3" stroke="currentColor" stroke-width="2.2"></rect>
              <path d="M22 39h12" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"></path>
            </svg>
            <svg v-else width="56" height="40" viewBox="0 0 56 40" fill="none" aria-hidden="true">
              <rect x="2" y="2" width="52" height="30" rx="4" stroke="currentColor" stroke-width="2.2"></rect>
              <path d="M20 38h16M28 32v6" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"></path>
            </svg>
            <CircleCheck v-if="config.output_name === d.name" :size="22" class="dili-check"></CircleCheck>
            <CircleIcon v-else :size="22" class="dili-uncheck"></CircleIcon>
          </div>
          <div class="dili-card-title">{{ d.name }}</div>
          <div class="dili-card-text">
            {{ d.portrait ? d.height : d.width }} × {{ d.portrait ? d.width : d.height }}
            <template v-if="d.max_hz"> · up to {{ d.max_hz }} Hz</template>
            <template v-if="d.portrait"> · portrait</template>
            <br>Shows exactly what is on this monitor.
          </div>
        </button>
      </section>

      <p v-if="!isVirtual" class="dili-muted dili-hint">
        Streaming a monitor may ask KDE for screen sharing permission once.
      </p>

      <section v-if="isVirtual" class="dili-section">
        <h2>While streaming</h2>
        <div class="dili-panel">
          <div class="dili-row">
            <div>
              <div class="dili-row-title">Show the taskbar on the streamed screen</div>
              <div class="dili-row-text">
                The streamed screen becomes your main screen, so games and the taskbar appear on your device.
                Your monitor gets it back when the stream ends.
              </div>
            </div>
            <button
              type="button"
              class="dili-switch"
              :class="{ on: primaryOn }"
              :aria-pressed="primaryOn ? 'true' : 'false'"
              aria-label="Show the taskbar on the streamed screen"
              @click="togglePrimary"
            >
              <span></span>
            </button>
          </div>
          <div class="dili-divider"></div>
          <div class="dili-row">
            <div>
              <div class="dili-row-title">Turn off my monitors</div>
              <div class="dili-row-text" v-if="monitors.available && monitors.monitors.length">
                Your monitors switch off when a stream starts, and back on when it ends.
                Found: {{ monitors.monitors.map((m) => m.model || 'Monitor ' + m.number).join(', ') }}.
              </div>
              <div class="dili-row-text" v-else-if="monitors.available">
                None of your monitors can be switched off by Dili. They need "DDC/CI" turned on in their own menu.
              </div>
              <div class="dili-row-text" v-else-if="monitorsLoaded">
                This needs the small tool "ddcutil", which is not installed on this PC.
              </div>
              <div class="dili-row-text" v-else>Checking your monitors…</div>
            </div>
            <button
              type="button"
              class="dili-switch"
              :class="{ on: monitorsOffOn }"
              :aria-pressed="monitorsOffOn ? 'true' : 'false'"
              aria-label="Turn off my monitors"
              :disabled="!monitorsOffOn && !(monitors.available && monitors.monitors.length)"
              @click="toggleMonitorsOff"
            >
              <span></span>
            </button>
          </div>
          <div class="dili-divider"></div>
          <div class="dili-row">
            <div>
              <div class="dili-row-title">Matches each device automatically</div>
              <div class="dili-row-text">
                Resolution and refresh rate follow the device, for example 4K at 60 Hz on a TV or 120 Hz on a phone.
              </div>
            </div>
            <span class="dili-always">Always on</span>
          </div>
        </div>
      </section>

      <div class="dili-actions">
        <button type="button" class="btn btn-primary" :disabled="!dirty" @click="save">Save</button>
        <button type="button" class="btn btn-success" v-if="saved && !restarted" @click="apply">Apply now</button>
        <span v-if="saved && !restarted" class="dili-muted">Saved. Apply restarts the streaming service and ends active streams.</span>
        <span v-if="restarted" class="dili-muted">Restarting… this page reloads in a few seconds.</span>
      </div>
    </template>
  </div>
</template>

<script>
  import Navbar from './Navbar.vue'
  import { apiFetch } from './fetch_utils'
  import { CircleCheck, Circle as CircleIcon } from '@lucide/vue'

  // Dili's "turn off my monitors" commands use the monitor's own power control (DDC/CI, VCP code D6)
  function isMonitorOff(p) {
    return /ddcutil --display \d+ setvcp D6 04/.test((p && p.do) || '');
  }

  export default {
    components: {
      Navbar,
      CircleCheck,
      CircleIcon,
    },
    data() {
      return {
        monitors: { available: false, monitors: [] },
        monitorsLoaded: false,
        loading: true,
        displays: [],
        virtualSupported: false,
        config: {},
        dirty: false,
        saved: false,
        restarted: false,
      };
    },
    computed: {
      prepList() {
        try {
          const list = JSON.parse(this.config.global_prep_cmd || '[]');
          return Array.isArray(list) ? list : [];
        } catch (e) {
          return [];
        }
      },
      monitorsOffOn() {
        return this.prepList.some((p) => isMonitorOff(p));
      },
      physicalDisplays() {
        return this.displays.filter((d) => !d.virtual);
      },
      isVirtual() {
        return this.config.output_name === 'virtual';
      },
      primaryOn() {
        const v = String(this.config.virtual_display_primary || '').toLowerCase();
        return v === 'enabled' || v === 'true' || v === 'on' || v === 'yes' || v === '1';
      },
    },
    async mounted() {
      try {
        this.monitors = await fetch('./api/monitors').then((r) => r.json());
      } catch (e) {
        this.monitors = { available: false, monitors: [] };
      } finally {
        this.monitorsLoaded = true;
      }
    },
    async created() {
      try {
        const [displays, config] = await Promise.all([
          fetch('./api/displays').then((r) => r.json()),
          fetch('./api/config').then((r) => r.json()),
        ]);
        this.displays = displays.displays || [];
        this.virtualSupported = !!displays.virtual_supported;
        delete config.platform;
        delete config.status;
        delete config.version;
        this.config = config;
      } finally {
        this.loading = false;
      }
    },
    methods: {
      toggleMonitorsOff() {
        // Dili's own entries in the commands that run for every app
        let list = this.prepList.filter((p) => !isMonitorOff(p));
        if (!this.monitorsOffOn) {
          const own = this.monitors.monitors.map((m) => ({
            // "|| true": a monitor that does not answer must never stop the stream from starting
            do: `sh -c "ddcutil --display ${m.number} setvcp D6 04 || true"`,
            undo: `sh -c "ddcutil --display ${m.number} setvcp D6 01 || true"`,
          }));
          list = [...list, ...own];
        }
        this.config.global_prep_cmd = JSON.stringify(list);
        this.dirty = true;
        this.saved = false;
      },
      select(name) {
        if (this.config.output_name === name) {
          return;
        }
        this.config.output_name = name;
        this.dirty = true;
        this.saved = false;
      },
      togglePrimary() {
        this.config.virtual_display_primary = this.primaryOn ? 'disabled' : 'enabled';
        this.dirty = true;
        this.saved = false;
      },
      save() {
        return apiFetch('./api/config', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(this.config),
        }).then((r) => {
          this.saved = r.status === 200;
          if (this.saved) {
            this.dirty = false;
          }
          return this.saved;
        });
      },
      apply() {
        this.restarted = true;
        apiFetch('./api/restart', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
        });
        setTimeout(() => window.location.reload(), 8000);
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

  .dili-hint {
    margin-top: -12px;
    font-size: 14px;
  }

  .dili-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 20px;
  }

  .dili-card {
    text-align: left;
    font: inherit;
    color: var(--color-text-base);
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
    padding: 24px;
    display: flex;
    flex-direction: column;
    gap: 14px;
    cursor: pointer;
  }

  .dili-card:hover {
    border-color: var(--color-border-strong);
  }

  .dili-card.selected {
    border: 2px solid var(--color-primary);
    padding: 23px;
  }

  .dili-card-top {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .dili-check {
    color: var(--color-primary);
  }

  .dili-uncheck {
    color: var(--color-border-strong);
  }

  .dili-card-title {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 18px;
    font-weight: 700;
  }

  .dili-card-text {
    font-size: 14px;
    line-height: 1.45;
    color: var(--color-text-muted);
  }

  .dili-badge {
    font-size: 12px;
    font-weight: 700;
    color: var(--color-on-primary);
    background: var(--color-primary);
    padding: 3px 10px;
    border-radius: 10px;
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

  .dili-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
    padding: 16px 20px;
  }

  .dili-row-title {
    font-size: 15px;
    font-weight: 600;
  }

  .dili-row-text {
    font-size: 13px;
    line-height: 1.4;
    color: var(--color-text-muted);
  }

  .dili-divider {
    height: 1px;
    background: var(--color-border);
    margin: 0 20px;
  }

  .dili-always {
    flex-shrink: 0;
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text-muted);
  }

  .dili-switch {
    flex-shrink: 0;
    width: 50px;
    height: 30px;
    border: none;
    border-radius: 15px;
    background: var(--color-border-strong);
    padding: 0;
    display: flex;
    align-items: center;
    cursor: pointer;
  }

  .dili-switch span {
    display: block;
    width: 26px;
    height: 26px;
    margin-left: 2px;
    border-radius: 13px;
    background: #ffffff;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
    transition: margin-left 0.15s ease;
  }

  .dili-switch.on {
    background: var(--color-success);
  }

  .dili-switch.on span {
    margin-left: 22px;
  }

  .dili-actions {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
  }
</style>
