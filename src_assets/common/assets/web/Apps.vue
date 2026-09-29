<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Applications</h1>
      <p>What your devices can start from Moonlight.</p>
    </header>

    <div v-if="loading" class="dili-muted">Loading…</div>

    <template v-else-if="editing">
      <!-- Simple editor -->
      <section class="dili-panel dili-editor">
        <h2 class="dili-editor-title">{{ editing.index === -1 ? 'Add an app' : (editing.lockName ? 'Picture for ' : 'Edit ') + (editing.original.name || 'app') }}</h2>

        <div class="dili-picture">
          <div class="dili-picture-preview">
            <img v-if="editing.index !== -1 && !editing.pictureChanged" :src="`./api/covers/${editing.index}`" alt="">
            <img v-else-if="editing.pictureUrl" :src="editing.pictureUrl" alt="">
          </div>
          <div class="dili-picture-side">
            <div class="dili-row-title">Picture</div>
            <div class="dili-help">This picture is shown for the app in Moonlight.</div>
            <button type="button" class="dili-pill dili-pill-outline dili-pill-small" @click="openCoverSearch">Find a picture</button>
          </div>
        </div>

        <div v-if="coverSearch" class="dili-covers">
          <div class="dili-cover-search">
            <input v-model="coverSearch.query" type="text" placeholder="Game name" @keyup.enter="runCoverSearch">
            <button type="button" class="dili-pill dili-pill-small" @click="runCoverSearch">Search</button>
          </div>
          <div v-if="coverSearch.loading" class="dili-help">Searching…</div>
          <div v-else-if="coverSearch.results.length === 0" class="dili-help">No pictures found. Try a shorter name.</div>
          <div class="dili-cover-grid">
            <button
              v-for="c in coverSearch.results"
              :key="c.key"
              type="button"
              class="dili-cover-pick"
              :title="c.name"
              @click="pickCover(c)"
            >
              <img :src="c.url" :alt="c.name" loading="lazy">
            </button>
          </div>
        </div>

        <div class="dili-field" v-if="!editing.lockName">
          <label for="appName">Name</label>
          <input id="appName" v-model="editing.name" type="text" placeholder="For example: Cyberpunk 2077">
          <span class="dili-help">This is what you see in Moonlight.</span>
        </div>

        <div class="dili-field" v-if="!editing.lockName">
          <label for="appCommand">Program to start</label>
          <input id="appCommand" v-model="editing.command" type="text" placeholder="For example: steam steam://rungameid/1091500">
          <span class="dili-help">Leave this empty to just show your desktop.</span>
        </div>

        <div class="dili-toggle-row" v-if="!editing.lockName">
          <div>
            <div class="dili-row-title">Close it when the stream ends</div>
            <div class="dili-help">When you stop streaming, Dili closes the program. If you quit the program, the stream ends too.</div>
          </div>
          <button
            type="button"
            class="dili-switch"
            :class="{ on: editing.closeOnEnd }"
            :aria-pressed="editing.closeOnEnd ? 'true' : 'false'"
            aria-label="Close it when the stream ends"
            @click="editing.closeOnEnd = !editing.closeOnEnd"
          >
            <span></span>
          </button>
        </div>

        <p v-if="editing.hasAdvanced" class="dili-help">
          This app has extra settings from the advanced editor. They are kept when you save here.
        </p>
        <p v-if="error" class="dili-error">{{ error }}</p>

        <div class="dili-editor-actions">
          <button
            v-if="editing.index !== -1 && !editing.lockName"
            type="button"
            class="dili-pill dili-pill-danger"
            @click="removeEditing"
          >
            {{ confirmDelete ? 'Tap again to remove' : 'Remove app' }}
          </button>
          <span class="dili-grow"></span>
          <button type="button" class="dili-pill dili-pill-outline" @click="cancelEdit">Cancel</button>
          <button type="button" class="dili-pill" :disabled="(!editing.lockName && !editing.name.trim()) || busy" @click="saveEditing">Save</button>
        </div>

        <RouterLink to="/apps/advanced" class="dili-advanced">
          <ChevronRight :size="18"></ChevronRight>
          Advanced app settings
        </RouterLink>
      </section>
    </template>

    <template v-else>
      <!-- Ready-made apps -->
      <section class="dili-section">
        <h2>Ready-made apps</h2>
        <p class="dili-help">Switch on the apps you want to see in Moonlight. Dili sets them up for you.</p>
        <div class="dili-panel">
          <template v-for="(preset, i) in presets" :key="preset.name">
            <div v-if="i > 0" class="dili-divider"></div>
            <div class="dili-toggle-row">
              <div class="dili-preset">
                <img :src="preset.icon" alt="" class="dili-preset-img">
                <div>
                  <div class="dili-row-title">{{ preset.name }}</div>
                  <div class="dili-help">{{ preset.text }}</div>
                </div>
              </div>
              <div class="dili-preset-side">
                <button
                  v-if="presetIndex(preset) !== -1"
                  type="button"
                  class="dili-link"
                  @click="startEdit(presetIndex(preset), true)"
                >
                  Picture
                </button>
                <span class="dili-state">{{ presetIndex(preset) !== -1 ? 'In Moonlight' : (preset.installed ? 'Off' : 'Not installed') }}</span>
              <button
                type="button"
                class="dili-switch"
                :class="{ on: presetIndex(preset) !== -1 }"
                :aria-pressed="presetIndex(preset) !== -1 ? 'true' : 'false'"
                :aria-label="preset.name"
                :disabled="busy || (!preset.installed && presetIndex(preset) === -1)"
                @click="togglePreset(preset)"
              >
                <span></span>
              </button>
              </div>
            </div>
          </template>
        </div>
        <p class="dili-help">Moonlight shows changes the next time you open your PC in Moonlight.</p>
      </section>

      <!-- Own apps -->
      <section class="dili-section">
        <h2>Your apps</h2>
        <div class="dili-grid">
          <button
            v-for="app in ownApps"
            :key="app.index"
            type="button"
            class="dili-app"
            @click="startEdit(app.index)"
          >
            <div class="dili-cover">
              <img :src="`./api/covers/${app.index}`" :alt="''" loading="lazy" @error="$event.target.style.display = 'none'">
              <span v-if="runningApp === app.name" class="dili-live">Playing now</span>
            </div>
            <div class="dili-app-name">{{ app.name }}</div>
            <div class="dili-help dili-ellipsis">{{ commandOf(app) || 'Desktop only' }}</div>
          </button>

          <button type="button" class="dili-app dili-add" @click="startAdd">
            <Plus :size="32"></Plus>
            <div class="dili-app-name">Add an app</div>
          </button>
        </div>
      </section>
    </template>
  </div>
