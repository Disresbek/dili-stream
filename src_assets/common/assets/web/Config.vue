<template>
  <Navbar></Navbar>
  <div id="content" class="container">
    <div class="dili-adv-head">
      <h1>Advanced</h1>
      <p>Everything Dili can do, for when you want to fine-tune. The important settings are on
        <RouterLink to="/displays">Display &amp; Quality</RouterLink>.</p>
    </div>

    <!-- Search Bar with Autocomplete -->
    <div class="toolbar mb-3 d-flex flex-wrap align-items-center gap-3">
      <div class="input-group config-search">
        <label for="config-search" class="visually-hidden">{{ $t('config.search_options') }}</label>
        <span class="input-group-text">
          <search :size="18" class="icon"></search>
        </span>
        <input
          id="config-search"
          type="text"
          class="form-control"
          v-model="searchQuery"
          :placeholder="$t('config.search_options')"
          @input="handleSearch"
          list="config-options"
        />
      </div>
      <datalist id="config-options">
        <option v-for="option in allConfigOptions" :key="option.key" :value="option.label">
          {{ option.tab }} - {{ option.label }}
        </option>
      </datalist>
      <span v-if="searchQuery && searchResults.length === 0" class="text-muted small flex-shrink-0">
        No results found for "{{ searchQuery }}"
      </span>
      <span v-else-if="searchQuery" class="text-muted small flex-shrink-0">
        Found {{ searchResults.length }} result(s)
      </span>
    </div>

    <div class="form" v-if="config">
      <div class="config-layout">
        <!-- Sidebar navigation -->
        <nav class="config-nav">
          <template v-for="section in navSections" :key="section.id">
            <div v-if="section.title" class="config-nav-heading">{{ section.title }}</div>
            <ul class="nav config-nav-list">
              <li class="nav-item" v-for="tab in section.tabs" :key="tab.id">
                <button type="button" class="nav-link" :class="{'active': tab.id === currentTab}"
                  @click="currentTab = tab.id">
                  <component :is="getTabIcon(tab.id)" :size="18" class="icon"></component>
                  {{ tabName(tab) }}
                  <span v-if="tab.id === activeEncoderTab" class="dili-in-use">In use</span>
                </button>
              </li>
            </ul>
            <button v-if="section.id === 'encoder' && activeEncoderTab && encoderTabs.length > 1" type="button"
              class="dili-nav-more" @click="showAllEncoders = !showAllEncoders">
              {{ showAllEncoders ? 'Show only the encoder in use' : 'Show all encoders' }}
            </button>
          </template>

        </nav>

        <!-- Tab content -->
        <div class="config-content">
          <div class="alert alert-success mb-4" v-if="saved && !restarted">
            <b>{{ $t('_common.success') }}</b> {{ $t('config.apply_note') }}
          </div>
          <div class="alert alert-success mb-4" v-if="restarted">
            <b>{{ $t('_common.success') }}</b> {{ $t('config.restart_note') }}
          </div>

      <!-- General Tab -->
      <general
        v-if="currentTab === 'general'"
        :config="config"
        :platform="platform">
      </general>

      <!-- Input Tab -->
      <inputs
        v-if="currentTab === 'input'"
        :config="config"
        :platform="platform">
      </inputs>

      <!-- Audio/Video Tab -->
      <audio-video
        v-if="currentTab === 'av'"
        :config="config"
        :platform="platform"
      >
      </audio-video>

      <!-- Network Tab -->
      <network
        v-if="currentTab === 'network'"
        :config="config"
        :platform="platform">
      </network>

      <!-- Files Tab -->
      <files
        v-if="currentTab === 'files'"
        :config="config"
        :platform="platform">
      </files>

      <!-- Advanced Tab -->
      <advanced
        v-if="currentTab === 'advanced'"
        :config="config"
        :platform="platform">
      </advanced>

      <div v-if="isEncoderTab" class="dili-encoder-note">
        <strong>{{ currentTab === activeEncoderTab ? 'Dili uses this encoder right now.' : 'Dili does not use this encoder on this PC.' }}</strong>
        <span>Dili tests your graphics card every time it starts and picks the fastest encoder automatically
          (right now: {{ activeEncoderLabel }}). These settings only fine-tune it. Most people never need to change them.</span>
      </div>
      <container-encoders
        :current-tab="currentTab"
        :config="config"
        :platform="platform">
      </container-encoders>
        </div>
      </div>
    </div>

    <!-- Appears only when something changed -->
    <div v-if="dirty || applying" class="dili-save-bar" role="region" aria-label="Unsaved changes">
      <span>{{ applying ? 'Saving and restarting Dili…' : 'You have unsaved changes.' }}</span>
      <button type="button" class="dili-bar-btn" :disabled="applying" @click="discardChanges">Discard</button>
      <button type="button" class="dili-bar-btn primary" :disabled="applying" @click="saveAndApply">Save &amp; Apply</button>
    </div>
  </div>
