<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Applications</h1>
      <p>What your devices can start from Moonlight.</p>
    </header>

    <div v-if="loading" class="dili-muted">Loading…</div>

    <!-- ===================== Editor ===================== -->
    <template v-else-if="editing">
      <section class="dili-editor">
        <!-- Top bar, like an iOS sheet -->
        <div class="dili-editor-bar">
          <button type="button" class="dili-text-btn" @click="cancelEdit">Cancel</button>
          <h2>{{ editing.index === -1 ? 'New app' : (editing.lockName ? 'Picture' : editing.original.name || 'App') }}</h2>
          <button type="button" class="dili-pill" :disabled="(!editing.lockName && !editing.name.trim()) || busy" @click="saveEditing">Save</button>
        </div>
        <p v-if="error" class="dili-error">{{ error }}</p>

        <div class="dili-editor-body">
          <!-- Left: picture -->
          <aside class="dili-editor-side">
            <div class="dili-cover dili-cover-large">
              <img v-if="editing.pictureUrl" :src="editing.pictureUrl" alt="">
              <img v-else-if="editing.index !== -1" :src="`./api/covers/${editing.index}`" alt="" @error="$event.target.style.display = 'none'">
            </div>
            <button type="button" class="dili-pill dili-pill-outline dili-full" @click="openCoverSearch">Find a picture</button>
            <button type="button" class="dili-text-btn dili-full" @click="openBrowser('file', pickLocalPicture)">Use a picture from this PC</button>
            <p class="dili-help dili-center">{{ editing.autoPicked ? 'Found automatically. Change it any time.' : 'Shown for this app in Moonlight.' }}</p>
          </aside>

          <!-- Right: settings -->
          <div class="dili-editor-main">
            <div v-if="coverSearch" class="dili-group">
              <div class="dili-group-row">
                <input v-model="coverSearch.query" type="text" class="dili-input" placeholder="Game name" @keyup.enter="runCoverSearch">
                <button type="button" class="dili-pill dili-pill-small" @click="runCoverSearch">Search</button>
                <button type="button" class="dili-icon-btn" aria-label="Close" @click="coverSearch = null"><X :size="18"></X></button>
              </div>
              <div class="dili-group-pad">
                <div v-if="coverSearch.loading" class="dili-help">Searching…</div>
                <div v-else-if="coverSearch.results.length === 0" class="dili-help">No pictures found. Try a shorter name.</div>
                <div class="dili-cover-grid">
                  <button v-for="c in coverSearch.results" :key="c.key" type="button" class="dili-cover-pick" :title="c.name" @click="pickCover(c)">
                    <img :src="c.url" :alt="c.name" loading="lazy">
                  </button>
                </div>
              </div>
            </div>

            <div v-if="browser" class="dili-group">
              <div class="dili-group-row">
                <button type="button" class="dili-icon-btn" :disabled="!browser.parent" aria-label="Up one folder" @click="browse(browser.parent)"><ArrowUp :size="18"></ArrowUp></button>
                <span class="dili-browser-path">{{ browser.path || '/' }}</span>
                <button v-if="browser.type === 'directory'" type="button" class="dili-pill dili-pill-small" @click="chooseBrowsed(browser.path)">Choose this folder</button>
                <button type="button" class="dili-icon-btn" aria-label="Close" @click="browser = null"><X :size="18"></X></button>
              </div>
              <div v-if="browser.error" class="dili-group-pad dili-error">{{ browser.error }}</div>
              <div class="dili-browser-list">
                <button v-for="e in browser.entries" :key="e.path" type="button" class="dili-browser-item" @click="e.type === 'directory' ? browse(e.path) : chooseBrowsed(e.path)">
                  <Folder v-if="e.type === 'directory'" :size="18"></Folder>
                  <FileIcon v-else :size="18"></FileIcon>
                  <span>{{ e.name }}</span>
                </button>
                <div v-if="!browser.loading && browser.entries.length === 0" class="dili-group-pad dili-help">This folder is empty.</div>
              </div>
            </div>

            <template v-if="!editing.lockName">
              <button v-if="!picker" type="button" class="dili-choose" @click="openPicker">
                <Search :size="20"></Search>
                <span>
                  <strong>Choose from this PC</strong>
                  <span class="dili-help">Pick an installed game or app. Dili fills in everything for you.</span>
                </span>
              </button>

              <div v-if="picker" class="dili-group">
                <div class="dili-group-row">
                  <input v-model="picker.query" type="text" class="dili-input" placeholder="Search your games and apps" autofocus>
                  <button type="button" class="dili-icon-btn" aria-label="Close" @click="picker = null"><X :size="18"></X></button>
                </div>
                <div class="dili-picker-list">
                  <div v-if="picker.loading" class="dili-group-pad dili-help">Looking for games and apps…</div>
                  <template v-else>
                    <div v-if="pickerGames.length" class="dili-picker-title">Steam games</div>
                    <button v-for="g in pickerGames" :key="'g' + g.command" type="button" class="dili-browser-item" @click="pickInstalled(g)">
                      <Gamepad2 :size="18"></Gamepad2><span>{{ g.name }}</span>
                    </button>
                    <div v-if="pickerApps.length" class="dili-picker-title">Games and apps</div>
                    <button v-for="a in pickerApps" :key="'a' + a.name" type="button" class="dili-browser-item" @click="pickInstalled(a)">
                      <Gamepad2 v-if="a.kind === 'game'" :size="18"></Gamepad2>
                      <AppWindow v-else :size="18"></AppWindow>
                      <span>{{ a.name }}</span>
                    </button>
                    <div v-if="!pickerGames.length && !pickerApps.length" class="dili-group-pad dili-help">Nothing found.</div>
                  </template>
                </div>
              </div>

              <!-- Basics -->
              <div class="dili-group">
                <label class="dili-group-row dili-labeled">
                  <span class="dili-row-label">Name</span>
                  <input v-model="editing.name" type="text" class="dili-input" placeholder="As shown in Moonlight" @blur="autoPicture">
                </label>
                <div class="dili-divider"></div>
                <div class="dili-group-row dili-labeled">
                  <span class="dili-row-label">Program</span>
                  <input v-model="editing.command" type="text" class="dili-input" placeholder="Leave empty for the desktop" @input="testResult = null">
                  <button type="button" class="dili-pill dili-pill-outline dili-pill-small" @click="openBrowser('file', (p) => (editing.command = quote(p)))">Browse…</button>
                  <button type="button" class="dili-pill dili-pill-outline dili-pill-small" :disabled="!editing.command.trim() || testing" @click="tryCommand">
                    {{ testing ? 'Starting…' : 'Try it' }}
                  </button>
                </div>
                <div v-if="testResult" class="dili-group-pad dili-small-pad" :class="testResult.ok ? 'dili-ok-text' : 'dili-error'">
                  {{ testResult.text }}
                </div>
                <div class="dili-divider"></div>
                <div class="dili-group-row">
                  <div class="dili-grow">
                    <div class="dili-row-title">Close it when the stream ends</div>
                    <div class="dili-help">If you quit the program, the stream ends too.</div>
                  </div>
                  <button type="button" class="dili-switch" :class="{ on: editing.closeOnEnd }" :aria-pressed="editing.closeOnEnd ? 'true' : 'false'"
                          aria-label="Close it when the stream ends" @click="editing.closeOnEnd = !editing.closeOnEnd"><span></span></button>
                </div>
              </div>

              <!-- More options -->
              <button type="button" class="dili-disclosure" :aria-expanded="showMore ? 'true' : 'false'" @click="showMore = !showMore">
                <ChevronRight :size="18" :class="{ open: showMore }"></ChevronRight>
                More options
              </button>

              <template v-if="showMore">
                <div class="dili-group-title">When the stream starts and ends</div>
                <div class="dili-group">
                  <div class="dili-group-row dili-pair-head">
                    <span>At the start</span><span>At the end</span><span></span>
                  </div>
                  <template v-for="(row, i) in editing.prep" :key="'p' + i">
                    <div class="dili-divider"></div>
                    <div class="dili-group-row dili-pair">
                      <input v-model="row.do" type="text" class="dili-input" placeholder="Optional">
                      <input v-model="row.undo" type="text" class="dili-input" placeholder="Optional">
                      <button type="button" class="dili-icon-btn" aria-label="Remove" @click="editing.prep.splice(i, 1)"><X :size="18"></X></button>
                    </div>
                  </template>
                  <div class="dili-divider"></div>
                  <button type="button" class="dili-group-row dili-add-row" @click="editing.prep.push({ do: '', undo: '' })">+ Add a command</button>
                </div>
                <p class="dili-group-note">For example, turn a monitor off at the start and back on at the end.</p>

                <div class="dili-group-title">Also open in the background</div>
                <div class="dili-group">
                  <template v-for="(cmd, i) in editing.background" :key="'b' + i">
                    <div class="dili-group-row">
                      <input v-model="editing.background[i]" type="text" class="dili-input" placeholder="Program">
                      <button type="button" class="dili-icon-btn" aria-label="Remove" @click="editing.background.splice(i, 1)"><X :size="18"></X></button>
                    </div>
                    <div class="dili-divider"></div>
                  </template>
                  <button type="button" class="dili-group-row dili-add-row" @click="editing.background.push('')">+ Add a program</button>
                </div>
                <p class="dili-group-note">Programs that start with the app and keep running on their own.</p>

                <div class="dili-group-title">Behavior</div>
                <div class="dili-group">
                  <div class="dili-group-row dili-labeled">
                    <span class="dili-row-label">Start in</span>
                    <input v-model="editing.workingDir" type="text" class="dili-input" placeholder="Folder, usually not needed">
                    <button type="button" class="dili-pill dili-pill-outline dili-pill-small" @click="openBrowser('directory', (p) => (editing.workingDir = p))">Browse…</button>
                  </div>
                  <div class="dili-divider"></div>
                  <div class="dili-group-row">
                    <div class="dili-grow">
                      <div class="dili-row-title">Keep streaming if the program closes right away</div>
                      <div class="dili-help">For launchers that open another window and then close themselves.</div>
                    </div>
                    <button type="button" class="dili-switch" :class="{ on: editing.autoDetach }" @click="editing.autoDetach = !editing.autoDetach"
                            :aria-pressed="editing.autoDetach ? 'true' : 'false'" aria-label="Keep streaming if the program closes right away"><span></span></button>
                  </div>
                  <div class="dili-divider"></div>
                  <div class="dili-group-row">
                    <div class="dili-grow">
                      <div class="dili-row-title">Wait for every part of the program</div>
                      <div class="dili-help">Keep streaming until all its windows have closed, not just the first one.</div>
                    </div>
                    <button type="button" class="dili-switch" :class="{ on: editing.waitAll }" @click="editing.waitAll = !editing.waitAll"
                            :aria-pressed="editing.waitAll ? 'true' : 'false'" aria-label="Wait for every part of the program"><span></span></button>
                  </div>
                  <div class="dili-divider"></div>
                  <div class="dili-group-row">
                    <div class="dili-grow">
                      <div class="dili-row-title">Use the commands for all apps</div>
                      <div class="dili-help">Also run the start and end commands set up for every app.</div>
                    </div>
                    <button type="button" class="dili-switch" :class="{ on: editing.globalPrep }" @click="editing.globalPrep = !editing.globalPrep"
                            :aria-pressed="editing.globalPrep ? 'true' : 'false'" aria-label="Use the commands for all apps"><span></span></button>
                  </div>
                  <div class="dili-divider"></div>
                  <label class="dili-group-row">
                    <span class="dili-grow dili-row-title">Seconds to wait when closing it</span>
                    <input v-model.number="editing.exitTimeout" type="number" min="0" max="120" class="dili-input dili-input-small">
                  </label>
                </div>

                <div class="dili-group-title">Troubleshooting</div>
                <div class="dili-group">
                  <div class="dili-group-row dili-labeled">
                    <span class="dili-row-label">Log file</span>
                    <input v-model="editing.output" type="text" class="dili-input" placeholder="Save the program's messages to a file">
                  </div>
                </div>
              </template>

              <div v-if="editing.index !== -1" class="dili-group dili-danger-group">
                <button type="button" class="dili-group-row dili-add-row dili-center-row" @click="duplicateEditing">Duplicate app</button>
                <div class="dili-divider"></div>
                <button type="button" class="dili-group-row dili-danger-row" @click="removeEditing">
                  {{ confirmDelete ? 'Tap again to remove this app' : 'Remove app' }}
                </button>
              </div>
            </template>
          </div>
        </div>
      </section>
    </template>

    <!-- ===================== Lists ===================== -->
    <template v-else>
      <section class="dili-section">
        <h2>Ready-made apps</h2>
        <p class="dili-help">Found on this PC. Switch on the ones you want to see in Moonlight, and Dili sets them up for you.</p>
        <div class="dili-panel">
          <template v-for="(preset, i) in installedPresets" :key="preset.id">
            <div v-if="i > 0" class="dili-divider"></div>
            <div class="dili-toggle-row">
              <div class="dili-preset">
                <img v-if="preset.icon" :src="preset.icon" alt="" class="dili-preset-img">
                <div v-else class="dili-preset-img dili-preset-letter">{{ preset.app.name.charAt(0) }}</div>
                <div>
                  <div class="dili-row-title">{{ preset.app.name }}</div>
                  <div class="dili-help">{{ preset.description }}</div>
                </div>
              </div>
              <div class="dili-preset-side">
                <button v-if="presetIndex(preset) !== -1" type="button" class="dili-link" @click="startEdit(presetIndex(preset), true)">Picture</button>
                <span class="dili-state" :class="{ on: presetIndex(preset) !== -1 }">{{ presetIndex(preset) !== -1 ? 'In Moonlight' : 'Not added' }}</span>
                <button type="button" class="dili-switch" :class="{ on: presetIndex(preset) !== -1 }"
                        :aria-pressed="presetIndex(preset) !== -1 ? 'true' : 'false'" :aria-label="'Show ' + preset.app.name + ' in Moonlight'"
                        :disabled="busy" @click="togglePreset(preset)"><span></span></button>
              </div>
            </div>
          </template>
        </div>
        <p v-if="missingPresets.length" class="dili-help">
          Not found on this PC: {{ missingPresets.map((p) => p.app.name).join(', ') }}. Install them and they show up here.
        </p>
        <p class="dili-help">Moonlight shows changes the next time you open your PC in Moonlight.</p>
      </section>

      <section class="dili-section">
        <h2>Your apps</h2>
        <div class="dili-grid">
          <button v-for="app in ownApps" :key="app.index" type="button" class="dili-app" @click="startEdit(app.index)">
            <div class="dili-cover">
              <img :src="`./api/covers/${app.index}`" alt="" loading="lazy" @error="$event.target.style.display = 'none'">
              <span v-if="runningApp === app.name" class="dili-live">Playing now</span>
            </div>
            <div class="dili-app-name">{{ app.name }}</div>
            <div class="dili-help dili-ellipsis">{{ summaryOf(app) }}</div>
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
  import { loadPresets, saveApp as savePresetApp } from './presets'
  import { searchCovers, useCover } from './covers'
  import { AppWindow, ArrowUp, ChevronRight, File as FileIcon, Folder, Gamepad2, Plus, Search, X } from '@lucide/vue'

  export default {
    components: {
      Navbar,
      AppWindow,
      Gamepad2,
      Search,
      ArrowUp,
      ChevronRight,
      FileIcon,
      Folder,
      Plus,
      X,
    },
    data() {
      return {
        loading: true,
        apps: [],
        presets: [],
        runningApp: '',
        editing: null,
        showMore: false,
        coverSearch: null,
        browser: null,
        picker: null,
        installed: null,
        testing: false,
        testResult: null,
        confirmDelete: false,
        busy: false,
        error: '',
      };
    },
    computed: {
      pickerGames() {
        return this.filterInstalled(this.installed ? this.installed.games : []);
      },
      pickerApps() {
        const apps = this.filterInstalled(this.installed ? this.installed.apps : []);
        // Games first, then everything else, alphabetically
        return [...apps].sort((a, b) => (a.kind === 'game') === (b.kind === 'game') ? a.name.localeCompare(b.name) : (a.kind === 'game' ? -1 : 1));
      },
      installedPresets() {
        return this.presets.filter((p) => p.installed || this.presetIndex(p) !== -1);
      },
      missingPresets() {
        return this.presets.filter((p) => !p.installed && this.presetIndex(p) === -1);
      },
      ownApps() {
        const presetNames = this.presets.map((p) => p.app.name);
        return this.apps.map((app, index) => ({ ...app, index })).filter((app) => !presetNames.includes(app.name));
      },
    },
    async created() {
      try {
        this.presets = await loadPresets();
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
        return this.apps.findIndex((a) => a.name === preset.app.name);
      },
      commandOf(app) {
        if (app.cmd) return app.cmd;
        if (app.detached && app.detached.length) return app.detached[0];
        return '';
      },
      summaryOf(app) {
        const cmd = this.commandOf(app);
        if (cmd) return cmd;
        const firstDo = (app['prep-cmd'] || []).map((p) => p.do).find(Boolean);
        return firstDo || 'Desktop only';
      },
      quote(path) {
        return /\s/.test(path) ? `"${path}"` : path;
      },
      async deleteApp(index) {
        const r = await apiFetch('./api/apps/' + index, { method: 'DELETE', headers: { 'Content-Type': 'application/json' } });
        return r.status === 200;
      },
      async togglePreset(preset) {
        this.busy = true;
        try {
          const index = this.presetIndex(preset);
          if (index === -1) {
            await savePresetApp(preset.app, -1);
          } else {
            await this.deleteApp(index);
          }
          await this.loadApps();
        } finally {
          this.busy = false;
        }
      },

      // ---------- editor ----------
      startAdd() {
        this.resetEditorState();
        this.editing = this.makeEditing(-1, {}, false);
      },
      startEdit(index, lockName = false) {
        this.resetEditorState();
        this.editing = this.makeEditing(index, this.apps[index], lockName);
        // Show the extra options right away when the app already uses them
        this.showMore = !lockName && (this.editing.prep.length > 0 || this.editing.background.length > 0 || !!this.editing.workingDir);
      },
      resetEditorState() {
        this.picker = null;
        this.testResult = null;
        this.error = '';
        this.confirmDelete = false;
        this.coverSearch = null;
        this.browser = null;
        this.showMore = false;
      },
      makeEditing(index, app, lockName) {
        const detached = app.detached || [];
        const usesCmd = !!app.cmd;
        return {
          index,
          original: app,
          lockName,
          name: app.name || '',
          command: this.commandOf(app),
          closeOnEnd: usesCmd || detached.length === 0,
          prep: (app['prep-cmd'] || []).map((p) => ({ do: p.do || '', undo: p.undo || '' })),
          background: usesCmd ? [...detached] : detached.slice(1),
          workingDir: app['working-dir'] || '',
          autoDetach: app['auto-detach'] !== false,
          waitAll: app['wait-all'] !== false,
          globalPrep: !app['exclude-global-prep-cmd'],
          exitTimeout: app['exit-timeout'] ?? 5,
          output: app.output || '',
          imagePath: '',
          pictureUrl: '',
          autoPicked: false,
        };
      },
      filterInstalled(list) {
        const q = this.picker ? this.picker.query.trim().toLowerCase() : '';
        return q ? list.filter((x) => x.name.toLowerCase().includes(q)) : list;
      },
      async openPicker() {
        this.picker = { query: '', loading: !this.installed };
        if (!this.installed) {
          try {
            this.installed = await fetch('./api/installed').then((r) => r.json());
          } catch (e) {
            this.installed = { games: [], apps: [] };
          }
          if (this.picker) this.picker.loading = false;
        }
      },
      pickInstalled(item) {
        this.editing.name = item.name;
        this.editing.command = item.command;
        // Steam starts the game and quits right away, so Dili just starts it
        this.editing.closeOnEnd = item.kind !== 'steam';
        this.picker = null;
        this.testResult = null;
        this.autoPicture();
      },
      async autoPicture() {
        const e = this.editing;
        if (!e || e.imagePath || e.pictureUrl || !e.name.trim()) return;
        if (e.index !== -1 && e.original['image-path'] && !['box.png', 'desktop.png'].includes(e.original['image-path'])) return;
        try {
          const results = await searchCovers(e.name.trim());
          if (results.length && this.editing === e && !e.imagePath) {
            e.imagePath = await useCover(results[0]);
            e.pictureUrl = results[0].url;
            e.autoPicked = true;
          }
        } catch (err) {
          // No picture found, that's fine
        }
      },
      async tryCommand() {
        this.testing = true;
        this.testResult = null;
        try {
          const r = await apiFetch('./api/apps/test', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ command: this.editing.command.trim() }),
          });
          const result = await r.json();
          if (!result.found) {
            this.testResult = { ok: false, text: "Dili can't find this program on your PC. Check the spelling, or use Choose from this PC." };
          } else {
            this.testResult = { ok: true, text: 'Found it and started it on your PC. Close it again when you are done checking.' };
          }
        } catch (err) {
          this.testResult = { ok: false, text: 'The test did not work. Please try again.' };
        } finally {
          this.testing = false;
        }
      },
      duplicateEditing() {
        const copy = { ...this.buildApp(), name: `${this.editing.name.trim() || this.editing.original.name} (copy)` };
        this.resetEditorState();
        this.editing = this.makeEditing(-1, copy, false);
        this.showMore = this.editing.prep.length > 0 || this.editing.background.length > 0 || !!this.editing.workingDir;
      },
      cancelEdit() {
        this.editing = null;
        this.browser = null;
      },
      buildApp() {
        const e = this.editing;
        const app = { ...e.original };
        if (e.imagePath) app['image-path'] = e.imagePath;
        if (e.lockName) return app;

        app.name = e.name.trim();
        const command = e.command.trim();
        const background = e.background.map((c) => c.trim()).filter(Boolean);
        delete app.cmd;
        if (command && e.closeOnEnd) {
          app.cmd = command;
          app.detached = background;
        } else {
          app.detached = command ? [command, ...background] : background;
        }
        app['prep-cmd'] = e.prep.map((p) => ({ do: p.do.trim(), undo: p.undo.trim() })).filter((p) => p.do || p.undo);
        if (e.workingDir.trim()) app['working-dir'] = e.workingDir.trim();
        else delete app['working-dir'];
        app['auto-detach'] = e.autoDetach;
        app['wait-all'] = e.waitAll;
        app['exclude-global-prep-cmd'] = !e.globalPrep;
        app['exit-timeout'] = Number.isFinite(e.exitTimeout) ? e.exitTimeout : 5;
        if (e.output.trim()) app.output = e.output.trim();
        else delete app.output;
        if (!app['image-path']) app['image-path'] = command ? 'box.png' : 'desktop.png';
        return app;
      },
      async saveEditing() {
        this.busy = true;
        this.error = '';
        try {
          if (await savePresetApp(this.buildApp(), this.editing.index)) {
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

      // ---------- pictures ----------
      openCoverSearch() {
        this.coverSearch = { query: this.editing.name || this.editing.original.name || '', loading: false, results: [] };
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
          this.coverSearch = null;
        } catch (e) {
          this.error = 'The picture could not be downloaded. Please try another one.';
        }
      },

      pickLocalPicture(path) {
        this.editing.imagePath = path;
        this.editing.pictureUrl = '';
        this.error = /\.png$/i.test(path) ? '' : 'Moonlight needs a PNG picture. Please choose a .png file.';
        if (this.error) this.editing.imagePath = '';
      },

      // ---------- file browser ----------
      openBrowser(type, onPick) {
        this.browser = { type, onPick, path: '', parent: '', entries: [], loading: false, error: '' };
        this.browse('');
      },
      async browse(path) {
        const b = this.browser;
        b.loading = true;
        b.error = '';
        try {
          const params = new URLSearchParams({ type: b.type });
          if (path) params.set('path', path);
          const r = await fetch(`./api/browse?${params.toString()}`);
          const data = await r.json();
          if (!r.ok) throw new Error(data.error || 'This folder cannot be opened.');
          b.path = data.path || '';
          b.parent = data.parent || '';
          b.entries = data.entries || [];
        } catch (e) {
          b.error = e.message;
        } finally {
          b.loading = false;
        }
      },
      chooseBrowsed(path) {
        this.browser.onPick(path);
        this.browser = null;
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
  .dili-state {
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text-muted);
    min-width: 92px;
    text-align: right;
  }

  .dili-state.on {
    color: var(--color-success);
  }

  .dili-preset-letter {
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 22px;
    font-weight: 700;
    color: var(--color-text-muted);
  }

  .dili-inline {
    display: flex;
    gap: 10px;
    align-items: center;
  }

  .dili-inline input,
  .dili-pair input,
  .dili-narrow input {
    flex-grow: 1;
    min-width: 0;
    height: 42px;
    padding: 0 12px;
    border-radius: 10px;
    border: 1px solid var(--color-border-strong);
    background: var(--color-bg-base);
    color: var(--color-text-base);
    font: inherit;
  }

  .dili-pair {
    display: grid;
    grid-template-columns: 1fr 1fr auto;
    gap: 10px;
    align-items: center;
  }

  .dili-narrow input {
    max-width: 120px;
  }

  .dili-icon-btn {
    width: 40px;
    height: 40px;
    flex-shrink: 0;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--color-border-strong);
    border-radius: 10px;
    background: transparent;
    color: var(--color-text-base);
    cursor: pointer;
  }

  .dili-icon-btn:disabled {
    opacity: 0.4;
    cursor: default;
  }

  .dili-disclosure {
    align-self: flex-start;
    display: inline-flex;
    align-items: center;
    gap: 6px;
    min-height: 40px;
    padding: 0;
    border: none;
    background: none;
    color: var(--color-text-base);
    font: inherit;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-disclosure svg {
    transition: transform 0.15s ease;
  }

  .dili-disclosure svg.open {
    transform: rotate(90deg);
  }

  .dili-more {
    display: flex;
    flex-direction: column;
    gap: 22px;
    padding-left: 14px;
    border-left: 2px solid var(--color-border);
  }

  .dili-box {
    display: flex;
    flex-direction: column;
    gap: 12px;
    padding: 16px;
    border: 1px solid var(--color-border);
    border-radius: 14px;
  }

  .dili-browser {
    padding: 16px;
    display: flex;
    flex-direction: column;
    gap: 12px;
    max-width: 640px;
  }

  .dili-browser-head {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .dili-browser-path {
    flex-grow: 1;
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    font-size: 13px;
    color: var(--color-text-muted);
  }

  .dili-browser-list {
    display: flex;
    flex-direction: column;
    max-height: 360px;
    overflow-y: auto;
  }

  .dili-browser-item {
    display: flex;
    align-items: center;
    gap: 10px;
    min-height: 40px;
    padding: 0 10px;
    border: none;
    border-radius: 8px;
    background: none;
    color: var(--color-text-base);
    font: inherit;
    text-align: left;
    cursor: pointer;
  }

  .dili-browser-item:hover {
    background: var(--color-bg-subtle);
  }
  /* ---------- editor (grouped, iOS settings style) ---------- */
  .dili-editor {
    display: flex;
    flex-direction: column;
    gap: 20px;
  }

  .dili-editor-bar {
    position: sticky;
    top: 0;
    z-index: 5;
    display: grid;
    grid-template-columns: 1fr auto 1fr;
    align-items: center;
    gap: 12px;
    padding: 12px 0;
    background: var(--color-bg-base);
    border-bottom: 1px solid var(--color-border);
  }

  .dili-editor-bar h2 {
    margin: 0;
    font-size: 17px;
    font-weight: 700;
    text-align: center;
  }

  .dili-editor-bar .dili-text-btn {
    justify-self: start;
  }

  .dili-editor-bar .dili-pill {
    justify-self: end;
  }

  .dili-editor-body {
    display: grid;
    grid-template-columns: 240px minmax(0, 1fr);
    gap: 32px;
    align-items: start;
  }

  @media (max-width: 900px) {
    .dili-editor-body {
      grid-template-columns: minmax(0, 1fr);
    }
  }

  .dili-editor-side {
    display: flex;
    flex-direction: column;
    gap: 10px;
    position: sticky;
    top: 80px;
  }

  .dili-cover-large {
    width: 100%;
  }

  .dili-full {
    width: 100%;
    justify-content: center;
  }

  .dili-center {
    text-align: center;
  }

  .dili-editor-main {
    display: flex;
    flex-direction: column;
    gap: 10px;
    min-width: 0;
  }

  .dili-text-btn {
    min-height: 40px;
    padding: 0 6px;
    border: none;
    background: none;
    color: var(--color-primary);
    font: inherit;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-group {
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 14px;
    overflow: hidden;
  }

  .dili-group .dili-divider {
    margin: 0 0 0 18px;
  }

  .dili-group-row {
    display: flex;
    align-items: center;
    gap: 12px;
    min-height: 52px;
    padding: 8px 18px;
    width: 100%;
    box-sizing: border-box;
  }

  .dili-labeled .dili-row-label {
    flex: 0 0 90px;
    font-size: 15px;
    font-weight: 600;
  }

  .dili-input {
    flex-grow: 1;
    min-width: 0;
    height: 38px;
    padding: 0 10px;
    border-radius: 9px;
    border: 1px solid transparent;
    background: var(--color-bg-subtle);
    color: var(--color-text-base);
    font: inherit;
    font-size: 15px;
  }

  .dili-input:focus {
    outline: 2px solid var(--color-primary);
    outline-offset: 0;
  }

  .dili-input-small {
    flex-grow: 0;
    width: 90px;
  }

  .dili-group-pad {
    padding: 8px 18px 16px 18px;
  }

  .dili-group-title {
    margin: 18px 0 0 18px;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.04em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .dili-group-note {
    margin: 0 18px;
    font-size: 13px;
    color: var(--color-text-muted);
  }

  .dili-pair,
  .dili-pair-head {
    display: grid;
    grid-template-columns: 1fr 1fr 40px;
  }

  .dili-pair-head {
    min-height: 36px;
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text-muted);
  }

  .dili-add-row {
    border: none;
    background: none;
    color: var(--color-primary);
    font: inherit;
    font-size: 15px;
    font-weight: 600;
    text-align: left;
    cursor: pointer;
  }

  .dili-add-row:hover {
    background: var(--color-bg-subtle);
  }

  .dili-danger-group {
    margin-top: 18px;
  }

  .dili-danger-row {
    justify-content: center;
    border: none;
    background: none;
    color: var(--color-danger);
    font: inherit;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-danger-row:hover {
    background: var(--color-bg-subtle);
  }

  .dili-editor .dili-disclosure {
    margin-top: 8px;
  }
  .dili-choose {
    display: flex;
    align-items: center;
    gap: 14px;
    width: 100%;
    padding: 16px 18px;
    border: 1px solid var(--color-primary);
    border-radius: 14px;
    background: var(--color-surface);
    color: var(--color-text-base);
    font: inherit;
    text-align: left;
    cursor: pointer;
  }

  .dili-choose svg {
    color: var(--color-primary);
    flex-shrink: 0;
  }

  .dili-choose > span {
    display: flex;
    flex-direction: column;
    gap: 2px;
  }

  .dili-choose:hover {
    background: var(--color-bg-subtle);
  }

  .dili-picker-list {
    max-height: 380px;
    overflow-y: auto;
    padding: 0 8px 8px 8px;
  }

  .dili-picker-title {
    margin: 12px 10px 4px 10px;
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 0.04em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .dili-small-pad {
    padding-top: 0;
    font-size: 13px;
  }

  .dili-ok-text {
    color: var(--color-success);
  }

  .dili-center-row {
    justify-content: center;
  }
</style>
