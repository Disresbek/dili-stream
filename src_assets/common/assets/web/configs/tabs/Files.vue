<script setup>
import { ref } from 'vue'

const props = defineProps({
  platform: String,
  config: Object,
})

const config = ref(props.config)

// Dili: show where the files really are, with a button to open the folder
import { apiFetch } from '../../fetch_utils'
const folder = ref('')
fetch('./api/paths').then((r) => r.json()).then((p) => { folder.value = p.folder || '' }).catch(() => {})
function openFolder() {
  apiFetch('./api/open-folder', { method: 'POST', headers: { 'Content-Type': 'application/json' } })
}
</script>

<template>
  <div id="files" class="config-page">
    <div class="mb-3 dili-folder">
      <div>
        <div class="form-label">Dili's folder</div>
        <div class="form-text">{{ folder || '…' }}</div>
        <div class="form-text">All files below are stored here unless you enter a full path.</div>
      </div>
      <button type="button" class="dili-folder-btn" :disabled="!folder" @click="openFolder">Open folder</button>
    </div>
    <!-- Apps File -->
    <div class="mb-3">
      <label for="file_apps" class="form-label">{{ $t('config.file_apps') }}</label>
      <input type="text" class="form-control" id="file_apps" placeholder="apps.json" v-model="config.file_apps" />
      <div class="form-text">{{ $t('config.file_apps_desc') }}</div>
    </div>

    <!-- Credentials File -->
    <div class="mb-3">
      <label for="credentials_file" class="form-label">{{ $t('config.credentials_file') }}</label>
      <input type="text" class="form-control" id="credentials_file" placeholder="sunshine_state.json" v-model="config.credentials_file" />
      <div class="form-text">{{ $t('config.credentials_file_desc') }}</div>
    </div>

    <!-- Log Path -->
    <div class="mb-3">
      <label for="log_path" class="form-label">{{ $t('config.log_path') }}</label>
      <input type="text" class="form-control" id="log_path" placeholder="sunshine.log" v-model="config.log_path" />
      <div class="form-text">{{ $t('config.log_path_desc') }}</div>
    </div>

    <!-- Private Key -->
    <div class="mb-3">
      <label for="pkey" class="form-label">{{ $t('config.pkey') }}</label>
      <input type="text" class="form-control" id="pkey" placeholder="/dir/pkey.pem" v-model="config.pkey" />
      <div class="form-text">{{ $t('config.pkey_desc') }}</div>
    </div>

    <!-- Certificate -->
    <div class="mb-3">
      <label for="cert" class="form-label">{{ $t('config.cert') }}</label>
      <input type="text" class="form-control" id="cert" placeholder="/dir/cert.pem" v-model="config.cert" />
      <div class="form-text">{{ $t('config.cert_desc') }}</div>
    </div>

    <!-- State File -->
    <div class="mb-3">
      <label for="file_state" class="form-label">{{ $t('config.file_state') }}</label>
      <input type="text" class="form-control" id="file_state" placeholder="sunshine_state.json"
             v-model="config.file_state" />
      <div class="form-text">{{ $t('config.file_state_desc') }}</div>
    </div>

  </div>
</template>

<style scoped>
  .dili-folder {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 16px;
  }

  .dili-folder-btn {
    flex-shrink: 0;
    height: 38px;
    padding: 0 16px;
    border-radius: 19px;
    border: 1px solid var(--color-border-strong);
    background: transparent;
    color: var(--color-text-base);
    font-weight: 600;
  }
</style>
