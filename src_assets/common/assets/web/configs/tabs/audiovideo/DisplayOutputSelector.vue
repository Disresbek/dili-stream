<script setup>
import { ref } from 'vue'
import { $tp } from '../../../platform-i18n'
import PlatformLayout from '../../../PlatformLayout.vue'

const props = defineProps({
  platform: String,
  config: Object,
})

const config = ref(props.config)
let _outputNamePlaceholder = '0';
if(props.platform === 'windows') {
  _outputNamePlaceholder = '{de9bb7e2-186e-505b-9e93-f48793333810}';
} else if(props.platform === 'linux' || props.platform === 'freebsd') {
  _outputNamePlaceholder = 'DP-0';
}
const outputNamePlaceholder = _outputNamePlaceholder;  // NOSONAR(javascript:S1481,javascript:S1854): Constant used by vue.js binding for placeholder below
</script>

<template>
  <div class="mb-3" v-if="platform === 'linux'">
    <label for="output_name" class="form-label">{{ $t('config.output_name') }}</label>
    <div class="dili-readonly" id="output_name">
      <span>{{ config.output_name === 'virtual' ? 'A virtual screen for every stream' : (config.output_name || 'Automatic') }}</span>
      <RouterLink to="/displays">Change on Display &amp; Quality</RouterLink>
    </div>
  </div>
  <div class="mb-3" v-else>
    <label for="output_name" class="form-label">{{ $t('config.output_name') }}</label>
    <input type="text" class="form-control" id="output_name" :placeholder="outputNamePlaceholder"
           v-model="config.output_name"/>
    <div class="form-text">
      {{ $tp('config.output_name_desc') }}<br>
      <PlatformLayout :platform="platform">
        <template #windows>
          <pre style="white-space: pre-line;">
            <b>&nbsp;&nbsp;{</b>
            <b>&nbsp;&nbsp;&nbsp;&nbsp;"device_id": "{de9bb7e2-186e-505b-9e93-f48793333810}"</b>
            <b>&nbsp;&nbsp;&nbsp;&nbsp;"display_name": "\\\\.\\DISPLAY1"</b>
            <b>&nbsp;&nbsp;&nbsp;&nbsp;"friendly_name": "ROG PG279Q"</b>
            <b>&nbsp;&nbsp;&nbsp;&nbsp;...</b>
            <b>&nbsp;&nbsp;}</b>
          </pre>
        </template>
        <template #freebsd>
          <pre style="white-space: pre-line;">
            Info: Detecting displays
            Info: Detected display: HDMI-A-1 connected: true
            Info: Detected display: DP-1 connected: true
            Info: Detected display: DP-2 connected: false
            Info: Detected display: DVI-D-3 connected: false
          </pre>
        </template>
        <template #linux>
          <pre style="white-space: pre-line;">
            Info: Detecting displays
            Info: Detected display: HDMI-A-1 connected: true
            Info: Detected display: DP-1 connected: true
            Info: Detected display: DP-2 connected: false
            Info: Detected display: DVI-D-3 connected: false
          </pre>
        </template>
        <template #macos>
          <pre style="white-space: pre-line;">
            Info: Detecting displays
            Info: Detected display: Monitor-0 (id: 3) connected: true
            Info: Detected display: Monitor-1 (id: 2) connected: true
          </pre>
        </template>
      </PlatformLayout>
    </div>
  </div>
</template>

<style scoped>
  .dili-readonly {
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 16px;
    padding: 10px 14px;
    border-radius: 10px;
    background: var(--color-bg-subtle);
  }

  .dili-readonly a {
    color: var(--color-primary);
    font-weight: 600;
    white-space: nowrap;
  }
</style>
