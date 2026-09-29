<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Featured Apps</h1>
      <p>Moonlight for all your devices, and a few tools that work well with Dili.</p>
    </header>

    <!-- Device filter -->
    <div class="dili-segment" role="tablist" aria-label="Show apps for">
      <button
        v-for="f in filters"
        :key="f.id"
        type="button"
        role="tab"
        class="dili-segment-btn"
        :class="{ active: filter === f.id }"
        :aria-selected="filter === f.id ? 'true' : 'false'"
        @click="filter = f.id"
      >
        {{ f.label }}
      </button>
    </div>

    <div v-if="loading" class="dili-muted">Loading apps…</div>
    <div v-else-if="error" class="dili-panel dili-empty">
      The list of apps could not be loaded. Check your internet connection and try again.
      <button type="button" class="dili-text-link" @click="loadFeaturedApps">Try again</button>
    </div>

    <template v-else>
      <section v-for="group in groups" :key="group.id" class="dili-section">
        <h2>{{ group.title }}</h2>
        <div class="dili-app-grid">
          <div v-for="app in group.apps" :key="app.id" class="dili-tile" role="button" tabindex="0"
               @click="open(app)" @keydown.enter="open(app)">
            <div class="dili-tile-icon">
              <img v-if="app.icon" :src="app.icon + '?size=128'" :alt="''" @error="$event.target.style.display = 'none'">
            </div>
            <div class="dili-tile-text">
              <div class="dili-tile-name">{{ app.name }}</div>
              <div class="dili-tile-tagline">{{ app.tagline || app.description }}</div>
              <div class="dili-tile-platforms">
                <span v-if="app.official" class="dili-official dili-official-inline">Official</span>
                {{ (app.platforms || []).map(platformName).join(' · ') }}
              </div>
            </div>
            <a v-if="primaryLink(app)" :href="primaryLink(app)" target="_blank" rel="noopener noreferrer"
               class="dili-get" @click.stop>Get</a>
          </div>
        </div>
      </section>
      <p v-if="!groups.length" class="dili-muted">No apps for this kind of device yet.</p>
    </template>

    <!-- Detail sheet -->
    <div v-if="selected" class="dili-sheet-backdrop" @click="selected = null">
      <section class="dili-sheet" role="dialog" :aria-label="selected.name" @click.stop>
        <div class="dili-sheet-head">
          <div class="dili-tile-icon dili-tile-icon-large">
            <img v-if="selected.icon" :src="selected.icon + '?size=128'" alt="">
          </div>
          <div class="dili-sheet-title">
            <h2>{{ selected.name }} <span v-if="selected.official" class="dili-official">Official</span></h2>
            <div class="dili-muted">{{ selected.tagline }}</div>
            <div class="dili-tile-platforms">{{ (selected.platforms || []).map(platformName).join(' · ') }}</div>
          </div>
          <button type="button" class="dili-close" aria-label="Close" @click="selected = null"><X :size="20"></X></button>
        </div>

        <p class="dili-sheet-desc">{{ selected.description }}</p>

        <div v-if="selected.screenshots && selected.screenshots.length" class="dili-shots">
          <img v-for="(shot, i) in selected.screenshots" :key="i" :src="shot" :alt="selected.name + ' screenshot ' + (i + 1)"
               loading="lazy" @click="zoom = shot">
        </div>

        <div class="dili-sheet-group-title">Download</div>
        <div class="dili-downloads">
          <template v-if="selected.downloads && selected.downloads.length">
            <a v-for="(d, i) in selected.downloads" :key="i" :href="d.url" target="_blank" rel="noopener noreferrer"
               :class="d.img ? 'dili-store-badge' : 'dili-pill'">
              <img v-if="d.img" :src="d.img" :alt="d.label">
              <template v-else>{{ d.label || 'Download' }}</template>
            </a>
          </template>
          <a v-else-if="primaryLink(selected)" :href="primaryLink(selected)" target="_blank" rel="noopener noreferrer" class="dili-pill">Get</a>
        </div>

        <div class="dili-sheet-links">
          <a v-if="selected.links && selected.links.website" :href="selected.links.website" target="_blank" rel="noopener noreferrer">Website</a>
          <a v-if="selected.links && selected.links.documentation" :href="selected.links.documentation" target="_blank" rel="noopener noreferrer">Help &amp; docs</a>
          <a v-if="selected.links && selected.links.github" :href="selected.links.github" target="_blank" rel="noopener noreferrer">Source code</a>
          <span v-if="selected.github && selected.github.stars !== undefined" class="dili-muted">★ {{ formatNumber(selected.github.stars) }}</span>
          <span v-if="selected.github && selected.github.lastUpdated" class="dili-muted">Updated {{ relativeDate(selected.github.lastUpdated) }}</span>
        </div>
      </section>
    </div>

    <!-- Screenshot zoom -->
    <div v-if="zoom" class="dili-zoom" @click="zoom = null">
      <img :src="zoom" alt="">
    </div>
  </div>