</template>

<script>
  import { computed, toRaw } from 'vue'
  import Navbar from './Navbar.vue'
  import { apiFetch } from './fetch_utils'
  import configTabs from './configs/config_tabs.json'
  import General from './configs/tabs/General.vue'
  import Inputs from './configs/tabs/Inputs.vue'
  import Network from './configs/tabs/Network.vue'
  import Files from './configs/tabs/Files.vue'
  import Advanced from './configs/tabs/Advanced.vue'
  import AudioVideo from './configs/tabs/AudioVideo.vue'
  import ContainerEncoders from './configs/tabs/ContainerEncoders.vue'
  import {
    Check,
    Cpu,
    FileCog,
    Gamepad2,
    Gpu,
    Network as NetworkIcon,
    Save,
    Search,
    Settings,
    Sliders,
    Volume2,
  } from '@lucide/vue'

  // Settings that are managed on other pages (Display & Quality), so no Reset here
  const CHANGED_MARK_SKIP = new Set(["output_name", "vaapi_quality", "nvenc_preset", "sw_preset"]);
  const ENCODER_TAB_IDS = new Set(["nv", "amd", "qsv", "vaapi", "vt", "vulkan", "sw"]);
  // Dili: rarely needed sections, grouped under "Expert"
  const EXPERT_TAB_IDS = new Set(["advanced", "files"]);
  // Friendlier section names
  const TAB_NAMES = {
    general: "General",
    input: "Controls",
    av: "Sound & Picture",
    network: "Network",
    advanced: "Streaming engine",
    files: "Files & paths",
    nv: "NVIDIA",
    amd: "AMD (Windows)",
    qsv: "Intel Quick Sync",
    vaapi: "AMD & Intel (VA-API)",
    vt: "Apple VideoToolbox",
    vulkan: "Vulkan",
    sw: "Software",
  };
  const ENCODER_TO_TAB = { nvenc: "nv", amdvce: "amd", quicksync: "qsv", vaapi: "vaapi", videotoolbox: "vt", vulkan: "vulkan", software: "sw" };
  const ENCODER_LABELS = {
    nvenc: "your NVIDIA graphics card",
    amdvce: "your AMD graphics card",
    quicksync: "your Intel graphics",
    vaapi: "your AMD or Intel graphics card (VA-API)",
    videotoolbox: "your Mac's video encoder",
    vulkan: "your graphics card through Vulkan",
    software: "your processor (software encoding)",
  };

  /**
   * Compare configuration values without coercing their types.
   *
   * @param {*} value Configured value.
   * @param {*} defaultValue Default value for the option.
   * @returns {boolean} Whether both values have the same type and contents.
   */
  function configValuesEqual(value, defaultValue) {
    if (Object.is(value, defaultValue)) {
      return true;
    }
    if (typeof value !== typeof defaultValue || value === null || defaultValue === null || typeof value !== 'object') {
      return false;
    }
    if (Array.isArray(value) !== Array.isArray(defaultValue)) {
      return false;
    }

    const valueKeys = Object.keys(value);
    const defaultKeys = Object.keys(defaultValue);
    return valueKeys.length === defaultKeys.length && valueKeys.every(key =>
      Object.hasOwn(defaultValue, key) && configValuesEqual(value[key], defaultValue[key])
    );
  }

  export default {
    components: {
      Navbar,
      General,
      Inputs,
      Network,
      Files,
      Advanced,
      // They will be accessible via audio-video, container-encoders only.
      AudioVideo,
      ContainerEncoders,
      // icons
      Cpu,
      Check,
      FileCog,
      Gamepad2,
      Gpu,
      NetworkIcon,
      Save,
      Search,
      Settings,
      Sliders,
      Volume2,
    },
    data() {
      return {
        platform: "",
        saved: false,
        restarted: false,
        config: null,
        currentTab: "general",
        searchQuery: "",
        showAllEncoders: false,
        snapshot: "",
        dirty: false,
        applying: false,
        activeEncoder: "",
        hashChangeHandler: null,
        // Keep a private copy because platform filtering replaces this array at runtime.
        tabs: structuredClone(configTabs),
      };
    },
    provide() {
       return {
         platform: computed(() => this.platform),
         searchQuery: computed(() => this.searchQuery),
       }
    },
    computed: {
      generalTabs() {
        return this.tabs.filter(tab => !ENCODER_TAB_IDS.has(tab.id));
      },
      mainTabs() {
        return this.generalTabs.filter(tab => !EXPERT_TAB_IDS.has(tab.id));
      },
      expertTabs() {
        return this.generalTabs.filter(tab => EXPERT_TAB_IDS.has(tab.id));
      },
      activeEncoderTab() {
        return ENCODER_TO_TAB[this.activeEncoder] || '';
      },
      activeEncoderLabel() {
        return ENCODER_LABELS[this.activeEncoder] || 'not decided yet';
      },
      visibleEncoderTabs() {
        if (this.showAllEncoders || !this.activeEncoderTab) return this.encoderTabs;
        return this.encoderTabs.filter(tab => tab.id === this.activeEncoderTab || tab.id === this.currentTab);
      },
      navSections() {
        return [
          { id: 'main', title: '', tabs: this.mainTabs },
          { id: 'encoder', title: 'Encoder', tabs: this.visibleEncoderTabs },
          { id: 'expert', title: 'Expert', tabs: this.expertTabs },
        ].filter((section) => section.tabs.length);
      },
      isEncoderTab() {
        return ENCODER_TAB_IDS.has(this.currentTab);
      },
      encoderTabs() {
        return this.tabs.filter(tab => ENCODER_TAB_IDS.has(tab.id));
      },
      allConfigOptions() {
        const options = [];
        this.tabs.forEach(tab => {
          Object.keys(tab.options).forEach(key => {
            options.push({
              key: key,
              label: key.replaceAll('_', ' ').replaceAll(/\b\w/g, l => l.toUpperCase()),
              tab: this.$t(tab.nameKey),
              tabId: tab.id
            });
          });
        });
        return options;
      },
      searchResults() {
        if (!this.searchQuery) return [];
        const query = this.searchQuery.toLowerCase();
        return this.allConfigOptions.filter(option =>
          option.key.toLowerCase().includes(query) ||
          option.label.toLowerCase().includes(query)
        );
      }
    },
    watch: {
      config: {
        deep: true,
        handler() {
          if (this.snapshot) {
            this.dirty = JSON.stringify(this.serialize()) !== this.snapshot;
          }
          this.syncChangedMarks();
        },
      },
      currentTab() {
        this.syncChangedMarks();
      },
    },
    created() {
      fetch("./api/status").then((r) => r.json()).then((st) => { this.activeEncoder = st.encoder || ""; }).catch(() => {});
      fetch("./api/config")
        .then((r) => r.json())
        .then((r) => {
          this.config = r;
          this.platform = this.config.platform;
          // Remember what is saved, to know when something changed
          setTimeout(() => {
            this.snapshot = JSON.stringify(this.serialize());
            this.dirty = false;
            this.syncChangedMarks();
          }, 300);

          if (this.platform === "windows") {
            this.tabs = this.tabs.filter((el) => {
              return el.id !== "vt" && el.id !== "vaapi" && el.id !== "vulkan";
            });
          }
          if (this.platform === "freebsd" || this.platform === "linux") {
            this.tabs = this.tabs.filter((el) => {
              return el.id !== "amd" && el.id !== "qsv" && el.id !== "vt";
            });
          }
          if (this.platform === "macos") {
            this.tabs = this.tabs.filter((el) => {
              return el.id !== "amd" && el.id !== "nv" && el.id !== "qsv" && el.id !== "vaapi" && el.id !== "vulkan";
            });
          }

          // remove values we don't want in the config file
          delete this.config.platform;
          delete this.config.status;
          delete this.config.version;

          // Parse the special options before population if available
          const specialOptions = ["dd_mode_remapping", "global_prep_cmd"]
          for (const optionKey of specialOptions) {
            if (this.config.hasOwnProperty(optionKey)) {
              this.config[optionKey] = JSON.parse(this.config[optionKey]);
            }
          }

          // Populate default values from tabs options
          this.tabs.forEach(tab => {
            Object.keys(tab.options).forEach(optionKey => {
              if (this.config[optionKey] === undefined) {
                // Make sure to copy by value
                this.config[optionKey] = structuredClone(toRaw(tab.options[optionKey]));
              }
            });
          });
        });
    },
    methods: {
      tabName(tab) {
        return TAB_NAMES[tab.id] || this.$t(tab.nameKey);
      },
      getTabIcon(tabId) {
        const iconMap = {
          'general': 'Settings',
          'input': 'Gamepad2',
          'av': 'Volume2',
          'network': 'NetworkIcon',
          'files': 'FileCog',
          'advanced': 'Sliders',
          'nv': 'Gpu',
          'amd': 'Gpu',
          'qsv': 'Gpu',
          'vaapi': 'Gpu',
          'vt': 'Gpu',
          'vulkan': 'Gpu',
          'sw': 'Cpu',
        };
        return iconMap[tabId] || 'Settings';
      },
      forceUpdate() {
        this.$forceUpdate()
      },
      serialize() {
        return structuredClone(toRaw(this.config));
      },
      save() {
        this.saved = false;
        this.restarted = false;

        // create a temp copy of this.config to use for the post request
        let config = this.serialize();

        // delete default values from this.config
        this.tabs.forEach(tab => {
          Object.keys(tab.options).forEach(optionKey => {
            if (configValuesEqual(config[optionKey], tab.options[optionKey])) {
              delete config[optionKey]
            }
          });
        });

        return apiFetch("./api/config", {
          method: "POST",
          headers: {
            'Content-Type': 'application/json'
          },
          body: JSON.stringify(config),
        }).then((r) => {
          if (r.status === 200) {
            this.saved = true
            return this.saved
          }
          else {
            return false
          }
        });
      },
      /**
       * Mark every setting that differs from its default with "Changed" and a Reset button.
       * (On/off switches do this themselves.)
       */
      syncChangedMarks() {
        this.$nextTick(() => {
          const tab = this.tabs.find((t) => t.id === this.currentTab);
          if (!tab || !this.config) return;
          Object.entries(tab.options).forEach(([key, def]) => {
            if (def !== null && typeof def === 'object') return;
            if (CHANGED_MARK_SKIP.has(key)) return;
            const el = document.getElementById(key);
            if (!el || el.getAttribute('role') === 'switch') return;
            const row = el.closest('.mb-3');
            if (!row) return;
            const changed = String(this.config[key] ?? '') !== String(def ?? '');
            let mark = row.querySelector(':scope > .dili-changed-mark');
            if (changed && !mark) {
              mark = document.createElement('div');
              mark.className = 'dili-changed-mark';
              const pill = document.createElement('span');
              pill.className = 'dili-changed-pill';
              pill.textContent = 'Changed';
              const reset = document.createElement('button');
              reset.type = 'button';
              reset.className = 'dili-changed-reset';
              reset.textContent = 'Reset to default';
              reset.addEventListener('click', () => { this.config[key] = def; });
              mark.append(pill, reset);
              row.appendChild(mark);
            } else if (!changed && mark) {
              mark.remove();
            }
          });
        });
      },
      discardChanges() {
        window.location.reload();
      },
      async saveAndApply() {
        this.applying = true;
        const ok = await this.save();
        if (ok === true) {
          this.snapshot = JSON.stringify(this.serialize());
          this.dirty = false;
          this.restarted = true;
          apiFetch("./api/restart", { method: "POST", headers: { "Content-Type": "application/json" } });
          setTimeout(() => window.location.reload(), 8000);
        } else {
          this.applying = false;
        }
      },
      apply() {
        this.saved = this.restarted = false;
        let saved = this.save();

        saved.then((result) => {
          if (result === true) {
            this.restarted = true;
            setTimeout(() => {
              this.saved = this.restarted = false;
            }, 5000);
            apiFetch("./api/restart", {
              method: "POST",
              headers: {
                "Content-Type": "application/json"
              }
            });
          }
        });
      },
      handleSearch() {
        // Clear all highlighting
        document.querySelectorAll('.config-search-highlight').forEach(el => {
          el.classList.remove('config-search-highlight');
        });

        if (!this.searchQuery) {
          // Show all form groups when search is cleared
          document.querySelectorAll('.mb-3').forEach(el => {
            el.style.display = '';
          });
          return;
        }

        const results = this.searchResults;

        if (results.length === 0) {
          return;
        }

        // Switch to the tab of the first result
        if (results.length > 0 && results[0].tabId !== this.currentTab) {
          this.currentTab = results[0].tabId;
        }

        // Wait for tab content to render
        this.$nextTick(() => {
          // Hide all form groups first
          document.querySelectorAll('.config-page .mb-3').forEach(el => {
            el.style.display = 'none';
          });

          // Show only matching elements
          results.forEach(result => {
            const element = document.getElementById(result.key);

            if (element) {
              // Show the element's container
              const container = element.closest('.mb-3');
              if (container) {
                container.style.display = '';
              }
            }
          });

          // Scroll to and highlight the first result
          if (results.length > 0) {
            const firstElement = document.getElementById(results[0].key);
            if (firstElement) {
              const container = firstElement.closest('.mb-3');
              if (container) {
                container.scrollIntoView({ behavior: 'smooth', block: 'center' });
                container.classList.add('config-search-highlight');
                setTimeout(() => {
                  container.classList.remove('config-search-highlight');
                }, 3000);
              }
            }
          }
        });
      },
    },
    mounted() {
      // Handle hashchange events
      this.hashChangeHandler = () => {
        let hash = window.location.hash;
        if (hash) {
          // remove the # from the hash
          let stripped_hash = hash.substring(1);

          this.tabs.forEach(tab => {
            Object.keys(tab.options).forEach(key => {
              if (tab.id === stripped_hash || key === stripped_hash) {
                this.currentTab = tab.id;
              }
              if (key === stripped_hash) {
                // sleep for 2 seconds to allow the page to load
                setTimeout(() => {
                  let element = document.getElementById(stripped_hash);
                  if (element) {
                    window.location.hash = hash;
                  }
                }, 2000);
              }

              if (this.currentTab === tab.id) {
                // stop looping
                return true;
              }
            });
          });
        }
      };

      // Call handleHash for the initial load
      this.hashChangeHandler();

      // Add hashchange event listener
      window.addEventListener("hashchange", this.hashChangeHandler);
    },
    beforeUnmount() {
      window.removeEventListener("hashchange", this.hashChangeHandler);
    },
  }
