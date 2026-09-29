<script setup>
import { ref, computed } from 'vue'
import Checkbox from "../../../Checkbox.vue";

const props = defineProps({
  platform: String,
  config: Object,
})

const config = ref(props.config)

// The quality preset is chosen on Display & Quality; show which one is active
const presetName = computed(() => {
  const q = config.value.vaapi_quality
  if (q === 'speed') return 'Performance'
  if (q === 'quality') return 'Quality'
  return 'Balanced'
})
</script>

<template>
  <div id="vaapi-encoder" class="config-page">
    <!-- VAAPI Rate Control -->
    <div class="mb-3">
      <label for="vaapi_rc" class="form-label">{{ $t('config.vaapi_rc') }}</label>
      <select id="vaapi_rc" class="form-select" v-model="config.vaapi_rc">
        <option value="auto">Automatic (recommended)</option>
        <option value="cbr">{{ $t('config.vaapi_rc_cbr') }}</option>
        <option value="vbr">{{ $t('config.vaapi_rc_vbr') }}</option>
        <option value="avbr">{{ $t('config.vaapi_rc_avbr') }}</option>
        <option value="qvbr">{{ $t('config.vaapi_rc_qvbr') }}</option>
        <option value="cqp">{{ $t('config.vaapi_rc_cqp') }}</option>
        <option value="icq">{{ $t('config.vaapi_rc_icq') }}</option>
      </select>
      <div class="form-text">{{ $t('config.vaapi_rc_desc') }}</div>
    </div>

    <!-- BLBRC -->
    <Checkbox class="mb-3"
              id="vaapi_blbrc"
              locale-prefix="config"
              v-model="config.vaapi_blbrc"
              default="false"
    ></Checkbox>

    <!-- Strict RC Buffer -->
    <Checkbox class="mb-3"
              id="vaapi_strict_rc_buffer"
              locale-prefix="config"
              v-model="config.vaapi_strict_rc_buffer"
              default="false"
    ></Checkbox>

    <!-- VAAPI Quality: set through the presets on Display & Quality -->
    <div class="mb-3">
      <label for="vaapi_quality" class="form-label">{{ $t('config.vaapi_quality') }}</label>
      <div class="dili-readonly" id="vaapi_quality">
        <span>{{ presetName }}</span>
        <RouterLink to="/displays#quality">Change on Display &amp; Quality</RouterLink>
      </div>
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
