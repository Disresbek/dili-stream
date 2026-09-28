<template>
  <div class="dili-setup">
    <section class="dili-setup-card">
      <div class="dili-steps" :aria-label="`Step ${step + 1} of ${steps}`">
        <span v-for="i in steps" :key="i" :class="{ done: i - 1 <= step }"></span>
      </div>

      <!-- 1. Welcome -->
      <template v-if="step === 0">
        <div class="dili-setup-logo">
          <svg width="72" height="72" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true"><defs><radialGradient id="diliSetupCore" cx="0.4" cy="0.38" r="0.7"><stop offset="0" stop-color="#FFFFFF"></stop><stop offset="0.45" style="stop-color: var(--color-primary)"></stop><stop offset="1" stop-color="#7C3AED"></stop></radialGradient></defs><g transform="rotate(0.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(22.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(45.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(67.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(90.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(112.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(135.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(157.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(180.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(202.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(225.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(247.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(270.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(292.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(315.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(337.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><circle cx="16" cy="16" r="8.9" stroke="#8B5CF6" stroke-width="1" opacity="0.5"></circle><circle cx="16" cy="16" r="7" fill="url(#diliSetupCore)"></circle></svg>
        </div>
        <div class="dili-setup-head">
          <h1>Welcome to Dili</h1>
          <p>Play the games on this PC on your TV, phone or tablet. This takes about a minute to set up.</p>
        </div>
        <div class="dili-setup-actions">
          <button type="button" class="dili-btn" @click="next">Get started</button>
        </div>
      </template>

      <!-- 2. Controls and network -->
      <template v-if="step === 1">
        <div class="dili-setup-head">
          <h1>One quick step</h1>
          <p v-if="status.controls_ready">Controls and network access are already set up on this PC.</p>
          <p v-else>To set up controls and network access, your system needs your password once.</p>
        </div>
        <div class="dili-items">
          <div class="dili-item">
            <div class="dili-item-icon"><gamepad-2 :size="22"></gamepad-2></div>
            <div>
              <div class="dili-item-title">Use your controller from anywhere <span v-if="status.controls_ready" class="dili-ok">Ready</span></div>
              <div class="dili-item-text">So your controller, keyboard and touch input work on the PC while streaming.</div>
            </div>
          </div>
          <div class="dili-item">
            <div class="dili-item-icon"><wifi :size="22"></wifi></div>
            <div>
              <div class="dili-item-title">Let your devices find this PC</div>
              <div class="dili-item-text">Only devices on your home network can connect, and each one is paired with a code first.</div>
            </div>
          </div>
        </div>
        <p v-if="!status.controls_ready" class="dili-note">
          Nothing else on your PC is changed. KDE will show a password window on this PC.
        </p>
        <p v-if="error" class="dili-error">{{ error }}</p>
        <div class="dili-setup-actions">
          <button v-if="status.controls_ready" type="button" class="dili-btn-text" @click="back">Back</button>
          <button v-else type="button" class="dili-btn-text" @click="next">Not now</button>
          <button v-if="status.controls_ready" type="button" class="dili-btn" @click="next">Continue</button>
          <button v-else type="button" class="dili-btn" :disabled="busy" @click="runPermissions">
            {{ busy ? 'Waiting for your password…' : 'Continue' }}
          </button>
        </div>
      </template>

      <!-- 3. Screen sharing -->
      <template v-if="step === 2">
        <div class="dili-setup-head">
          <h1>Show your screen on other devices</h1>
          <p v-if="status.screen_sharing_ready">Screen sharing is already allowed. Streams will start without asking.</p>
          <p v-else>KDE asks once whether Dili may share your screen. Choose <strong>Allow</strong> and tick <strong>Remember</strong>, so it never asks again.</p>
        </div>
        <p v-if="error" class="dili-error">{{ error }}</p>
        <div class="dili-setup-actions">
          <button type="button" class="dili-btn-text" @click="back">Back</button>
          <button v-if="!status.screen_sharing_ready" type="button" class="dili-btn-text" @click="next">Not now</button>
          <button v-if="status.screen_sharing_ready" type="button" class="dili-btn" @click="next">Continue</button>
          <button v-else type="button" class="dili-btn" :disabled="busy" @click="runScreenSharing">
            {{ busy ? 'Waiting for KDE…' : 'Allow screen sharing' }}
          </button>
        </div>
      </template>

      <!-- 4. Autostart -->
      <template v-if="step === 3">
        <div class="dili-setup-head">
          <h1>Always ready</h1>
          <p>Dili can start quietly in the background when you log in, so your devices can connect any time.</p>
        </div>
        <div class="dili-toggle">
          <div class="dili-item-title">Start Dili when you log in</div>
          <button
            type="button"
            class="dili-switch"
            :class="{ on: autostart }"
            :aria-pressed="autostart ? 'true' : 'false'"
            aria-label="Start Dili when you log in"
            @click="autostart = !autostart"
          >
            <span></span>
          </button>
        </div>
        <p v-if="error" class="dili-error">{{ error }}</p>
        <div class="dili-setup-actions">
          <button type="button" class="dili-btn-text" @click="back">Back</button>
          <button type="button" class="dili-btn" :disabled="busy" @click="saveAutostart">Continue</button>
        </div>
      </template>

      <!-- 5. Done -->
      <template v-if="step === 4">
        <div class="dili-setup-logo">
          <div class="dili-done"><check :size="36"></check></div>
        </div>
        <div class="dili-setup-head">
          <h1>You're all set</h1>
          <p>Open Moonlight on your TV, phone or tablet and select <strong>{{ hostName || 'this PC' }}</strong>. Then pair it here with the 4-digit code.</p>
        </div>
        <div class="dili-setup-actions">
          <button type="button" class="dili-btn-text" @click="finish('/')">Go to Home</button>
          <button type="button" class="dili-btn" @click="finish('/devices')">Pair a device</button>
        </div>
      </template>
    </section>
  </div>
