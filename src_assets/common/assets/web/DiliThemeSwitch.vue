<template>
  <div class="dili-appearance" ref="root">
    <!-- The three main choices -->
    <div class="dili-segment" role="radiogroup" aria-label="Appearance">
      <button
        v-for="option in main"
        :key="option.value"
        type="button"
        role="radio"
        class="dili-segment-btn"
        :class="{ active: theme === option.value }"
        :aria-checked="theme === option.value ? 'true' : 'false'"
        :title="option.label"
        @click="choose(option.value)"
      >
        <component :is="option.icon" :size="16"></component>
        <span>{{ option.label }}</span>
      </button>
    </div>

    <!-- All other themes, in their own panel next to the sidebar -->
    <button type="button" class="dili-more-themes" :class="{ active: isExtra }" :aria-expanded="open ? 'true' : 'false'" @click.stop="open = !open">
      <Palette :size="16"></Palette>
      <span>{{ isExtra ? currentLabel : 'More themes' }}</span>
      <ChevronRight :size="16" class="dili-more-chevron"></ChevronRight>
    </button>

    <div v-if="open" class="dili-theme-panel" role="dialog" aria-label="More themes" @click.stop>
      <div class="dili-theme-panel-head">
        <strong>More themes</strong>
        <button type="button" class="dili-theme-close" aria-label="Close" @click="open = false"><X :size="16"></X></button>
      </div>
      <p class="dili-theme-note">These themes change the colours of the whole page.</p>
      <div class="dili-theme-columns">
        <div v-for="group in groups" :key="group.title">
          <div class="dili-theme-group">{{ group.title }}</div>
          <button
            v-for="t in group.themes"
            :key="t.value"
            type="button"
            class="dili-theme-item"
            :class="{ active: theme === t.value }"
            @click="choose(t.value)"
          >
            <span>{{ t.label }}</span>
            <Check v-if="theme === t.value" :size="16"></Check>
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
  import { Check, ChevronRight, Monitor, Moon, Palette, Sun, X } from '@lucide/vue'
  import { chooseTheme, currentTheme, loadAutoTheme } from './theme'

  const DARK = [
    ['dracula', 'Dracula'], ['mocha', 'Mocha'], ['ember', 'Ember'], ['rose-pine', 'Rosé Pine'],
    ['moonlight', 'Moonlight'], ['slate', 'Slate'], ['midnight', 'Midnight'], ['nord', 'Nord'],
  ];
  const LIGHT = [
    ['alucard', 'Alucard'], ['latte', 'Latte'], ['ember-light', 'Ember Light'], ['rose-pine-dawn', 'Rosé Pine Dawn'],
    ['sunshine', 'Sunshine'], ['indigo', 'Indigo'], ['ocean', 'Ocean'], ['forest', 'Forest'],
    ['rose', 'Rose'], ['lavender', 'Lavender'], ['monochrome', 'Monochrome'],
  ];
  const toItems = (list) => list.map(([value, label]) => ({ value, label }));

  export default {
    name: 'DiliThemeSwitch',
    components: { Check, ChevronRight, Palette, X },
    data() {
      return {
        theme: 'auto',
        open: false,
        main: [
          { value: 'auto', label: 'Auto', icon: Monitor },
          { value: 'light', label: 'Light', icon: Sun },
          { value: 'dark', label: 'Dark', icon: Moon },
        ],
        groups: [
          { title: 'Dark', themes: toItems(DARK) },
          { title: 'Light', themes: toItems(LIGHT) },
        ],
      };
    },
    computed: {
      isExtra() {
        return !['auto', 'light', 'dark'].includes(this.theme);
      },
      currentLabel() {
        const all = [...DARK, ...LIGHT];
        const found = all.find(([value]) => value === this.theme);
        return found ? found[1] : 'More themes';
      },
    },
    mounted() {
      loadAutoTheme();
      this.theme = currentTheme();
      document.addEventListener('click', this.close);
      document.addEventListener('keydown', this.onKey);
    },
    beforeUnmount() {
      document.removeEventListener('click', this.close);
      document.removeEventListener('keydown', this.onKey);
    },
    methods: {
      choose(value) {
        this.theme = value;
        chooseTheme(value);
      },
      close() {
        this.open = false;
      },
      onKey(e) {
        if (e.key === 'Escape') this.open = false;
      },
    },
  };
</script>

<style scoped>
  .dili-appearance {
    display: flex;
    flex-direction: column;
    gap: 6px;
    width: 100%;
  }

  .dili-segment {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 2px;
    padding: 3px;
    border-radius: 10px;
    background: var(--color-bg-muted);
  }

  .dili-segment-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 5px;
    height: 32px;
    border: none;
    border-radius: 8px;
    background: transparent;
    color: var(--color-text-muted);
    font: inherit;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-segment-btn:hover {
    color: var(--color-text-base);
  }

  .dili-segment-btn.active {
    background: var(--color-surface);
    color: var(--color-text-base);
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.2);
  }

  .dili-more-themes {
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

  .dili-more-themes:hover,
  .dili-more-themes.active {
    background: var(--color-bg-muted);
  }

  .dili-more-chevron {
    margin-left: auto;
    color: var(--color-text-muted);
  }

  /* Fixed position, so the sidebar can never cut it off */
  .dili-theme-panel {
    position: fixed;
    left: 284px;
    bottom: 16px;
    z-index: 2000;
    width: 380px;
    max-height: calc(100vh - 32px);
    overflow-y: auto;
    padding: 16px;
    box-sizing: border-box;
    border-radius: 16px;
    border: 1px solid var(--color-border);
    background: var(--color-surface);
    box-shadow: 0 12px 40px rgba(0, 0, 0, 0.35);
  }

  @media (max-width: 900px) {
    .dili-theme-panel {
      left: 12px;
      right: 12px;
      width: auto;
    }
  }

  .dili-theme-panel-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .dili-theme-close {
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
    border: none;
    border-radius: 8px;
    background: transparent;
    color: var(--color-text-base);
    cursor: pointer;
  }

  .dili-theme-note {
    margin: 4px 0 12px 0;
    font-size: 13px;
    color: var(--color-text-muted);
  }

  .dili-theme-columns {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 14px;
  }

  .dili-theme-group {
    margin: 0 8px 4px 8px;
    font-size: 12px;
    font-weight: 600;
    letter-spacing: 0.05em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .dili-theme-item {
    display: flex;
    align-items: center;
    justify-content: space-between;
    width: 100%;
    min-height: 36px;
    padding: 0 8px;
    border: none;
    border-radius: 8px;
    background: transparent;
    color: var(--color-text-base);
    font: inherit;
    font-size: 14px;
    text-align: left;
    cursor: pointer;
  }

  .dili-theme-item:hover {
    background: var(--color-bg-subtle);
  }

  .dili-theme-item.active {
    background: var(--color-primary);
    color: var(--color-on-primary);
  }
</style>
