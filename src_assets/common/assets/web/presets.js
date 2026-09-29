import { apiFetch } from './fetch_utils'

const DESCRIPTIONS = {
  'desktop': 'Your normal desktop, on a screen that fits your device.',
  'steam-big-picture': 'Opens Steam Big Picture when you connect, and closes it again when you stop.',
  'lutris': 'Opens Lutris, and closes it again when you stop.',
  'heroic': 'Opens the Heroic Games Launcher for your Epic, GOG and Amazon games.',
  'retrodeck': 'Opens RetroDECK for your emulated games.',
}

/**
 * Ready-made apps: the desktop plus the game launchers found on this PC.
 * Dili works out the right commands (normal install or Flatpak) on the server side.
 *
 * @returns {Promise<object[]>} Presets with id, installed, description, icon and the app entry to save.
 */
export async function loadPresets() {
  const presets = [
    {
      id: 'desktop',
      installed: true,
      description: DESCRIPTIONS.desktop,
      icon: './assets/apps/desktop.png',
      app: { name: 'Desktop', 'image-path': 'desktop.png' },
    },
  ]
  let launchers = []
  try {
    const r = await fetch('./api/launchers').then((x) => x.json())
    launchers = r.launchers || []
  } catch (e) {
    launchers = []
  }
  for (const l of launchers) {
    if (!DESCRIPTIONS[l.id]) continue  // only the launchers Dili offers
    const app = { name: l.name, 'image-path': l.image || 'box.png' }
    if (l.installed) {
      app.detached = [`setsid ${l.detached}`]
      if (l.undo) {
        app['prep-cmd'] = [{ do: '', undo: l.undo }]
      }
    }
    presets.push({
      id: l.id,
      installed: !!l.installed,
      description: DESCRIPTIONS[l.id],
      icon: l.image ? `./assets/apps/${l.image}` : '',
      app,
    })
  }
  return presets
}

export async function saveApp(app, index = -1) {
  const body = { detached: [], 'prep-cmd': [], ...app, index }
  const r = await apiFetch('./api/apps', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  })
  return r.status === 200
}

/**
 * Add Desktop and Steam Big Picture (if Steam is installed) when they are missing.
 * Used at the end of the setup wizard.
 */
export async function addMissingPresets() {
  const [presets, list] = await Promise.all([
    loadPresets(),
    fetch('./api/apps').then((r) => r.json()),
  ])
  const names = new Set((list.apps || []).map((a) => a.name))
  for (const preset of presets) {
    const wanted = preset.id === 'desktop' || preset.id === 'steam-big-picture'
    if (wanted && preset.installed && !names.has(preset.app.name)) {
      await saveApp(preset.app)
    }
  }
}
