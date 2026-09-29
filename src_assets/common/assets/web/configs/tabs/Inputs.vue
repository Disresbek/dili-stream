<script setup>
import { computed, ref, watch } from 'vue'
import Checkbox from "../../Checkbox.vue";
import VirtualKeyCodeSelect from '../VirtualKeyCodeSelect.vue'
import { isValidVirtualKeyCode } from '../virtual_key_codes.js'
import {
  ArrowRight,
  ExternalLink,
  Plus,
  Trash2,
} from '@lucide/vue'

let nextKeybindingId = 0

/**
 * @brief Create one editable keybinding pair with a stable rendering key.
 *
 * @param {string} source Client virtual-key code.
 * @param {string} destination Host virtual-key code.
 * @return {{id: number, source: string, destination: string}} Editable keybinding pair.
 */
function createKeybinding(source = '', destination = '') {
  return {
    id: nextKeybindingId++,
    source,
    destination,
  }
}

/**
 * @brief Parse the serialized integer list used by the configuration API into pairs.
 *
 * @param {string} value Serialized keybinding list.
 * @return {Array<{id: number, source: string, destination: string}>} Editable keybinding pairs.
 */
function parseKeybindings(value) {
  const serialized = String(value ?? '').trim()
  const contents = serialized.startsWith('[') && serialized.endsWith(']')
    ? serialized.slice(1, -1)
    : serialized

  if (contents.trim() === '') {
    return []
  }

  const values = contents.split(',').map(keyCode => keyCode.trim())
  const pairs = []
  for (let index = 0; index < values.length; index += 2) {
    pairs.push(createKeybinding(values[index], values[index + 1] ?? ''))
  }
  return pairs
}

/**
 * @brief Serialize complete, valid keybinding pairs for the configuration API.
 *
 * @param {Array<{source: string, destination: string}>} pairs Editable keybinding pairs.
 * @return {string} Serialized integer list containing only valid pairs.
 */
function serializeKeybindings(pairs) {
  const values = pairs
    .filter(pair => isValidVirtualKeyCode(pair.source) && isValidVirtualKeyCode(pair.destination))
    .flatMap(pair => [pair.source.trim(), pair.destination.trim()])
  return `[${values.join(',')}]`
}

const props = defineProps({
  platform: String,
  config: Object,
})

const config = ref(props.config)

// Dili: the Guide button setting as a switch with simple choices
const showKeybindings = ref(false)
const guideOn = computed(() => Number(config.value.back_button_timeout) >= 0)
const guideTooShort = computed(() => guideOn.value && Number(config.value.back_button_timeout) < 300)
function toggleGuide() {
  config.value.back_button_timeout = guideOn.value ? -1 : 1000
}
const keybindingPairs = ref(parseKeybindings(config.value.keybindings))

const vigembusGamepads = new Set(['auto', 'x360', 'ds4'])
const vigembusOnly = computed(() => props.platform === 'windows' && config.value.gamepad_driver === 'vigembus')
const gamepadDriverSelection = computed({
  get: () => props.platform === 'macos' && !config.value.gamepad_driver ? 'virtualhid' : config.value.gamepad_driver,
  set: value => { config.value.gamepad_driver = value },
})

/**
 * @brief Add an empty keybinding row.
 */
function addKeybinding() {
  keybindingPairs.value.push(createKeybinding())
}

/**
 * @brief Remove a keybinding row.
 *
 * @param {number} index Index of the row to remove.
 */
function removeKeybinding(index) {
  keybindingPairs.value.splice(index, 1)
}

watch(
  keybindingPairs,
  pairs => {
    config.value.keybindings = serializeKeybindings(pairs)
  },
  { deep: true },
)

watch(
  () => config.value.gamepad_driver,
  (gamepadDriver) => {
    if (props.platform === 'windows' && gamepadDriver === 'vigembus' && !vigembusGamepads.has(config.value.gamepad)) {
      config.value.gamepad = 'auto'
    }
  },
)
</script>