</template>

<script>
  import Navbar from './Navbar.vue'
  import { formatDistanceToNow } from 'date-fns'
  import { X } from '@lucide/vue'

  // Which device kinds each filter shows
  const FILTERS = [
    { id: 'all', label: 'All', platforms: null },
    { id: 'tv', label: 'TV', platforms: ['tv'] },
    { id: 'mobile', label: 'Phone & Tablet', platforms: ['ios', 'android'] },
    { id: 'computer', label: 'Computer', platforms: ['windows', 'macos', 'linux', 'web'] },
    { id: 'handheld', label: 'Handheld & Console', platforms: ['handheld', 'console'] },
  ];

  const PLATFORM_NAMES = {
    android: 'Android',
    console: 'Console',
    handheld: 'Handheld',
    ios: 'iPhone & iPad',
    linux: 'Linux',
    macos: 'Mac',
    tv: 'TV',
    web: 'Web',
    windows: 'Windows',
  };

  export default {
    components: { Navbar, X },
    data() {
      return {
        apps: [],
        categories: [],
        loading: true,
        error: null,
        filter: 'all',
        filters: FILTERS,
        selected: null,
        zoom: null,
      };
    },
    computed: {
      visibleApps() {
        const f = FILTERS.find((x) => x.id === this.filter);
        return this.apps
          // Windows-only tools cannot be used with Dili on Linux
          .filter((app) => !((app.platforms || []).length === 1 && app.platforms[0] === 'windows' && !this.isClient(app)))
          .filter((app) => !f.platforms || (app.platforms || []).some((p) => f.platforms.includes(p)))
          .sort((a, b) => (b.official === true) - (a.official === true) || (b.github?.stars || 0) - (a.github?.stars || 0));
      },
      groups() {
        const clients = this.visibleApps.filter((a) => this.isClient(a));
        const tools = this.visibleApps.filter((a) => !this.isClient(a));
        return [
          { id: 'clients', title: 'Moonlight for your devices', apps: clients },
          { id: 'tools', title: 'Tools', apps: tools },
        ].filter((g) => g.apps.length);
      },
    },
    created() {
      this.loadFeaturedApps();
      window.addEventListener('keydown', this.onKey);
    },
    beforeUnmount() {
      window.removeEventListener('keydown', this.onKey);
    },
    methods: {
      async loadFeaturedApps() {
        this.loading = true;
        this.error = null;
        try {
          const response = await fetch('https://app.lizardbyte.dev/app-directory/sunshine.json');
          if (!response.ok) throw new Error('Failed to load featured apps');
          const data = await response.json();
          this.apps = data.apps || [];
          this.categories = data.categories || [];
        } catch (err) {
          console.error('Error loading featured apps:', err);
          this.error = err.message;
        } finally {
          this.loading = false;
        }
      },
      isClient(app) {
        const cat = this.categories.find((c) => c.id === app.category);
        const key = (cat && (cat.originalId || cat.id)) || app.category || '';
        return /client/i.test(String(key)) || /moonlight/i.test(app.name || '');
      },
      platformName(p) {
        return PLATFORM_NAMES[p] || p;
      },
      primaryLink(app) {
        if (app.downloads && app.downloads.length) return app.downloads[0].url;
        return (app.links && (app.links.download || app.links.website || app.links.github)) || '';
      },
      open(app) {
        this.selected = app;
      },
      onKey(e) {
        if (e.key !== 'Escape') return;
        if (this.zoom) this.zoom = null;
        else this.selected = null;
      },
      formatNumber(num) {
        if (num === undefined || num === null) return '0';
        if (Math.abs(num) >= 1000000) return (num / 1000000).toFixed(1) + 'M';
        if (Math.abs(num) >= 1000) return (num / 1000).toFixed(1) + 'k';
        return String(num);
      },
      relativeDate(date) {
        try {
          return formatDistanceToNow(new Date(date), { addSuffix: true });
        } catch (e) {
          return '';
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
    gap: 24px;
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

  .dili-segment {
    align-self: flex-start;
    display: inline-flex;
    flex-wrap: wrap;
    gap: 2px;
    padding: 3px;
    border-radius: 10px;
    background: var(--color-bg-muted);
  }

  .dili-segment-btn {
    height: 34px;
    padding: 0 16px;
    border: none;
    border-radius: 8px;
    background: transparent;
    color: var(--color-text-muted);
    font: inherit;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-segment-btn.active {
    background: var(--color-surface);
    color: var(--color-text-base);
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
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

  .dili-empty {
    padding: 20px;
    display: flex;
    gap: 12px;
    align-items: center;
    flex-wrap: wrap;
  }

  .dili-text-link {
    border: none;
    background: none;
    padding: 0;
    color: var(--color-primary);
    font: inherit;
    font-weight: 600;
    cursor: pointer;
  }

  /* App Store style list */
  .dili-app-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(320px, 1fr));
    grid-auto-rows: 1fr;  /* every card has the same height */
    gap: 12px;
  }

  .dili-tile {
    height: 100%;
    box-sizing: border-box;
    display: flex;
    align-items: center;
    gap: 14px;
    padding: 14px;
    border-radius: 16px;
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    cursor: pointer;
    min-width: 0;
  }

  .dili-tile:hover,
  .dili-tile:focus-visible {
    border-color: var(--color-primary);
    outline: none;
  }

  .dili-tile-icon {
    width: 60px;
    height: 60px;
    flex-shrink: 0;
    border-radius: 14px;
    overflow: hidden;
    background: var(--color-bg-muted);
  }

  .dili-tile-icon img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .dili-tile-icon-large {
    width: 88px;
    height: 88px;
    border-radius: 20px;
  }

  .dili-tile-text {
    flex-grow: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .dili-tile-name {
    font-size: 15px;
    font-weight: 700;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }

  .dili-tile-tagline {
    font-size: 13px;
    color: var(--color-text-muted);
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }

  .dili-tile-platforms {
    font-size: 12px;
    color: var(--color-text-muted);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
  }

  .dili-official {
    margin-left: 6px;
    padding: 1px 8px;
    border-radius: 10px;
    font-size: 11px;
    font-weight: 700;
    color: var(--color-on-primary);
    background: var(--color-primary);
    vertical-align: middle;
  }

  .dili-official-inline {
    margin-left: 0;
    margin-right: 4px;
  }

  .dili-get {
    flex-shrink: 0;
    display: inline-flex;
    align-items: center;
    height: 32px;
    padding: 0 18px;
    border-radius: 16px;
    background: var(--color-bg-muted);
    color: var(--color-primary);
    font-size: 14px;
    font-weight: 700;
    text-decoration: none;
  }

  .dili-get:hover {
    background: var(--color-primary);
    color: var(--color-on-primary);
  }

  /* Detail sheet */
  .dili-sheet-backdrop {
    position: fixed;
    inset: 0;
    z-index: 3000;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    background: rgba(0, 0, 0, 0.5);
  }

  .dili-sheet {
    width: 100%;
    max-width: 720px;
    max-height: calc(100vh - 48px);
    overflow-y: auto;
    padding: 28px;
    box-sizing: border-box;
    border-radius: 20px;
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    box-shadow: 0 20px 60px rgba(0, 0, 0, 0.4);
    display: flex;
    flex-direction: column;
    gap: 18px;
  }

  .dili-sheet-head {
    display: flex;
    gap: 18px;
    align-items: center;
  }

  .dili-sheet-title {
    flex-grow: 1;
    display: flex;
    flex-direction: column;
    gap: 4px;
  }

  .dili-sheet-title h2 {
    margin: 0;
    font-size: 22px;
    font-weight: 700;
  }

  .dili-close {
    align-self: flex-start;
    width: 36px;
    height: 36px;
    display: flex;
    align-items: center;
    justify-content: center;
    border: none;
    border-radius: 18px;
    background: var(--color-bg-muted);
    color: var(--color-text-base);
    cursor: pointer;
  }

  .dili-sheet-desc {
    margin: 0;
    line-height: 1.55;
  }

  .dili-shots {
    display: flex;
    gap: 10px;
    overflow-x: auto;
    padding-bottom: 4px;
  }

  .dili-shots img {
    height: 180px;
    border-radius: 12px;
    cursor: zoom-in;
    flex-shrink: 0;
  }

  .dili-sheet-group-title {
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .dili-downloads {
    display: flex;
    flex-wrap: wrap;
    gap: 10px;
    align-items: center;
  }

  .dili-store-badge img {
    height: 40px;
  }

  .dili-pill {
    display: inline-flex;
    align-items: center;
    height: 40px;
    padding: 0 18px;
    border-radius: 20px;
    background: var(--color-primary);
    color: var(--color-on-primary);
    font-weight: 600;
    text-decoration: none;
  }

  .dili-sheet-links {
    display: flex;
    flex-wrap: wrap;
    gap: 16px;
    align-items: center;
    padding-top: 12px;
    border-top: 1px solid var(--color-border);
    font-size: 14px;
  }

  .dili-sheet-links a {
    color: var(--color-primary);
    font-weight: 600;
    text-decoration: none;
  }

  .dili-zoom {
    position: fixed;
    inset: 0;
    z-index: 3100;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    background: rgba(0, 0, 0, 0.8);
    cursor: zoom-out;
  }

  .dili-zoom img {
    max-width: 100%;
    max-height: 100%;
    border-radius: 12px;
  }
</style>
