<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Quality</h1>
      <p>Pick how your stream should feel. You can change this any time.</p>
    </header>

    <div v-if="loading" class="dili-muted">Loading…</div>

    <template v-else>
      <section class="dili-grid" aria-label="Quality preset">
        <button
          v-for="p in presets"
          :key="p.id"
          type="button"
          class="dili-card"
          :class="{ selected: current === p.id }"
          :aria-pressed="current === p.id ? 'true' : 'false'"
          @click="select(p.id)"
        >
          <div class="dili-card-top">
            <span class="dili-card-title">{{ p.name }}</span>
            <CircleCheck v-if="current === p.id" :size="22" class="dili-check"></CircleCheck>
            <CircleIcon v-else :size="22" class="dili-uncheck"></CircleIcon>
          </div>
          <div class="dili-card-text">{{ p.text }}</div>
          <div class="dili-card-detail">{{ p.detail }}</div>
        </button>
      </section>

      <p class="dili-muted dili-hint">
        The sharpness of the picture also depends on the bitrate you choose in Moonlight on each device.
        Higher bitrate means a sharper picture, but needs a faster network.
      </p>

      <div class="dili-actions">
        <button type="button" class="btn btn-primary" :disabled="!dirty" @click="save">Save</button>
        <button type="button" class="btn btn-success" v-if="saved && !restarted" @click="apply">Apply now</button>
        <span v-if="saved && !restarted" class="dili-muted">Saved. Apply restarts the streaming service and ends active streams.</span>
        <span v-if="restarted" class="dili-muted">Restarting… this page reloads in a few seconds.</span>
      </div>

      <RouterLink to="/config" class="dili-advanced">
        <ChevronRight :size="18"></ChevronRight>
        Show all encoder settings
      </RouterLink>
    </template>
  </div>
</template>

<script>
  import Navbar from './Navbar.vue'
  import { apiFetch } from './fetch_utils'
  import { ChevronRight, CircleCheck, Circle as CircleIcon } from '@lucide/vue'

  // Each preset sets the matching option for every encoder type,
  // so it works the same on AMD/Intel (VAAPI), NVIDIA and software encoding.
  const PRESETS = [
    {
      id: 'performance',
      name: 'Performance',
      text: 'The lowest delay. Best for fast games and busy Wi-Fi.',
      detail: 'Fastest encoding',
      values: { vaapi_quality: 'speed', nvenc_preset: '1', sw_preset: 'ultrafast' },
    },
    {
      id: 'balanced',
      name: 'Balanced',
      text: 'Sharp picture with low delay. The right choice for most people.',
      detail: 'Recommended',
      values: { vaapi_quality: 'balanced', nvenc_preset: '3', sw_preset: 'superfast' },
    },
    {
      id: 'quality',
      name: 'Quality',
      text: 'The cleanest picture. Best on a wired network or for slower games.',
      detail: 'Best picture, slightly more delay',
      values: { vaapi_quality: 'quality', nvenc_preset: '6', sw_preset: 'veryfast' },
    },
  ];

  export default {
    components: {
      Navbar,
      ChevronRight,
      CircleCheck,
      CircleIcon,
    },
    data() {
      return {
        loading: true,
        presets: PRESETS,
        config: {},
        dirty: false,
        saved: false,
        restarted: false,
      };
    },
    computed: {
      current() {
        const q = this.config.vaapi_quality;
        if (q === 'speed') return 'performance';
        if (q === 'quality') return 'quality';
        return 'balanced';
      },
    },
    async created() {
      try {
        const config = await fetch('./api/config').then((r) => r.json());
        delete config.platform;
        delete config.status;
        delete config.version;
        this.config = config;
      } finally {
        this.loading = false;
      }
    },
    methods: {
      select(id) {
        if (this.current === id && this.config.vaapi_quality) {
          return;
        }
        const preset = PRESETS.find((p) => p.id === id);
        Object.assign(this.config, preset.values);
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

  .dili-card-detail {
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text-base);
    margin-top: 4px;
  }

  .dili-advanced {
    align-self: flex-start;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    min-height: 44px;
    color: var(--color-text-base);
    font-weight: 600;
    text-decoration: none;
  }

  .dili-advanced:hover {
    color: var(--color-primary);
  }
</style>