<template>
  <div id="input" class="config-page">
    <!-- Enable Gamepad Input -->
    <Checkbox class="mb-3"
              id="controller"
              locale-prefix="config"
              v-model="config.controller"
              default="true"
    ></Checkbox>

    <!-- Virtual gamepad backend policy -->
    <div class="mb-3" v-if="platform === 'windows' || platform === 'macos'">
      <label for="gamepad_driver" class="form-label">{{ $t('config.gamepad_driver') }}</label>
      <select id="gamepad_driver" class="form-select" v-model="gamepadDriverSelection" required>
        <option v-if="platform === 'windows'" value="" disabled>{{ $t('config.gamepad_driver_select') }}</option>
        <option v-if="platform === 'windows'" value="all">{{ $t('config.gamepad_driver_all') }}</option>
        <option value="virtualhid">{{ $t('config.gamepad_driver_virtualhid') }}</option>
        <option v-if="platform === 'windows'" value="vigembus">{{ $t('config.gamepad_driver_vigembus') }}</option>
        <option value="none">{{ $t('config.gamepad_driver_none') }}</option>
      </select>
      <div class="form-text">{{ $t(platform === 'macos' ? 'config.gamepad_driver_macos_desc' : 'config.gamepad_driver_desc') }}</div>
    </div>

    <!-- Emulated Gamepad Type -->
    <div class="mb-3" v-if="config.controller === 'enabled' && config.gamepad_driver !== 'none'">
      <label for="gamepad" class="form-label">{{ $t('config.gamepad') }}</label>
      <select id="gamepad" class="form-select" v-model="config.gamepad">
        <option value="auto">{{ $t('_common.auto') }}</option>

        <option v-if="!vigembusOnly" value="generic">{{ $t("config.gamepad_generic") }}</option>
        <option value="x360">{{ $t('config.gamepad_x360') }}</option>
        <option v-if="!vigembusOnly" value="xone">{{ $t("config.gamepad_xone") }}</option>
        <option v-if="!vigembusOnly" value="xseries">{{ $t("config.gamepad_xseries") }}</option>
        <option value="ds4">{{ $t('config.gamepad_ds4') }}</option>
        <option v-if="!vigembusOnly" value="ds5">{{ $t("config.gamepad_ds5") }}</option>
        <option v-if="!vigembusOnly" value="switch">{{ $t("config.gamepad_switch") }}</option>
      </select>
      <div class="form-text">{{ $t('config.gamepad_desc') }}</div>
    </div>

    <!-- Additional options based on gamepad type -->
    <template v-if="config.controller === 'enabled' && config.gamepad_driver !== 'none'">
      <template v-if="config.gamepad === 'ds4' || config.gamepad === 'ds5' || config.gamepad === 'auto'">
        <div class="mb-3 accordion">
          <div class="accordion-item">
            <h2 class="accordion-header">
              <button class="accordion-button" type="button" data-bs-toggle="collapse"
                      data-bs-target="#panelsStayOpen-collapseOne">
                {{ $t(config.gamepad === 'auto' ? 'config.gamepad_auto' : 'config.gamepad_ds4_manual') }}
              </button>
            </h2>
            <div id="panelsStayOpen-collapseOne" class="accordion-collapse collapse show"
                 aria-labelledby="panelsStayOpen-headingOne">
              <div class="accordion-body">
                <!-- Automatic PlayStation-style detection options -->
                <template v-if="config.gamepad === 'auto'">
                  <!-- Gamepad with motion capability as a PlayStation-style controller -->
                  <Checkbox class="mb-3"
                            id="motion_as_ds4"
                            locale-prefix="config"
                            v-model="config.motion_as_ds4"
                            default="true"
                  ></Checkbox>
                  <!-- Gamepad with touch capability as a PlayStation-style controller -->
                  <Checkbox class="mb-3"
                            id="touchpad_as_ds4"
                            locale-prefix="config"
                            v-model="config.touchpad_as_ds4"
                            default="true"
                  ></Checkbox>
                </template>
                <!-- PlayStation-style option: Back/Select as touchpad click -->
                <template v-if="config.gamepad === 'ds4' || config.gamepad === 'ds5' || config.gamepad === 'auto'">
                  <Checkbox class="mb-3"
                            id="ds4_back_as_touchpad_click"
                            locale-prefix="config"
                            v-model="config.ds4_back_as_touchpad_click"
                            default="true"
                  ></Checkbox>
                </template>
                <!-- Virtual HID option: Controller MAC randomization -->
                <template v-if="config.gamepad_driver !== 'vigembus' && (config.gamepad === 'ds4' || config.gamepad === 'ds5' || config.gamepad === 'auto')">
                  <Checkbox class="mb-3"
                            id="virtualhid_randomize_mac"
                            locale-prefix="config"
                            v-model="config.virtualhid_randomize_mac"
                            default="true"
                  ></Checkbox>
                </template>
              </div>
            </div>
          </div>
        </div>
      </template>
    </template>

    <!-- Home/Guide Button Emulation Timeout -->
    <div class="mb-3" v-if="config.controller === 'enabled' && config.gamepad_driver !== 'none'">
      <div class="dili-guide-row">
        <div>
          <label for="back_button_timeout" class="form-label">{{ $t('config.back_button_timeout') }}</label>
          <div class="form-text">For controllers without a Guide (Xbox/Home) button: hold Back/Select to press it.</div>
        </div>
        <button type="button" role="switch" class="dili-guide-switch" :class="{ on: guideOn }"
                :aria-checked="guideOn ? 'true' : 'false'" aria-label="Hold Back for the Guide button" @click="toggleGuide">
          <span></span>
        </button>
      </div>
      <select v-if="guideOn" id="back_button_timeout" class="form-select dili-guide-select" v-model="config.back_button_timeout">
        <option :value="500">Hold for half a second</option>
        <option :value="1000">Hold for 1 second</option>
        <option :value="1500">Hold for 1.5 seconds</option>
        <option :value="2000">Hold for 2 seconds</option>
        <option v-if="![500, 1000, 1500, 2000].includes(Number(config.back_button_timeout))" :value="config.back_button_timeout">
          {{ config.back_button_timeout }} milliseconds
        </option>
      </select>
      <div v-if="guideTooShort" class="dili-guide-warning">
        {{ config.back_button_timeout }} milliseconds is so short that almost every press of Back counts as the Guide button.
        <button type="button" class="dili-guide-fix" @click="config.back_button_timeout = 1000">Use 1 second</button>
      </div>
    </div>

    <!-- Dili controller extras -->
    <template v-if="config.controller === 'enabled'">
      <Checkbox class="mb-3"
                id="controller_shortcuts"
                locale-prefix="config"
                v-model="config.controller_shortcuts"
                default="true"
      ></Checkbox>
      <div class="mb-3 dili-shortcut-list" v-if="config.controller_shortcuts === 'enabled'">
        <div class="form-label">While holding Select</div>
        <div class="dili-shortcuts">
          <span><kbd>B</kbd> Close the game</span>
          <span><kbd>D-pad up / down</kbd> Volume</span>
          <span><kbd>D-pad left</kbd> Mute</span>
          <span><kbd>Y</kbd> On-screen keyboard (Steam)</span>
          <span><kbd>RB</kbd> Screenshot</span>
          <span><kbd>X</kbd> Steam Big Picture</span>
        </div>
        <div class="form-text">Select on its own still works in games. Hold Start for a second to use the controller as a mouse.</div>
      </div>

      <Checkbox class="mb-3"
                id="controller_feedback"
                locale-prefix="config"
                v-model="config.controller_feedback"
                default="true"
      ></Checkbox>

      <div class="mb-3">
        <label for="mouse_mode_speed" class="form-label">{{ $t('config.mouse_mode_speed') }}</label>
        <div class="dili-slider">
          <span>Slow</span>
          <input id="mouse_mode_speed" type="range" min="1" max="10" step="1" v-model.number="config.mouse_mode_speed">
          <span>Fast</span>
        </div>
        <div class="form-text">{{ $t('config.mouse_mode_speed_desc') }}</div>
      </div>

      <Checkbox class="mb-3"
                id="nintendo_layout"
                locale-prefix="config"
                v-model="config.nintendo_layout"
                default="false"
      ></Checkbox>

      <div class="mb-3">
        <label for="stick_deadzone" class="form-label">{{ $t('config.stick_deadzone') }}</label>
        <select id="stick_deadzone" class="form-select dili-guide-select" v-model="config.stick_deadzone">
          <option :value="0">Off</option>
          <option :value="5">Small (5%)</option>
          <option :value="10">Medium (10%)</option>
          <option :value="15">Large (15%)</option>
          <option :value="20">Very large (20%)</option>
        </select>
        <div class="form-text">{{ $t('config.stick_deadzone_desc') }}</div>
      </div>
    </template>

    <!-- Enable Keyboard Input -->
    <hr>
    <Checkbox class="mb-3"
              id="keyboard"
              locale-prefix="config"
              v-model="config.keyboard"
              default="true"
    ></Checkbox>

    <!-- Key Repeat Delay-->
    <div class="mb-3" v-if="config.keyboard === 'enabled' && platform === 'windows'">
      <label for="key_repeat_delay" class="form-label">{{ $t('config.key_repeat_delay') }}</label>
      <input type="text" class="form-control" id="key_repeat_delay" placeholder="500"
             v-model="config.key_repeat_delay" />
      <div class="form-text">{{ $t('config.key_repeat_delay_desc') }}</div>
    </div>

    <!-- Key Repeat Frequency-->
    <div class="mb-3" v-if="config.keyboard === 'enabled' && platform === 'windows'">
      <label for="key_repeat_frequency" class="form-label">{{ $t('config.key_repeat_frequency') }}</label>
      <input type="text" class="form-control" id="key_repeat_frequency" placeholder="24.9"
             v-model="config.key_repeat_frequency" />
      <div class="form-text">{{ $t('config.key_repeat_frequency_desc') }}</div>
    </div>

    <!-- Always send scancodes -->
    <Checkbox v-if="config.keyboard === 'enabled' && platform === 'windows'"
              class="mb-3"
              id="always_send_scancodes"
              locale-prefix="config"
              v-model="config.always_send_scancodes"
              default="true"
    ></Checkbox>

    <!-- Mapping Key AltRight to Key Windows -->
    <Checkbox v-if="config.keyboard === 'enabled'"
              class="mb-3"
              id="key_rightalt_to_key_win"
              locale-prefix="config"
              v-model="config.key_rightalt_to_key_win"
              default="false"
    ></Checkbox>

    <!-- Custom key mappings -->
    <div id="keybindings" class="mb-3" v-if="config.keyboard === 'enabled'">
      <div class="d-flex flex-wrap justify-content-between align-items-center gap-2">
        <div class="form-label mb-0">{{ $t('config.keybindings') }}</div>
        <a href="https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes"
           target="_blank" rel="noopener noreferrer" class="small">
          {{ $t('config.keybindings_reference') }}
          <ExternalLink :size="14" />
        </a>
      </div>
      <div class="form-text mb-3">{{ $t('config.keybindings_desc') }}</div>
      <button type="button" class="dili-fold" :aria-expanded="showKeybindings ? 'true' : 'false'" @click="showKeybindings = !showKeybindings">
        {{ showKeybindings ? 'Hide key remapping' : `Show key remapping (${keybindingPairs.length} ${keybindingPairs.length === 1 ? 'rule' : 'rules'})` }}
      </button>
      <template v-if="showKeybindings">

      <div v-if="keybindingPairs.length === 0" class="alert alert-secondary py-2">
        {{ $t('config.keybindings_empty') }}
      </div>

      <div v-if="keybindingPairs.length > 0" class="keybinding-grid">
        <div class="form-label small mb-0 keybinding-source-heading">
          {{ $t('config.keybindings_source') }}
        </div>
        <div class="keybinding-heading-spacer keybinding-arrow-heading" aria-hidden="true"></div>
        <div class="form-label small mb-0 keybinding-destination-heading">
          {{ $t('config.keybindings_destination') }}
        </div>
        <div class="keybinding-heading-spacer keybinding-remove-heading" aria-hidden="true"></div>

        <template v-for="(binding, index) in keybindingPairs" :key="binding.id">
          <div class="keybinding-field keybinding-source">
            <label :for="`keybinding-source-${binding.id}`" class="form-label small keybinding-field-label">
              {{ $t('config.keybindings_source') }}
            </label>
            <VirtualKeyCodeSelect :id="`keybinding-source-${binding.id}`" v-model="binding.source" />
          </div>

          <div class="keybinding-arrow" aria-hidden="true">
            <ArrowRight :size="20" />
          </div>

          <div class="keybinding-field keybinding-destination">
            <label :for="`keybinding-destination-${binding.id}`" class="form-label small keybinding-field-label">
              {{ $t('config.keybindings_destination') }}
            </label>
            <VirtualKeyCodeSelect :id="`keybinding-destination-${binding.id}`"
                                  v-model="binding.destination" />
          </div>

          <div class="keybinding-remove">
            <button type="button" class="btn btn-danger"
                    :aria-label="$t('config.keybindings_remove')" :title="$t('config.keybindings_remove')"
                    @click="removeKeybinding(index)">
              <Trash2 :size="16" class="icon" />
            </button>
          </div>
        </template>
      </div>

      <button type="button" class="btn btn-success mt-2" @click="addKeybinding">
        <Plus :size="16" />
        {{ $t('config.keybindings_add') }}
      </button>
      </template>
    </div>

    <!-- Enable Mouse Input -->
    <hr>
    <Checkbox class="mb-3"
              id="mouse"
              locale-prefix="config"
              v-model="config.mouse"
              default="true"
    ></Checkbox>

    <!-- High resolution scrolling support -->
    <Checkbox v-if="config.mouse === 'enabled'"
              class="mb-3"
              id="high_resolution_scrolling"
              locale-prefix="config"
              v-model="config.high_resolution_scrolling"
              default="true"
    ></Checkbox>

    <!-- Native pen/touch support -->
    <Checkbox v-if="config.mouse === 'enabled'"
              class="mb-3"
              id="native_pen_touch"
              locale-prefix="config"
              v-model="config.native_pen_touch"
              default="true"
    ></Checkbox>
  </div>
