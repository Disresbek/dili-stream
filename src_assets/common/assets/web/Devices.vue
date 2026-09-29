<template>
  <Navbar></Navbar>
  <div id="content" class="container dili-page">
    <header class="dili-header">
      <h1>Devices</h1>
      <p>Devices that are allowed to stream from this PC.</p>
    </header>

    <!-- Pairing -->
    <section class="dili-pair">
      <div class="dili-pair-text">
        <div class="dili-pair-title">Pair a new device</div>
        <template v-if="pending.length === 0">
          <div class="dili-muted">
            Open Moonlight on the device and select <strong>{{ hostName || 'this PC' }}</strong>.
            The request appears here automatically.
          </div>
        </template>
        <template v-else>
          <div class="dili-muted">
            Type the 4-digit code that Moonlight shows on
            <strong>{{ selected ? (selected.name || 'the device') : 'the device' }}</strong>.
          </div>
          <div v-if="pending.length > 1" class="dili-chips" aria-label="Pairing requests">
            <button
              v-for="p in pending"
              :key="p.id"
              type="button"
              class="dili-chip"
              :class="{ selected: selectedId === p.id }"
              @click="selectedId = p.id"
            >
              {{ p.name || 'Device' }} · {{ p.address }}
            </button>
          </div>
          <label class="dili-name-label" for="dili-device-name">Name this device (optional)</label>
          <input id="dili-device-name" v-model="deviceName" type="text" class="dili-name" :placeholder="selected && selected.name ? selected.name : 'For example: Living room TV'">
        </template>
      </div>

      <form v-if="pending.length" class="dili-pin-form" @submit.prevent="pair">
        <input
          v-for="(d, i) in digits"
          :key="i"
          ref="pin"
          v-model="digits[i]"
          type="text"
          inputmode="numeric"
          maxlength="1"
          class="dili-pin"
          :aria-label="`Code digit ${i + 1}`"
          @input="onDigit(i)"
          @keydown.backspace="onBackspace(i, $event)"
        >
        <button type="submit" class="dili-pill" :disabled="!codeComplete || busy">Pair</button>
      </form>
    </section>

    <div v-if="message" class="alert" :class="message.ok ? 'alert-success' : 'alert-danger'" role="status">
      {{ message.text }}
    </div>

    <!-- Paired devices -->
    <section class="dili-section">
      <div class="dili-section-head">
        <h2>Paired devices</h2>
        <button v-if="clients.length > 1" type="button" class="dili-link-danger" @click="removeAll">
          {{ confirmAll ? 'Tap again to remove all' : 'Remove all' }}
        </button>
      </div>
      <div class="dili-panel">
        <div v-if="clients.length === 0" class="dili-empty">No devices paired yet.</div>
        <template v-for="(c, i) in clients" :key="c.uuid">
          <div v-if="i > 0" class="dili-divider"></div>
          <div class="dili-device">
            <div class="dili-tile">
              <monitor-smartphone :size="22"></monitor-smartphone>
            </div>
            <div class="dili-device-name">
              {{ c.name || 'Unnamed device' }}
              <div class="dili-device-state">{{ c.enabled === false ? 'Blocked: this device cannot start a stream' : 'Allowed to stream' }}</div>
            </div>
            <button
              type="button"
              class="dili-switch"
              :class="{ on: c.enabled !== false }"
              :aria-pressed="c.enabled !== false ? 'true' : 'false'"
              :aria-label="'Allow ' + (c.name || 'this device') + ' to stream'"
              @click="toggleAllowed(c)"
            >
              <span></span>
            </button>
            <button
              type="button"
              class="dili-pill dili-pill-outline dili-pill-small"
              @click="remove(c)"
            >
              {{ confirmUuid === c.uuid ? 'Tap again to remove' : 'Remove' }}
            </button>
          </div>
        </template>
      </div>
    </section>
  </div>
</template>