</template>

<script>
  import Navbar from './Navbar.vue'
  import { apiFetch } from './fetch_utils'
  import { loadPresets } from './presets'
  import { searchCovers, useCover } from './covers'
  import { ChevronRight, Plus } from '@lucide/vue'

  // Ready-made apps (shared with the setup wizard), shown with their picture
  function toCards(presets) {
    return presets.map((p) => ({
      name: p.app.name,
      text: p.installed === false ? 'Steam is not installed on this PC.' : p.description,
      installed: p.installed !== false,
      icon: `./assets/apps/${p.app['image-path']}`,
      app: p.app,
    }));
  }

  export default {
    components: {
      Navbar,
      ChevronRight,
      Plus,
    },
    data() {
      return {
        loading: true,
        apps: [],
        presets: [],
        runningApp: '',
        editing: null,
        coverSearch: null,
        confirmDelete: false,
        busy: false,
        error: '',
      };
    },
    computed: {
      ownApps() {
        const presetNames = this.presets.map((p) => p.name);
        return this.apps
          .map((app, index) => ({ ...app, index }))
          .filter((app) => !presetNames.includes(app.name));
      },
    },
    async created() {
      try {
        this.presets = toCards(await loadPresets());
        const status = await fetch('./api/status').then((r) => r.json());
        this.runningApp = status.app || '';
      } catch (e) {
        console.error(e);
      }
      await this.loadApps();
      this.loading = false;
    },
    methods: {
      async loadApps() {
        const r = await fetch('./api/apps').then((x) => x.json());
        this.apps = r.apps || [];
      },
      presetIndex(preset) {
        return this.apps.findIndex((a) => a.name === preset.name);
      },
      commandOf(app) {
        if (app.cmd) return app.cmd;
        if (app.detached && app.detached.length) return app.detached[0];
        return '';
      },
      async saveApp(app, index) {
        const body = { 'prep-cmd': [], detached: [], ...app, index };
        const r = await apiFetch('./api/apps', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body),
        });
        return r.status === 200;
      },
      async deleteApp(index) {
        const r = await apiFetch('./api/apps/' + index, {
          method: 'DELETE',
          headers: { 'Content-Type': 'application/json' },
        });
        return r.status === 200;
      },
      async togglePreset(preset) {
        this.busy = true;
        try {
          const index = this.presetIndex(preset);
          if (index === -1) {
            await this.saveApp(preset.app, -1);
          } else {
            await this.deleteApp(index);
          }
          await this.loadApps();
        } finally {
          this.busy = false;
        }
      },
      openCoverSearch() {
        this.coverSearch = { query: this.editing.name || '', loading: false, results: [] };
        this.runCoverSearch();
      },
      async runCoverSearch() {
        const q = this.coverSearch.query.trim();
        if (!q) return;
        this.coverSearch.loading = true;
        try {
          this.coverSearch.results = await searchCovers(q);
        } catch (e) {
          this.coverSearch.results = [];
        } finally {
          this.coverSearch.loading = false;
        }
      },
      async pickCover(cover) {
        try {
          this.editing.imagePath = await useCover(cover);
          this.editing.pictureUrl = cover.url;
          this.editing.pictureChanged = true;
          this.coverSearch = null;
        } catch (e) {
          this.error = 'The picture could not be downloaded. Please try another one.';
        }
      },
      startAdd() {
        this.error = '';
        this.confirmDelete = false;
        this.coverSearch = null;
        this.editing = { index: -1, original: {}, name: '', command: '', closeOnEnd: true, hasAdvanced: false, lockName: false, imagePath: '', pictureUrl: '', pictureChanged: false };
      },
      startEdit(index, lockName = false) {
        const app = this.apps[index];
        this.coverSearch = null;
        this.error = '';
        this.confirmDelete = false;
        const hasPrep = (app['prep-cmd'] || []).length > 0;
        const manyDetached = (app.detached || []).length > 1;
        this.editing = {
          index,
          original: app,
          name: app.name || '',
          command: this.commandOf(app),
          closeOnEnd: !!app.cmd || !(app.detached && app.detached.length),
          hasAdvanced: !lockName && (hasPrep || manyDetached),
          lockName,
          imagePath: '',
          pictureUrl: '',
          pictureChanged: false,
        };
      },
      cancelEdit() {
        this.editing = null;
      },
      async saveEditing() {
        const e = this.editing;
        if (e.lockName) {
          const app = { ...e.original };
          if (e.imagePath) app['image-path'] = e.imagePath;
          this.busy = true;
          try {
            if (await this.saveApp(app, e.index)) {
              await this.loadApps();
              this.editing = null;
            } else {
              this.error = 'Saving did not work. Please try again.';
            }
          } finally {
            this.busy = false;
          }
          return;
        }
        const app = { ...e.original, name: e.name.trim() };
        const command = e.command.trim();
        // One simple rule: "close on end" means Dili watches the program (cmd), otherwise it just starts it (detached)
        const otherDetached = (app.detached || []).slice(1);
        delete app.cmd;
        app.detached = otherDetached;
        if (command) {
          if (e.closeOnEnd) {
            app.cmd = command;
          } else {
            app.detached = [command, ...otherDetached];
          }
        }
        if (e.imagePath) {
          app['image-path'] = e.imagePath;
        }
        if (!app['image-path']) {
          app['image-path'] = command ? 'box.png' : 'desktop.png';
        }
        this.busy = true;
        this.error = '';
        try {
          if (await this.saveApp(app, e.index)) {
            await this.loadApps();
            this.editing = null;
          } else {
            this.error = 'Saving did not work. Please try again.';
          }
        } finally {
          this.busy = false;
        }
      },
      async removeEditing() {
        if (!this.confirmDelete) {
          this.confirmDelete = true;
          setTimeout(() => (this.confirmDelete = false), 4000);
          return;
        }
        this.busy = true;
        try {
          await this.deleteApp(this.editing.index);
          await this.loadApps();
          this.editing = null;
        } finally {
          this.busy = false;
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
  .dili-muted,
  .dili-help {
    margin: 0;
    color: var(--color-text-muted);
  }

  .dili-help {
    font-size: 13px;
    line-height: 1.4;
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

  .dili-divider {
    height: 1px;
    background: var(--color-border);
    margin: 0 20px;
  }

  .dili-toggle-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
    padding: 16px 20px;
  }

  .dili-preset {
    display: flex;
    align-items: center;
    gap: 16px;
  }

  .dili-preset-img {
    width: 48px;
    height: 64px;
    object-fit: cover;
    border-radius: 8px;
    background: var(--color-bg-muted);
    flex-shrink: 0;
  }

  .dili-row-title {
    font-size: 15px;
    font-weight: 600;
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

  .dili-switch:disabled {
    opacity: 0.6;
    cursor: default;
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

  .dili-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(170px, 1fr));
    gap: 18px;
  }

  .dili-app {
    text-align: left;
    font: inherit;
    color: var(--color-text-base);
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
    padding: 12px;
    display: flex;
    flex-direction: column;
    gap: 8px;
    cursor: pointer;
    min-width: 0;
  }

  .dili-app:hover {
    border-color: var(--color-primary);
  }

  .dili-cover {
    position: relative;
    aspect-ratio: 3 / 4;
    border-radius: 10px;
    overflow: hidden;
    background: var(--color-bg-muted);
  }

  .dili-cover img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .dili-live {
    position: absolute;
    left: 8px;
    bottom: 8px;
    font-size: 12px;
    font-weight: 700;
    color: var(--color-on-primary);
    background: var(--color-primary);
    padding: 3px 10px;
    border-radius: 10px;
  }

  .dili-app-name {
    font-size: 15px;
    font-weight: 600;
  }

  .dili-ellipsis {
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
  }

  .dili-add {
    align-items: center;
    justify-content: center;
    min-height: 200px;
    border-style: dashed;
    color: var(--color-text-muted);
  }

  .dili-editor {
    padding: 28px;
    display: flex;
    flex-direction: column;
    gap: 20px;
    max-width: 640px;
  }

  .dili-editor-title {
    margin: 0;
    font-size: 22px;
    font-weight: 700;
  }

  .dili-editor .dili-toggle-row {
    padding: 0;
  }

  .dili-field {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .dili-field label {
    font-size: 14px;
    font-weight: 600;
  }

  .dili-field input {
    height: 46px;
    padding: 0 14px;
    border-radius: 12px;
    border: 1px solid var(--color-border-strong);
    background: var(--color-bg-base);
    color: var(--color-text-base);
    font: inherit;
  }

  .dili-field input:focus {
    outline: 2px solid var(--color-primary);
    outline-offset: 1px;
  }

  .dili-error {
    margin: 0;
    color: var(--color-danger);
    font-size: 14px;
  }

  .dili-editor-actions {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }

  .dili-grow {
    flex-grow: 1;
  }

  .dili-pill {
    height: 44px;
    padding: 0 20px;
    border-radius: 22px;
    border: none;
    background: var(--color-primary);
    color: var(--color-on-primary);
    font: inherit;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-pill:disabled {
    opacity: 0.5;
    cursor: default;
  }

  .dili-pill-outline {
    background: transparent;
    color: var(--color-text-base);
    border: 1px solid var(--color-border-strong);
  }

  .dili-pill-danger {
    background: transparent;
    color: var(--color-danger);
    border: 1px solid var(--color-danger);
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
  .dili-preset-side {
    display: flex;
    align-items: center;
    gap: 14px;
    flex-shrink: 0;
  }

  .dili-state {
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text-muted);
    min-width: 92px;
    text-align: right;
  }

  .dili-link {
    border: none;
    background: none;
    padding: 0 4px;
    min-height: 36px;
    font: inherit;
    font-size: 13px;
    font-weight: 600;
    color: var(--color-primary);
    cursor: pointer;
  }

  .dili-picture {
    display: flex;
    gap: 18px;
    align-items: center;
  }

  .dili-picture-preview {
    width: 90px;
    aspect-ratio: 3 / 4;
    border-radius: 10px;
    overflow: hidden;
    background: var(--color-bg-muted);
    flex-shrink: 0;
  }

  .dili-picture-preview img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }

  .dili-picture-side {
    display: flex;
    flex-direction: column;
    align-items: flex-start;
    gap: 6px;
  }

  .dili-pill-small {
    height: 36px;
    padding: 0 14px;
    font-size: 13px;
  }

  .dili-covers {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 16px;
    border: 1px solid var(--color-border);
    border-radius: 14px;
  }

  .dili-cover-search {
    display: flex;
    gap: 10px;
  }

  .dili-cover-search input {
    flex-grow: 1;
    height: 36px;
    padding: 0 12px;
    border-radius: 10px;
    border: 1px solid var(--color-border-strong);
    background: var(--color-bg-base);
    color: var(--color-text-base);
    font: inherit;
  }

  .dili-cover-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(90px, 1fr));
    gap: 10px;
    max-height: 360px;
    overflow-y: auto;
  }

  .dili-cover-pick {
    padding: 0;
    border: 2px solid transparent;
    border-radius: 10px;
    overflow: hidden;
    background: var(--color-bg-muted);
    aspect-ratio: 3 / 4;
    cursor: pointer;
  }

  .dili-cover-pick:hover {
    border-color: var(--color-primary);
  }

  .dili-cover-pick img {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
</style>