</template>

<style scoped>
.keybinding-grid {
  display: grid;
  grid-template-columns: minmax(0, 1fr) auto minmax(0, 1fr) auto;
  gap: 0.5rem;
  align-items: start;
}

.keybinding-source-heading,
.keybinding-source {
  grid-column: 1;
}

.keybinding-arrow-heading,
.keybinding-arrow {
  grid-column: 2;
}

.keybinding-destination-heading,
.keybinding-destination {
  grid-column: 3;
}

.keybinding-remove-heading,
.keybinding-remove {
  grid-column: 4;
}

.keybinding-arrow {
  display: flex;
  min-height: 38px;
  align-items: center;
  justify-content: center;
}

@media (min-width: 768px) {
  .keybinding-field-label {
    position: absolute;
    width: 1px;
    height: 1px;
    padding: 0;
    margin: -1px;
    overflow: hidden;
    clip: rect(0, 0, 0, 0);
    white-space: nowrap;
    border: 0;
  }
}

@media (max-width: 767.98px) {
  .keybinding-grid {
    grid-template-columns: minmax(0, 1fr) auto;
  }

  .keybinding-source-heading,
  .keybinding-destination-heading,
  .keybinding-heading-spacer {
    display: none;
  }

  .keybinding-source {
    grid-column: 1 / -1;
  }

  .keybinding-arrow {
    display: none;
  }

  .keybinding-destination {
    grid-column: 1;
  }

  .keybinding-remove {
    grid-column: 2;
    margin-top: 2rem;
  }
}
</style>