<script>
  import Navbar from './Navbar.vue'
  import { apiFetch } from './fetch_utils'
  import { MonitorSmartphone } from '@lucide/vue'

  export default {
    components: {
      Navbar,
      MonitorSmartphone,
    },
    data() {
      return {
        hostName: '',
        pending: [],
        selectedId: '',
        deviceName: '',
        digits: ['', '', '', ''],
        clients: [],
        message: null,
        busy: false,
        confirmUuid: '',
        confirmAll: false,
        timer: null,
      };
    },
    computed: {
      selected() {
        return this.pending.find((p) => p.id === this.selectedId) || null;
      },
      codeComplete() {
        return this.digits.every((d) => /^[0-9]$/.test(d));
      },
    },
    mounted() {
      this.refresh();
      this.loadClients();
      this.timer = setInterval(this.refresh, 2000);
      fetch('./api/status')
        .then((r) => r.json())
        .then((s) => (this.hostName = s.host_name || ''))
        .catch(() => {});
    },
    beforeUnmount() {
      clearInterval(this.timer);
    },
    methods: {
      async refresh() {
        try {
          const r = await apiFetch('./api/pin', { method: 'GET' });
          const body = await r.json();
          this.pending = body.pairings || [];
          if (!this.pending.some((p) => p.id === this.selectedId)) {
            this.selectedId = this.pending.length ? this.pending[0].id : '';
          }
        } catch (e) {
          console.error(e);
        }
      },
      async loadClients() {
        try {
          const r = await fetch('./api/clients/list').then((x) => x.json());
          this.clients = r.named_certs || [];
        } catch (e) {
          console.error(e);
        }
      },
      onDigit(i) {
        this.digits[i] = (this.digits[i] || '').replace(/[^0-9]/g, '').slice(-1);
        if (this.digits[i] && i < 3) {
          this.$refs.pin?.[i + 1]?.focus();
        }
      },
      onBackspace(i, event) {
        if (!this.digits[i] && i > 0) {
          event.preventDefault();
          this.digits[i - 1] = '';
          this.$refs.pin?.[i - 1]?.focus();
        }
      },
      async pair() {
        if (!this.selectedId || !this.codeComplete) {
          return;
        }
        this.busy = true;
        this.message = null;
        try {
          const r = await apiFetch('./api/pin', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
              pairing_id: this.selectedId,
              pin: this.digits.join(''),
              name: this.deviceName || (this.selected && this.selected.name) || 'Device',
            }),
          });
          const result = await r.json();
          if (result.status === true) {
            this.message = { ok: true, text: 'Paired! Continue on your device.' };
            this.digits = ['', '', '', ''];
            this.deviceName = '';
          } else {
            this.message = { ok: false, text: 'That code did not match. Check the code on your device and try again.' };
          }
        } finally {
          this.busy = false;
          this.refresh();
          setTimeout(this.loadClients, 1500);
        }
      },
      async toggleAllowed(client) {
        const enabled = client.enabled === false;
        await apiFetch('./api/clients/update', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ uuid: client.uuid, enabled }),
        });
        this.loadClients();
      },
      async removeAll() {
        if (!this.confirmAll) {
          this.confirmAll = true;
          setTimeout(() => (this.confirmAll = false), 4000);
          return;
        }
        this.confirmAll = false;
        await apiFetch('./api/clients/unpair-all', { method: 'POST', headers: { 'Content-Type': 'application/json' } });
        this.loadClients();
      },
      async remove(client) {
        if (this.confirmUuid !== client.uuid) {
          this.confirmUuid = client.uuid;
          setTimeout(() => {
            if (this.confirmUuid === client.uuid) this.confirmUuid = '';
          }, 4000);
          return;
        }
        this.confirmUuid = '';
        await apiFetch('./api/clients/unpair', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ uuid: client.uuid }),
        });
        this.loadClients();
      },
    },
  };
</script>

