<template>
  <Notification></Notification>
  <main class="dili-welcome" role="main">
    <section class="dili-welcome-card">
      <div class="dili-welcome-logo">
        <svg width="72" height="72" viewBox="0 0 32 32" fill="none" xmlns="http://www.w3.org/2000/svg" aria-hidden="true"><defs><radialGradient id="diliWelcomeCore" cx="0.4" cy="0.38" r="0.7"><stop offset="0" stop-color="#FFFFFF"></stop><stop offset="0.45" style="stop-color: var(--color-primary)"></stop><stop offset="1" stop-color="#7C3AED"></stop></radialGradient></defs><g transform="rotate(0.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(22.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(45.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(67.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(90.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(112.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(135.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(157.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(180.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(202.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(225.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(247.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(270.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(292.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><g transform="rotate(315.0 16 16)"><path d="M16 0.6C17.5 2.6 18.6 4.2 17.9 5.9C17.5 6.9 16.8 6.8 16.6 6.2C16.4 7 15.2 7.1 14.8 6.2C14.2 4.9 15.1 3.1 16 0.6Z" style="fill: var(--color-primary)"></path><path d="M16.1 3.1C16.8 4.1 17.2 4.9 16.9 5.6C16.7 6.1 15.7 6.1 15.5 5.5C15.3 4.8 15.7 4 16.1 3.1Z" fill="#FFFFFF" opacity="0.85"></path></g><g transform="rotate(337.5 16 16)"><path d="M16 2.3C16.9 3.5 17.5 4.5 17.1 5.6C16.9 6.2 16.4 6.2 16.2 5.8C16 6.3 15.3 6.3 15 5.7C14.7 4.8 15.3 3.6 16 2.3Z" fill="#8B5CF6"></path></g><circle cx="16" cy="16" r="8.9" stroke="#8B5CF6" stroke-width="1" opacity="0.5"></circle><circle cx="16" cy="16" r="7" fill="url(#diliWelcomeCore)"></circle></svg>
      </div>

      <div class="dili-welcome-head">
        <h1>Welcome to Dili</h1>
        <p>First, create a login for this page. You will use it to change settings and pair your devices.</p>
      </div>

      <form class="dili-form" @submit.prevent="save">
        <div class="dili-field">
          <label for="usernameInput">{{ $t('_common.username') }}</label>
          <input id="usernameInput" v-model="passwordData.newUsername" type="text" autocomplete="username" required>
        </div>
        <div class="dili-field">
          <label for="passwordInput">{{ $t('_common.password') }}</label>
          <input id="passwordInput" v-model="passwordData.newPassword" type="password" autocomplete="new-password" required>
        </div>
        <div class="dili-field">
          <label for="confirmPasswordInput">{{ $t('welcome.confirm_password') }}</label>
          <input id="confirmPasswordInput" v-model="passwordData.confirmNewPassword" type="password" autocomplete="new-password" required>
          <span v-if="mismatch" class="dili-hint-bad">The passwords do not match yet.</span>
        </div>

        <p class="dili-note">
          Keep this login somewhere safe. If you forget it, it can only be reset on this PC.
        </p>

        <div v-if="error" class="dili-message bad" role="alert">{{ error }}</div>
        <div v-if="success" class="dili-message good" role="status">
          Login created. Your browser will now ask for it once, then Dili opens.
        </div>

        <button type="submit" class="dili-btn" :disabled="loading || success || mismatch || !passwordData.newPassword">
          {{ loading ? 'Creating…' : 'Create login' }}
        </button>
      </form>
    </section>

    <p class="dili-credit">Dili is based on Sunshine by LizardByte.</p>
  </main>
</template>

<script>
  import { apiFetch } from './fetch_utils'
  import Notification from './Notification.vue'

  export default {
    components: {
      Notification,
    },
    data() {
      return {
        error: null,
        success: false,
        loading: false,
        passwordData: {
          newUsername: 'dili',
          newPassword: '',
          confirmNewPassword: '',
        },
      };
    },
    computed: {
      mismatch() {
        const { newPassword, confirmNewPassword } = this.passwordData;
        return confirmNewPassword.length > 0 && newPassword !== confirmNewPassword;
      },
    },
    methods: {
      save() {
        this.error = null;
        this.loading = true;
        apiFetch('./api/password', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify(this.passwordData),
        }).then((r) => {
          this.loading = false;
          if (r.status === 200) {
            r.json().then((rj) => {
              this.success = rj.status;
              if (this.success === true) {
                setTimeout(() => {
                  document.location.reload();
                }, 3000);
              } else {
                this.error = rj.error;
              }
            });
          } else {
            this.error = 'Something went wrong. Please try again.';
          }
        });
      },
    },
  }
</script>

<style scoped>
  .dili-welcome {
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 18px;
    padding: 24px;
    box-sizing: border-box;
    background: var(--color-bg-base);
  }

  .dili-welcome-card {
    width: 100%;
    max-width: 480px;
    background: var(--color-surface);
    border: 1px solid var(--color-border);
    border-radius: 22px;
    padding: 44px;
    box-sizing: border-box;
    display: flex;
    flex-direction: column;
    gap: 26px;
  }

  .dili-welcome-logo {
    display: flex;
    justify-content: center;
  }

  .dili-welcome-head {
    display: flex;
    flex-direction: column;
    gap: 10px;
    text-align: center;
  }

  .dili-welcome-head h1 {
    margin: 0;
    font-size: 30px;
    font-weight: 700;
    letter-spacing: -0.02em;
  }

  .dili-welcome-head p {
    margin: 0;
    font-size: 16px;
    line-height: 1.5;
    color: var(--color-text-muted);
  }

  .dili-form {
    display: flex;
    flex-direction: column;
    gap: 16px;
  }

  .dili-field {
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .dili-field label {
    font-size: 14px;
    font-weight: 600;
  }

  .dili-field input {
    height: 46px;
    padding: 0 14px;
    border-radius: 12px;
    border: 1px solid var(--color-border-strong);
    background: var(--color-bg-base);
    color: var(--color-text-base);
    font: inherit;
    font-size: 16px;
  }

  .dili-field input:focus {
    outline: 2px solid var(--color-primary);
    outline-offset: 1px;
  }

  .dili-hint-bad {
    font-size: 13px;
    color: var(--color-danger);
  }

  .dili-note {
    margin: 0;
    font-size: 13px;
    line-height: 1.5;
    color: var(--color-text-muted);
  }

  .dili-message {
    padding: 12px 14px;
    border-radius: 12px;
    font-size: 14px;
  }

  .dili-message.bad {
    color: var(--color-danger);
    background: var(--color-bg-subtle);
  }

  .dili-message.good {
    color: var(--color-text-base);
    background: var(--color-bg-subtle);
    border-left: 4px solid var(--color-success);
  }

  .dili-btn {
    height: 48px;
    border: none;
    border-radius: 24px;
    background: var(--color-primary);
    color: var(--color-on-primary);
    font: inherit;
    font-size: 16px;
    font-weight: 600;
    cursor: pointer;
  }

  .dili-btn:hover {
    background: var(--color-primary-hover);
  }

  .dili-btn:disabled {
    opacity: 0.6;
    cursor: default;
  }

  .dili-credit {
    margin: 0;
    font-size: 13px;
    color: var(--color-text-muted);
  }
</style>
