<template>
  <nav class="navbar navbar-sunshine">
    <div class="container-fluid">
      <span class="navbar-brand" title="Dili">
        <DiliLogo on-dark></DiliLogo>
      </span>
      <DiliThemeSwitch style="width: 260px"></DiliThemeSwitch>
    </div>
  </nav>
</template>

<script>
import DiliThemeSwitch from './DiliThemeSwitch.vue'
import DiliLogo from './DiliLogo.vue'

export default {
  components: {
    DiliLogo,
    DiliThemeSwitch,
  },
}
</script>
