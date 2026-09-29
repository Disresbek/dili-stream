<template>
  <nav class="navbar navbar-sunshine">
    <div class="container-fluid">
      <span class="navbar-brand" title="Dili">
        <DiliLogo on-dark></DiliLogo>
      </span>
      <ThemeToggle/>
    </div>
  </nav>
</template>

<script>
import ThemeToggle from './ThemeToggle.vue'
import DiliLogo from './DiliLogo.vue'

export default {
  components: {
    DiliLogo,
    ThemeToggle,
  },
}
</script>