</script>

<style>
  /* ---------- Dili: Apple-style grouped settings ---------- */
  .dili-adv-head {
    margin: 36px 0 20px 0;
  }

  .dili-adv-head h1 {
    margin: 0 0 6px 0;
    font-size: 32px;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .dili-adv-head p {
    margin: 0;
    color: var(--color-text-muted);
  }

  .dili-adv-head a {
    color: var(--color-primary);
    font-weight: 600;
  }

  .config-content {
    background: transparent !important;
    border: none !important;
    padding: 0 !important;
  }

  /* Every option becomes a row inside one rounded group */
  .config-content > div:not(.alert):not(.dili-encoder-note):not(:empty) {
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
    padding: 4px 20px;
  }

  .config-content .mb-3,
  .config-content .form-check.dili-check-row {
    margin: 0 !important;
    padding: 16px 0 !important;
    border-bottom: 1px solid var(--color-border);
  }

  .config-content .mb-3:last-child,
  .config-content .form-check.dili-check-row:last-child {
    border-bottom: none;
  }

  .config-content .mb-3 > .form-check.dili-check-row {
    padding: 0 !important;
    border-bottom: none;
  }

  .config-content .form-label,
  .config-content label {
    font-size: 15px;
    font-weight: 600;
    margin-bottom: 6px;
  }

  .config-content .form-text {
    font-size: 13px;
    color: var(--color-text-muted);
    line-height: 1.45;
  }

  .config-content .form-control,
  .config-content .form-select {
    border-radius: 10px;
    border: 1px solid transparent;
    background-color: var(--color-bg-subtle);
    min-height: 40px;
  }

  .config-content .form-control:focus,
  .config-content .form-select:focus {
    border-color: var(--color-primary);
    box-shadow: 0 0 0 2px color-mix(in srgb, var(--color-primary) 30%, transparent);
  }

  .config-nav .nav-link {
    border-radius: 10px;
  }

  .dili-in-use {
    margin-left: auto;
    padding: 1px 8px;
    border-radius: 10px;
    font-size: 11px;
    font-weight: 700;
    color: #ffffff;
    background: var(--color-success);
  }

  .config-nav .nav-link:has(.dili-in-use) {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .dili-nav-more {
    margin: 4px 0 0 12px;
    padding: 0;
    border: none;
    background: none;
    color: var(--color-primary);
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-encoder-note {
    display: flex;
    flex-direction: column;
    gap: 4px;
    margin-bottom: 16px;
    padding: 14px 18px;
    border-radius: 14px;
    background: var(--color-bg-subtle);
    font-size: 14px;
    line-height: 1.45;
  }

  .dili-encoder-note span {
    color: var(--color-text-muted);
  }
  .dili-save-bar {
    position: fixed;
    left: calc(272px + (100vw - 272px) / 2);
    transform: translateX(-50%);
    bottom: 20px;
    z-index: 1500;
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 10px 12px 10px 20px;
    border-radius: 16px;
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.35);
    font-size: 14px;
    font-weight: 600;
  }

  @media (max-width: 900px) {
    .dili-save-bar {
      left: 50%;
    }
  }

  .dili-bar-btn {
    height: 38px;
    padding: 0 16px;
    border-radius: 19px;
    border: 1px solid var(--color-border-strong);
    background: transparent;
    color: var(--color-text-base);
    font: inherit;
    cursor: pointer;
  }

  .dili-bar-btn.primary {
    border: none;
    background: var(--color-primary);
    color: var(--color-on-primary);
  }

  .dili-bar-btn:disabled {
    opacity: 0.6;
    cursor: default;
  }
  .dili-changed-mark {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-top: 8px;
  }

  .dili-changed-pill {
    padding: 1px 8px;
    border-radius: 10px;
    font-size: 11px;
    font-weight: 700;
    color: var(--color-on-primary);
    background: var(--color-primary);
  }

  .dili-changed-reset {
    padding: 0;
    border: none;
    background: none;
    color: var(--color-primary);
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-unit {
    max-width: 260px;
  }
</style>
