<script setup>
import { ref } from 'vue'
import { $tp } from '../../../platform-i18n'
import PlatformLayout from '../../../PlatformLayout.vue'

const props = defineProps({
  platform: String,
  config: Object,
})

const config = ref(props.config)

// Dili: list the graphics cards Dili finds; with only one, there is nothing to choose
const gpus = ref(null)
fetch('./api/hardware')
  .then((r) => r.json())
  .then((h) => { gpus.value = h.gpus || [] })
  .catch(() => { gpus.value = [] })
</script>

<template>
  <div class="mb-3" v-if="platform !== 'macos' && !(platform === 'linux' && gpus && gpus.length <= 1 && !config.adapter_name)">
    <label for="adapter_name" class="form-label">{{ $t('config.adapter_name') }}</label>
    <select v-if="platform === 'linux' && gpus && gpus.length" id="adapter_name" class="form-select" v-model="config.adapter_name">
      <option value="">Automatic</option>
      <option v-for="g in gpus" :key="g.path" :value="g.path">{{ g.name }}</option>
      <option v-if="config.adapter_name && !gpus.some((g) => g.path === config.adapter_name)" :value="config.adapter_name">
        {{ config.adapter_name }} (not found right now)
      </option>
    </select>
    <input v-else type="text" class="form-control" id="adapter_name"
           :placeholder="$tp('config.adapter_name_placeholder', '/dev/dri/renderD128')"
           v-model="config.adapter_name" />
    <div class="form-text">
      <PlatformLayout :platform="platform">
        <template #windows>
          {{ $t('config.adapter_name_desc_windows') }}<br>
          <pre>tools\dxgi-info.exe</pre>
        </template>
        <template #freebsd>
          {{ $t('config.adapter_name_desc_linux_1') }}<br>
          <pre>ls /dev/dri/renderD*  # {{ $t('config.adapter_name_desc_linux_2') }}</pre>
          <pre>
              vainfo --display drm --device /dev/dri/renderD129 | \
                grep -E "((VAProfileH264High|VAProfileHEVCMain|VAProfileHEVCMain10).*VAEntrypointEncSlice)|Driver version"
            </pre>
          {{ $t('config.adapter_name_desc_linux_3') }}<br>
          <i>VAProfileH264High   : VAEntrypointEncSlice</i>
        </template>
        <template #linux>
          {{ $t('config.adapter_name_desc_linux_1') }}
        </template>
      </PlatformLayout>
    </div>
  </div>
</template>