</template>

<script>
  import { apiFetch } from './fetch_utils'
  import { Check, Gamepad2, Wifi } from '@lucide/vue'

  export default {
    components: {
      Check,
      Gamepad2,
      Wifi,
    },
    data() {
      return {
        step: 0,
        steps: 5,
        status: { controls_ready: false, screen_sharing_ready: false },
        autostart: true,
        hostName: '',
        busy: false,
        error: '',
      };
    },
    async created() {
      await this.loadStatus();
      try {
        const a = await fetch('./api/autostart').then((r) => r.json());
        this.autostart = !!a.supported;  // recommended: on
      } catch (e) {
        console.error(e);
      }
      try {
        const s = await fetch('./api/status').then((r) => r.json());
        this.hostName = s.host_name || '';
      } catch (e) {
        console.error(e);
      }
    },
    methods: {
      async loadStatus() {
        try {
          this.status = await fetch('./api/setup').then((r) => r.json());
        } catch (e) {
          console.error(e);
        }
      },
      back() {
        this.error = '';
        this.step = Math.max(this.step - 1, 0);
      },
      next() {
        this.error = '';
        this.step = Math.min(this.step + 1, this.steps - 1);
      },
      async post(url, body) {
        const r = await apiFetch(url, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(body),
        });
        return r.json();
      },
      async runPermissions() {
        this.busy = true;
        this.error = '';
        try {
          const result = await this.post('./api/setup', { action: 'permissions' });
          await this.loadStatus();
          if (result.status === true) {
            this.next();
          } else {
            this.error = 'That did not work. If you closed the password window, try again, or choose "Not now".';
          }
        } finally {
          this.busy = false;
        }
      },
      async runScreenSharing() {
        this.busy = true;
        this.error = '';
        try {
          const result = await this.post('./api/setup', { action: 'screen_sharing' });
          await this.loadStatus();
          if (result.status === true) {
            this.next();
          } else {
            this.error = 'Screen sharing was not allowed. Try again and choose Allow, or choose "Not now".';
          }
        } finally {
          this.busy = false;
        }
      },
      async saveAutostart() {
        this.busy = true;
        this.error = '';
        try {
          const result = await this.post('./api/autostart', { enabled: this.autostart });
          if (result.status === true || !this.autostart) {
            this.next();
          } else {
            this.error = 'Autostart could not be changed. You can also turn it on later on the Home page.';
            this.next();
          }
        } finally {
          this.busy = false;
        }
      },
      async finish(target) {
        await this.post('./api/setup', { action: 'complete' });
        this.$router.push(target);
      },
    },
  };
</script>

<style scoped>
  .dili-setup {
    min-height: 100vh;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
    box-sizing: border-box;
    background: var(--color-bg-base);
  }

  .dili-setup-card {
    width: 100%;
    max-width: 560px;
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 22px;
    padding: 44px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 26px;
  }

  .dili-steps {
    display: flex;
    gap: 8px;
  }

  .dili-steps span {
    width: 28px;
    height: 6px;
    border-radius: 3px;
    background: var(--color-border);
  }

  .dili-steps span.done {
    background: var(--color-primary);
  }

  .dili-setup-logo {
    display: flex;
    justify-content: center;
  }

  .dili-setup-head {
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .dili-setup-head h1 {
    margin: 0;
    font-size: 30px;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .dili-setup-head p {
    margin: 0;
    font-size: 16px;
    line-height: 1.5;
    color: var(--color-text-muted);
  }

  .dili-items {
    display: flex;
    flex-direction: column;
    gap: 18px;
  }

  .dili-item {
    display: flex;
    gap: 16px;
  }

  .dili-item-icon {
    width: 40px;
    height: 40px;
    border-radius: 10px;
    flex-shrink: 0;
    display: flex;
    align-items: center;
    justify-content: center;
    background: var(--color-bg-muted);
  }

  .dili-item-title {
    font-size: 16px;
    font-weight: 600;
  }

  .dili-item-text {
    font-size: 14px;
    line-height: 1.45;
    color: var(--color-text-muted);
  }

  .dili-ok {
    margin-left: 6px;
    font-size: 12px;
    font-weight: 700;
    color: #ffffff;
    background: var(--color-success);
    padding: 2px 8px;
    border-radius: 10px;
  }

  .dili-note {
    margin: 0;
    font-size: 14px;
    line-height: 1.5;
    color: var(--color-text-muted);
  }

  .dili-error {
    margin: 0;
    font-size: 14px;
    color: var(--color-danger);
  }

  .dili-setup-actions {
    display: flex;
    justify-content: flex-end;
    gap: 12px;
  }

  .dili-btn,
  .dili-btn-text {
    height: 46px;
    padding: 0 22px;
    border-radius: 23px;
    border: none;
    font: inherit;
    font-size: 15px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-btn {
    background: var(--color-primary);
    color: var(--color-on-primary);
  }

  .dili-btn:hover {
    background: var(--color-primary-hover);
  }

  .dili-btn:disabled {
    opacity: 0.6;
    cursor: default;
  }

  .dili-btn-text {
    background: transparent;
    color: var(--color-text-base);
  }

  .dili-btn-text:empty {
    display: none;
  }

  .dili-toggle {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 16px 20px;
    border: 1px solid var(--color-border);
    border-radius: 14px;
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

  .dili-done {
    width: 72px;
    height: 72px;
    border-radius: 36px;
    display: flex;
    align-items: center;
    justify-content: center;
    color: #ffffff;
    background: var(--color-success);
  }
</style>