<style scoped>
  .dili-shortcuts {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
    gap: 8px 16px;
    margin: 4px 0 8px 0;
    font-size: 14px;
  }

  .dili-shortcuts kbd {
    display: inline-block;
    min-width: 28px;
    margin-right: 8px;
    padding: 2px 8px;
    border-radius: 6px;
    background: var(--color-bg-muted);
    color: var(--color-text-base);
    font-family: inherit;
    font-size: 12px;
    font-weight: 700;
    text-align: center;
  }

  .dili-slider {
    display: flex;
    align-items: center;
    gap: 12px;
    max-width: 420px;
    font-size: 13px;
    color: var(--color-text-muted);
  }

  .dili-slider input {
    flex-grow: 1;
    accent-color: var(--color-primary);
  }

  .dili-fold {
    padding: 0;
    margin-bottom: 8px;
    border: none;
    background: none;
    color: var(--color-primary);
    font: inherit;
    font-size: 14px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-guide-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
  }

  .dili-guide-switch {
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

  .dili-guide-switch span {
    display: block;
    width: 26px;
    height: 26px;
    margin-left: 2px;
    border-radius: 13px;
    background: #ffffff;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
    transition: margin-left 0.15s ease;
  }

  .dili-guide-switch.on {
    background: var(--color-success);
  }

  .dili-guide-switch.on span {
    margin-left: 22px;
  }

  .dili-guide-select {
    margin-top: 10px;
    max-width: 320px;
  }

  .dili-guide-warning {
    margin-top: 10px;
    padding: 10px 14px;
    border-radius: 10px;
    background: var(--color-bg-subtle);
    font-size: 13px;
  }

  .dili-guide-fix {
    margin-left: 6px;
    padding: 0;
    border: none;
    background: none;
    color: var(--color-primary);
    font: inherit;
    font-weight: 700;
    cursor: pointer;
  }
</style>
