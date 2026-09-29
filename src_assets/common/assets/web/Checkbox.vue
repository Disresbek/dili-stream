<script setup>
import { computed } from 'vue';
const model = defineModel({
  type: [Boolean, Number, String],
  required: true,
});
const slots = defineSlots();
const props = defineProps({
  class: {
    type: String,
    default: ""
  },
  desc: {
    type: String,
    default: null
  },
  id: {
    type: String,
    required: true
  },
  label: {
    type: String,
    default: null
  },
  localePrefix: {
    type: String,
    default: "missing-prefix"
  },
  inverseValues: {
    type: Boolean,
    default: false,
  },
  default: {
    type: undefined,
    default: null,
  }
});

// Add the mandatory class values
const extendedClassStr = (() => {
  let values = props.class.split(" ");
  if (!values.includes("form-check")) {
    values.push("form-check");
  }
  return values.join(" ");
})();

// Map the value to boolean representation if possible, otherwise return null.
const mapToBoolRepresentation = (value) => {
  // Try literal values first
  if (value === true || value === false) {
    return { possibleValues: [true, false], value: value };
  }
  if (value === 1 || value === 0) {
    return { possibleValues: [1, 0], value: value };
  }

  const stringPairs = [
    ["true", "false"],
    ["1", "0"],
    ["enabled", "disabled"],
    ["enable", "disable"],
    ["yes", "no"],
    ["on", "off"]
  ];

  value = `${value}`.toLowerCase().trim();
  for (const pair of stringPairs) {
    if (value === pair[0] || value === pair[1]) {
      return { possibleValues: pair, value: value };
    }
  }

  return null;
}

// Determine the true/false values for the checkbox
const checkboxValues = (() => {
  const mappedValues = (() => {
    const boolValues = mapToBoolRepresentation(model.value);
    if (boolValues !== null) {
      return boolValues.possibleValues;
    }

    // Return fallback if nothing matches
    console.error(`Checkbox value ${model.value} did not match any acceptable pattern!`);
    return ["true", "false"];
  })();

  const truthyIndex = props.inverseValues ? 1 : 0;
  const falsyIndex = props.inverseValues ? 0 : 1;
  return { truthy: mappedValues[truthyIndex], falsy: mappedValues[falsyIndex] };
})();
const parsedDefaultPropValue = (() => {
  const boolValues = mapToBoolRepresentation(props.default);
  if (boolValues !== null) {
    // Convert truthy to true/false.
    return boolValues.value === boolValues.possibleValues[0];
  }

  return null;
})();

const labelField = props.label ?? `${props.localePrefix}.${props.id}`;
const descField = props.desc ?? `${props.localePrefix}.${props.id}_desc`;
const showDesc = props.desc !== "" || Object.entries(slots).length > 0;
const showDefValue = parsedDefaultPropValue !== null;
const defValue = parsedDefaultPropValue ? "_common.enabled_def_cbox" : "_common.disabled_def_cbox";

// Dili: show a small "Changed" label and a Reset button when the value differs from the default
const isOn = computed(() => model.value === checkboxValues.truthy);
const changed = computed(() => showDefValue && isOn.value !== parsedDefaultPropValue);
const toggle = () => {
  model.value = isOn.value ? checkboxValues.falsy : checkboxValues.truthy;
};
const reset = () => {
  model.value = parsedDefaultPropValue ? checkboxValues.truthy : checkboxValues.falsy;
};
</script>

<template>
  <div :class="extendedClassStr + ' dili-check-row'">
    <div class="dili-check-text">
      <label :for="props.id" class="dili-check-label">
        {{ $t(labelField) }}
        <span v-if="changed" class="dili-changed">Changed</span>
      </label>
      <div class="form-text" v-if="showDesc">
        {{ $t(descField) }}
        <slot></slot>
      </div>
      <button v-if="changed" type="button" class="dili-reset" @click="reset">Reset to {{ parsedDefaultPropValue ? 'on' : 'off' }}</button>
    </div>
    <button
      type="button"
      role="switch"
      class="dili-toggle"
      :class="{ on: isOn }"
      :id="props.id"
      :aria-checked="isOn ? 'true' : 'false'"
      @click="toggle"
    >
      <span></span>
    </button>
  </div>
</template>

<style scoped>
  .dili-check-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
    padding-left: 0;
  }

  .dili-check-text {
    display: flex;
    flex-direction: column;
    gap: 2px;
    min-width: 0;
  }

  .dili-check-label {
    font-size: 15px;
    font-weight: 600;
  }

  .dili-changed {
    margin-left: 8px;
    padding: 1px 8px;
    border-radius: 10px;
    font-size: 11px;
    font-weight: 700;
    color: var(--color-on-primary);
    background: var(--color-primary);
    vertical-align: middle;
  }

  .dili-reset {
    align-self: flex-start;
    padding: 0;
    margin-top: 2px;
    border: none;
    background: none;
    color: var(--color-primary);
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-toggle {
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

  .dili-toggle span {
    display: block;
    width: 26px;
    height: 26px;
    margin-left: 2px;
    border-radius: 13px;
    background: #ffffff;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
    transition: margin-left 0.15s ease;
  }

  .dili-toggle.on {
    background: var(--color-success);
  }

  .dili-toggle.on span {
    margin-left: 22px;
  }
</style>