<style scoped>
  .dili-page {
    max-width: 1100px;
    padding-top: 36px;
    padding-bottom: 48px;
    display: flex;
    flex-direction: column;
    gap: 28px;
  }

  .dili-header h1 {
    margin: 0 0 6px 0;
    font-size: 32px;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .dili-header p,
  .dili-muted {
    margin: 0;
    color: var(--color-text-muted);
  }

  .dili-hint {
    margin-top: -12px;
    font-size: 14px;
  }

  .dili-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 20px;
  }

  .dili-card {
    text-align: left;
    font: inherit;
    color: var(--color-text-base);
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
    padding: 24px;
    display: flex;
    flex-direction: column;
    gap: 14px;
    cursor: pointer;
  }

  .dili-card:hover {
    border-color: var(--color-border-strong);
  }

  .dili-card.selected {
    border: 2px solid var(--color-primary);
    padding: 23px;
  }

  .dili-card-top {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .dili-check {
    color: var(--color-primary);
  }

  .dili-uncheck {
    color: var(--color-border-strong);
  }

  .dili-card-title {
    display: flex;
    align-items: center;
    gap: 10px;
    font-size: 18px;
    font-weight: 700;
  }

  .dili-card-text {
    font-size: 14px;
    line-height: 1.45;
    color: var(--color-text-muted);
  }

  .dili-badge {
    font-size: 12px;
    font-weight: 700;
    color: var(--color-on-primary);
    background: var(--color-primary);
    padding: 3px 10px;
    border-radius: 10px;
  }

  .dili-section {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }

  .dili-section h2 {
    margin: 0;
    font-size: 13px;
    font-weight: 600;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: var(--color-text-muted);
  }

  .dili-panel {
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
  }

  .dili-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 24px;
    padding: 16px 20px;
  }

  .dili-row-title {
    font-size: 15px;
    font-weight: 600;
  }

  .dili-row-text {
    font-size: 13px;
    line-height: 1.4;
    color: var(--color-text-muted);
  }

  .dili-divider {
    height: 1px;
    background: var(--color-border);
    margin: 0 20px;
  }

  .dili-always {
    flex-shrink: 0;
    font-size: 13px;
    font-weight: 600;
    color: var(--color-text-muted);
  }

  .dili-switch {
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

  .dili-switch span {
    display: block;
    width: 26px;
    height: 26px;
    margin-left: 2px;
    border-radius: 13px;
    background: #ffffff;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
    transition: margin-left 0.15s ease;
  }

  .dili-switch.on {
    background: var(--color-success);
  }

  .dili-switch.on span {
    margin-left: 22px;
  }

  .dili-actions {
    display: flex;
    align-items: center;
    gap: 12px;
    flex-wrap: wrap;
  }

  .dili-pair {
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 16px;
    padding: 28px;
    display: flex;
    align-items: center;
    gap: 36px;
    flex-wrap: wrap;
  }

  .dili-pair-text {
    flex: 1 1 320px;
    display: flex;
    flex-direction: column;
    gap: 8px;
  }

  .dili-pair-title {
    font-size: 20px;
    font-weight: 700;
  }

  .dili-chips {
    display: flex;
    gap: 8px;
    flex-wrap: wrap;
    margin-top: 4px;
  }

  .dili-chip {
    font: inherit;
    font-size: 13px;
    min-height: 36px;
    padding: 0 14px;
    border-radius: 18px;
    border: 1px solid var(--color-border-strong);
    background: transparent;
    color: var(--color-text-base);
    cursor: pointer;
  }

  .dili-chip.selected {
    border-color: var(--color-primary);
    background: var(--color-primary);
    color: var(--color-on-primary);
  }

  .dili-name-label {
    margin-top: 8px;
    font-size: 13px;
    font-weight: 600;
  }

  .dili-name {
    max-width: 320px;
    height: 40px;
    padding: 0 12px;
    border-radius: 10px;
    border: 1px solid var(--color-border-strong);
    background: var(--color-bg-base);
    color: var(--color-text-base);
    font: inherit;
  }

  .dili-pin-form {
    display: flex;
    align-items: center;
    gap: 10px;
  }

  .dili-pin {
    width: 56px;
    height: 64px;
    box-sizing: border-box;
    border: 1px solid var(--color-border-strong);
    border-radius: 12px;
    background: var(--color-bg-base);
    color: var(--color-text-base);
    font: inherit;
    font-size: 28px;
    font-weight: 700;
    text-align: center;
  }

  .dili-pin:focus {
    outline: 2px solid var(--color-primary);
    outline-offset: 1px;
  }

  .dili-pill {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    height: 48px;
    margin-left: 10px;
    padding: 0 22px;
    border-radius: 24px;
    border: none;
    background: var(--color-primary);
    color: var(--color-on-primary);
    font: inherit;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-pill:disabled {
    opacity: 0.5;
    cursor: default;
  }

  .dili-pill-outline {
    background: transparent;
    color: var(--color-text-base);
    border: 1px solid var(--color-border-strong);
  }

  .dili-pill-small {
    height: 36px;
    margin-left: 0;
    padding: 0 14px;
    font-size: 13px;
  }

  .dili-empty {
    padding: 24px;
    color: var(--color-text-muted);
  }

  .dili-device {
    display: flex;
    align-items: center;
    gap: 18px;
    padding: 16px 20px;
  }

  .dili-tile {
    width: 44px;
    height: 44px;
    border-radius: 11px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: var(--color-bg-muted);
  }

  .dili-device-name {
    flex-grow: 1;
    font-size: 16px;
    font-weight: 600;
  }

  .dili-section-head {
    display: flex;
    align-items: center;
    justify-content: space-between;
  }

  .dili-link-danger {
    border: none;
    background: none;
    color: var(--color-danger);
    font: inherit;
    font-size: 13px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-device-state {
    font-size: 13px;
    font-weight: 400;
    color: var(--color-text-muted);
  }

  .dili-switch {
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

  .dili-switch span {
    display: block;
    width: 26px;
    height: 26px;
    margin-left: 2px;
    border-radius: 13px;
    background: #ffffff;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.25);
    transition: margin-left 0.15s ease;
  }

  .dili-switch.on {
    background: var(--color-success);
  }

  .dili-switch.on span {
    margin-left: 22px;
  }
</style>
